#include "video_editor/presentation/delegates/playback_delegate.h"

#include <QDebug>
#include <algorithm>
#include <cmath>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PlaybackDelegate::PlaybackDelegate(TimelineOperations* ops)
    : ops_(ops)
    , lastTickTime_(std::chrono::steady_clock::now()) {}

PlaybackDelegate::~PlaybackDelegate() = default;

// ---------------------------------------------------------------------------
// State queries
// ---------------------------------------------------------------------------

double PlaybackDelegate::playbackRate() const {
    if (shuttleSpeed_ == 0) return 1.0;
    return std::pow(2.0, std::abs(shuttleSpeed_) - 1)
           * (shuttleSpeed_ > 0 ? 1.0 : -1.0);
}

double PlaybackDelegate::getProgress() const {
    const double dur = getDuration();
    if (dur <= 0.0) return 0.0;
    return ops_->currentState().playback.positionSeconds / dur;
}

double PlaybackDelegate::getDuration() const {
    const auto& state = ops_->currentState();
    return state.project ? state.project->duration() : 0.0;
}

// ---------------------------------------------------------------------------
// Playback — play / pause / toggle
// ---------------------------------------------------------------------------

void PlaybackDelegate::play() {
    auto state = ops_->currentState();
    if (state.playback.isPlaying) return;

    status_ = PlaybackStatus::Playing;
    state.playback.isPlaying = true;

    // Record lastTickTime so the first tick computes a correct delta
    lastTickTime_ = std::chrono::steady_clock::now();

    ops_->setState(std::move(state));
}

void PlaybackDelegate::pause() {
    auto state = ops_->currentState();
    if (!state.playback.isPlaying) return;

    status_ = PlaybackStatus::Paused;
    state.playback.isPlaying = false;
    shuttleSpeed_ = 0;
    ops_->setState(std::move(state));
    ops_->renderCurrentFrame();
}

void PlaybackDelegate::togglePlayPause() {
    if (ops_->currentState().playback.isPlaying) pause();
    else play();
}

// ---------------------------------------------------------------------------
// Seek — with explicit SEEKING state
// ---------------------------------------------------------------------------

void PlaybackDelegate::seek(double positionSeconds) {
    auto state = ops_->currentState();

    // Remember if we were playing so we can resume after seek
    wasPlayingBeforeSeek_ = state.playback.isPlaying;

    // Transition to SEEKING state
    status_ = PlaybackStatus::Seeking;

    const double duration = state.project ? state.project->duration() : 0.0;
    state.playback.positionSeconds = std::clamp(positionSeconds, 0.0, duration);
    ops_->setState(std::move(state));

    // Seek the decode thread via the engine
    ops_->updateActiveVideo();

    // Render the frame at the new position
    ops_->renderCurrentFrame();

    // Restore previous state: resume PLAYING if was playing, else PAUSED
    if (wasPlayingBeforeSeek_) {
        status_ = PlaybackStatus::Playing;
        lastTickTime_ = std::chrono::steady_clock::now();
    } else {
        status_ = PlaybackStatus::Paused;
    }
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

    // Snap to markers
    for (const auto& marker : project->markers) {
        const double dist = std::abs(rawPos - marker.positionSeconds);
        if (dist < bestDist) {
            bestDist = dist;
            best = marker.positionSeconds;
        }
        if (marker.endPositionSeconds.has_value()) {
            const double distEnd = std::abs(rawPos - *marker.endPositionSeconds);
            if (distEnd < bestDist) {
                bestDist = distEnd;
                best = *marker.endPositionSeconds;
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

    for (const auto& track : project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.timelineIn < current - kEpsilon && clip.timelineIn > bestPos)
                bestPos = clip.timelineIn;
            if (clip.timelineOut < current - kEpsilon && clip.timelineOut > bestPos)
                bestPos = clip.timelineOut;
        }
    }
    // Include markers
    for (const auto& marker : project->markers) {
        if (marker.positionSeconds < current - kEpsilon && marker.positionSeconds > bestPos)
            bestPos = marker.positionSeconds;
        if (marker.endPositionSeconds.has_value()) {
            double ep = *marker.endPositionSeconds;
            if (ep < current - kEpsilon && ep > bestPos)
                bestPos = ep;
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
    // Include markers
    for (const auto& marker : project->markers) {
        if (marker.positionSeconds > current + kEpsilon && marker.positionSeconds < bestPos)
            bestPos = marker.positionSeconds;
        if (marker.endPositionSeconds.has_value()) {
            double ep = *marker.endPositionSeconds;
            if (ep > current + kEpsilon && ep < bestPos)
                bestPos = ep;
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
// Playback tick — called at ~33ms intervals by the owning notifier's timer
//
// Uses real elapsed time (steady_clock) for frame-accurate playback.
// This compensates for timer jitter: if a tick fires at 40ms instead of
// 33ms, the position advances by the correct 40ms * playbackRate.
// ---------------------------------------------------------------------------

void PlaybackDelegate::onPlaybackTick() {
    auto state = ops_->currentState();
    if (!state.playback.isPlaying) return;

    // Compute real elapsed time since last tick
    auto now = std::chrono::steady_clock::now();
    double elapsedSec = std::chrono::duration<double>(now - lastTickTime_).count();
    lastTickTime_ = now;

    // Clamp elapsed to avoid huge jumps (e.g., after debugger pause or system sleep)
    elapsedSec = std::clamp(elapsedSec, 0.0, 0.25);

    // Apply playback rate (includes shuttle speed and active clip speed)
    double rate = playbackRate();

    // Factor in the active clip's speed at the current position
    if (state.project && rate > 0.0) {
        double pos = state.playback.positionSeconds;
        for (const auto& track : state.project->tracks) {
            for (const auto& clip : track.clips) {
                if ((clip.sourceType == ClipSourceType::Video ||
                     clip.sourceType == ClipSourceType::Image) &&
                    pos >= clip.timelineIn - 0.001 && pos < clip.timelineOut + 0.001) {
                    // Check for speed ramp keyframes first
                    const auto* speedTrack = clip.keyframes.trackFor(KeyframeProperty::Speed);
                    if (speedTrack && speedTrack->keyframes.size() >= 2) {
                        double relTime = pos - clip.timelineIn;
                        double evaluated = speedTrack->evaluate(relTime);
                        rate *= std::max(0.1, evaluated);
                    } else if (std::abs(clip.speed) > 0.01 && std::abs(clip.speed - 1.0) > 0.01) {
                        rate *= std::abs(clip.speed);
                    }
                    goto done_speed;  // found active clip, stop searching
                }
            }
        }
        done_speed:;
    }

    double newPos = state.playback.positionSeconds + elapsedSec * rate;

    // Clamp to timeline bounds
    const double duration = state.project ? state.project->duration() : 0.0;
    const double outPt = state.playback.outPoint.value_or(duration);

    if (newPos >= outPt) {
        newPos = outPt;
        state.playback.isPlaying = false;
        status_ = PlaybackStatus::Stopped;
    } else if (newPos < 0.0) {
        newPos = 0.0;
        state.playback.isPlaying = false;
        status_ = PlaybackStatus::Stopped;
    }

    state.playback.positionSeconds = newPos;
    ops_->setState(std::move(state));
    // throttledRenderFrame() calls renderCurrentFrame() which already
    // seeks the engine — no need for a separate updateActiveVideo() call.
    ops_->throttledRenderFrame();
}

} // namespace gopost::video_editor
