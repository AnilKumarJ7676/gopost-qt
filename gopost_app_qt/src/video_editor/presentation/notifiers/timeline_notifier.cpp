#include "video_editor/presentation/notifiers/timeline_notifier.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>
#include <QDebug>
#include <algorithm>
#include <deque>

#include "rendering_bridge/stub_video_timeline_engine.h"
#include "video_editor/data/services/proxy_generation_service.h"
#include "video_editor/presentation/notifiers/video_frame_provider.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// TimelineState helpers
// ---------------------------------------------------------------------------

const VideoClip* TimelineState::selectedClip() const {
    if (!project || !selectedClipId) return nullptr;
    for (const auto& track : project->tracks)
        for (const auto& clip : track.clips)
            if (clip.id == *selectedClipId) return &clip;
    return nullptr;
}

const QList<VideoTrack>& TimelineState::tracks() const {
    static const QList<VideoTrack> kEmpty;
    return project ? project->tracks : kEmpty;
}

// ---------------------------------------------------------------------------
// Undo stack implementation
// ---------------------------------------------------------------------------

struct TimelineNotifier::UndoStack {
    static constexpr int kMaxSize = 50;
    std::deque<VideoProject> undoStack;
    std::deque<VideoProject> redoStack;

    void push(const VideoProject& proj) {
        undoStack.push_back(proj);
        if (static_cast<int>(undoStack.size()) > kMaxSize)
            undoStack.pop_front();
        redoStack.clear();
    }

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TimelineNotifier::TimelineNotifier(QObject* parent)
    : QObject(parent)
    , undoStack_(std::make_unique<UndoStack>())
    , stubEngine_(std::make_unique<rendering::StubVideoTimelineEngine>())
    , proxyService_(&ProxyGenerationService::instance())
{
    playback_      = std::make_unique<PlaybackDelegate>(this);
    trackClip_     = std::make_unique<TrackClipDelegate>(this);
    effectColor_   = std::make_unique<EffectColorDelegate>(this);
    keyframeAudio_ = std::make_unique<KeyframeAudioDelegate>(this);
    advancedEdit_  = std::make_unique<AdvancedEditDelegate>(this);

    // Playback timer
    playbackTimer_.setInterval(PlaybackDelegate::kPlaybackTickMs);
    connect(&playbackTimer_, &QTimer::timeout, this, [this] {
        playback_->onPlaybackTick();
    });

    // Debounced render
    renderDebounce_.setSingleShot(true);
    renderDebounce_.setInterval(50);
    connect(&renderDebounce_, &QTimer::timeout, this, [this] {
        renderCurrentFrame();
    });
}

TimelineNotifier::~TimelineNotifier() = default;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void TimelineNotifier::initTimeline() {
    state_.phase = TimelinePhase::initializing;
    emit stateChanged();

    auto project = std::make_shared<VideoProject>();
    project->width     = 1920;
    project->height    = 1080;
    project->frameRate = kDefaultFps;

    // Create default tracks: effect, video, audio
    VideoTrack effectTrack;
    effectTrack.type  = TrackType::Effect;
    effectTrack.index = 0;
    effectTrack.label = QStringLiteral("Effect 1");

    VideoTrack videoTrack;
    videoTrack.type  = TrackType::Video;
    videoTrack.index = 1;
    videoTrack.label = QStringLiteral("Video 1");

    VideoTrack audioTrack;
    audioTrack.type  = TrackType::Audio;
    audioTrack.index = 2;
    audioTrack.label = QStringLiteral("Audio 1");

    project->tracks = {effectTrack, videoTrack, audioTrack};

    state_.project = std::move(project);
    state_.phase   = TimelinePhase::ready;
    syncNativeToProject();
    playbackTimer_.start();
    emit stateChanged();

    qDebug() << "[TimelineNotifier] initTimeline done — tracks:" << state_.project->tracks.size()
             << "isReady:" << state_.isReady() << "trackCount:" << trackCount();
}

void TimelineNotifier::loadProject(const QString& projectJson) {
    state_.phase = TimelinePhase::initializing;
    emit stateChanged();

    auto doc = QJsonDocument::fromJson(projectJson.toUtf8());
    if (doc.isNull()) {
        state_.phase = TimelinePhase::error;
        state_.errorMessage = QStringLiteral("Invalid project JSON");
        emit stateChanged();
        return;
    }

    auto project = std::make_shared<VideoProject>(VideoProject::fromMap(doc.object()));

    state_.project = std::move(project);
    state_.phase   = TimelinePhase::ready;
    syncNativeToProject();
    playbackTimer_.start();
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// TimelineOperations implementation
// ---------------------------------------------------------------------------

void TimelineNotifier::setState(TimelineState state) {
    state_ = std::move(state);
    emit stateChanged();
}

UndoRedoStack* TimelineNotifier::undoRedo() {
    return nullptr; // Using internal undoStack_ instead
}

void TimelineNotifier::pushUndo(const VideoProject& before) {
    undoStack_->push(before);
    syncUndoRedoState();
}

void TimelineNotifier::syncUndoRedoState() {
    state_.canUndo = undoStack_->canUndo();
    state_.canRedo = undoStack_->canRedo();
    emit stateChanged();
}

void TimelineNotifier::updateClip(int clipId,
                                  std::function<VideoClip(const VideoClip&)> mutator) {
    if (!state_.project) return;
    for (auto& track : state_.project->tracks) {
        for (auto& clip : track.clips) {
            if (clip.id == clipId) {
                clip = mutator(clip);
                emit stateChanged();
                return;
            }
        }
    }
}

NativeVideoEngine* TimelineNotifier::engine() {
    // Delegates check for nullptr and gracefully degrade.
    // The stub engine is used internally for sync operations.
    return nullptr;
}

ProxyGenerationService* TimelineNotifier::proxyService() {
    return proxyService_;
}

void TimelineNotifier::renderCurrentFrame() {
    if (engineTimelineId_ >= 0 && stubEngine_) {
        stubEngine_->seek(engineTimelineId_, state_.playback.positionSeconds);

        // Render frame and push to provider for QML preview
        if (frameProvider_) {
            auto decoded = stubEngine_->renderFrame(engineTimelineId_);
            if (decoded.has_value() && decoded->width > 0 && decoded->height > 0
                && !decoded->pixels.isEmpty()) {
                QImage frame(reinterpret_cast<const uchar*>(decoded->pixels.constData()),
                             decoded->width, decoded->height, QImage::Format_RGBA8888);
                frameProvider_->updateFrame(frame.copy());  // deep copy — engine owns pixel data
                frameVersion_++;
                emit frameReady();
            }
        }
    }
    emit stateChanged();
}

void TimelineNotifier::debouncedRenderFrame() {
    renderDebounce_.start();
}

void TimelineNotifier::throttledRenderFrame() {
    if (!renderDebounce_.isActive())
        renderDebounce_.start();
}

void TimelineNotifier::updateActiveVideo() {
    if (engineTimelineId_ >= 0 && stubEngine_) {
        stubEngine_->seek(engineTimelineId_, state_.playback.positionSeconds);
    }
}

QString TimelineNotifier::resolvePlaybackPath(const VideoClip& clip) const {
    if (state_.useProxyPlayback && clip.proxyPath.has_value())
        return *clip.proxyPath;
    return clip.sourcePath;
}

int TimelineNotifier::toEngineSourceType(ClipSourceType t) const {
    return static_cast<int>(t);
}

void TimelineNotifier::syncNativeToProject() {
    if (!stubEngine_ || !state_.project) return;

    // Destroy previous timeline if any
    if (engineTimelineId_ >= 0) {
        stubEngine_->destroyTimeline(engineTimelineId_);
        engineTimelineId_ = -1;
    }

    // Create a new timeline matching the project settings
    rendering::TimelineConfig cfg;
    cfg.width     = state_.project->width;
    cfg.height    = state_.project->height;
    cfg.frameRate = state_.project->frameRate;
    engineTimelineId_ = stubEngine_->createTimeline(cfg);

    // Add tracks
    for (const auto& track : state_.project->tracks) {
        auto engineTrackType = rendering::VideoTrackType::Video;
        switch (track.type) {
            case TrackType::Video:  engineTrackType = rendering::VideoTrackType::Video; break;
            case TrackType::Audio:  engineTrackType = rendering::VideoTrackType::Audio; break;
            case TrackType::Title:  engineTrackType = rendering::VideoTrackType::Title; break;
            case TrackType::Effect: engineTrackType = rendering::VideoTrackType::Effect; break;
            default: break;
        }
        stubEngine_->addTrack(engineTimelineId_, engineTrackType);
    }

    // Add clips
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            rendering::ClipDescriptor desc;
            desc.sourceType  = static_cast<rendering::VideoClipSourceType>(static_cast<int>(clip.sourceType));
            desc.sourcePath  = clip.sourcePath;
            desc.trackIndex  = clip.trackIndex;
            desc.timelineRange.inTime  = clip.timelineIn;
            desc.timelineRange.outTime = clip.timelineOut;
            desc.sourceRange.sourceIn  = clip.sourceIn;
            desc.sourceRange.sourceOut = clip.sourceOut;
            desc.speed       = clip.speed;
            desc.opacity     = clip.opacity;
            desc.blendMode   = clip.blendMode;
            stubEngine_->addClip(engineTimelineId_, desc);
        }
    }

    qDebug() << "[TimelineNotifier] syncNativeToProject: timeline" << engineTimelineId_
             << "tracks:" << state_.project->tracks.size();
}

void TimelineNotifier::restoreClipS10State(const VideoClip& clip) {
    Q_UNUSED(clip);
}

void TimelineNotifier::syncEffectsToEngine(int clipId) {
    if (engineTimelineId_ < 0 || !stubEngine_ || !state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;

    // Sync color grading to engine
    const auto& cg = clip->colorGrading;
    stubEngine_->setClipColorGrading(engineTimelineId_, clipId,
        cg.brightness, cg.contrast, cg.saturation, cg.exposure,
        cg.temperature, cg.tint, cg.highlights, cg.shadows,
        cg.vibrance, cg.hue);

    // Sync volume
    stubEngine_->setClipVolume(engineTimelineId_, clipId, clip->audio.volume);
    stubEngine_->setClipPan(engineTimelineId_, clipId, clip->audio.pan);
    stubEngine_->setClipFadeIn(engineTimelineId_, clipId, clip->audio.fadeInSeconds);
    stubEngine_->setClipFadeOut(engineTimelineId_, clipId, clip->audio.fadeOutSeconds);
}

// ---------------------------------------------------------------------------
// Facade methods
// ---------------------------------------------------------------------------

void TimelineNotifier::play()            { playback_->play(); }
void TimelineNotifier::pause()           { playback_->pause(); }
void TimelineNotifier::togglePlayPause() { playback_->togglePlayPause(); }
void TimelineNotifier::seek(double pos)  { playback_->seek(pos); }
void TimelineNotifier::stepForward()     { playback_->stepForward(); }
void TimelineNotifier::stepBackward()    { playback_->stepBackward(); }
void TimelineNotifier::jumpToStart()     { playback_->jumpToStart(); }
void TimelineNotifier::jumpToEnd()       { playback_->jumpToEnd(); }
void TimelineNotifier::shuttleForward()  { playback_->shuttleForward(); }
void TimelineNotifier::shuttleReverse()  { playback_->shuttleReverse(); }
void TimelineNotifier::shuttleStop()     { playback_->shuttleStop(); }
void TimelineNotifier::setInPoint()      { playback_->setInPoint(); }
void TimelineNotifier::clearInPoint()    { playback_->clearInPoint(); }
void TimelineNotifier::setOutPoint()     { playback_->setOutPoint(); }
void TimelineNotifier::clearOutPoint()   { playback_->clearOutPoint(); }
void TimelineNotifier::scrubTo(double p) { playback_->scrubTo(p); }
void TimelineNotifier::setPixelsPerSecond(double pps) { playback_->setPixelsPerSecond(pps); }
void TimelineNotifier::setScrollOffset(double o)      { playback_->setScrollOffset(o); }
void TimelineNotifier::zoomAtPosition(double f, double a) { playback_->zoomAtPosition(f, a); }

void TimelineNotifier::zoomIn() {
    setPixelsPerSecond(state_.pixelsPerSecond * 1.25);
}

void TimelineNotifier::zoomOut() {
    setPixelsPerSecond(state_.pixelsPerSecond / 1.25);
}

void TimelineNotifier::togglePlayback() { togglePlayPause(); }

void TimelineNotifier::stepForwardN(int n) {
    if (!state_.project) return;
    double step = n / state_.project->frameRate;
    playback_->seek(state_.playback.positionSeconds + step);
}

void TimelineNotifier::stepBackwardN(int n) {
    if (!state_.project) return;
    double step = n / state_.project->frameRate;
    playback_->seek(std::max(0.0, state_.playback.positionSeconds - step));
}

void TimelineNotifier::clearInOutPoints() {
    playback_->clearInPoint();
    playback_->clearOutPoint();
}

void TimelineNotifier::jumpToPreviousSnapPoint() {
    playback_->jumpToPreviousSnapPoint();
}

void TimelineNotifier::jumpToNextSnapPoint() {
    playback_->jumpToNextSnapPoint();
}

int TimelineNotifier::clipCountForTrack(int trackIndex) const {
    if (!state_.project) return 0;
    const auto& tracks = state_.project->tracks;
    if (trackIndex < 0 || trackIndex >= tracks.size()) return 0;
    return tracks[trackIndex].clips.size();
}

QVariantMap TimelineNotifier::clipData(int trackIndex, int clipIndex) const {
    QVariantMap result;
    if (!state_.project) return result;
    const auto& tracks = state_.project->tracks;
    if (trackIndex < 0 || trackIndex >= tracks.size()) return result;
    const auto& clips = tracks[trackIndex].clips;
    if (clipIndex < 0 || clipIndex >= clips.size()) return result;
    const auto& clip = clips[clipIndex];
    result[QStringLiteral("clipId")] = clip.id;
    result[QStringLiteral("displayName")] = clip.displayName;
    result[QStringLiteral("timelineIn")] = clip.timelineIn;
    result[QStringLiteral("timelineOut")] = clip.timelineOut;
    result[QStringLiteral("duration")] = clip.duration();
    result[QStringLiteral("sourceType")] = static_cast<int>(clip.sourceType);
    result[QStringLiteral("speed")] = clip.speed;
    result[QStringLiteral("opacity")] = clip.opacity;
    result[QStringLiteral("sourcePath")] = clip.sourcePath;
    return result;
}

void TimelineNotifier::addTrack(int trackType) {
    trackClip_->addTrack(static_cast<TrackType>(trackType));
}
void TimelineNotifier::removeTrack(int i)            { trackClip_->removeTrack(i); }
void TimelineNotifier::toggleTrackVisibility(int i)  { trackClip_->toggleTrackVisibility(i); }
void TimelineNotifier::toggleTrackLock(int i)        { trackClip_->toggleTrackLock(i); }
void TimelineNotifier::toggleTrackMute(int i)        { trackClip_->toggleTrackMute(i); }
void TimelineNotifier::toggleTrackSolo(int i)        { trackClip_->toggleTrackSolo(i); }
int  TimelineNotifier::addClip(int ti, int st, const QString& sp,
                               const QString& dn, double dur) {
    return trackClip_->addClip(ti, static_cast<ClipSourceType>(st), sp, dn, dur);
}
void TimelineNotifier::removeClip(int id)    { trackClip_->removeClip(id); }
void TimelineNotifier::moveClip(int id, int t, double time) { trackClip_->moveClip(id, t, time); }
void TimelineNotifier::trimClip(int id, double i, double o) { trackClip_->trimClip(id, i, o); }
void TimelineNotifier::splitClipAtPlayhead() { trackClip_->splitClipAtPlayhead(); }
void TimelineNotifier::rippleDelete(int id)  { trackClip_->rippleDelete(id); }
void TimelineNotifier::selectClip(int id)    { trackClip_->selectClip(id); }
void TimelineNotifier::deselectClip()        { trackClip_->deselectClip(); }
void TimelineNotifier::updateClipDuration(int id, double d) { trackClip_->updateClipDuration(id, d); }
void TimelineNotifier::updateClipDisplayName(int clipId, const QString& name) {
    updateClip(clipId, [&](const VideoClip& c) {
        VideoClip updated = c;
        updated.displayName = name;
        return updated;
    });
}

void TimelineNotifier::addEffect(int cid, int et, double v) {
    VideoEffect fx;
    fx.type  = static_cast<EffectType>(et);
    fx.value = v;
    effectColor_->addEffect(cid, fx);
}
void TimelineNotifier::removeEffect(int cid, int et) {
    effectColor_->removeEffect(cid, static_cast<EffectType>(et));
}
void TimelineNotifier::toggleEffect(int cid, int et) {
    effectColor_->toggleEffect(cid, static_cast<EffectType>(et));
}
void TimelineNotifier::updateEffect(int cid, int et, double v) {
    effectColor_->updateEffect(cid, static_cast<EffectType>(et), v);
}
void TimelineNotifier::clearEffects(int cid) { effectColor_->clearEffects(cid); }
void TimelineNotifier::applyPresetFilter(int cid, int pid) {
    effectColor_->applyPresetFilter(cid, static_cast<PresetFilterId>(pid));
}
void TimelineNotifier::resetColorGrading(int cid) { effectColor_->resetColorGrading(cid); }
void TimelineNotifier::setColorGradingProperty(int clipId, const QString& prop, double value) {
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    ColorGrading cg = clip->colorGrading;
    if (prop == QStringLiteral("brightness"))       cg.brightness  = value;
    else if (prop == QStringLiteral("contrast"))     cg.contrast    = value;
    else if (prop == QStringLiteral("saturation"))   cg.saturation  = value;
    else if (prop == QStringLiteral("temperature"))  cg.temperature = value;
    else if (prop == QStringLiteral("tint"))          cg.tint        = value;
    else if (prop == QStringLiteral("shadows"))       cg.shadows     = value;
    else if (prop == QStringLiteral("highlights"))    cg.highlights  = value;
    else if (prop == QStringLiteral("vibrance"))      cg.vibrance    = value;
    else if (prop == QStringLiteral("fade"))           cg.fade        = value;
    else if (prop == QStringLiteral("vignette"))       cg.vignette    = value;
    else return;
    effectColor_->setColorGrading(clipId, cg);
}
void TimelineNotifier::setClipOpacity(int cid, double o) { effectColor_->setClipOpacity(cid, o); }
void TimelineNotifier::setClipBlendMode(int cid, int m)  { effectColor_->setClipBlendMode(cid, m); }

void TimelineNotifier::setTransitionIn(int cid, int tt, double dur, int easing) {
    ClipTransition t;
    t.type            = static_cast<TransitionType>(tt);
    t.durationSeconds = dur;
    t.easing          = static_cast<EasingCurve>(easing);
    effectColor_->setTransitionIn(cid, t);
}
void TimelineNotifier::setTransitionOut(int cid, int tt, double dur, int easing) {
    ClipTransition t;
    t.type            = static_cast<TransitionType>(tt);
    t.durationSeconds = dur;
    t.easing          = static_cast<EasingCurve>(easing);
    effectColor_->setTransitionOut(cid, t);
}
void TimelineNotifier::removeTransitionIn(int cid)  { effectColor_->removeTransitionIn(cid); }
void TimelineNotifier::removeTransitionOut(int cid) { effectColor_->removeTransitionOut(cid); }

void TimelineNotifier::addKeyframe(int cid, int prop, double time, double val) {
    keyframeAudio_->addKeyframe(cid, static_cast<KeyframeProperty>(prop),
                                Keyframe{time, val});
}
void TimelineNotifier::removeKeyframe(int cid, int prop, double time) {
    keyframeAudio_->removeKeyframe(cid, static_cast<KeyframeProperty>(prop), time);
}
void TimelineNotifier::clearKeyframes(int cid, int prop) {
    keyframeAudio_->clearKeyframes(cid, static_cast<KeyframeProperty>(prop));
}
void TimelineNotifier::setClipVolume(int cid, double v) { keyframeAudio_->setClipVolume(cid, v); }
void TimelineNotifier::setClipPan(int cid, double p)    { keyframeAudio_->setClipPan(cid, p); }
void TimelineNotifier::setClipFadeIn(int cid, double s) { keyframeAudio_->setClipFadeIn(cid, s); }
void TimelineNotifier::setClipFadeOut(int cid, double s){ keyframeAudio_->setClipFadeOut(cid, s); }
void TimelineNotifier::setClipMuted(int cid, bool m)    { keyframeAudio_->setClipMuted(cid, m); }
void TimelineNotifier::setTrackVolume(int ti, double v) { keyframeAudio_->setTrackVolume(ti, v); }
void TimelineNotifier::setTrackPan(int ti, double p)    { keyframeAudio_->setTrackPan(ti, p); }

void TimelineNotifier::setClipSpeed(int cid, double s)  { advancedEdit_->setClipSpeed(cid, s); }
void TimelineNotifier::reverseClip(int cid)              { advancedEdit_->reverseClip(cid); }
void TimelineNotifier::freezeFrame(int cid)              { advancedEdit_->freezeFrame(cid); }
void TimelineNotifier::duplicateClip(int cid)            { advancedEdit_->duplicateClip(cid); }
void TimelineNotifier::rateStretch(int cid, double d)    { advancedEdit_->rateStretch(cid, d); }
void TimelineNotifier::addMarker() {
    advancedEdit_->addMarker(MarkerType::Chapter, QStringLiteral("Marker"));
}
void TimelineNotifier::addMarkerWithType(int t, const QString& l) {
    advancedEdit_->addMarker(static_cast<MarkerType>(t), l);
}
void TimelineNotifier::removeMarker(int id)     { advancedEdit_->removeMarker(id); }
void TimelineNotifier::updateMarker(int id, const QString& l) { advancedEdit_->updateMarker(id, l); }
void TimelineNotifier::navigateToMarker(int id) { advancedEdit_->navigateToMarker(id); }
void TimelineNotifier::navigateToNextMarker()   { advancedEdit_->navigateToNextMarker(); }
void TimelineNotifier::navigateToPreviousMarker() { advancedEdit_->navigateToPreviousMarker(); }
void TimelineNotifier::setProjectDimensions(int w, int h) { advancedEdit_->setProjectDimensions(w, h); }
void TimelineNotifier::generateProxyForClip(int cid)      { advancedEdit_->generateProxyForClip(cid); }
void TimelineNotifier::toggleProxyPlayback()               { advancedEdit_->toggleProxyPlayback(); }
void TimelineNotifier::applySpeedRamp(int clipId, int presetIndex) {
    auto presets = AdvancedEditDelegate::speedRampPresets();
    if (presetIndex >= 0 && presetIndex < static_cast<int>(presets.size()))
        advancedEdit_->applySpeedRamp(clipId, presets[presetIndex]);
}

QVariantMap TimelineNotifier::clipColorGrading(int clipId) const {
    QVariantMap result;
    if (!state_.project) return result;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return result;
    const auto& cg = clip->colorGrading;
    result[QStringLiteral("brightness")]  = cg.brightness;
    result[QStringLiteral("contrast")]    = cg.contrast;
    result[QStringLiteral("saturation")]  = cg.saturation;
    result[QStringLiteral("temperature")] = cg.temperature;
    result[QStringLiteral("tint")]        = cg.tint;
    result[QStringLiteral("shadows")]     = cg.shadows;
    result[QStringLiteral("highlights")]  = cg.highlights;
    result[QStringLiteral("vibrance")]    = cg.vibrance;
    result[QStringLiteral("fade")]        = cg.fade;
    result[QStringLiteral("vignette")]    = cg.vignette;
    return result;
}

QVariantList TimelineNotifier::clipEffects(int clipId) const {
    QVariantList result;
    if (!state_.project) return result;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return result;
    for (const auto& fx : clip->effects) {
        QVariantMap m;
        m[QStringLiteral("type")]    = static_cast<int>(fx.type);
        m[QStringLiteral("value")]   = fx.value;
        m[QStringLiteral("enabled")] = fx.enabled;
        result.append(m);
    }
    return result;
}

double TimelineNotifier::clipSpeed(int clipId) const {
    if (!state_.project) return 1.0;
    const auto* clip = state_.project->findClip(clipId);
    return clip ? clip->speed : 1.0;
}

double TimelineNotifier::clipOpacity(int clipId) const {
    if (!state_.project) return 1.0;
    const auto* clip = state_.project->findClip(clipId);
    return clip ? clip->opacity : 1.0;
}

QVariantList TimelineNotifier::clipKeyframeTimes(int clipId, int prop) const {
    QVariantList result;
    if (!state_.project) return result;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return result;
    auto kfProp = static_cast<KeyframeProperty>(prop);
    for (const auto& track : clip->keyframes.tracks) {
        if (track.property == kfProp) {
            for (const auto& kf : track.keyframes) {
                QVariantMap m;
                m[QStringLiteral("time")]  = kf.time;
                m[QStringLiteral("value")] = kf.value;
                result.append(m);
            }
            break;
        }
    }
    return result;
}

QVariantMap TimelineNotifier::clipTextData(int clipId) const {
    QVariantMap result;
    if (!state_.project) return result;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return result;
    // Text data stored in displayName; font/style in adjustmentData or keyframes
    result[QStringLiteral("text")]        = clip->displayName;
    result[QStringLiteral("sourceType")]  = static_cast<int>(clip->sourceType);
    result[QStringLiteral("speed")]       = clip->speed;
    result[QStringLiteral("opacity")]     = clip->opacity;
    return result;
}

void TimelineNotifier::setActivePanel(int panel) {
    state_.activePanel = static_cast<BottomPanelTab>(panel);
    emit stateChanged();
}

void TimelineNotifier::undo() {
    if (!undoStack_->canUndo() || !state_.project) return;
    undoStack_->redoStack.push_back(*state_.project);
    state_.project = std::make_shared<VideoProject>(undoStack_->undoStack.back());
    undoStack_->undoStack.pop_back();
    syncUndoRedoState();
    syncNativeToProject();
}

void TimelineNotifier::redo() {
    if (!undoStack_->canRedo() || !state_.project) return;
    undoStack_->undoStack.push_back(*state_.project);
    state_.project = std::make_shared<VideoProject>(undoStack_->redoStack.back());
    undoStack_->redoStack.pop_back();
    syncUndoRedoState();
    syncNativeToProject();
}

// ---------------------------------------------------------------------------
// Clip audio property accessors
// ---------------------------------------------------------------------------

double TimelineNotifier::clipVolume() const {
    auto* clip = state_.selectedClip();
    return clip ? clip->audio.volume : 1.0;
}

double TimelineNotifier::clipPan() const {
    auto* clip = state_.selectedClip();
    return clip ? clip->audio.pan : 0.0;
}

double TimelineNotifier::clipFadeIn() const {
    auto* clip = state_.selectedClip();
    return clip ? clip->audio.fadeInSeconds : 0.0;
}

double TimelineNotifier::clipFadeOut() const {
    auto* clip = state_.selectedClip();
    return clip ? clip->audio.fadeOutSeconds : 0.0;
}

bool TimelineNotifier::clipIsMuted() const {
    auto* clip = state_.selectedClip();
    return clip ? clip->audio.isMuted : false;
}

// ---------------------------------------------------------------------------
// Tracks variant (for QML ListView model)
// ---------------------------------------------------------------------------

QVariantList TimelineNotifier::tracksVariant() const {
    QVariantList result;
    if (!state_.project) return result;
    for (const auto& track : state_.project->tracks) {
        QVariantMap m;
        m[QStringLiteral("index")] = track.index;
        m[QStringLiteral("label")] = track.label;
        m[QStringLiteral("type")] = static_cast<int>(track.type);
        m[QStringLiteral("isVisible")] = track.isVisible;
        m[QStringLiteral("isLocked")] = track.isLocked;
        m[QStringLiteral("isMuted")] = track.isMuted;
        m[QStringLiteral("isSolo")] = track.isSolo;
        m[QStringLiteral("volume")] = track.audioSettings.volume;
        m[QStringLiteral("pan")] = track.audioSettings.pan;
        m[QStringLiteral("clipCount")] = track.clips.size();
        result.append(m);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Marker accessors
// ---------------------------------------------------------------------------

int TimelineNotifier::markerCount() const {
    return state_.project ? state_.project->markers.size() : 0;
}

QVariantList TimelineNotifier::markersVariant() const {
    QVariantList result;
    if (!state_.project) return result;
    for (const auto& marker : state_.project->markers) {
        QVariantMap m;
        m[QStringLiteral("id")] = marker.id;
        m[QStringLiteral("position")] = marker.positionSeconds;
        m[QStringLiteral("type")] = static_cast<int>(marker.type);
        m[QStringLiteral("label")] = marker.label;
        if (marker.color.has_value())
            m[QStringLiteral("color")] = *marker.color;
        result.append(m);
    }
    return result;
}

QVariantMap TimelineNotifier::selectedClipVariant() const {
    QVariantMap result;
    if (!state_.project || !state_.selectedClipId.has_value()) return result;
    int clipId = *state_.selectedClipId;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.id == clipId) {
                result[QStringLiteral("clipId")]      = clip.id;
                result[QStringLiteral("displayName")] = clip.displayName;
                result[QStringLiteral("timelineIn")]  = clip.timelineIn;
                result[QStringLiteral("timelineOut")] = clip.timelineOut;
                result[QStringLiteral("duration")]    = clip.duration();
                result[QStringLiteral("sourceType")]  = static_cast<int>(clip.sourceType);
                result[QStringLiteral("speed")]       = clip.speed;
                result[QStringLiteral("opacity")]     = clip.opacity;
                result[QStringLiteral("sourcePath")]  = clip.sourcePath;
                result[QStringLiteral("trackIndex")]  = clip.trackIndex;
                result[QStringLiteral("blendMode")]   = clip.blendMode;
                result[QStringLiteral("sourceIn")]    = clip.sourceIn;
                result[QStringLiteral("sourceOut")]   = clip.sourceOut;
                return result;
            }
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Active clip at playhead (for video preview)
// ---------------------------------------------------------------------------

QString TimelineNotifier::activeClipSource() const {
    if (!state_.project) return {};
    double pos = state_.playback.positionSeconds;
    // Search from lowest track index (topmost visually) for a video/image clip
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Video ||
                clip.sourceType == ClipSourceType::Image) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    return clip.sourcePath;
                }
            }
        }
    }
    return {};
}

double TimelineNotifier::activeClipOffset() const {
    if (!state_.project) return 0.0;
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Video ||
                clip.sourceType == ClipSourceType::Image) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    // Offset into the source file, accounting for sourceIn and speed
                    double relTime = pos - clip.timelineIn;
                    return clip.sourceIn + relTime * clip.speed;
                }
            }
        }
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
// Transform / Crop stubs
// ---------------------------------------------------------------------------

void TimelineNotifier::setKeyframe(int clipId, int prop, double value) {
    // Convenience: set keyframe at current position
    addKeyframe(clipId, prop, state_.playback.positionSeconds, value);
}

void TimelineNotifier::resetTransform(int clipId) {
    // Reset position, rotation, scale keyframes
    clearKeyframes(clipId, 0); // posX
    clearKeyframes(clipId, 1); // posY
    clearKeyframes(clipId, 2); // scale
    clearKeyframes(clipId, 3); // rotation
}

void TimelineNotifier::applyKenBurns(int clipId, double startScale, double endScale) {
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    addKeyframe(clipId, 2, clip->timelineIn, startScale);  // scale at start
    addKeyframe(clipId, 2, clip->timelineOut, endScale);    // scale at end
}

void TimelineNotifier::applyKenBurnsPan(int clipId, double startX, double startY,
                                         double endX, double endY) {
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    addKeyframe(clipId, 0, clip->timelineIn, startX);
    addKeyframe(clipId, 1, clip->timelineIn, startY);
    addKeyframe(clipId, 0, clip->timelineOut, endX);
    addKeyframe(clipId, 1, clip->timelineOut, endY);
}

void TimelineNotifier::createAdjustmentClip() {
    if (!state_.project) return;
    // Find or create an effect track, then add an adjustment clip
    int effectTrackIdx = -1;
    for (const auto& track : state_.project->tracks) {
        if (track.type == TrackType::Effect) { effectTrackIdx = track.index; break; }
    }
    if (effectTrackIdx < 0) {
        addTrack(static_cast<int>(TrackType::Effect));
        effectTrackIdx = state_.project->tracks.size() - 1;
    }
    addClip(effectTrackIdx, static_cast<int>(ClipSourceType::Adjustment),
            QString(), QStringLiteral("Adjustment"), 5.0);
}

void TimelineNotifier::editMarker(int markerId) {
    // Navigate to marker position for editing
    navigateToMarker(markerId);
}

} // namespace gopost::video_editor
