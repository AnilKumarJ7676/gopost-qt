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

    // Render debounce timer — coalesces rapid render requests.
    // 150ms gives batch operations (add clips → select → autoFit) time to
    // settle before triggering a full engine rebuild + render.  The old 50ms
    // interval wasn't enough: a multi-file drop would fire 3-4 syncs.
    renderDebounceTimer_.setSingleShot(true);
    renderDebounceTimer_.setInterval(150);
    connect(&renderDebounceTimer_, &QTimer::timeout, this, [this] {
        renderCurrentFrame();
    });

    // Auto-save timer (Version History)
    autoSaveTimer_.setInterval(autoSaveIntervalMs_);
    connect(&autoSaveTimer_, &QTimer::timeout, this, [this] {
        if (state_.project && state_.isReady()) {
            saveVersion(QStringLiteral("Auto-save"));
        }
    });
    autoSaveTimer_.start();
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
    videoTrack.isMagneticPrimary = true;  // Primary storyline (magnetic timeline)

    VideoTrack audioTrack;
    audioTrack.type  = TrackType::Audio;
    audioTrack.index = 2;
    audioTrack.label = QStringLiteral("Audio 1");

    project->tracks = {effectTrack, videoTrack, audioTrack};

    state_.project = std::move(project);
    state_.phase   = TimelinePhase::ready;
    syncNativeToProjectImmediate();  // immediate on init — engine must be ready
    playbackTimer_.start();

    // Initialize first timeline tab
    timelineTabs_.clear();
    activeTabIndex_ = 0;
    nextTabId_ = 1;
    stackedView_ = false;
    stackedSecondaryIndex_ = -1;
    TimelineTab firstTab;
    firstTab.id = nextTabId_++;
    firstTab.label = QStringLiteral("Timeline 1");
    firstTab.state = state_;
    firstTab.engineTimelineId = engineTimelineId_;
    timelineTabs_.append(std::move(firstTab));

    // Emit all signals on init so QML picks up the full initial state
    emit stateChanged();
    emit playbackChanged();
    emit tracksChanged();
    emit selectionChanged();
    emit zoomChanged();
    emit tabsChanged();

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
    syncNativeToProjectImmediate();  // immediate on load
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
        state.phase                 != state_.phase ||
        state.canUndo               != state_.canUndo ||
        state.canRedo               != state_.canRedo ||
        state.activePanel           != state_.activePanel ||
        state.useProxyPlayback      != state_.useProxyPlayback ||
        state.errorMessage          != state_.errorMessage ||
        state.magneticTimelineEnabled != state_.magneticTimelineEnabled ||
        state.positionOverrideActive  != state_.positionOverrideActive;

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
    // Lazy sync: rebuild engine only when a frame is actually requested
    if (syncNeeded_) {
        syncNativeToProjectImmediate();
    }

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
    // Use render debounce timer to coalesce rapid calls
    renderDebounceTimer_.start();
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
    // Lazy sync: just mark dirty. Actual rebuild happens in renderCurrentFrame()
    // when a frame is actually needed (playback, scrub, seek).
    syncNeeded_ = true;
}

void TimelineNotifier::syncNativeToProjectImmediate() {
    if (!stubEngine_ || !state_.project) return;
    syncNeeded_ = false;

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

    // Sync track-level state (mute, solo, volume, pan) to engine
    for (const auto& track : state_.project->tracks) {
        stubEngine_->setTrackMute(engineTimelineId_, track.index, track.isMuted);
        stubEngine_->setTrackSolo(engineTimelineId_, track.index, track.isSolo);
        stubEngine_->setTrackVolume(engineTimelineId_, track.index, track.audioSettings.volume);
        stubEngine_->setTrackPan(engineTimelineId_, track.index, track.audioSettings.pan);
    }

    // Sync clip data to engine — skip clips with default state for performance
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            // Only sync clips that have non-default effects/transitions/keyframes
            if (clip.hasEffects() || clip.hasColorGrading() || clip.hasTransition() ||
                clip.hasKeyframes() || clip.hasAudioMod() ||
                clip.audio.volume != 1.0 || clip.audio.pan != 0.0) {
                syncEffectsToEngine(clip.id);
            }
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
        cg.vibrance, cg.hue, cg.fade, cg.vignette);

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

int TimelineNotifier::shuttleSpeed() const {
    return playback_ ? playback_->playbackRate() == 1.0 && !state_.playback.isPlaying ? 0
        : static_cast<int>(std::round(std::log2(std::abs(playback_->playbackRate())) + 1)
            * (playback_->playbackRate() >= 0 ? 1 : -1))
        : 0;
}

QString TimelineNotifier::shuttleSpeedDisplay() const {
    if (!playback_ || !state_.playback.isPlaying) return {};
    double rate = playback_->playbackRate();
    if (std::abs(rate - 1.0) < 0.01) return {};
    if (rate > 0) {
        QString arrows = rate >= 8 ? QStringLiteral(">>>>")
                       : rate >= 4 ? QStringLiteral(">>>")
                       : rate >= 2 ? QStringLiteral(">>")
                       : QStringLiteral(">");
        return arrows + QStringLiteral(" %1x").arg(QString::number(rate, 'g', 3));
    } else {
        double absRate = std::abs(rate);
        QString arrows = absRate >= 8 ? QStringLiteral("<<<<")
                       : absRate >= 4 ? QStringLiteral("<<<")
                       : absRate >= 2 ? QStringLiteral("<<")
                       : QStringLiteral("<");
        return arrows + QStringLiteral(" %1x").arg(QString::number(absRate, 'g', 3));
    }
}

void TimelineNotifier::playInToOut() {
    if (!state_.project) return;
    if (!state_.playback.inPoint.has_value() || !state_.playback.outPoint.has_value()) {
        // No in/out set — just play from current
        playback_->play();
        return;
    }
    playback_->seek(*state_.playback.inPoint);
    playback_->play();
}

void TimelineNotifier::playAroundCurrent(double prerollSec, double postrollSec) {
    if (!state_.project) return;
    double pos = state_.playback.positionSeconds;
    double startPos = std::max(0.0, pos - prerollSec);
    playback_->seek(startPos);
    playback_->play();
    // Note: playback will auto-stop at outPoint or timeline end
}

void TimelineNotifier::jumpToPreviousEditPoint() {
    if (!state_.project) return;
    double current = state_.playback.positionSeconds;
    double bestPos = 0.0;
    constexpr double kEpsilon = 0.001;

    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.timelineIn < current - kEpsilon && clip.timelineIn > bestPos)
                bestPos = clip.timelineIn;
            if (clip.timelineOut < current - kEpsilon && clip.timelineOut > bestPos)
                bestPos = clip.timelineOut;
        }
    }
    playback_->seek(bestPos);
}

void TimelineNotifier::jumpToNextEditPoint() {
    if (!state_.project) return;
    double current = state_.playback.positionSeconds;
    double duration = state_.project->duration();
    double bestPos = duration;
    constexpr double kEpsilon = 0.001;

    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.timelineIn > current + kEpsilon && clip.timelineIn < bestPos)
                bestPos = clip.timelineIn;
            if (clip.timelineOut > current + kEpsilon && clip.timelineOut < bestPos)
                bestPos = clip.timelineOut;
        }
    }
    playback_->seek(bestPos);
}

void TimelineNotifier::jumpToPreviousMarkerOnly() {
    advancedEdit_->navigateToPreviousMarker();
}

void TimelineNotifier::jumpToNextMarkerOnly() {
    advancedEdit_->navigateToNextMarker();
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
    result[QStringLiteral("isFreezeFrame")] = clip.isFreezeFrame;
    result[QStringLiteral("hasEffects")] = clip.hasEffects();
    result[QStringLiteral("effectCount")] = static_cast<int>(clip.effects.size());
    result[QStringLiteral("hasColorGrading")] = clip.hasColorGrading();
    result[QStringLiteral("hasTransition")] = clip.hasTransition();
    result[QStringLiteral("transitionInType")] = static_cast<int>(clip.transitionIn.type);
    result[QStringLiteral("transitionOutType")] = static_cast<int>(clip.transitionOut.type);
    result[QStringLiteral("transitionInDur")] = clip.transitionIn.durationSeconds;
    result[QStringLiteral("transitionOutDur")] = clip.transitionOut.durationSeconds;
    result[QStringLiteral("connectedToPrimaryClipId")] = clip.connectedToPrimaryClipId.value_or(-1);
    result[QStringLiteral("sourceDuration")] = clip.sourceDuration;
    result[QStringLiteral("sourceIn")] = clip.sourceIn;
    result[QStringLiteral("sourceOut")] = clip.sourceOut;
    result[QStringLiteral("isDisabled")] = clip.isDisabled;
    return result;
}

void TimelineNotifier::addTrack(int trackType) {
    trackClip_->addTrack(static_cast<TrackType>(trackType));
}
void TimelineNotifier::removeTrack(int i)            { trackClip_->removeTrack(i); }
void TimelineNotifier::toggleTrackVisibility(int i)  { trackClip_->toggleTrackVisibility(i); }
void TimelineNotifier::toggleTrackLock(int i)        { trackClip_->toggleTrackLock(i); }
void TimelineNotifier::toggleTrackMute(int i)        { trackClip_->toggleTrackMute(i); }
void TimelineNotifier::toggleTrackSolo(int i) {
    trackClip_->toggleTrackSolo(i);
    // Sync all track solo/mute state to engine so rendering reflects solo mode
    if (stubEngine_ && engineTimelineId_ >= 0 && state_.project) {
        for (const auto& track : state_.project->tracks) {
            stubEngine_->setTrackSolo(engineTimelineId_, track.index, track.isSolo);
            stubEngine_->setTrackMute(engineTimelineId_, track.index, track.isMuted);
        }
        debouncedRenderFrame();
    }
}
void TimelineNotifier::soloSelectedTrack() {
    if (!state_.project || !state_.selectedClipId.has_value()) return;
    const auto* clip = state_.project->findClip(*state_.selectedClipId);
    if (clip) toggleTrackSolo(clip->trackIndex);
}
int  TimelineNotifier::addClip(int ti, int st, const QString& sp,
                               const QString& dn, double dur) {
    return trackClip_->addClip(ti, static_cast<ClipSourceType>(st), sp, dn, dur);
}
QVariantList TimelineNotifier::addClipsBatch(int trackIndex, const QVariantList& clips) {
    QVariantList result;
    QList<ClipSourceType> types;
    QStringList paths, names;
    QList<double> durs;
    for (const auto& v : clips) {
        auto m = v.toMap();
        types.append(static_cast<ClipSourceType>(m.value(QStringLiteral("sourceType"), 0).toInt()));
        paths.append(m.value(QStringLiteral("sourcePath")).toString());
        names.append(m.value(QStringLiteral("displayName"), QStringLiteral("Untitled")).toString());
        durs.append(m.value(QStringLiteral("duration"), 5.0).toDouble());
    }
    auto ids = trackClip_->addClipsBatch(trackIndex, types, paths, names, durs);
    for (int id : ids) result.append(id);
    return result;
}

void TimelineNotifier::removeClip(int id) {
    // Magnetic timeline: disconnect connected clips only if removing from a magnetic primary track
    if (state_.project && state_.magneticTimelineEnabled) {
        const auto* clip = state_.project->findClip(id);
        if (clip && clip->trackIndex >= 0 &&
            clip->trackIndex < static_cast<int>(state_.project->tracks.size()) &&
            state_.project->tracks[clip->trackIndex].isMagneticPrimary) {
            for (auto& track : state_.project->tracks) {
                for (auto& c : track.clips) {
                    if (c.connectedToPrimaryClipId.has_value() &&
                        *c.connectedToPrimaryClipId == id) {
                        c.connectedToPrimaryClipId.reset();
                        c.syncPointOffset.reset();
                    }
                }
            }
        }
    }
    trackClip_->removeClip(id, state_.rippleMode);
}
void TimelineNotifier::moveClip(int id, int t, double time) {
    if (!state_.project) { trackClip_->moveClip(id, t, time); return; }

    // Capture old position for connected clip delta
    double oldTimelineIn = 0;
    {
        const auto* clip = state_.project->findClip(id);
        if (clip) oldTimelineIn = clip->timelineIn;
    }

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

    // Magnetic timeline: move connected clips only if this clip is on a magnetic primary track
    if (state_.magneticTimelineEnabled && state_.project) {
        const auto* movedClip = state_.project->findClip(id);
        if (movedClip &&
            movedClip->trackIndex >= 0 &&
            movedClip->trackIndex < static_cast<int>(state_.project->tracks.size()) &&
            state_.project->tracks[movedClip->trackIndex].isMagneticPrimary) {
            double delta = movedClip->timelineIn - oldTimelineIn;
            if (std::abs(delta) > 0.001)
                trackClip_->moveConnectedClips(id, delta);
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
void TimelineNotifier::setTrimEditMode(int mode) {
    auto m = static_cast<TrimEditMode>(std::clamp(mode, 0, 4));
    if (state_.trimEditMode == m) {
        state_.trimEditMode = TrimEditMode::Normal;  // toggle off
    } else {
        state_.trimEditMode = m;
    }
    // Sync rippleMode with trim edit mode for backwards compatibility
    state_.rippleMode = (state_.trimEditMode == TrimEditMode::Ripple);
    // Deactivate razor mode when entering a trim edit mode
    if (state_.trimEditMode != TrimEditMode::Normal)
        state_.razorModeEnabled = false;
    emit stateChanged();
}
void TimelineNotifier::rollEdit(int clipId, bool leftEdge, double delta) {
    advancedEdit_->rollEdit(clipId, leftEdge, delta);
}
void TimelineNotifier::slipEdit(int clipId, double delta) {
    advancedEdit_->slipEdit(clipId, delta);
}
void TimelineNotifier::slideEdit(int clipId, double delta) {
    advancedEdit_->slideEdit(clipId, delta);
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
        if (it != track.clips.end()) {
            track.clips.erase(it);
            // Magnetic timeline: compact source track if it's magnetic primary
            if (track.isMagneticPrimary && state_.magneticTimelineEnabled &&
                !state_.positionOverrideActive) {
                TrackClipDelegate::compactTrack(track.clips);
            }
            break;
        }
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

// Gap management
void TimelineNotifier::closeTrackGaps(int ti) { trackClip_->closeTrackGaps(ti); syncNativeToProject(); }
void TimelineNotifier::deleteAllGaps()        { trackClip_->deleteAllGaps(); }
void TimelineNotifier::closeGapAt(int ti, double gs) { trackClip_->closeGapAt(ti, gs); }
void TimelineNotifier::insertSlugAt(int ti, double gs, double gd) { trackClip_->insertSlugAt(ti, gs, gd); }

QVariantList TimelineNotifier::detectGaps(int trackIndex) const {
    QVariantList result;
    if (!state_.project) return result;
    const auto& tracks = state_.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return result;
    const auto& clips = tracks[trackIndex].clips;
    if (clips.isEmpty()) return result;

    // Build sorted list of clip intervals
    QList<std::pair<double, double>> intervals;
    for (const auto& c : clips)
        intervals.append({c.timelineIn, c.timelineOut});
    std::sort(intervals.begin(), intervals.end());

    // Check gap before first clip
    if (intervals.first().first > 0.01) {
        QVariantMap gap;
        gap[QStringLiteral("start")]    = 0.0;
        gap[QStringLiteral("end")]      = intervals.first().first;
        gap[QStringLiteral("duration")] = intervals.first().first;
        result.append(gap);
    }

    // Check gaps between consecutive clips
    for (int i = 1; i < intervals.size(); ++i) {
        double prevEnd = intervals[i - 1].second;
        double nextStart = intervals[i].first;
        if (nextStart - prevEnd > 0.01) {
            QVariantMap gap;
            gap[QStringLiteral("start")]    = prevEnd;
            gap[QStringLiteral("end")]      = nextStart;
            gap[QStringLiteral("duration")] = nextStart - prevEnd;
            result.append(gap);
        }
    }
    return result;
}

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

void TimelineNotifier::reorderEffect(int clipId, int fromIndex, int toIndex) {
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    auto effects = clip->effects;
    if (fromIndex < 0 || fromIndex >= effects.size() || toIndex < 0 || toIndex >= effects.size()) return;
    if (fromIndex == toIndex) return;
    auto before = *state_.project;
    auto moved = effects.takeAt(fromIndex);
    effects.insert(toIndex, moved);
    updateClip(clipId, [&effects](const VideoClip& c) {
        VideoClip u = c;
        u.effects = effects;
        return u;
    });
    pushUndo(before);
    syncEffectsToEngine(clipId);
}

void TimelineNotifier::copyEffects(int clipId) {
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    clipboardEffects_ = clip->effects;
    clipboardColorGrading_ = clip->colorGrading;
}

void TimelineNotifier::pasteEffects(int clipId) {
    if (!state_.project || clipboardEffects_.isEmpty()) return;
    auto before = *state_.project;
    updateClip(clipId, [this](const VideoClip& c) {
        VideoClip u = c;
        u.effects = clipboardEffects_;
        u.colorGrading = clipboardColorGrading_;
        return u;
    });
    pushUndo(before);
    syncEffectsToEngine(clipId);
}

bool TimelineNotifier::hasClipboardEffects() const {
    return !clipboardEffects_.isEmpty();
}

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
    else if (prop == QStringLiteral("exposure"))     cg.exposure    = value;
    else if (prop == QStringLiteral("temperature"))  cg.temperature = value;
    else if (prop == QStringLiteral("tint"))          cg.tint        = value;
    else if (prop == QStringLiteral("shadows"))       cg.shadows     = value;
    else if (prop == QStringLiteral("highlights"))    cg.highlights  = value;
    else if (prop == QStringLiteral("vibrance"))      cg.vibrance    = value;
    else if (prop == QStringLiteral("hue"))            cg.hue         = value;
    else if (prop == QStringLiteral("fade"))           cg.fade        = value;
    else if (prop == QStringLiteral("vignette"))       cg.vignette    = value;
    else return;
    effectColor_->setColorGrading(clipId, cg);
}
void TimelineNotifier::setClipOpacity(int cid, double o) { effectColor_->setClipOpacity(cid, o); }
void TimelineNotifier::setClipBlendMode(int cid, int m)  { effectColor_->setClipBlendMode(cid, m); }

void TimelineNotifier::setTransitionIn(int cid, int tt, double dur, int easing) {
    qDebug() << "[TimelineNotifier] setTransitionIn: clip=" << cid << "type=" << tt << "dur=" << dur << "easing=" << easing;
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

void TimelineNotifier::applyDefaultTransition(int clipId) {
    // Apply Cross Dissolve (type 2) with 0.5s duration as default
    ClipTransition t;
    t.type = TransitionType::Dissolve;
    t.durationSeconds = 0.5;
    t.easing = EasingCurve::EaseInOut;
    effectColor_->setTransitionIn(clipId, t);
}

QVariantList TimelineNotifier::transitionLibrary() const {
    static const QVariantList lib = {
        QVariantMap{{QStringLiteral("type"), 1},  {QStringLiteral("name"), QStringLiteral("Fade")},          {QStringLiteral("category"), QStringLiteral("Basic")}},
        QVariantMap{{QStringLiteral("type"), 2},  {QStringLiteral("name"), QStringLiteral("Cross Dissolve")},{QStringLiteral("category"), QStringLiteral("Basic")}},
        QVariantMap{{QStringLiteral("type"), 20}, {QStringLiteral("name"), QStringLiteral("Dip to Black")},  {QStringLiteral("category"), QStringLiteral("Basic")}},
        QVariantMap{{QStringLiteral("type"), 3},  {QStringLiteral("name"), QStringLiteral("Slide Left")},    {QStringLiteral("category"), QStringLiteral("Slide")}},
        QVariantMap{{QStringLiteral("type"), 4},  {QStringLiteral("name"), QStringLiteral("Slide Right")},   {QStringLiteral("category"), QStringLiteral("Slide")}},
        QVariantMap{{QStringLiteral("type"), 5},  {QStringLiteral("name"), QStringLiteral("Slide Up")},      {QStringLiteral("category"), QStringLiteral("Slide")}},
        QVariantMap{{QStringLiteral("type"), 6},  {QStringLiteral("name"), QStringLiteral("Slide Down")},    {QStringLiteral("category"), QStringLiteral("Slide")}},
        QVariantMap{{QStringLiteral("type"), 7},  {QStringLiteral("name"), QStringLiteral("Wipe Left")},     {QStringLiteral("category"), QStringLiteral("Wipe")}},
        QVariantMap{{QStringLiteral("type"), 8},  {QStringLiteral("name"), QStringLiteral("Wipe Right")},    {QStringLiteral("category"), QStringLiteral("Wipe")}},
        QVariantMap{{QStringLiteral("type"), 9},  {QStringLiteral("name"), QStringLiteral("Wipe Up")},       {QStringLiteral("category"), QStringLiteral("Wipe")}},
        QVariantMap{{QStringLiteral("type"), 10}, {QStringLiteral("name"), QStringLiteral("Wipe Down")},     {QStringLiteral("category"), QStringLiteral("Wipe")}},
        QVariantMap{{QStringLiteral("type"), 11}, {QStringLiteral("name"), QStringLiteral("Zoom")},          {QStringLiteral("category"), QStringLiteral("Motion")}},
        QVariantMap{{QStringLiteral("type"), 12}, {QStringLiteral("name"), QStringLiteral("Push")},          {QStringLiteral("category"), QStringLiteral("Motion")}},
        QVariantMap{{QStringLiteral("type"), 13}, {QStringLiteral("name"), QStringLiteral("Reveal")},        {QStringLiteral("category"), QStringLiteral("Motion")}},
        QVariantMap{{QStringLiteral("type"), 14}, {QStringLiteral("name"), QStringLiteral("Iris")},          {QStringLiteral("category"), QStringLiteral("Creative")}},
        QVariantMap{{QStringLiteral("type"), 15}, {QStringLiteral("name"), QStringLiteral("Clock Wipe")},    {QStringLiteral("category"), QStringLiteral("Creative")}},
        QVariantMap{{QStringLiteral("type"), 16}, {QStringLiteral("name"), QStringLiteral("Blur")},          {QStringLiteral("category"), QStringLiteral("Creative")}},
        QVariantMap{{QStringLiteral("type"), 17}, {QStringLiteral("name"), QStringLiteral("Glitch")},        {QStringLiteral("category"), QStringLiteral("Creative")}},
        QVariantMap{{QStringLiteral("type"), 18}, {QStringLiteral("name"), QStringLiteral("Morph")},         {QStringLiteral("category"), QStringLiteral("Creative")}},
        QVariantMap{{QStringLiteral("type"), 19}, {QStringLiteral("name"), QStringLiteral("Flash")},         {QStringLiteral("category"), QStringLiteral("Creative")}},
    };
    return lib;
}

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
void TimelineNotifier::addFreezeFrame(int cid, double d) { advancedEdit_->addFreezeFrame(cid, d, false); }
void TimelineNotifier::freezeAndExtend(int cid, double d){ advancedEdit_->addFreezeFrame(cid, d, true); }
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

// ---------------------------------------------------------------------------
// Duplicate Frame Detection
// ---------------------------------------------------------------------------
void TimelineNotifier::toggleDuplicateFrameDetection() {
    state_.showDuplicateFrames = !state_.showDuplicateFrames;
    emit stateChanged();
}

QVariantList TimelineNotifier::duplicateFrameOverlaps(int clipId) const {
    QVariantList result;
    if (!state_.project) return result;

    // Find the target clip
    const VideoClip* target = state_.project->findClip(clipId);
    if (!target) return result;

    // Only video/image/audio clips have meaningful source ranges
    if (target->sourcePath.isEmpty()) return result;

    // Compare against all other clips on all tracks
    for (const auto& track : state_.project->tracks) {
        for (const auto& other : track.clips) {
            if (other.id == clipId) continue;
            if (other.sourcePath != target->sourcePath) continue;

            // Check if source frame ranges overlap:
            // Clip A uses [sourceIn, sourceOut) from source file
            // Clip B uses [sourceIn, sourceOut) from source file
            // They overlap if A.sourceIn < B.sourceOut && B.sourceIn < A.sourceOut
            if (target->sourceIn < other.sourceOut && other.sourceIn < target->sourceOut) {
                QVariantMap overlap;
                overlap[QStringLiteral("clipId")] = other.id;
                overlap[QStringLiteral("displayName")] = other.displayName;
                overlap[QStringLiteral("trackIndex")] = other.trackIndex;
                // Overlapping region in source time
                double overlapStart = std::max(target->sourceIn, other.sourceIn);
                double overlapEnd = std::min(target->sourceOut, other.sourceOut);
                overlap[QStringLiteral("overlapStart")] = overlapStart;
                overlap[QStringLiteral("overlapEnd")] = overlapEnd;
                overlap[QStringLiteral("overlapDuration")] = overlapEnd - overlapStart;
                result.append(overlap);
            }
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Tabbed / Stacked Timelines
// ---------------------------------------------------------------------------
void TimelineNotifier::saveActiveTabState() {
    if (activeTabIndex_ >= 0 && activeTabIndex_ < timelineTabs_.size()) {
        timelineTabs_[activeTabIndex_].state = state_;
        timelineTabs_[activeTabIndex_].engineTimelineId = engineTimelineId_;
    }
}

void TimelineNotifier::loadTabState(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= timelineTabs_.size()) return;
    state_ = timelineTabs_[tabIndex].state;
    engineTimelineId_ = timelineTabs_[tabIndex].engineTimelineId;

    // Reset caches so QML re-reads everything
    cachedTracksGen_ = -1;
    cachedMarkersGen_ = -1;
    cachedSelectionGen_ = -1;
    prevTrackGen_ = -1;
    prevSelectionGen_ = -1;

    // Emit all signals so QML updates fully
    emit stateChanged();
    emit playbackChanged();
    emit tracksChanged();
    emit selectionChanged();
    emit zoomChanged();
}

QVariantList TimelineNotifier::timelineTabsVariant() const {
    QVariantList result;
    for (int i = 0; i < timelineTabs_.size(); ++i) {
        QVariantMap tab;
        tab[QStringLiteral("id")] = timelineTabs_[i].id;
        tab[QStringLiteral("label")] = timelineTabs_[i].label;
        tab[QStringLiteral("index")] = i;
        tab[QStringLiteral("isActive")] = (i == activeTabIndex_);
        tab[QStringLiteral("parentTabIndex")] = timelineTabs_[i].parentTabIndex;
        tab[QStringLiteral("sourceClipId")] = timelineTabs_[i].sourceClipId;
        tab[QStringLiteral("isCompound")] = (timelineTabs_[i].parentTabIndex >= 0);
        result.append(tab);
    }
    return result;
}

QString TimelineNotifier::breadcrumbPath() const {
    if (timelineTabs_.isEmpty()) return QStringLiteral("Project");

    // Build breadcrumb by walking up the parentTabIndex chain
    QStringList parts;
    int idx = activeTabIndex_;
    while (idx >= 0 && idx < timelineTabs_.size()) {
        parts.prepend(timelineTabs_[idx].label);
        idx = timelineTabs_[idx].parentTabIndex;
    }
    parts.prepend(QStringLiteral("Project"));
    return parts.join(QStringLiteral(" > "));
}

int TimelineNotifier::addTimelineTab(const QString& label) {
    saveActiveTabState();

    // Create a fresh project for the new tab
    auto project = std::make_shared<VideoProject>();
    project->width = 1920;
    project->height = 1080;
    project->frameRate = kDefaultFps;

    VideoTrack videoTrack;
    videoTrack.type = TrackType::Video;
    videoTrack.index = 0;
    videoTrack.label = QStringLiteral("Video 1");
    videoTrack.isMagneticPrimary = true;

    VideoTrack audioTrack;
    audioTrack.type = TrackType::Audio;
    audioTrack.index = 1;
    audioTrack.label = QStringLiteral("Audio 1");

    project->tracks = {videoTrack, audioTrack};

    TimelineTab newTab;
    newTab.id = nextTabId_++;
    newTab.label = label.isEmpty()
        ? QStringLiteral("Timeline %1").arg(timelineTabs_.size() + 1)
        : label;

    // Create engine timeline for the new tab
    if (stubEngine_) {
        rendering::TimelineConfig cfg;
        cfg.width = project->width;
        cfg.height = project->height;
        cfg.frameRate = project->frameRate;
        newTab.engineTimelineId = stubEngine_->createTimeline(cfg);
    }

    newTab.state.project = std::move(project);
    newTab.state.phase = TimelinePhase::ready;

    int newIndex = timelineTabs_.size();
    timelineTabs_.append(std::move(newTab));

    // Switch to new tab
    activeTabIndex_ = newIndex;
    loadTabState(newIndex);
    emit tabsChanged();

    return newIndex;
}

void TimelineNotifier::removeTimelineTab(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= timelineTabs_.size()) return;
    if (timelineTabs_.size() <= 1) return; // can't remove last tab

    // Destroy engine timeline
    if (stubEngine_ && timelineTabs_[tabIndex].engineTimelineId >= 0) {
        stubEngine_->destroyTimeline(timelineTabs_[tabIndex].engineTimelineId);
    }

    // Remove any child tabs (compound clips opened from this tab)
    for (int i = timelineTabs_.size() - 1; i >= 0; --i) {
        if (timelineTabs_[i].parentTabIndex == tabIndex && i != tabIndex) {
            timelineTabs_.removeAt(i);
            if (i < tabIndex) tabIndex--;
            if (activeTabIndex_ >= i) activeTabIndex_ = std::max(0, activeTabIndex_ - 1);
        }
    }

    timelineTabs_.removeAt(tabIndex);

    // Fix up parentTabIndex references
    for (auto& tab : timelineTabs_) {
        if (tab.parentTabIndex > tabIndex) tab.parentTabIndex--;
    }

    // Adjust active index
    if (activeTabIndex_ >= timelineTabs_.size())
        activeTabIndex_ = timelineTabs_.size() - 1;
    if (activeTabIndex_ < 0) activeTabIndex_ = 0;

    loadTabState(activeTabIndex_);
    emit tabsChanged();
}

void TimelineNotifier::switchTimelineTab(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= timelineTabs_.size()) return;
    if (tabIndex == activeTabIndex_) return;

    saveActiveTabState();
    activeTabIndex_ = tabIndex;
    loadTabState(tabIndex);
    emit tabsChanged();
}

void TimelineNotifier::renameTimelineTab(int tabIndex, const QString& label) {
    if (tabIndex < 0 || tabIndex >= timelineTabs_.size()) return;
    timelineTabs_[tabIndex].label = label;
    emit tabsChanged();
}

void TimelineNotifier::setStackedView(bool stacked) {
    if (stackedView_ == stacked) return;
    stackedView_ = stacked;
    if (stacked && stackedSecondaryIndex_ < 0 && timelineTabs_.size() > 1) {
        // Auto-pick the second tab if none selected
        stackedSecondaryIndex_ = (activeTabIndex_ == 0) ? 1 : 0;
    }
    if (!stacked) stackedSecondaryIndex_ = -1;
    emit tabsChanged();
}

void TimelineNotifier::setStackedSecondaryIndex(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= timelineTabs_.size()) return;
    if (tabIndex == activeTabIndex_) return;
    stackedSecondaryIndex_ = tabIndex;
    emit tabsChanged();
}

void TimelineNotifier::openCompoundClip(int clipId) {
    if (!state_.project) return;

    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;

    saveActiveTabState();

    // Create a new tab for the compound clip
    TimelineTab newTab;
    newTab.id = nextTabId_++;
    newTab.label = QStringLiteral("Compound: %1").arg(clip->displayName);
    newTab.parentTabIndex = activeTabIndex_;
    newTab.sourceClipId = clipId;

    // Create a project from the clip's source range
    auto project = std::make_shared<VideoProject>();
    project->width = state_.project->width;
    project->height = state_.project->height;
    project->frameRate = state_.project->frameRate;

    VideoTrack videoTrack;
    videoTrack.type = TrackType::Video;
    videoTrack.index = 0;
    videoTrack.label = QStringLiteral("Video");
    videoTrack.isMagneticPrimary = true;

    // Add the clip to the compound timeline at position 0
    VideoClip compoundClip = *clip;
    compoundClip.timelineIn = 0.0;
    compoundClip.timelineOut = clip->duration();
    videoTrack.clips.append(compoundClip);

    project->tracks = {videoTrack};

    if (stubEngine_) {
        rendering::TimelineConfig cfg;
        cfg.width = project->width;
        cfg.height = project->height;
        cfg.frameRate = project->frameRate;
        newTab.engineTimelineId = stubEngine_->createTimeline(cfg);
    }

    newTab.state.project = std::move(project);
    newTab.state.phase = TimelinePhase::ready;

    int newIndex = timelineTabs_.size();
    timelineTabs_.append(std::move(newTab));

    activeTabIndex_ = newIndex;
    loadTabState(newIndex);
    emit tabsChanged();
}

void TimelineNotifier::closeCompoundClip() {
    if (activeTabIndex_ < 0 || activeTabIndex_ >= timelineTabs_.size()) return;
    int parentIdx = timelineTabs_[activeTabIndex_].parentTabIndex;
    if (parentIdx < 0) return; // not a compound clip tab

    // Remove the compound tab and switch back to parent
    removeTimelineTab(activeTabIndex_);
    // After removal, parent index may have shifted
    if (parentIdx < timelineTabs_.size()) {
        switchTimelineTab(parentIdx);
    }
}

// ---------------------------------------------------------------------------
// Secondary Timeline Accessors (Stacked View)
// ---------------------------------------------------------------------------

const TimelineState* TimelineNotifier::secondaryState() const {
    if (!stackedView_ || stackedSecondaryIndex_ < 0
        || stackedSecondaryIndex_ >= timelineTabs_.size())
        return nullptr;
    return &timelineTabs_[stackedSecondaryIndex_].state;
}

QVariantList TimelineNotifier::secondaryTracksVariant() const {
    auto* sec = secondaryState();
    if (!sec || !sec->project) {
        cachedSecondaryTracksGen_ = -1;
        cachedSecondaryTracks_.clear();
        return cachedSecondaryTracks_;
    }
    if (cachedSecondaryTracksGen_ == sec->trackGeneration)
        return cachedSecondaryTracks_;
    cachedSecondaryTracksGen_ = sec->trackGeneration;
    cachedSecondaryTracks_.clear();
    for (const auto& track : sec->project->tracks) {
        QVariantMap m;
        m[QStringLiteral("index")] = track.index;
        m[QStringLiteral("label")] = track.label;
        m[QStringLiteral("type")] = static_cast<int>(track.type);
        m[QStringLiteral("isVisible")] = track.isVisible;
        m[QStringLiteral("isLocked")] = track.isLocked;
        m[QStringLiteral("isMuted")] = track.isMuted;
        m[QStringLiteral("clipCount")] = track.clips.size();
        m[QStringLiteral("trackHeight")] = track.trackHeight;
        // Include clip data inline for simplified secondary rendering
        QVariantList clipList;
        for (const auto& clip : track.clips) {
            QVariantMap cm;
            cm[QStringLiteral("clipId")] = clip.id;
            cm[QStringLiteral("displayName")] = clip.displayName;
            cm[QStringLiteral("timelineIn")] = clip.timelineIn;
            cm[QStringLiteral("timelineOut")] = clip.timelineOut;
            cm[QStringLiteral("duration")] = clip.duration();
            cm[QStringLiteral("sourceType")] = static_cast<int>(clip.sourceType);
            cm[QStringLiteral("colorTag")] = clip.colorTag.value_or(QString());
            clipList.append(cm);
        }
        m[QStringLiteral("clips")] = clipList;
        cachedSecondaryTracks_.append(m);
    }
    return cachedSecondaryTracks_;
}

int TimelineNotifier::secondaryTrackCount() const {
    auto* sec = secondaryState();
    return (sec && sec->project) ? sec->project->tracks.size() : 0;
}

double TimelineNotifier::secondaryPixelsPerSecond() const {
    auto* sec = secondaryState();
    return sec ? sec->pixelsPerSecond : 80.0;
}

double TimelineNotifier::secondaryScrollOffset() const {
    auto* sec = secondaryState();
    return sec ? sec->scrollOffset : 0.0;
}

double TimelineNotifier::secondaryTotalDuration() const {
    auto* sec = secondaryState();
    return (sec && sec->project) ? sec->project->duration() : 0.0;
}

double TimelineNotifier::secondaryPosition() const {
    auto* sec = secondaryState();
    return sec ? sec->playback.positionSeconds : 0.0;
}

QString TimelineNotifier::secondaryTabLabel() const {
    if (!stackedView_ || stackedSecondaryIndex_ < 0
        || stackedSecondaryIndex_ >= timelineTabs_.size())
        return {};
    return timelineTabs_[stackedSecondaryIndex_].label;
}

void TimelineNotifier::swapActiveAndSecondary() {
    if (!stackedView_ || stackedSecondaryIndex_ < 0
        || stackedSecondaryIndex_ >= timelineTabs_.size())
        return;
    if (stackedSecondaryIndex_ == activeTabIndex_) return;

    saveActiveTabState();
    std::swap(activeTabIndex_, stackedSecondaryIndex_);
    loadTabState(activeTabIndex_);

    // Invalidate secondary cache
    cachedSecondaryTracksGen_ = -1;

    emit tabsChanged();
}

void TimelineNotifier::setSecondaryPixelsPerSecond(double pps) {
    if (stackedSecondaryIndex_ < 0 || stackedSecondaryIndex_ >= timelineTabs_.size()) return;
    pps = std::max(0.01, std::min(400.0, pps));
    timelineTabs_[stackedSecondaryIndex_].state.pixelsPerSecond = pps;
    emit tabsChanged();
}

void TimelineNotifier::setSecondaryScrollOffset(double offset) {
    if (stackedSecondaryIndex_ < 0 || stackedSecondaryIndex_ >= timelineTabs_.size()) return;
    timelineTabs_[stackedSecondaryIndex_].state.scrollOffset = std::max(0.0, offset);
    emit tabsChanged();
}

void TimelineNotifier::moveClipToSecondaryTimeline(int clipId, int toTrack, double toTime) {
    if (!stackedView_ || stackedSecondaryIndex_ < 0
        || stackedSecondaryIndex_ >= timelineTabs_.size())
        return;

    saveActiveTabState();

    auto& srcState = timelineTabs_[activeTabIndex_].state;
    auto& dstState = timelineTabs_[stackedSecondaryIndex_].state;
    if (!srcState.project || !dstState.project) return;

    auto srcBefore = *srcState.project;

    // Find and remove clip from source
    VideoClip moving;
    bool found = false;
    for (auto& track : srcState.project->tracks) {
        auto it = std::find_if(track.clips.begin(), track.clips.end(),
            [clipId](const VideoClip& c) { return c.id == clipId; });
        if (it != track.clips.end()) {
            moving = *it;
            track.clips.erase(it);
            found = true;
            break;
        }
    }
    if (!found) return;

    // Update clip position
    double dur = moving.duration();
    moving.timelineIn = std::max(0.0, toTime);
    moving.timelineOut = moving.timelineIn + dur;

    // Ensure target track exists in destination
    toTrack = std::max(0, toTrack);
    while (toTrack >= static_cast<int>(dstState.project->tracks.size())) {
        VideoTrack newTrack;
        newTrack.index = dstState.project->tracks.size();
        newTrack.type = TrackType::Video;
        newTrack.label = QStringLiteral("Track %1").arg(newTrack.index + 1);
        dstState.project->tracks.append(newTrack);
    }

    moving.trackIndex = toTrack;
    dstState.project->tracks[toTrack].clips.append(moving);

    // Sort clips by timelineIn
    std::sort(dstState.project->tracks[toTrack].clips.begin(),
              dstState.project->tracks[toTrack].clips.end(),
              [](const VideoClip& a, const VideoClip& b) { return a.timelineIn < b.timelineIn; });

    // Bump track generations
    srcState.trackGeneration++;
    dstState.trackGeneration++;

    // Deselect the moved clip in source
    if (srcState.selectedClipId == clipId) {
        srcState.selectedClipId.reset();
        srcState.selectedClipIds.remove(clipId);
        srcState.selectionGeneration++;
    }

    // Reload active state
    state_ = srcState;

    // Push undo for active project
    pushUndo(srcBefore);

    // Invalidate caches
    cachedTracksGen_ = -1;
    cachedSecondaryTracksGen_ = -1;

    emit tracksChanged();
    emit selectionChanged();
    emit tabsChanged();
}

// ---------------------------------------------------------------------------
// Razor / Blade Tool
// ---------------------------------------------------------------------------
void TimelineNotifier::setRazorMode(bool enabled) {
    if (state_.razorModeEnabled == enabled) return;
    state_.razorModeEnabled = enabled;
    // Deactivate trim edit mode when entering razor mode
    if (enabled && state_.trimEditMode != TrimEditMode::Normal) {
        state_.trimEditMode = TrimEditMode::Normal;
        state_.rippleMode = false;
    }
    emit stateChanged();
}

void TimelineNotifier::toggleRazorMode() {
    setRazorMode(!state_.razorModeEnabled);
}

void TimelineNotifier::splitClipAtPosition(int clipId, double timelinePosition) {
    trackClip_->splitClipAtPosition(clipId, timelinePosition);
}

void TimelineNotifier::splitAllAtPosition(double timelinePosition) {
    if (!state_.project) return;
    auto before = *state_.project;
    bool anySplit = false;

    for (auto& track : state_.project->tracks) {
        if (track.isLocked) continue;
        for (int i = 0; i < static_cast<int>(track.clips.size()); ++i) {
            auto& clip = track.clips[i];
            if (timelinePosition > clip.timelineIn + 0.001 &&
                timelinePosition < clip.timelineOut - 0.001) {
                const double timelineElapsed = timelinePosition - clip.timelineIn;
                const double sourceElapsed   = timelineElapsed * clip.speed;

                VideoClip secondHalf    = clip;
                secondHalf.id           = trackClip_->nextClipId();
                secondHalf.timelineIn   = timelinePosition;
                secondHalf.sourceIn     = clip.sourceIn + sourceElapsed;

                clip.timelineOut = timelinePosition;
                clip.sourceOut   = clip.sourceIn + sourceElapsed;

                track.clips.insert(track.clips.begin() + i + 1, std::move(secondHalf));
                anySplit = true;
                break; // only one clip per track can be at this position
            }
        }
    }

    if (anySplit) {
        state_.trackGeneration++;
        setState(std::move(state_));
        pushUndo(before);
        syncNativeToProject();
        debouncedRenderFrame();
    }
}

// ---------------------------------------------------------------------------
// Disabled / Muted Clips
// ---------------------------------------------------------------------------
void TimelineNotifier::toggleClipDisabled(int clipId) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [](const VideoClip& c) {
        return c.copyWith({.isDisabled = !c.isDisabled});
    });
    pushUndo(before);
}

bool TimelineNotifier::isClipDisabled(int clipId) const {
    if (!state_.project) return false;
    const auto* clip = state_.project->findClip(clipId);
    return clip ? clip->isDisabled : false;
}

// ---------------------------------------------------------------------------
// Magnetic Timeline (FCP-style)
// ---------------------------------------------------------------------------
void TimelineNotifier::toggleMagneticTimeline() {
    state_.magneticTimelineEnabled = !state_.magneticTimelineEnabled;
    setState(std::move(state_));
}

void TimelineNotifier::setPositionOverride(bool active) {
    if (state_.positionOverrideActive == active) return;
    state_.positionOverrideActive = active;
    setState(std::move(state_));
}

void TimelineNotifier::connectClipToPrimary(int connectedClipId, int primaryClipId, double syncOffset) {
    if (!state_.project) return;
    auto before = *state_.project;
    // updateClip() already bumps trackGeneration and calls setState()
    updateClip(connectedClipId, [primaryClipId, syncOffset](const VideoClip& c) {
        return c.copyWith({.connectedToPrimaryClipId = primaryClipId,
                           .syncPointOffset = syncOffset});
    });
    pushUndo(before);
}

void TimelineNotifier::disconnectClip(int clipId) {
    if (!state_.project) return;
    auto before = *state_.project;
    // updateClip() already bumps trackGeneration and calls setState()
    updateClip(clipId, [](const VideoClip& c) {
        return c.copyWith({.clearConnectedToPrimaryClipId = true});
    });
    pushUndo(before);
}

bool TimelineNotifier::isTrackMagneticPrimary(int trackIndex) const {
    if (!state_.project) return false;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(state_.project->tracks.size())) return false;
    return state_.project->tracks[trackIndex].isMagneticPrimary;
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

QVariantList TimelineNotifier::clipVolumeKeyframes(int clipId) const {
    QVariantList result;
    if (!state_.project) return result;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return result;
    const auto* volTrack = clip->keyframes.trackFor(KeyframeProperty::Volume);
    if (!volTrack) return result;
    double dur = clip->duration();
    for (const auto& kf : volTrack->keyframes) {
        QVariantMap pt;
        pt[QStringLiteral("time")] = kf.time;
        pt[QStringLiteral("normalizedTime")] = dur > 0 ? kf.time / dur : 0.0;
        pt[QStringLiteral("value")] = kf.value;
        pt[QStringLiteral("interpolation")] = static_cast<int>(kf.interpolation);
        result.append(pt);
    }
    return result;
}

void TimelineNotifier::moveVolumeKeyframe(int clipId, double oldTime, double newTime, double newValue) {
    if (!state_.project) return;
    auto before = *state_.project;
    newValue = std::max(0.0, std::min(2.0, newValue));
    newTime = std::max(0.0, newTime);
    keyframeAudio_->removeKeyframe(clipId, KeyframeProperty::Volume, oldTime);
    Keyframe kf;
    kf.time = newTime;
    kf.value = newValue;
    kf.interpolation = KeyframeInterpolation::Linear;
    keyframeAudio_->addKeyframe(clipId, KeyframeProperty::Volume, kf);
    keyframeAudio_->commitKeyframe(clipId, before);
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
    result[QStringLiteral("exposure")]    = cg.exposure;
    result[QStringLiteral("temperature")] = cg.temperature;
    result[QStringLiteral("tint")]        = cg.tint;
    result[QStringLiteral("shadows")]     = cg.shadows;
    result[QStringLiteral("highlights")]  = cg.highlights;
    result[QStringLiteral("vibrance")]    = cg.vibrance;
    result[QStringLiteral("hue")]         = cg.hue;
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

QVariantMap TimelineNotifier::clipColorGradingMap() const {
    auto* clip = state_.selectedClip();
    if (!clip) return {};
    const auto& cg = clip->colorGrading;
    return {
        {QStringLiteral("brightness"),  cg.brightness},
        {QStringLiteral("contrast"),    cg.contrast},
        {QStringLiteral("saturation"),  cg.saturation},
        {QStringLiteral("exposure"),    cg.exposure},
        {QStringLiteral("temperature"), cg.temperature},
        {QStringLiteral("tint"),        cg.tint},
        {QStringLiteral("shadows"),     cg.shadows},
        {QStringLiteral("highlights"),  cg.highlights},
        {QStringLiteral("vibrance"),    cg.vibrance},
        {QStringLiteral("hue"),         cg.hue},
        {QStringLiteral("fade"),        cg.fade},
        {QStringLiteral("vignette"),    cg.vignette},
    };
}

QVariantList TimelineNotifier::clipEffectsList() const {
    auto* clip = state_.selectedClip();
    if (!clip) return {};
    QVariantList result;
    for (const auto& fx : clip->effects) {
        QVariantMap m;
        m[QStringLiteral("type")]    = static_cast<int>(fx.type);
        m[QStringLiteral("value")]   = fx.value;
        m[QStringLiteral("enabled")] = fx.enabled;
        result.append(m);
    }
    return result;
}

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
        m[QStringLiteral("trackHeight")] = track.trackHeight;
        m[QStringLiteral("isMagneticPrimary")] = track.isMagneticPrimary;
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
                cachedSelectedClip_[QStringLiteral("connectedToPrimaryClipId")] = clip.connectedToPrimaryClipId.value_or(-1);
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
            if (clip.isDisabled) continue;  // skip disabled clips during playback
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
            if (clip.isDisabled) continue;  // skip disabled clips during playback
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
            if (clip.isDisabled) continue;  // skip disabled clips during playback
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
            if (clip.isDisabled) continue;  // skip disabled clips during playback
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
            if (clip.isDisabled) continue;  // skip disabled clips during playback
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
            if (clip.isDisabled) continue;  // skip disabled clips during playback
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
            if (clip.isDisabled) continue;  // skip disabled clips during playback
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
    // Check if any track is soloed
    bool anySolo = false;
    for (const auto& t : state_.project->tracks)
        if (t.isSolo) { anySolo = true; break; }

    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.isDisabled) continue;  // skip disabled clips during playback
            if (clip.sourceType == ClipSourceType::Video ||
                clip.sourceType == ClipSourceType::Image) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    // Solo logic: if any track is soloed, mute tracks that aren't soloed
                    if (anySolo && !track.isSolo) return true;
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
            if (clip.isDisabled) continue;  // skip disabled clips during playback
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
    // Check if any track is soloed
    bool anySolo = false;
    for (const auto& t : state_.project->tracks)
        if (t.isSolo) { anySolo = true; break; }

    double pos = state_.playback.positionSeconds;
    for (const auto& track : state_.project->tracks) {
        if (track.type != TrackType::Audio) continue;
        for (const auto& clip : track.clips) {
            if (clip.isDisabled) continue;  // skip disabled clips during playback
            if (clip.sourceType == ClipSourceType::Audio) {
                if (pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    // Solo logic: if any track is soloed, mute tracks that aren't soloed
                    if (anySolo && !track.isSolo) return true;
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

// ---------------------------------------------------------------------------
// Detach / Reattach Audio
// ---------------------------------------------------------------------------

bool TimelineNotifier::canDetachAudio(int clipId) const {
    if (!state_.project) return false;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return false;
    if (clip->sourceType != ClipSourceType::Video) return false;
    if (clip->sourcePath.isEmpty()) return false;
    if (clip->linkedClipId.has_value()) return false;  // already linked/detached
    if (clip->audio.isMuted) return false;              // already detached
    return true;
}

bool TimelineNotifier::canReattachAudio(int clipId) const {
    if (!state_.project) return false;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return false;
    if (!clip->linkedClipId.has_value()) return false;
    int linkedId = *clip->linkedClipId;
    const auto* linked = state_.project->findClip(linkedId);
    if (!linked) return false;
    // One must be Video, the other Audio
    return (clip->sourceType == ClipSourceType::Video && linked->sourceType == ClipSourceType::Audio)
        || (clip->sourceType == ClipSourceType::Audio && linked->sourceType == ClipSourceType::Video);
}

void TimelineNotifier::detachAudio(int clipId) {
    if (!state_.project) return;
    if (!canDetachAudio(clipId)) return;
    auto before = *state_.project;

    auto* videoClip = const_cast<VideoClip*>(state_.project->findClip(clipId));
    if (!videoClip) return;

    // Find the video clip's track index
    int videoTrackIdx = videoClip->trackIndex;

    // Find or create an audio track below the video track
    int audioTrackIdx = -1;
    for (int i = videoTrackIdx + 1; i < state_.project->tracks.size(); ++i) {
        if (state_.project->tracks[i].type == TrackType::Audio) {
            audioTrackIdx = i;
            break;
        }
    }
    if (audioTrackIdx < 0) {
        // Create a new audio track
        VideoTrack newTrack;
        newTrack.index = state_.project->tracks.size();
        newTrack.type = TrackType::Audio;
        newTrack.label = QStringLiteral("Audio %1").arg(newTrack.index + 1);
        state_.project->tracks.append(newTrack);
        audioTrackIdx = newTrack.index;
    }

    // Generate new clip ID
    int maxId = 0;
    for (const auto& t : state_.project->tracks)
        for (const auto& c : t.clips)
            maxId = std::max(maxId, c.id);
    int audioClipId = maxId + 1;

    // Create the audio clip mirroring the video clip
    VideoClip audioClip;
    audioClip.id = audioClipId;
    audioClip.trackIndex = audioTrackIdx;
    audioClip.sourceType = ClipSourceType::Audio;
    audioClip.sourcePath = videoClip->sourcePath;
    audioClip.displayName = videoClip->displayName + QStringLiteral(" (Audio)");
    audioClip.timelineIn = videoClip->timelineIn;
    audioClip.timelineOut = videoClip->timelineOut;
    audioClip.sourceIn = videoClip->sourceIn;
    audioClip.sourceOut = videoClip->sourceOut;
    audioClip.sourceDuration = videoClip->sourceDuration;
    audioClip.speed = videoClip->speed;
    audioClip.isReversed = videoClip->isReversed;
    audioClip.audio = videoClip->audio;  // copy volume/pan/fade settings
    audioClip.linkedClipId = clipId;     // link back to video

    // Insert audio clip into the audio track (sorted by timelineIn)
    auto& audioTrack = state_.project->tracks[audioTrackIdx];
    audioTrack.clips.append(audioClip);
    std::sort(audioTrack.clips.begin(), audioTrack.clips.end(),
              [](const VideoClip& a, const VideoClip& b) { return a.timelineIn < b.timelineIn; });

    // Link video to audio and mute video's embedded audio
    // Re-find videoClip since we may have moved memory
    for (auto& t : state_.project->tracks) {
        for (auto& c : t.clips) {
            if (c.id == clipId) {
                c.linkedClipId = audioClipId;
                c.audio.isMuted = true;
                break;
            }
        }
    }

    state_.trackGeneration++;
    setState(std::move(state_));
    pushUndo(before);
}

void TimelineNotifier::reattachAudio(int clipId) {
    if (!state_.project) return;
    if (!canReattachAudio(clipId)) return;
    auto before = *state_.project;

    const auto* clip = state_.project->findClip(clipId);
    if (!clip || !clip->linkedClipId.has_value()) return;
    int linkedId = *clip->linkedClipId;

    // Determine which is video and which is audio
    int videoId = clipId, audioId = linkedId;
    if (clip->sourceType == ClipSourceType::Audio) {
        videoId = linkedId;
        audioId = clipId;
    }

    // Unmute video clip's audio and clear link
    for (auto& t : state_.project->tracks) {
        for (auto& c : t.clips) {
            if (c.id == videoId) {
                c.audio.isMuted = false;
                c.linkedClipId.reset();
            }
        }
    }

    // Remove the audio clip
    for (auto& t : state_.project->tracks) {
        auto it = std::find_if(t.clips.begin(), t.clips.end(),
            [audioId](const VideoClip& c) { return c.id == audioId; });
        if (it != t.clips.end()) {
            t.clips.erase(it);
            break;
        }
    }

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

    // Collect new clip IDs for incremental engine sync
    QList<int> newClipIds;
    QList<int> modifiedClipIds;

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

                modifiedClipIds.append(clip.id);
                newClipIds.append(secondHalf.id);

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
        debouncedRenderFrame();
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

// ---------------------------------------------------------------------------
// Clip Clipboard (cut/copy/paste)
// ---------------------------------------------------------------------------
void TimelineNotifier::cutSelectedClips() {
    if (!state_.project || state_.selectedClipIds.isEmpty()) return;
    clipboardClips_.clear();
    clipboardIsCut_ = true;

    // Find the earliest timelineIn among selected clips (used as paste offset base)
    double minTime = 1e18;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (state_.selectedClipIds.contains(clip.id)) {
                clipboardClips_.append(clip);
                if (clip.timelineIn < minTime) minTime = clip.timelineIn;
            }
        }
    }
    // Normalize clipboard clip positions relative to earliest clip
    for (auto& clip : clipboardClips_) {
        double dur = clip.duration();
        clip.timelineIn -= minTime;
        clip.timelineOut = clip.timelineIn + dur;
    }

    // Remove the clips from timeline
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

void TimelineNotifier::copySelectedClips() {
    if (!state_.project || state_.selectedClipIds.isEmpty()) return;
    clipboardClips_.clear();
    clipboardIsCut_ = false;

    double minTime = 1e18;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (state_.selectedClipIds.contains(clip.id)) {
                clipboardClips_.append(clip);
                if (clip.timelineIn < minTime) minTime = clip.timelineIn;
            }
        }
    }
    // Normalize positions relative to earliest clip
    for (auto& clip : clipboardClips_) {
        double dur = clip.duration();
        clip.timelineIn -= minTime;
        clip.timelineOut = clip.timelineIn + dur;
    }
}

void TimelineNotifier::pasteClipsAtPlayhead() {
    if (!state_.project || clipboardClips_.isEmpty()) return;
    auto before = *state_.project;
    double pasteTime = state_.playback.positionSeconds;

    // Build a map of how much space each track needs for pasted clips.
    // We push existing clips right per-track by the span of pasted clips on that track.
    QHash<int, double> trackPasteSpan; // trackIndex -> max (pasteEnd - pasteTime)
    for (const auto& srcClip : clipboardClips_) {
        int targetTrack = srcClip.trackIndex;
        if (targetTrack < 0 || targetTrack >= static_cast<int>(state_.project->tracks.size()))
            targetTrack = 0;
        double clipEnd = pasteTime + srcClip.timelineIn + srcClip.duration();
        double span = clipEnd - pasteTime;
        if (span > trackPasteSpan.value(targetTrack, 0.0))
            trackPasteSpan[targetTrack] = span;
    }

    // Push existing clips right on each affected track
    constexpr double kTolerance = 0.01;
    for (auto it = trackPasteSpan.cbegin(); it != trackPasteSpan.cend(); ++it) {
        int trackIdx = it.key();
        double span = it.value();
        if (trackIdx < 0 || trackIdx >= static_cast<int>(state_.project->tracks.size()))
            continue;
        auto& track = state_.project->tracks[trackIdx];
        if (track.isLocked) continue;
        for (auto& clip : track.clips) {
            // Shift clips that start at or after the paste point
            if (clip.timelineIn >= pasteTime - kTolerance) {
                double dur = clip.duration();
                clip.timelineIn += span;
                clip.timelineOut = clip.timelineIn + dur;
            }
            // Split clips that straddle the paste point: handled by
            // shifting only clips whose start >= pasteTime. Clips that
            // merely overlap (start before, end after) keep their position
            // and the pasted clip will sit after them once the downstream
            // clips have been pushed.
        }
    }

    // Insert pasted clips
    state_.selectedClipIds.clear();
    for (const auto& srcClip : clipboardClips_) {
        int targetTrack = srcClip.trackIndex;
        if (targetTrack < 0 || targetTrack >= static_cast<int>(state_.project->tracks.size()))
            targetTrack = 0;

        VideoClip newClip = srcClip;
        newClip.id = trackClip_->nextClipId();
        double dur = srcClip.duration();
        newClip.timelineIn = pasteTime + srcClip.timelineIn;
        newClip.timelineOut = newClip.timelineIn + dur;
        newClip.trackIndex = targetTrack;

        state_.selectedClipIds.insert(newClip.id);
        state_.project->tracks[targetTrack].clips.append(std::move(newClip));
    }

    state_.trackGeneration++;
    state_.selectionGeneration++;
    setState(std::move(state_));
    pushUndo(before);
    syncNativeToProject();
    debouncedRenderFrame();
}

void TimelineNotifier::pasteClipsOverwrite() {
    if (!state_.project || clipboardClips_.isEmpty()) return;
    auto before = *state_.project;
    double pasteTime = state_.playback.positionSeconds;

    constexpr double kTolerance = 0.01;

    // For each pasted clip, clear the region it will occupy by
    // trimming, splitting, or removing existing clips in that zone.
    for (const auto& srcClip : clipboardClips_) {
        int targetTrack = srcClip.trackIndex;
        if (targetTrack < 0 || targetTrack >= static_cast<int>(state_.project->tracks.size()))
            targetTrack = 0;
        auto& track = state_.project->tracks[targetTrack];
        if (track.isLocked) continue;

        double regionIn  = pasteTime + srcClip.timelineIn;
        double regionOut = regionIn + srcClip.duration();

        QList<VideoClip> kept;
        for (const auto& clip : track.clips) {
            bool startsBeforeRegion = clip.timelineIn < regionIn - kTolerance;
            bool endsAfterRegion    = clip.timelineOut > regionOut + kTolerance;
            bool startsInRegion     = clip.timelineIn >= regionIn - kTolerance
                                   && clip.timelineIn < regionOut - kTolerance;
            bool endsInRegion       = clip.timelineOut > regionIn + kTolerance
                                   && clip.timelineOut <= regionOut + kTolerance;

            if (startsBeforeRegion && endsAfterRegion) {
                // Clip spans the entire paste region — split into two pieces
                VideoClip leftPart = clip;
                double leftDur = regionIn - clip.timelineIn;
                leftPart.timelineOut = clip.timelineIn + leftDur;
                leftPart.sourceOut = clip.sourceIn + leftDur / clip.speed;
                kept.append(leftPart);

                VideoClip rightPart = clip;
                rightPart.id = trackClip_->nextClipId();
                double rightStart = regionOut;
                double trimmedFromSource = (regionOut - clip.timelineIn) / clip.speed;
                rightPart.sourceIn = clip.sourceIn + trimmedFromSource;
                rightPart.timelineIn = rightStart;
                // timelineOut stays the same
                kept.append(rightPart);
            } else if (startsBeforeRegion && endsInRegion) {
                // Clip overlaps left side — trim its right edge
                VideoClip trimmed = clip;
                double newDur = regionIn - clip.timelineIn;
                trimmed.timelineOut = clip.timelineIn + newDur;
                trimmed.sourceOut = clip.sourceIn + newDur / clip.speed;
                kept.append(trimmed);
            } else if (startsInRegion && endsAfterRegion) {
                // Clip overlaps right side — trim its left edge
                VideoClip trimmed = clip;
                double trimAmount = regionOut - clip.timelineIn;
                trimmed.sourceIn = clip.sourceIn + trimAmount / clip.speed;
                trimmed.timelineIn = regionOut;
                kept.append(trimmed);
            } else if (startsInRegion && endsInRegion) {
                // Clip entirely within paste region — remove it
                // (do not append)
            } else {
                // Clip does not overlap paste region at all
                kept.append(clip);
            }
        }
        track.clips = kept;
    }

    // Insert pasted clips
    state_.selectedClipIds.clear();
    for (const auto& srcClip : clipboardClips_) {
        int targetTrack = srcClip.trackIndex;
        if (targetTrack < 0 || targetTrack >= static_cast<int>(state_.project->tracks.size()))
            targetTrack = 0;

        VideoClip newClip = srcClip;
        newClip.id = trackClip_->nextClipId();
        double dur = srcClip.duration();
        newClip.timelineIn = pasteTime + srcClip.timelineIn;
        newClip.timelineOut = newClip.timelineIn + dur;
        newClip.trackIndex = targetTrack;

        state_.selectedClipIds.insert(newClip.id);
        state_.project->tracks[targetTrack].clips.append(std::move(newClip));
    }

    state_.trackGeneration++;
    state_.selectionGeneration++;
    setState(std::move(state_));
    pushUndo(before);
    syncNativeToProject();
    debouncedRenderFrame();
}

bool TimelineNotifier::hasClipboardClips() const {
    return !clipboardClips_.isEmpty();
}

QVariantList TimelineNotifier::pasteAttributeOptions() const {
    QVariantList opts;
    opts.append(QVariantMap{{QStringLiteral("key"), QStringLiteral("effects")},
                            {QStringLiteral("label"), QStringLiteral("Effects")}});
    opts.append(QVariantMap{{QStringLiteral("key"), QStringLiteral("colorGrading")},
                            {QStringLiteral("label"), QStringLiteral("Color Grading")}});
    opts.append(QVariantMap{{QStringLiteral("key"), QStringLiteral("transforms")},
                            {QStringLiteral("label"), QStringLiteral("Transforms")}});
    opts.append(QVariantMap{{QStringLiteral("key"), QStringLiteral("speed")},
                            {QStringLiteral("label"), QStringLiteral("Speed")}});
    opts.append(QVariantMap{{QStringLiteral("key"), QStringLiteral("audio")},
                            {QStringLiteral("label"), QStringLiteral("Audio Settings")}});
    opts.append(QVariantMap{{QStringLiteral("key"), QStringLiteral("transitions")},
                            {QStringLiteral("label"), QStringLiteral("Transitions")}});
    return opts;
}

void TimelineNotifier::pasteAttributes(bool effects, bool colorGrading,
                                        bool transforms, bool speed,
                                        bool audio, bool transitions) {
    if (!state_.project || clipboardClips_.isEmpty() || state_.selectedClipIds.isEmpty()) return;
    const auto& src = clipboardClips_.first();  // paste from first clipboard clip
    auto before = *state_.project;

    for (auto& track : state_.project->tracks) {
        for (auto& clip : track.clips) {
            if (!state_.selectedClipIds.contains(clip.id)) continue;

            if (effects)
                clip.effects = src.effects;
            if (colorGrading)
                clip.colorGrading = src.colorGrading;
            if (transforms) {
                clip.positionX = src.positionX;
                clip.positionY = src.positionY;
                clip.scaleX = src.scaleX;
                clip.scaleY = src.scaleY;
                clip.rotation = src.rotation;
                clip.cropLeft = src.cropLeft;
                clip.cropTop = src.cropTop;
                clip.cropRight = src.cropRight;
                clip.cropBottom = src.cropBottom;
            }
            if (speed) {
                clip.speed = src.speed;
                clip.isReversed = src.isReversed;
            }
            if (audio)
                clip.audio = src.audio;
            if (transitions) {
                clip.transitionIn = src.transitionIn;
                clip.transitionOut = src.transitionOut;
            }
        }
    }

    state_.trackGeneration++;
    state_.selectionGeneration++;
    setState(std::move(state_));
    pushUndo(before);
    syncNativeToProject();
    debouncedRenderFrame();
}

void TimelineNotifier::selectTrackForward(int clipId) {
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    int trackIdx = clip->trackIndex;
    double startTime = clip->timelineIn;

    state_.selectedClipIds.clear();
    for (const auto& track : state_.project->tracks) {
        if (track.index != trackIdx) continue;
        for (const auto& c : track.clips) {
            if (c.timelineIn >= startTime - 0.001)
                state_.selectedClipIds.insert(c.id);
        }
    }
    state_.selectedClipId = clipId;
    state_.selectionGeneration++;
    setState(std::move(state_));
}

void TimelineNotifier::selectTrackBackward(int clipId) {
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    int trackIdx = clip->trackIndex;
    double endTime = clip->timelineOut;

    state_.selectedClipIds.clear();
    for (const auto& track : state_.project->tracks) {
        if (track.index != trackIdx) continue;
        for (const auto& c : track.clips) {
            if (c.timelineOut <= endTime + 0.001)
                state_.selectedClipIds.insert(c.id);
        }
    }
    state_.selectedClipId = clipId;
    state_.selectionGeneration++;
    setState(std::move(state_));
}

void TimelineNotifier::selectClipRange(int fromClipId, int toClipId) {
    if (!state_.project) return;
    const auto* fromClip = state_.project->findClip(fromClipId);
    const auto* toClip = state_.project->findClip(toClipId);
    if (!fromClip || !toClip) return;

    double rangeStart = std::min(fromClip->timelineIn, toClip->timelineIn);
    double rangeEnd = std::max(fromClip->timelineOut, toClip->timelineOut);

    state_.selectedClipIds.clear();
    for (const auto& track : state_.project->tracks) {
        for (const auto& c : track.clips) {
            // Select clips that overlap the range
            if (c.timelineIn < rangeEnd + 0.001 && c.timelineOut > rangeStart - 0.001)
                state_.selectedClipIds.insert(c.id);
        }
    }
    state_.selectedClipId = toClipId;
    state_.selectionGeneration++;
    setState(std::move(state_));
}

void TimelineNotifier::selectNextClipOnTrack() {
    if (!state_.project || !state_.selectedClipId.has_value()) return;
    const auto* clip = state_.project->findClip(*state_.selectedClipId);
    if (!clip) return;

    const VideoClip* best = nullptr;
    double bestTime = 1e18;
    for (const auto& track : state_.project->tracks) {
        if (track.index != clip->trackIndex) continue;
        for (const auto& c : track.clips) {
            if (c.timelineIn > clip->timelineIn + 0.001 && c.timelineIn < bestTime) {
                bestTime = c.timelineIn;
                best = &c;
            }
        }
    }
    if (best) {
        state_.selectedClipIds.clear();
        state_.selectedClipIds.insert(best->id);
        state_.selectedClipId = best->id;
        state_.selectionGeneration++;
        setState(std::move(state_));
    }
}

void TimelineNotifier::selectPrevClipOnTrack() {
    if (!state_.project || !state_.selectedClipId.has_value()) return;
    const auto* clip = state_.project->findClip(*state_.selectedClipId);
    if (!clip) return;

    const VideoClip* best = nullptr;
    double bestTime = -1.0;
    for (const auto& track : state_.project->tracks) {
        if (track.index != clip->trackIndex) continue;
        for (const auto& c : track.clips) {
            if (c.timelineIn < clip->timelineIn - 0.001 && c.timelineIn > bestTime) {
                bestTime = c.timelineIn;
                best = &c;
            }
        }
    }
    if (best) {
        state_.selectedClipIds.clear();
        state_.selectedClipIds.insert(best->id);
        state_.selectedClipId = best->id;
        state_.selectionGeneration++;
        setState(std::move(state_));
    }
}

void TimelineNotifier::zoomToFit(double viewportWidth) {
    if (!state_.project || viewportWidth <= 0) return;
    double dur = state_.project->duration();
    if (dur <= 0) dur = 30.0;
    double fitPps = std::clamp(viewportWidth / dur, 0.01, 400.0);
    state_.pixelsPerSecond = fitPps;
    state_.scrollOffset = 0.0;
    emit zoomChanged();
}

void TimelineNotifier::toggleZoomFit(double viewportWidth) {
    if (!state_.project || viewportWidth <= 0) return;
    double dur = state_.project->duration();
    if (dur <= 0) dur = 30.0;
    double fitPps = std::clamp(viewportWidth / dur, 0.01, 400.0);

    // If currently near the fit zoom, restore saved zoom
    if (savedZoomPps_ > 0 && std::abs(state_.pixelsPerSecond - fitPps) < 1.0) {
        state_.pixelsPerSecond = savedZoomPps_;
        savedZoomPps_ = -1.0;
    } else {
        savedZoomPps_ = state_.pixelsPerSecond;
        state_.pixelsPerSecond = fitPps;
        state_.scrollOffset = 0.0;
    }
    emit zoomChanged();
}

void TimelineNotifier::seekToTimecode(const QString& timecode) {
    // Parse timecode in format "MM:SS:FF" or "HH:MM:SS:FF" or just seconds
    double fps = state_.project ? state_.project->frameRate : 30.0;
    QStringList parts = timecode.split(QStringLiteral(":"));
    double seconds = 0.0;

    if (parts.size() == 3) {
        // MM:SS:FF
        seconds = parts[0].toDouble() * 60.0
                + parts[1].toDouble()
                + parts[2].toDouble() / fps;
    } else if (parts.size() == 4) {
        // HH:MM:SS:FF
        seconds = parts[0].toDouble() * 3600.0
                + parts[1].toDouble() * 60.0
                + parts[2].toDouble()
                + parts[3].toDouble() / fps;
    } else if (parts.size() == 1) {
        // Try as raw seconds
        seconds = timecode.toDouble();
    }

    if (seconds >= 0.0) {
        playback_->seek(seconds);
    }
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
    result[QStringLiteral("trackHeight")] = track.trackHeight;
    result[QStringLiteral("volume")]    = track.audioSettings.volume;
    result[QStringLiteral("pan")]       = track.audioSettings.pan;
    return result;
}

// ---------------------------------------------------------------------------
// Track height
// ---------------------------------------------------------------------------

void TimelineNotifier::setTrackHeight(int trackIndex, double height) {
    if (!state_.project) return;
    auto& tracks = state_.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    height = std::clamp(height, 28.0, 200.0);
    if (std::abs(tracks[trackIndex].trackHeight - height) < 0.5) return;
    tracks[trackIndex].trackHeight = height;
    state_.trackGeneration++;
    setState(std::move(state_));
}

void TimelineNotifier::expandAllTracks() {
    if (!state_.project) return;
    for (auto& track : state_.project->tracks)
        track.trackHeight = 120.0;
    state_.trackGeneration++;
    setState(std::move(state_));
}

void TimelineNotifier::collapseAllTracks() {
    if (!state_.project) return;
    for (auto& track : state_.project->tracks)
        track.trackHeight = 28.0;
    state_.trackGeneration++;
    setState(std::move(state_));
}

// ---------------------------------------------------------------------------
// Version history / auto-save
// ---------------------------------------------------------------------------

void TimelineNotifier::saveVersion(const QString& label) {
    if (!state_.project) return;

    VersionEntry entry;
    entry.label = label.isEmpty() ? QStringLiteral("Checkpoint") : label;
    entry.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    entry.projectData = state_.project->toMap();
    entry.isAutoSave = (label == QStringLiteral("Auto-save"));

    versionHistory_.prepend(entry);  // newest first

    // Enforce max versions — remove oldest auto-saves first
    while (versionHistory_.size() > kMaxVersions) {
        // Find oldest auto-save to remove, or just remove last
        bool removed = false;
        for (int i = versionHistory_.size() - 1; i >= 0; --i) {
            if (versionHistory_[i].isAutoSave) {
                versionHistory_.removeAt(i);
                removed = true;
                break;
            }
        }
        if (!removed) versionHistory_.removeLast();
    }

    emit stateChanged();
    qDebug() << "[VersionHistory] saved:" << entry.label << "at" << entry.timestamp
             << "total:" << versionHistory_.size();
}

void TimelineNotifier::restoreVersion(int index) {
    if (index < 0 || index >= versionHistory_.size()) return;
    if (!state_.project) return;

    // Save current state as undo point before restoring
    auto before = *state_.project;

    const auto& entry = versionHistory_[index];
    auto restored = VideoProject::fromMap(entry.projectData);
    *state_.project = restored;

    state_.trackGeneration++;
    state_.selectionGeneration++;
    state_.selectedClipId.reset();
    state_.selectedClipIds.clear();
    setState(std::move(state_));
    pushUndo(before);
    syncNativeToProject();
    debouncedRenderFrame();

    qDebug() << "[VersionHistory] restored version:" << entry.label << "from" << entry.timestamp;
}

QVariantList TimelineNotifier::getVersionHistory() const {
    QVariantList result;
    for (int i = 0; i < versionHistory_.size(); ++i) {
        const auto& v = versionHistory_[i];
        QVariantMap m;
        m[QStringLiteral("index")] = i;
        m[QStringLiteral("label")] = v.label;
        m[QStringLiteral("timestamp")] = v.timestamp;
        m[QStringLiteral("isAutoSave")] = v.isAutoSave;
        result.append(m);
    }
    return result;
}

void TimelineNotifier::setAutoSaveInterval(int seconds) {
    autoSaveIntervalMs_ = std::max(10, seconds) * 1000;
    autoSaveTimer_.setInterval(autoSaveIntervalMs_);
}

void TimelineNotifier::clearVersionHistory() {
    versionHistory_.clear();
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Typed track lane helpers
// ---------------------------------------------------------------------------

bool TimelineNotifier::isTrackCompatible(int trackIndex, int sourceType) const {
    if (!state_.project) return false;
    const auto& tracks = state_.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return false;
    if (tracks[trackIndex].isLocked) return false;
    auto st = static_cast<ClipSourceType>(sourceType);
    auto tt = tracks[trackIndex].type;
    switch (st) {
    case ClipSourceType::Video:
    case ClipSourceType::Image:      return tt == TrackType::Video;
    case ClipSourceType::Audio:      return tt == TrackType::Audio;
    case ClipSourceType::Title:      return tt == TrackType::Title || tt == TrackType::Video;
    case ClipSourceType::Color:
    case ClipSourceType::Adjustment: return tt == TrackType::Effect;
    default:                         return tt == TrackType::Video;
    }
}

int TimelineNotifier::findCompatibleTrack(int nearTrackIndex, int sourceType) const {
    if (!state_.project) return nearTrackIndex;
    const auto& tracks = state_.project->tracks;
    // If the target track is already compatible, return it
    if (nearTrackIndex >= 0 && nearTrackIndex < static_cast<int>(tracks.size())
        && isTrackCompatible(nearTrackIndex, sourceType))
        return nearTrackIndex;
    // Find nearest compatible track
    int bestIdx = -1, bestDist = 9999;
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
        if (isTrackCompatible(i, sourceType)) {
            int dist = std::abs(i - std::max(0, nearTrackIndex));
            if (dist < bestDist) { bestDist = dist; bestIdx = i; }
        }
    }
    return bestIdx >= 0 ? bestIdx : nearTrackIndex;
}

QVariantMap TimelineNotifier::batchClipSpan(const QVariantList& clipIds) const {
    QVariantMap result;
    if (!state_.project || clipIds.isEmpty()) {
        result[QStringLiteral("minIn")]  = 0.0;
        result[QStringLiteral("maxOut")] = 0.0;
        return result;
    }
    // Build a QSet for O(1) lookup instead of O(D) per clip
    QSet<int> idSet;
    idSet.reserve(clipIds.size());
    for (const auto& v : clipIds)
        idSet.insert(v.toInt());

    double minIn  = std::numeric_limits<double>::max();
    double maxOut = 0.0;
    // Single O(T×C) pass over all tracks/clips
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (idSet.contains(clip.id)) {
                minIn  = std::min(minIn, clip.timelineIn);
                maxOut = std::max(maxOut, clip.timelineOut);
            }
        }
    }
    if (minIn > maxOut) minIn = 0.0;  // none found
    result[QStringLiteral("minIn")]  = minIn;
    result[QStringLiteral("maxOut")] = maxOut;
    return result;
}

// ===========================================================================
// Project JSON export
// ===========================================================================

QString TimelineNotifier::projectJson() const {
    if (!state_.project) return QStringLiteral("{}");
    return QJsonDocument(state_.project->toMap()).toJson(QJsonDocument::Compact);
}

// ===========================================================================
// Template placeholder support
// ===========================================================================

QVariantList TimelineNotifier::templatePlaceholders() const {
    QVariantList result;
    if (!state_.project) return result;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.isPlaceholder) {
                QVariantMap entry;
                entry[QStringLiteral("clipId")] = clip.id;
                entry[QStringLiteral("displayName")] = clip.displayName;
                entry[QStringLiteral("fieldType")] =
                    clip.sourceType == ClipSourceType::Audio ? QStringLiteral("audio")
                    : clip.sourceType == ClipSourceType::Title ? QStringLiteral("text")
                    : QStringLiteral("media");
                entry[QStringLiteral("placeholderLabel")] = clip.placeholderLabel;
                result.append(entry);
            }
        }
    }
    return result;
}

bool TimelineNotifier::hasUnassignedClips() const {
    if (!state_.project) return false;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (!clip.isPlaceholder) return true;
        }
    }
    return false;
}

void TimelineNotifier::addPlaceholder(int clipId) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [](const VideoClip& c) {
        return c.copyWith({.isPlaceholder = true});
    });
    pushUndo(before);
    emit tracksChanged();
}

void TimelineNotifier::removePlaceholder(int index) {
    if (!state_.project) return;
    auto before = *state_.project;
    int count = 0;
    for (const auto& track : state_.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.isPlaceholder) {
                if (count == index) {
                    updateClip(clip.id, [](const VideoClip& c) {
                        return c.copyWith({.isPlaceholder = false});
                    });
                    pushUndo(before);
                    emit tracksChanged();
                    return;
                }
                ++count;
            }
        }
    }
}

void TimelineNotifier::togglePlaceholder(int clipId) {
    if (!state_.project) return;
    const auto* clip = state_.project->findClip(clipId);
    if (!clip) return;
    auto before = *state_.project;
    bool newVal = !clip->isPlaceholder;
    updateClip(clipId, [newVal](const VideoClip& c) {
        return c.copyWith({.isPlaceholder = newVal});
    });
    pushUndo(before);
    emit tracksChanged();
}

void TimelineNotifier::showPlaceholderPicker() {
    emit placeholderPickerRequested();
}

// ===========================================================================
// Text clip content
// ===========================================================================

void TimelineNotifier::setClipTextContent(int clipId, const QString& text) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [&text](const VideoClip& c) {
        return c.copyWith({.textContent = text});
    });
    pushUndo(before);
    emit selectionChanged();
}

void TimelineNotifier::setClipFontFamily(int clipId, const QString& family) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [&family](const VideoClip& c) {
        return c.copyWith({.fontFamily = family});
    });
    pushUndo(before);
    emit selectionChanged();
}

void TimelineNotifier::setClipFontSize(int clipId, double size) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [size](const VideoClip& c) {
        return c.copyWith({.fontSize = size});
    });
    pushUndo(before);
    emit selectionChanged();
}

void TimelineNotifier::setClipFontColor(int clipId, const QString& color) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [&color](const VideoClip& c) {
        return c.copyWith({.fontColor = color});
    });
    pushUndo(before);
    emit selectionChanged();
}

void TimelineNotifier::setClipTextAlignment(int clipId, int alignment) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [alignment](const VideoClip& c) {
        return c.copyWith({.textAlignment = alignment});
    });
    pushUndo(before);
    emit selectionChanged();
}

// ===========================================================================
// Transform
// ===========================================================================

void TimelineNotifier::setClipPosition(int clipId, double x, double y) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [x, y](const VideoClip& c) {
        return c.copyWith({.positionX = x, .positionY = y});
    });
    pushUndo(before);
    emit selectionChanged();
}

void TimelineNotifier::setClipScale(int clipId, double sx, double sy) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [sx, sy](const VideoClip& c) {
        return c.copyWith({.scaleX = sx, .scaleY = sy});
    });
    pushUndo(before);
    emit selectionChanged();
}

void TimelineNotifier::setClipRotation(int clipId, double degrees) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [degrees](const VideoClip& c) {
        return c.copyWith({.rotation = degrees});
    });
    pushUndo(before);
    emit selectionChanged();
}

void TimelineNotifier::setClipCrop(int clipId, double left, double top,
                                    double right, double bottom) {
    if (!state_.project) return;
    auto before = *state_.project;
    updateClip(clipId, [left, top, right, bottom](const VideoClip& c) {
        return c.copyWith({.cropLeft = left, .cropTop = top,
                           .cropRight = right, .cropBottom = bottom});
    });
    pushUndo(before);
    emit selectionChanged();
}

} // namespace gopost::video_editor
