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

#include <chrono>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// Performance tracing — logs stats every 3 seconds during playback
// ---------------------------------------------------------------------------
namespace {
struct PerfCounters {
    int setStateCalls       = 0;
    int playbackEmits       = 0;
    int tracksEmits         = 0;
    int selectionEmits      = 0;
    int zoomEmits           = 0;
    int metaEmits           = 0;
    int frameReadyEmits     = 0;
    int renderCalls         = 0;
    int renderThrottled     = 0;
    double renderTotalMs    = 0.0;
    double renderMaxMs      = 0.0;
    std::chrono::steady_clock::time_point lastReport =
        std::chrono::steady_clock::now();

    void maybeReport() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - lastReport).count();
        if (elapsed < 3000) return;

        double avgRenderMs = renderCalls > 0 ? renderTotalMs / renderCalls : 0.0;
        qDebug().nospace()
            << "[PERF 3s] setState=" << setStateCalls
            << " | signals: playback=" << playbackEmits
            << " tracks=" << tracksEmits
            << " selection=" << selectionEmits
            << " zoom=" << zoomEmits
            << " meta=" << metaEmits
            << " frameReady=" << frameReadyEmits
            << " | render: calls=" << renderCalls
            << " throttled=" << renderThrottled
            << " avgMs=" << QString::number(avgRenderMs, 'f', 2)
            << " maxMs=" << QString::number(renderMaxMs, 'f', 2);

        // Reset
        *this = PerfCounters{};
        lastReport = now;
    }
};
static PerfCounters g_perf;
} // anonymous

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

    // Render throttle timer (Phase 3)
    renderThrottleTimer_.setSingleShot(true);
    renderThrottleTimer_.setInterval(kRenderIntervalMs);
    connect(&renderThrottleTimer_, &QTimer::timeout, this, [this] {
        lastRenderTime_ = std::chrono::steady_clock::now();
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

    // Emit all signals on init so QML picks up the full initial state
    emit stateChanged();
    emit playbackChanged();
    emit tracksChanged();
    emit selectionChanged();
    emit zoomChanged();

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

    // Emit all signals on load so QML picks up the full loaded state
    emit stateChanged();
    emit playbackChanged();
    emit tracksChanged();
    emit selectionChanged();
    emit zoomChanged();
}

// ---------------------------------------------------------------------------
// TimelineOperations implementation
// ---------------------------------------------------------------------------

void TimelineNotifier::setState(TimelineState state) {
    // Diff old vs new to emit only the signals whose category changed.
    // During playback this means only playbackChanged() fires (~30x/sec).
    const bool playbackDirty =
        state.playback.positionSeconds != prevPosition_ ||
        state.playback.isPlaying       != prevIsPlaying_ ||
        state.playback.inPoint         != prevInPoint_ ||
        state.playback.outPoint        != prevOutPoint_;

    const bool tracksDirty =
        state.trackGeneration != prevTrackGen_;

    const bool selectionDirty =
        state.selectedClipId      != prevSelectedClipId_ ||
        state.selectionGeneration != prevSelectionGen_;

    const bool zoomDirty =
        state.pixelsPerSecond != prevPps_ ||
        state.scrollOffset    != prevScrollOffset_;

    const bool metaDirty =
        state.phase            != state_.phase ||
        state.canUndo          != state_.canUndo ||
        state.canRedo          != state_.canRedo ||
        state.activePanel      != state_.activePanel ||
        state.useProxyPlayback != state_.useProxyPlayback ||
        state.errorMessage     != state_.errorMessage;

    // Apply new state
    state_ = std::move(state);

    // Keep selectedClipIds in sync with selectedClipId.
    // Delegates (split, select, etc.) set selectedClipId but may not know about
    // selectedClipIds. If selectedClipId was set but not in the set, add it.
    if (state_.selectedClipId.has_value() &&
        !state_.selectedClipIds.contains(*state_.selectedClipId)) {
        // A delegate set selectedClipId directly — add to set (don't clear others)
        state_.selectedClipIds.insert(*state_.selectedClipId);
    }

    // Update snapshots
    prevPosition_      = state_.playback.positionSeconds;
    prevIsPlaying_     = state_.playback.isPlaying;
    prevInPoint_       = state_.playback.inPoint;
    prevOutPoint_      = state_.playback.outPoint;
    prevTrackGen_      = state_.trackGeneration;
    prevSelectionGen_  = state_.selectionGeneration;
    prevSelectedClipId_ = state_.selectedClipId;
    prevPps_           = state_.pixelsPerSecond;
    prevScrollOffset_  = state_.scrollOffset;

    // Sync frame provider scaling mode (fast during playback, smooth when paused)
    if (frameProvider_)
        frameProvider_->setPlaybackMode(state_.playback.isPlaying);

    // Emit only the signals for categories that actually changed
    g_perf.setStateCalls++;
    if (playbackDirty)  { g_perf.playbackEmits++; emit playbackChanged(); }
    if (tracksDirty)    { g_perf.tracksEmits++;    emit tracksChanged(); }
    if (selectionDirty) { g_perf.selectionEmits++; emit selectionChanged(); }
    if (zoomDirty)      { g_perf.zoomEmits++;      emit zoomChanged(); }
    if (metaDirty)      { g_perf.metaEmits++;      emit stateChanged(); }
    g_perf.maybeReport();
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
    emit stateChanged();  // canUndo/canRedo are meta properties
}

void TimelineNotifier::updateClip(int clipId,
                                  std::function<VideoClip(const VideoClip&)> mutator) {
    if (!state_.project) return;
    for (auto& track : state_.project->tracks) {
        for (auto& clip : track.clips) {
            if (clip.id == clipId) {
                clip = mutator(clip);
                // Clip content changed — bump both generations and go
                // through setState() so prev* snapshots stay in sync.
                state_.selectionGeneration++;
                state_.trackGeneration++;
                setState(std::move(state_));
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
    auto renderStart = std::chrono::steady_clock::now();
    if (engineTimelineId_ >= 0 && stubEngine_) {
        stubEngine_->seek(engineTimelineId_, state_.playback.positionSeconds);

        // Render frame at preview resolution and push to provider for QML
        if (frameProvider_) {
            auto decoded = stubEngine_->renderFrame(
                engineTimelineId_, kPreviewWidth, kPreviewHeight);
            if (decoded.has_value() && decoded->width > 0 && decoded->height > 0
                && !decoded->pixels.isEmpty()) {
                // Double-buffer: write into the inactive buffer, then swap
                auto& buf = frameBuffers_[currentBuffer_];
                const int dw = decoded->width;
                const int dh = decoded->height;
                if (buf.width() != dw || buf.height() != dh) {
                    buf = QImage(dw, dh, QImage::Format_RGBA8888);
                }
                const auto copySize = std::min(
                    static_cast<size_t>(decoded->pixels.size()),
                    static_cast<size_t>(buf.sizeInBytes()));
                memcpy(buf.bits(), decoded->pixels.constData(), copySize);
                frameProvider_->updateFrame(buf);  // implicit-share (no deep copy)
                currentBuffer_ = 1 - currentBuffer_;
                frameVersion_++;
                g_perf.frameReadyEmits++;
                emit frameReady();
            }
        }
    }
    // NOTE: intentionally no emit stateChanged() here — frameReady is
    // sufficient for the preview image, and the caller (setState or
    // playback tick) already emitted the appropriate granular signal.
    auto renderEnd = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(renderEnd - renderStart).count();
    g_perf.renderCalls++;
    g_perf.renderTotalMs += ms;
    if (ms > g_perf.renderMaxMs) g_perf.renderMaxMs = ms;
}

void TimelineNotifier::debouncedRenderFrame() {
    // Immediate render for interactive feedback (scrub, seek, pause)
    renderCurrentFrame();
}

void TimelineNotifier::throttledRenderFrame() {
    // True throttle: render at most once per kRenderIntervalMs (33ms).
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now - lastRenderTime_).count();

    if (elapsed >= kRenderIntervalMs) {
        lastRenderTime_ = now;
        renderCurrentFrame();
    } else {
        g_perf.renderThrottled++;
        if (!renderThrottleTimer_.isActive()) {
            renderThrottleTimer_.start(
                static_cast<int>(kRenderIntervalMs - elapsed));
        }
    }
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

    // Sync all clip data (effects, transitions, keyframes, audio) to engine
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            syncEffectsToEngine(clip.id);
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

    // 1. Color grading
    const auto& cg = clip->colorGrading;
    stubEngine_->setClipColorGrading(engineTimelineId_, clipId,
        cg.brightness, cg.contrast, cg.saturation, cg.exposure,
        cg.temperature, cg.tint, cg.highlights, cg.shadows,
        cg.vibrance, cg.hue);

    // 2. Audio params
    stubEngine_->setClipVolume(engineTimelineId_, clipId, clip->audio.volume);
    stubEngine_->setClipPan(engineTimelineId_, clipId, clip->audio.pan);
    stubEngine_->setClipFadeIn(engineTimelineId_, clipId, clip->audio.fadeInSeconds);
    stubEngine_->setClipFadeOut(engineTimelineId_, clipId, clip->audio.fadeOutSeconds);

    // 3. Effects
    for (const auto& fx : clip->effects) {
        auto instanceId = stubEngine_->addClipEffect(
            engineTimelineId_, clipId,
            QString::number(static_cast<int>(fx.type)));
        stubEngine_->setEffectParam(engineTimelineId_, clipId, instanceId,
            QStringLiteral("value"), fx.value);
        stubEngine_->setEffectMix(engineTimelineId_, clipId, instanceId, fx.mix);
        stubEngine_->setEffectEnabled(engineTimelineId_, clipId, instanceId, fx.enabled);
    }

    // 4. Transitions
    if (!clip->transitionIn.isNone()) {
        stubEngine_->setClipTransitionIn(engineTimelineId_, clipId,
            static_cast<int>(clip->transitionIn.type),
            clip->transitionIn.durationSeconds,
            static_cast<int>(clip->transitionIn.easing));
    }
    if (!clip->transitionOut.isNone()) {
        stubEngine_->setClipTransitionOut(engineTimelineId_, clipId,
            static_cast<int>(clip->transitionOut.type),
            clip->transitionOut.durationSeconds,
            static_cast<int>(clip->transitionOut.easing));
    }

    // 5. Keyframes
    for (const auto& track : clip->keyframes.tracks) {
        for (const auto& kf : track.keyframes) {
            stubEngine_->setClipKeyframe(engineTimelineId_, clipId,
                static_cast<int>(track.property), kf.time, kf.value,
                static_cast<int>(kf.interpolation));
        }
    }
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
    result[QStringLiteral("linkedClipId")] = clip.linkedClipId.value_or(-1);
    result[QStringLiteral("colorTag")] = clip.colorTag.value_or(QString());
    result[QStringLiteral("customLabel")] = clip.customLabel;
    result[QStringLiteral("isReversed")] = clip.isReversed;
    result[QStringLiteral("hasSpeedRamp")] = clip.hasSpeedRamp();
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
void TimelineNotifier::removeClip(int id)    { trackClip_->removeClip(id, state_.rippleMode); }
void TimelineNotifier::moveClip(int id, int t, double time) {
    if (!state_.project) { trackClip_->moveClip(id, t, time); return; }

    // Feature 10: Capture linked clip info BEFORE the move (pointers invalidated after)
    double linkedDelta = 0;
    int linkedId = -1;
    int linkedTrack = -1;
    {
        const auto* clip = state_.project->findClip(id);
        if (clip && clip->linkedClipId.has_value()) {
            linkedId = *clip->linkedClipId;
            const auto* linkedClip = state_.project->findClip(linkedId);
            if (linkedClip) {
                linkedDelta = time - clip->timelineIn;
                linkedTrack = linkedClip->trackIndex;
            }
        }
    }

    trackClip_->moveClip(id, t, time);

    // Feature 10: Move linked clip (safe — uses fresh findClip on updated state)
    if (linkedId >= 0 && linkedTrack >= 0 && state_.project) {
        const auto* movedLinked = state_.project->findClip(linkedId);
        if (movedLinked) {
            double linkedNewTime = std::max(0.0, movedLinked->timelineIn + linkedDelta);
            trackClip_->moveClip(linkedId, linkedTrack, linkedNewTime);
        }
    }

    // Feature 9: Auto-crossfade detection is done via QML context menu
    // (not auto-detected on move, since moveClip resolves collisions)
}
void TimelineNotifier::trimClip(int id, double i, double o) { trackClip_->trimClip(id, i, o); }
void TimelineNotifier::toggleRippleMode() {
    state_.rippleMode = !state_.rippleMode;
    emit stateChanged();
}
void TimelineNotifier::toggleInsertMode() {
    state_.insertMode = !state_.insertMode;
    emit stateChanged();
}
void TimelineNotifier::reorderClipInsert(int clipId, int trackIndex, double atTime) {
    if (!state_.project) return;

    // Find the clip and COPY it before removing
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    auto before = *state_.project;
    VideoClip moved = *clip;  // copy BEFORE removing from track
    const double clipDur = moved.duration();

    // Remove from old position
    for (auto& track : state_.project->tracks) {
        auto it = std::find_if(track.clips.begin(), track.clips.end(),
                               [clipId](const VideoClip& c) { return c.id == clipId; });
        if (it != track.clips.end()) { track.clips.erase(it); break; }
    }

    // Push clips right at insertion point
    if (trackIndex >= 0 && trackIndex < static_cast<int>(state_.project->tracks.size())) {
        auto& destClips = state_.project->tracks[trackIndex].clips;
        for (auto& c : destClips) {
            if (c.timelineIn >= atTime - 0.01) {
                c.timelineIn  += clipDur;
                c.timelineOut += clipDur;
            }
        }

        // Place the copied clip at the insertion point
        moved.trackIndex  = trackIndex;
        moved.timelineIn  = atTime;
        moved.timelineOut = atTime + clipDur;
        destClips.push_back(std::move(moved));
    }

    state_.trackGeneration++;
    setState(std::move(state_));
    pushUndo(before);
    syncNativeToProject();
}
void TimelineNotifier::reorderClipOverwrite(int clipId, int trackIndex, double atTime) {
    // Overwrite: remove from old position and place at new position,
    // trimming/removing anything underneath
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    auto before = *state_.project;
    VideoClip moved = *clip;
    const double clipDur = moved.duration();

    // Remove from old position
    for (auto& track : state_.project->tracks) {
        auto it = std::find_if(track.clips.begin(), track.clips.end(),
                               [clipId](const VideoClip& c) { return c.id == clipId; });
        if (it != track.clips.end()) { track.clips.erase(it); break; }
    }

    // Remove/trim overlapping clips at target
    if (trackIndex >= 0 && trackIndex < static_cast<int>(state_.project->tracks.size())) {
        auto& destClips = state_.project->tracks[trackIndex].clips;
        double rangeEnd = atTime + clipDur;
        QList<VideoClip> kept;
        for (auto& c : destClips) {
            if (c.timelineIn >= atTime && c.timelineOut <= rangeEnd) continue; // fully inside
            if (c.timelineIn < atTime && c.timelineOut > atTime && c.timelineOut <= rangeEnd) {
                c.sourceOut = c.sourceIn + (atTime - c.timelineIn) * c.speed;
                c.timelineOut = atTime;
            } else if (c.timelineIn >= atTime && c.timelineIn < rangeEnd && c.timelineOut > rangeEnd) {
                c.sourceIn += (rangeEnd - c.timelineIn) * c.speed;
                c.timelineIn = rangeEnd;
            }
            kept.append(c);
        }
        destClips = kept;

        moved.trackIndex  = trackIndex;
        moved.timelineIn  = atTime;
        moved.timelineOut = atTime + clipDur;
        destClips.push_back(std::move(moved));
    }

    state_.trackGeneration++;
    setState(std::move(state_));
    pushUndo(before);
    syncNativeToProject();
}
void TimelineNotifier::splitClipAtPlayhead() { trackClip_->splitClipAtPlayhead(); }
void TimelineNotifier::rippleDelete(int id)  { trackClip_->rippleDelete(id); }
void TimelineNotifier::selectClip(int id) {
    // Set selectedClipIds BEFORE delegate call so that when setState()
    // emits selectionChanged, QML sees consistent multi-select state
    state_.selectedClipIds.clear();
    state_.selectedClipIds.insert(id);
    trackClip_->selectClip(id);
}
void TimelineNotifier::deselectClip() {
    state_.selectedClipIds.clear();
    trackClip_->deselectClip();
}
void TimelineNotifier::updateClipDuration(int id, double d) { trackClip_->updateClipDuration(id, d); }
void TimelineNotifier::updateClipDisplayName(int clipId, const QString& name) {
    updateClip(clipId, [&](const VideoClip& c) {
        VideoClip updated = c;
        updated.displayName = name;
        return updated;
    });
}

// ---------------------------------------------------------------------------
// Clip color tag / custom label
// ---------------------------------------------------------------------------

void TimelineNotifier::setClipColorTag(int clipId, const QString& colorHex) {
    trackClip_->setClipColorTag(clipId, colorHex);
}

void TimelineNotifier::clearClipColorTag(int clipId) {
    trackClip_->clearClipColorTag(clipId);
}

void TimelineNotifier::setClipCustomLabel(int clipId, const QString& label) {
    trackClip_->setClipCustomLabel(clipId, label);
}

int TimelineNotifier::clipColorPresetCount() const {
    return gopost::video_editor::kClipColorPresetCount;
}

QString TimelineNotifier::clipColorPresetHex(int index) const {
    return gopost::video_editor::clipColorPresetHex(index);
}

QString TimelineNotifier::clipColorPresetName(int index) const {
    return gopost::video_editor::clipColorPresetName(index);
}

QString TimelineNotifier::autoColorForClipSourceType(int sourceType) const {
    return gopost::video_editor::autoColorForSourceType(
        static_cast<ClipSourceType>(sourceType));
}

// ---------------------------------------------------------------------------
// Track color
// ---------------------------------------------------------------------------

void TimelineNotifier::setTrackColor(int trackIndex, const QString& colorHex) {
    trackClip_->setTrackColor(trackIndex, colorHex);
}

void TimelineNotifier::clearTrackColor(int trackIndex) {
    trackClip_->clearTrackColor(trackIndex);
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

void TimelineNotifier::setClipSpeed(int cid, double s)  { qDebug() << "[TimelineNotifier] setClipSpeed:" << cid << s; advancedEdit_->setClipSpeed(cid, s); }
void TimelineNotifier::reverseClip(int cid)              { qDebug() << "[TimelineNotifier] reverseClip:" << cid; advancedEdit_->reverseClip(cid); }
void TimelineNotifier::freezeFrame(int cid)              { qDebug() << "[TimelineNotifier] freezeFrame:" << cid; advancedEdit_->freezeFrame(cid); }
void TimelineNotifier::duplicateClip(int cid)            { advancedEdit_->duplicateClip(cid); }
void TimelineNotifier::rateStretch(int cid, double d)    { advancedEdit_->rateStretch(cid, d); }
void TimelineNotifier::addMarker() {
    advancedEdit_->addMarker(MarkerType::Chapter);
}
void TimelineNotifier::addMarkerWithType(int t, const QString& l) {
    advancedEdit_->addMarker(static_cast<MarkerType>(t), l);
}
void TimelineNotifier::addMarkerAtPosition(double pos, int t, const QString& l, const QString& c) {
    advancedEdit_->addMarkerAtPosition(pos, static_cast<MarkerType>(t), l, c);
}
void TimelineNotifier::addClipMarker(int clipId, int t, const QString& l) {
    advancedEdit_->addClipMarker(clipId, static_cast<MarkerType>(t), l);
}
void TimelineNotifier::addMarkerRegion(double s, double e, int t, const QString& l) {
    advancedEdit_->addMarkerRegion(s, e, static_cast<MarkerType>(t), l);
}
void TimelineNotifier::removeMarker(int id)     { advancedEdit_->removeMarker(id); }
void TimelineNotifier::updateMarker(int id, const QString& l) { advancedEdit_->updateMarker(id, l); }
void TimelineNotifier::updateMarkerFull(int id, const QString& l, const QString& n, const QString& c, int t) {
    advancedEdit_->updateMarkerFull(id, l, n, c, t);
}
void TimelineNotifier::updateMarkerPosition(int id, double pos) {
    advancedEdit_->updateMarkerPosition(id, pos);
}
void TimelineNotifier::navigateToMarker(int id) { advancedEdit_->navigateToMarker(id); }
void TimelineNotifier::navigateToNextMarker()   { advancedEdit_->navigateToNextMarker(); }
void TimelineNotifier::navigateToPreviousMarker() { advancedEdit_->navigateToPreviousMarker(); }
QVariantMap TimelineNotifier::markerById(int markerId) const {
    if (!state_.project) return {};
    for (const auto& marker : state_.project->markers) {
        if (marker.id == markerId) {
            QVariantMap m;
            m[QStringLiteral("id")] = marker.id;
            m[QStringLiteral("position")] = marker.positionSeconds;
            m[QStringLiteral("type")] = static_cast<int>(marker.type);
            m[QStringLiteral("label")] = marker.label;
            m[QStringLiteral("color")] = marker.color.value_or(TimelineMarker::defaultColorForType(marker.type));
            m[QStringLiteral("notes")] = marker.notes;
            if (marker.endPositionSeconds.has_value())
                m[QStringLiteral("endPosition")] = *marker.endPositionSeconds;
            if (marker.clipId.has_value())
                m[QStringLiteral("clipId")] = *marker.clipId;
            return m;
        }
    }
    return {};
}
void TimelineNotifier::setProjectDimensions(int w, int h) { advancedEdit_->setProjectDimensions(w, h); }
void TimelineNotifier::generateProxyForClip(int cid)      { advancedEdit_->generateProxyForClip(cid); }
void TimelineNotifier::toggleProxyPlayback()               { advancedEdit_->toggleProxyPlayback(); }
void TimelineNotifier::toggleWaveform() {
    state_.showWaveforms = !state_.showWaveforms;
    emit stateChanged();
}
void TimelineNotifier::toggleWaveformStereo() {
    state_.waveformStereo = !state_.waveformStereo;
    emit stateChanged();
}
void TimelineNotifier::applySpeedRamp(int clipId, int presetIndex) {
    qDebug() << "[TimelineNotifier] applySpeedRamp: clipId=" << clipId << "presetIndex=" << presetIndex;
    auto presets = AdvancedEditDelegate::speedRampPresets();
    if (presetIndex >= 0 && presetIndex < static_cast<int>(presets.size()))
        advancedEdit_->applySpeedRamp(clipId, presets[presetIndex]);
    else
        qWarning() << "[TimelineNotifier] applySpeedRamp: invalid presetIndex" << presetIndex;
}

void TimelineNotifier::addSpeedRampPoint(int clipId, double normalizedTime, double speed) {
    advancedEdit_->addSpeedRampPoint(clipId, normalizedTime, speed);
}

void TimelineNotifier::removeSpeedRampPoint(int clipId, double time) {
    advancedEdit_->removeSpeedRampPoint(clipId, time);
}

void TimelineNotifier::moveSpeedRampPoint(int clipId, double oldTime, double newTime, double newSpeed) {
    advancedEdit_->moveSpeedRampPoint(clipId, oldTime, newTime, newSpeed);
}

void TimelineNotifier::clearSpeedRamp(int clipId) {
    advancedEdit_->clearSpeedRamp(clipId);
}

bool TimelineNotifier::clipHasSpeedRamp(int clipId) const {
    if (!state_.project) return false;
    const auto* clip = state_.project->findClip(clipId);
    return clip ? clip->hasSpeedRamp() : false;
}

bool TimelineNotifier::clipIsReversed(int clipId) const {
    if (!state_.project) return false;
    const auto* clip = state_.project->findClip(clipId);
    return clip ? clip->isReversed : false;
}

QVariantList TimelineNotifier::clipSpeedRampPoints(int clipId) const {
    QVariantList result;
    if (!state_.project) return result;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return result;
    const auto* speedTrack = clip->keyframes.trackFor(KeyframeProperty::Speed);
    if (!speedTrack) return result;
    double dur = clip->duration();
    for (const auto& kf : speedTrack->keyframes) {
        QVariantMap pt;
        pt[QStringLiteral("time")] = kf.time;
        pt[QStringLiteral("normalizedTime")] = dur > 0 ? kf.time / dur : 0.0;
        pt[QStringLiteral("speed")] = kf.value;
        pt[QStringLiteral("interpolation")] = static_cast<int>(kf.interpolation);
        result.append(pt);
    }
    return result;
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

    // Bump generations so QML re-evaluates all clip/track bindings
    state_.trackGeneration++;
    state_.selectionGeneration++;
    syncUndoRedoState();

    // Emit all data signals so QML fully refreshes
    emit tracksChanged();
    emit selectionChanged();
    emit playbackChanged();

    syncNativeToProject();
}

void TimelineNotifier::redo() {
    if (!undoStack_->canRedo() || !state_.project) return;
    undoStack_->undoStack.push_back(*state_.project);
    state_.project = std::make_shared<VideoProject>(undoStack_->redoStack.back());
    undoStack_->redoStack.pop_back();

    // Bump generations so QML re-evaluates all clip/track bindings
    state_.trackGeneration++;
    state_.selectionGeneration++;
    syncUndoRedoState();

    // Emit all data signals so QML fully refreshes
    emit tracksChanged();
    emit selectionChanged();
    emit playbackChanged();

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
    if (cachedTracksGen_ == state_.trackGeneration)
        return cachedTracks_;

    cachedTracksGen_ = state_.trackGeneration;
    cachedTracks_.clear();
    if (!state_.project) return cachedTracks_;
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
        cachedTracks_.append(m);
    }
    return cachedTracks_;
}

// ---------------------------------------------------------------------------
// Marker accessors
// ---------------------------------------------------------------------------

int TimelineNotifier::markerCount() const {
    return state_.project ? state_.project->markers.size() : 0;
}

QVariantList TimelineNotifier::markersVariant() const {
    if (cachedMarkersGen_ == state_.trackGeneration)
        return cachedMarkers_;

    cachedMarkersGen_ = state_.trackGeneration;
    cachedMarkers_.clear();
    if (!state_.project) return cachedMarkers_;
    for (const auto& marker : state_.project->markers) {
        QVariantMap m;
        m[QStringLiteral("id")] = marker.id;
        m[QStringLiteral("position")] = marker.positionSeconds;
        m[QStringLiteral("type")] = static_cast<int>(marker.type);
        m[QStringLiteral("label")] = marker.label;
        m[QStringLiteral("color")] = marker.color.value_or(
            TimelineMarker::defaultColorForType(marker.type));
        m[QStringLiteral("notes")] = marker.notes;
        m[QStringLiteral("isRegion")] = marker.isRegion();
        if (marker.endPositionSeconds.has_value())
            m[QStringLiteral("endPosition")] = *marker.endPositionSeconds;
        if (marker.clipId.has_value())
            m[QStringLiteral("clipId")] = *marker.clipId;
        cachedMarkers_.append(m);
    }
    return cachedMarkers_;
}

QVariantMap TimelineNotifier::selectedClipVariant() const {
    if (cachedSelectionGen_ == state_.selectionGeneration &&
        cachedSelectedClip_.contains(QStringLiteral("clipId")) ==
            state_.selectedClipId.has_value()) {
        return cachedSelectedClip_;
    }

    cachedSelectionGen_ = state_.selectionGeneration;
    cachedSelectedClip_.clear();
    if (!state_.project || !state_.selectedClipId.has_value())
        return cachedSelectedClip_;

    int clipId = *state_.selectedClipId;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.id == clipId) {
                cachedSelectedClip_[QStringLiteral("clipId")]      = clip.id;
                cachedSelectedClip_[QStringLiteral("displayName")] = clip.displayName;
                cachedSelectedClip_[QStringLiteral("timelineIn")]  = clip.timelineIn;
                cachedSelectedClip_[QStringLiteral("timelineOut")] = clip.timelineOut;
                cachedSelectedClip_[QStringLiteral("duration")]    = clip.duration();
                cachedSelectedClip_[QStringLiteral("sourceType")]  = static_cast<int>(clip.sourceType);
                cachedSelectedClip_[QStringLiteral("speed")]       = clip.speed;
                cachedSelectedClip_[QStringLiteral("opacity")]     = clip.opacity;
                cachedSelectedClip_[QStringLiteral("sourcePath")]  = clip.sourcePath;
                cachedSelectedClip_[QStringLiteral("trackIndex")]  = clip.trackIndex;
                cachedSelectedClip_[QStringLiteral("blendMode")]   = clip.blendMode;
                cachedSelectedClip_[QStringLiteral("sourceIn")]    = clip.sourceIn;
                cachedSelectedClip_[QStringLiteral("sourceOut")]   = clip.sourceOut;
                return cachedSelectedClip_;
            }
        }
    }
    return cachedSelectedClip_;
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
                    double relTime = pos - clip.timelineIn;
                    return clip.sourceIn + relTime * clip.speed;
                }
            }
        }
    }
    return 0.0;
}

QString TimelineNotifier::activeAudioSource() const {
    if (!state_.project) return {};
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        if (track.type != TrackType::Audio) continue;
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Audio) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    return clip.sourcePath;
                }
            }
        }
    }
    return {};
}

double TimelineNotifier::activeAudioOffset() const {
    if (!state_.project) return 0.0;
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        if (track.type != TrackType::Audio) continue;
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Audio) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    double relTime = pos - clip.timelineIn;
                    return clip.sourceIn + relTime * clip.speed;
                }
            }
        }
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
// Active clip mute/volume at playhead (for preview AudioOutput binding)
// ---------------------------------------------------------------------------

double TimelineNotifier::activeClipSpeed() const {
    if (!state_.project) return 1.0;
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Video ||
                clip.sourceType == ClipSourceType::Image) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    // If clip has speed ramp keyframes, evaluate at current position
                    const auto* speedTrack = clip.keyframes.trackFor(KeyframeProperty::Speed);
                    if (speedTrack && speedTrack->keyframes.size() >= 2) {
                        double relTime = pos - clip.timelineIn;
                        double evaluated = speedTrack->evaluate(relTime);
                        return std::max(0.1, evaluated);
                    }
                    return std::abs(clip.speed) > 0.01 ? std::abs(clip.speed) : 1.0;
                }
            }
        }
    }
    return 1.0;
}

bool TimelineNotifier::activeClipReversed() const {
    if (!state_.project) return false;
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Video ||
                clip.sourceType == ClipSourceType::Image) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    return clip.isReversed;
                }
            }
        }
    }
    return false;
}

double TimelineNotifier::activeClipVolume() const {
    if (!state_.project) return 1.0;
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Video ||
                clip.sourceType == ClipSourceType::Image) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    double vol = clip.audio.volume * track.audioSettings.volume;
                    return vol;
                }
            }
        }
    }
    return 1.0;
}

bool TimelineNotifier::activeClipMuted() const {
    if (!state_.project) return false;
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Video ||
                clip.sourceType == ClipSourceType::Image) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    return clip.audio.isMuted || track.isMuted;
                }
            }
        }
    }
    return false;
}

double TimelineNotifier::activeAudioVolume() const {
    if (!state_.project) return 1.0;
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        if (track.type != TrackType::Audio) continue;
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Audio) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    double vol = clip.audio.volume * track.audioSettings.volume;
                    return vol;
                }
            }
        }
    }
    return 1.0;
}

bool TimelineNotifier::activeAudioMuted() const {
    if (!state_.project) return false;
    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        if (track.type != TrackType::Audio) continue;
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Audio) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    return clip.audio.isMuted || track.isMuted;
                }
            }
        }
    }
    return false;
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

// ===========================================================================
// Feature 9: Auto-Crossfade on Overlap
// ===========================================================================

void TimelineNotifier::addCrossfade(int clipAId, int clipBId, double duration, int type) {
    if (!state_.project) return;
    auto before = *state_.project;

    // Remove any existing crossfade between these clips
    auto& transitions = state_.project->transitions;
    transitions.erase(
        std::remove_if(transitions.begin(), transitions.end(),
            [clipAId, clipBId](const TimelineTransition& t) {
                return (t.clipAId == clipAId && t.clipBId == clipBId) ||
                       (t.clipAId == clipBId && t.clipBId == clipAId);
            }),
        transitions.end());

    TimelineTransition tr;
    tr.clipAId  = clipAId;
    tr.clipBId  = clipBId;
    tr.duration = duration;
    tr.type     = static_cast<CrossfadeType>(type);
    transitions.push_back(tr);

    state_.trackGeneration++;
    setState(std::move(state_));
    pushUndo(before);
}

void TimelineNotifier::removeCrossfade(int clipAId, int clipBId) {
    if (!state_.project) return;
    auto before = *state_.project;

    auto& transitions = state_.project->transitions;
    auto it = std::remove_if(transitions.begin(), transitions.end(),
        [clipAId, clipBId](const TimelineTransition& t) {
            return (t.clipAId == clipAId && t.clipBId == clipBId) ||
                   (t.clipAId == clipBId && t.clipBId == clipAId);
        });
    if (it == transitions.end()) return;
    transitions.erase(it, transitions.end());

    state_.trackGeneration++;
    setState(std::move(state_));
    pushUndo(before);
}

void TimelineNotifier::changeCrossfadeType(int clipAId, int clipBId, int newType) {
    if (!state_.project) return;
    auto before = *state_.project;

    for (auto& t : state_.project->transitions) {
        if ((t.clipAId == clipAId && t.clipBId == clipBId) ||
            (t.clipAId == clipBId && t.clipBId == clipAId)) {
            t.type = static_cast<CrossfadeType>(newType);
            state_.trackGeneration++;
            setState(std::move(state_));
            pushUndo(before);
            return;
        }
    }
}

QVariantMap TimelineNotifier::crossfadeAt(int clipAId, int clipBId) const {
    QVariantMap result;
    if (!state_.project) return result;
    for (const auto& t : state_.project->transitions) {
        if ((t.clipAId == clipAId && t.clipBId == clipBId) ||
            (t.clipAId == clipBId && t.clipBId == clipAId)) {
            result[QStringLiteral("clipAId")]  = t.clipAId;
            result[QStringLiteral("clipBId")]  = t.clipBId;
            result[QStringLiteral("duration")] = t.duration;
            result[QStringLiteral("type")]     = static_cast<int>(t.type);
            return result;
        }
    }
    return result;
}

QVariantList TimelineNotifier::transitionsVariant() const {
    QVariantList result;
    if (!state_.project) return result;
    for (const auto& t : state_.project->transitions) {
        QVariantMap m;
        m[QStringLiteral("clipAId")]  = t.clipAId;
        m[QStringLiteral("clipBId")]  = t.clipBId;
        m[QStringLiteral("duration")] = t.duration;
        m[QStringLiteral("type")]     = static_cast<int>(t.type);
        result.append(m);
    }
    return result;
}

// ===========================================================================
// Feature 10: Linked Audio/Video Movement
// ===========================================================================

void TimelineNotifier::linkClips(int clipAId, int clipBId) {
    if (!state_.project) return;
    auto before = *state_.project;

    bool foundA = false, foundB = false;
    for (auto& track : state_.project->tracks) {
        for (auto& clip : track.clips) {
            if (clip.id == clipAId) { clip.linkedClipId = clipBId; foundA = true; }
            if (clip.id == clipBId) { clip.linkedClipId = clipAId; foundB = true; }
        }
    }
    if (!foundA || !foundB) return;

    state_.trackGeneration++;
    setState(std::move(state_));
    pushUndo(before);
}

void TimelineNotifier::unlinkClip(int clipId) {
    if (!state_.project) return;
    auto before = *state_.project;

    int linkedId = -1;
    for (auto& track : state_.project->tracks) {
        for (auto& clip : track.clips) {
            if (clip.id == clipId && clip.linkedClipId.has_value()) {
                linkedId = *clip.linkedClipId;
                clip.linkedClipId.reset();
            }
        }
    }
    // Also clear the other end
    if (linkedId >= 0) {
        for (auto& track : state_.project->tracks) {
            for (auto& clip : track.clips) {
                if (clip.id == linkedId && clip.linkedClipId.has_value() &&
                    *clip.linkedClipId == clipId) {
                    clip.linkedClipId.reset();
                }
            }
        }
    }

    state_.trackGeneration++;
    setState(std::move(state_));
    pushUndo(before);
}

int TimelineNotifier::linkedClipId(int clipId) const {
    if (!state_.project) return -1;
    const auto* clip = state_.project->findClip(clipId);
    return (clip && clip->linkedClipId.has_value()) ? *clip->linkedClipId : -1;
}

bool TimelineNotifier::isClipLinked(int clipId) const {
    if (!state_.project) return false;
    const auto* clip = state_.project->findClip(clipId);
    return clip && clip->linkedClipId.has_value();
}

// ===========================================================================
// Feature 11: Timeline Zoom (enhanced)
// ===========================================================================

void TimelineNotifier::setZoomPercentage(double percent) {
    double pps = (percent / 100.0) * 80.0;
    setPixelsPerSecond(pps);
}

// ===========================================================================
// Feature 12: Split Clip (enhanced - all unlocked tracks)
// ===========================================================================

void TimelineNotifier::splitAllAtPlayhead() {
    if (!state_.project) return;
    const double pos = state_.playback.positionSeconds;
    auto before = *state_.project;
    bool anySplit = false;

    for (auto& track : state_.project->tracks) {
        if (track.isLocked) continue;
        for (int i = 0; i < static_cast<int>(track.clips.size()); ++i) {
            auto& clip = track.clips[i];
            if (pos > clip.timelineIn + 0.001 && pos < clip.timelineOut - 0.001) {
                const double timelineElapsed = pos - clip.timelineIn;
                const double sourceElapsed   = timelineElapsed * clip.speed;

                VideoClip secondHalf    = clip;
                secondHalf.id           = trackClip_->nextClipId();
                secondHalf.timelineIn   = pos;
                secondHalf.sourceIn     = clip.sourceIn + sourceElapsed;

                clip.timelineOut = pos;
                clip.sourceOut   = clip.sourceIn + sourceElapsed;

                track.clips.insert(track.clips.begin() + i + 1, std::move(secondHalf));
                anySplit = true;
                break; // only one clip per track can be under playhead
            }
        }
    }

    if (anySplit) {
        state_.trackGeneration++;
        setState(std::move(state_));
        pushUndo(before);
        syncNativeToProject();
        renderCurrentFrame();
    }
}

// ===========================================================================
// Feature 13: Multi-Select and Group Drag
// ===========================================================================

void TimelineNotifier::addToSelection(int clipId) {
    auto state = state_;  // copy current state
    state.selectedClipIds.insert(clipId);
    state.selectedClipId = clipId;  // primary selection for property panel
    state.selectionGeneration++;
    setState(std::move(state));
}

void TimelineNotifier::removeFromSelection(int clipId) {
    auto state = state_;
    state.selectedClipIds.remove(clipId);
    if (state.selectedClipId.has_value() && *state.selectedClipId == clipId) {
        state.selectedClipId = state.selectedClipIds.isEmpty()
            ? std::optional<int>{}
            : std::optional<int>{*state.selectedClipIds.begin()};
    }
    state.selectionGeneration++;
    setState(std::move(state));
}

void TimelineNotifier::toggleClipSelection(int clipId) {
    if (state_.selectedClipIds.contains(clipId))
        removeFromSelection(clipId);
    else
        addToSelection(clipId);
}

void TimelineNotifier::selectAll() {
    if (!state_.project) return;
    auto state = state_;
    state.selectedClipIds.clear();
    for (const auto& track : state.project->tracks) {
        for (const auto& clip : track.clips) {
            state.selectedClipIds.insert(clip.id);
        }
    }
    if (!state.selectedClipIds.isEmpty())
        state.selectedClipId = *state.selectedClipIds.begin();
    state.selectionGeneration++;
    setState(std::move(state));
}

void TimelineNotifier::deselectAll() {
    auto state = state_;
    state.selectedClipIds.clear();
    state.selectedClipId.reset();
    state.selectionGeneration++;
    setState(std::move(state));
}

bool TimelineNotifier::isClipSelected(int clipId) const {
    if (clipId < 0) return false;
    return state_.selectedClipIds.contains(clipId);
}

void TimelineNotifier::moveSelectedClips(double deltaTime, int deltaTrack) {
    if (!state_.project || state_.selectedClipIds.isEmpty()) return;
    if (std::abs(deltaTime) < 0.001 && deltaTrack == 0) return;  // no-op
    auto before = *state_.project;

    // Clamp deltaTime so the earliest selected clip doesn't go below 0
    double earliestIn = 1e12;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (state_.selectedClipIds.contains(clip.id))
                earliestIn = std::min(earliestIn, clip.timelineIn);
        }
    }
    double clampedDelta = deltaTime;
    if (earliestIn + clampedDelta < 0)
        clampedDelta = -earliestIn;

    // Apply time shift and track change to all selected clips
    for (auto& track : state_.project->tracks) {
        for (auto& clip : track.clips) {
            if (!state_.selectedClipIds.contains(clip.id)) continue;

            clip.timelineIn  += clampedDelta;
            clip.timelineOut += clampedDelta;

            int newTrack = clip.trackIndex + deltaTrack;
            newTrack = std::clamp(newTrack, 0,
                static_cast<int>(state_.project->tracks.size()) - 1);
            clip.trackIndex = newTrack;
        }
    }

    // Re-distribute clips to their target tracks
    if (deltaTrack != 0) {
        QList<VideoClip> allMoving;
        for (auto& track : state_.project->tracks) {
            QList<VideoClip> kept;
            for (auto& clip : track.clips) {
                if (state_.selectedClipIds.contains(clip.id) && clip.trackIndex != track.index) {
                    allMoving.push_back(clip);
                } else {
                    kept.push_back(clip);
                }
            }
            track.clips = kept;
        }
        for (auto& clip : allMoving) {
            if (clip.trackIndex >= 0 && clip.trackIndex < static_cast<int>(state_.project->tracks.size())) {
                state_.project->tracks[clip.trackIndex].clips.push_back(clip);
            }
        }
    }

    state_.trackGeneration++;
    setState(std::move(state_));
    pushUndo(before);
    syncNativeToProject();
}

void TimelineNotifier::deleteSelectedClips() {
    if (!state_.project || state_.selectedClipIds.isEmpty()) return;
    auto before = *state_.project;

    for (auto& track : state_.project->tracks) {
        if (track.isLocked) continue;
        QList<VideoClip> kept;
        for (const auto& clip : track.clips) {
            if (!state_.selectedClipIds.contains(clip.id))
                kept.push_back(clip);
        }
        track.clips = kept;
    }

    state_.selectedClipIds.clear();
    state_.selectedClipId.reset();
    state_.trackGeneration++;
    state_.selectionGeneration++;
    setState(std::move(state_));
    pushUndo(before);
    syncNativeToProject();
}

QVariantList TimelineNotifier::selectedClipIdsVariant() const {
    QVariantList result;
    for (int id : state_.selectedClipIds)
        result.append(id);
    return result;
}

// ===========================================================================
// Feature 15: Track header editing
// ===========================================================================

void TimelineNotifier::renameTrack(int trackIndex, const QString& name) {
    if (!state_.project) return;
    auto& tracks = state_.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    auto before = *state_.project;
    tracks[trackIndex].label = name;
    state_.trackGeneration++;
    emit tracksChanged();
    pushUndo(before);
}

QVariantMap TimelineNotifier::trackInfo(int trackIndex) const {
    QVariantMap result;
    if (!state_.project) return result;
    const auto& tracks = state_.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
        qDebug() << "[TimelineNotifier] trackInfo: index" << trackIndex << "out of range (size:" << tracks.size() << ")";
        return result;
    }
    const auto& track = tracks[trackIndex];
    result[QStringLiteral("index")]     = track.index;
    result[QStringLiteral("label")]     = track.label;
    result[QStringLiteral("type")]      = static_cast<int>(track.type);
    result[QStringLiteral("isVisible")] = track.isVisible;
    result[QStringLiteral("isLocked")]  = track.isLocked;
    result[QStringLiteral("isMuted")]   = track.isMuted;
    result[QStringLiteral("isSolo")]    = track.isSolo;
    result[QStringLiteral("color")]     = track.color.value_or(QString());
    return result;
}

} // namespace gopost::video_editor
