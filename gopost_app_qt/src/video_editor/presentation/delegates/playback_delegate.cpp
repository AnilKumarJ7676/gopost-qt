#include "video_editor/presentation/delegates/playback_delegate.h"

#include <algorithm>
#include <cmath>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PlaybackDelegate::PlaybackDelegate(TimelineOperations* ops)
    : ops_(ops) {}

PlaybackDelegate::~PlaybackDelegate() = default;

// ---------------------------------------------------------------------------
// Playback
// ---------------------------------------------------------------------------

void PlaybackDelegate::play() {
    auto state = ops_->currentState();
    if (state.playback.isPlaying) return;
    state.playback.isPlaying = true;
    ops_->setState(std::move(state));
}

void PlaybackDelegate::pause() {
    auto state = ops_->currentState();
    if (!state.playback.isPlaying) return;
    state.playback.isPlaying = false;
    shuttleSpeed_ = 0;
    ops_->setState(std::move(state));
    ops_->renderCurrentFrame();
}

void PlaybackDelegate::togglePlayPause() {
    if (ops_->currentState().playback.isPlaying) pause();
    else play();
}

void PlaybackDelegate::seek(double positionSeconds) {
    auto state = ops_->currentState();
    const double duration = state.project ? state.project->duration() : 0.0;
    state.playback.positionSeconds = std::clamp(positionSeconds, 0.0, duration);
    ops_->setState(std::move(state));
    ops_->updateActiveVideo();
    ops_->renderCurrentFrame();
}

void PlaybackDelegate::stepForward() {
    constexpr double kFrameDuration = 1.0 / 30.0;
    seek(ops_->currentState().playback.positionSeconds + kFrameDuration);
}

void PlaybackDelegate::stepBackward() {
    constexpr double kFrameDuration = 1.0 / 30.0;
    seek(ops_->currentState().playback.positionSeconds - kFrameDuration);
}

void PlaybackDelegate::jumpToStart() {
    seek(ops_->currentState().playback.inPoint.value_or(0.0));
}

void PlaybackDelegate::jumpToEnd() {
    const auto& s = ops_->currentState();
    const double dur = s.project ? s.project->duration() : 0.0;
    seek(s.playback.outPoint.value_or(dur));
}

// ---------------------------------------------------------------------------
// JKL shuttle
// ---------------------------------------------------------------------------

void PlaybackDelegate::shuttleForward() {
    shuttleSpeed_ = std::clamp(shuttleSpeed_ + 1, -3, 3);
    if (shuttleSpeed_ == 0) { pause(); return; }
    play();
}

void PlaybackDelegate::shuttleReverse() {
    shuttleSpeed_ = std::clamp(shuttleSpeed_ - 1, -3, 3);
    if (shuttleSpeed_ == 0) { pause(); return; }
    play();
}

void PlaybackDelegate::shuttleStop() {
    shuttleSpeed_ = 0;
    pause();
}

// ---------------------------------------------------------------------------
// In / Out points
// ---------------------------------------------------------------------------

void PlaybackDelegate::setInPoint() {
    auto state = ops_->currentState();
    state.playback.inPoint = state.playback.positionSeconds;
    ops_->setState(std::move(state));
}

void PlaybackDelegate::clearInPoint() {
    auto state = ops_->currentState();
    state.playback.inPoint.reset();
    ops_->setState(std::move(state));
}

void PlaybackDelegate::setOutPoint() {
    auto state = ops_->currentState();
    state.playback.outPoint = state.playback.positionSeconds;
    ops_->setState(std::move(state));
}

void PlaybackDelegate::clearOutPoint() {
    auto state = ops_->currentState();
    state.playback.outPoint.reset();
    ops_->setState(std::move(state));
}

// ---------------------------------------------------------------------------
// Scrub (serialised seek queue)
// ---------------------------------------------------------------------------

void PlaybackDelegate::scrubTo(double positionSeconds) {
    seekQueue_.push_back({positionSeconds});
    processSeekQueue();
}

void PlaybackDelegate::processSeekQueue() {
    if (isSeeking_ || seekQueue_.empty()) return;
    isSeeking_ = true;

    // Collapse to latest request
    const auto req = seekQueue_.back();
    seekQueue_.clear();

    seek(req.position);
    isSeeking_ = false;

    // Drain anything that arrived while we were seeking
    if (!seekQueue_.empty()) processSeekQueue();
}

// ---------------------------------------------------------------------------
// Snap
// ---------------------------------------------------------------------------

double PlaybackDelegate::snapPosition(double rawPos) const {
    if (!snapEnabled_) return rawPos;

    const auto& state = ops_->currentState();
    const auto* project = state.project.get();
    if (!project) return rawPos;

    constexpr double kSnapThresholdSec = 0.1;
    double best = rawPos;
    double bestDist = kSnapThresholdSec;

    // Snap to clip edges
    for (const auto& track : project->tracks) {
        for (const auto& clip : track.clips) {
            for (double edge : {clip.timelineIn, clip.timelineOut}) {
                const double dist = std::abs(rawPos - edge);
                if (dist < bestDist) {
                    bestDist = dist;
                    best = edge;
                }
            }
        }
    }
    return best;
}

void PlaybackDelegate::setSnapEnabled(bool enabled) {
    snapEnabled_ = enabled;
}

void PlaybackDelegate::jumpToPreviousSnapPoint() {
    const auto& state = ops_->currentState();
    const auto* project = state.project.get();
    if (!project) return;

    const double current = state.playback.positionSeconds;
    double bestPos = 0.0;
    constexpr double kEpsilon = 0.001;

    // Collect all clip edges and find the closest one before current position
    for (const auto& track : project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.timelineIn < current - kEpsilon && clip.timelineIn > bestPos)
                bestPos = clip.timelineIn;
            if (clip.timelineOut < current - kEpsilon && clip.timelineOut > bestPos)
                bestPos = clip.timelineOut;
        }
    }
    seek(bestPos);
}

void PlaybackDelegate::jumpToNextSnapPoint() {
    const auto& state = ops_->currentState();
    const auto* project = state.project.get();
    if (!project) return;

    const double current = state.playback.positionSeconds;
    const double duration = project->duration();
    double bestPos = duration;
    constexpr double kEpsilon = 0.001;

    for (const auto& track : project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.timelineIn > current + kEpsilon && clip.timelineIn < bestPos)
                bestPos = clip.timelineIn;
            if (clip.timelineOut > current + kEpsilon && clip.timelineOut < bestPos)
                bestPos = clip.timelineOut;
        }
    }
    seek(bestPos);
}

// ---------------------------------------------------------------------------
// Zoom / Scroll
// ---------------------------------------------------------------------------

void PlaybackDelegate::setPixelsPerSecond(double pps) {
    auto state = ops_->currentState();
    state.pixelsPerSecond = std::clamp(pps, kMinZoom, kMaxZoom);
    ops_->setState(std::move(state));
}

void PlaybackDelegate::setScrollOffset(double offset) {
    auto state = ops_->currentState();
    state.scrollOffset = std::max(0.0, offset);
    ops_->setState(std::move(state));
}

void PlaybackDelegate::zoomAtPosition(double factor, double anchorX) {
    auto state = ops_->currentState();
    const double oldPps = state.pixelsPerSecond;
    const double newPps = std::clamp(oldPps * factor, kMinZoom, kMaxZoom);

    // Keep the anchor point (e.g. mouse position) stable
    const double anchorTime = (state.scrollOffset + anchorX) / oldPps;
    const double newOffset  = anchorTime * newPps - anchorX;

    state.pixelsPerSecond = newPps;
    state.scrollOffset    = std::max(0.0, newOffset);
    ops_->setState(std::move(state));
}

// ---------------------------------------------------------------------------
// Render pipeline
// ---------------------------------------------------------------------------

void PlaybackDelegate::requestRender() {
    if (isRendering_) {
        renderRequested_ = true;
        return;
    }
    executeRender();
}

void PlaybackDelegate::executeRender() {
    isRendering_ = true;
    ops_->renderCurrentFrame();
    isRendering_ = false;

    if (renderRequested_) {
        renderRequested_ = false;
        executeRender();
    }
}

// ---------------------------------------------------------------------------
// Playback tick (called at ~33 ms intervals by the owning notifier's timer)
// ---------------------------------------------------------------------------

void PlaybackDelegate::onPlaybackTick() {
    auto state = ops_->currentState();
    if (!state.playback.isPlaying) return;

    constexpr double kTickSec = kPlaybackTickMs / 1000.0;
    const double speedMul = (shuttleSpeed_ != 0) ? std::pow(2.0, std::abs(shuttleSpeed_) - 1)
                                                    * (shuttleSpeed_ > 0 ? 1.0 : -1.0)
                                                  : 1.0;
    double newPos = state.playback.positionSeconds + kTickSec * speedMul;
    const double duration = state.project ? state.project->duration() : 0.0;
    const double outPt = state.playback.outPoint.value_or(duration);

    if (newPos >= outPt) {
        // Stop at end — don't loop. User can press play again to restart.
        newPos = outPt;
        state.playback.isPlaying = false;
    } else if (newPos < 0.0) {
        newPos = 0.0;
        state.playback.isPlaying = false;
    }

    state.playback.positionSeconds = newPos;
    ops_->setState(std::move(state));
    ops_->updateActiveVideo();
    // Use throttled render during playback to avoid redundant stateChanged emissions.
    // setState() above already emits stateChanged for QML updates.
    ops_->throttledRenderFrame();
}

} // namespace gopost::video_editor
