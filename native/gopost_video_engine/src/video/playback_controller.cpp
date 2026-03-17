#include "playback_controller.hpp"

#include <algorithm>
#include <cmath>

namespace gopost {
namespace video {

// ============================================================================
// Construction
// ============================================================================

PlaybackController::PlaybackController(DecodeThread& decodeThread,
                                       RingFrameBuffer& ring,
                                       double totalDuration)
    : decodeThread_(decodeThread)
    , ring_(ring)
    , totalDuration_(totalDuration)
{
}

// ============================================================================
// Transport Controls
// ============================================================================

void PlaybackController::play() {
    if (state_ == PlaybackState::Playing) return;

    // Start the decode thread if it's not already running
    if (!decodeThread_.is_running()) {
        decodeThread_.start();
    }

    state_ = PlaybackState::Playing;
    lastTickTime_ = Clock::now();
}

void PlaybackController::pause() {
    if (state_ != PlaybackState::Playing) return;
    state_ = PlaybackState::Paused;
}

void PlaybackController::stop() {
    if (state_ == PlaybackState::Stopped) return;

    state_ = PlaybackState::Stopped;
    currentTimestamp_ = 0.0;
    lastFrame_.reset();
    underrunCount_ = 0;
}

void PlaybackController::seekTo(double timestampSeconds) {
    // Clamp to valid range
    timestampSeconds = std::clamp(timestampSeconds, 0.0, totalDuration_);

    // Remember if we were playing so we can resume after seek
    wasPlayingBeforeSeek_ = (state_ == PlaybackState::Playing);

    state_ = PlaybackState::Seeking;
    currentTimestamp_ = timestampSeconds;

    // Tell the decode thread to seek — it will clear the ring buffer
    // and restart decoding from the new position
    decodeThread_.seek_to(timestampSeconds);

    // Transition out of seeking state
    if (wasPlayingBeforeSeek_) {
        state_ = PlaybackState::Playing;
        lastTickTime_ = Clock::now();
    } else {
        state_ = PlaybackState::Paused;
    }
}

// ============================================================================
// Per-Frame Update
// ============================================================================

std::optional<DecodedFrame> PlaybackController::tick(bool* underrun) {
    if (underrun) *underrun = false;

    // Advance the clock if playing
    if (state_ == PlaybackState::Playing) {
        advanceClock();
    }

    // Pop the best available frame from the ring buffer
    popBestFrame();

    // Report underrun if we're playing but have no new frame
    if (state_ == PlaybackState::Playing && !lastFrame_.has_value()) {
        if (underrun) *underrun = true;
        ++underrunCount_;
    }

    // Check for end of playback
    if (state_ == PlaybackState::Playing && decodeThread_.is_eof() && ring_.empty()) {
        // All frames consumed — pause at end
        state_ = PlaybackState::Paused;
        currentTimestamp_ = totalDuration_;
    }

    return lastFrame_;
}

// ============================================================================
// Queries
// ============================================================================

double PlaybackController::getProgress() const {
    if (totalDuration_ <= 0.0) return 0.0;
    return std::clamp(currentTimestamp_ / totalDuration_, 0.0, 1.0);
}

bool PlaybackController::isFinished() const {
    return decodeThread_.is_eof() && ring_.empty() &&
           currentTimestamp_ >= totalDuration_ - 0.001;
}

// ============================================================================
// Configuration
// ============================================================================

void PlaybackController::setPlaybackRate(double rate) {
    playbackRate_ = std::clamp(rate, 0.1, 16.0);
}

void PlaybackController::setDuration(double seconds) {
    totalDuration_ = std::max(seconds, 0.0);
}

// ============================================================================
// Internal
// ============================================================================

void PlaybackController::advanceClock() {
    auto now = Clock::now();
    double elapsedSeconds =
        std::chrono::duration<double>(now - lastTickTime_).count();
    lastTickTime_ = now;

    // Guard against huge jumps (e.g. debugger pause, system sleep)
    if (elapsedSeconds > 0.5) {
        elapsedSeconds = 1.0 / 30.0;  // Cap to ~1 frame at 30fps
    }

    currentTimestamp_ += elapsedSeconds * playbackRate_;

    // Clamp to duration
    if (currentTimestamp_ > totalDuration_) {
        currentTimestamp_ = totalDuration_;
    }
}

void PlaybackController::popBestFrame() {
    // Pop all available frames whose timestamp <= currentTimestamp,
    // keeping only the most recent one (skip intermediate frames
    // that are older than the current playback position).
    bool gotNew = false;
    while (true) {
        auto frame = ring_.try_pop();
        if (!frame.has_value()) break;

        lastFrame_ = std::move(frame);
        gotNew = true;

        // If the frame we just popped is at or past our target time,
        // stop — we have the right frame to display.
        if (lastFrame_->timestamp_seconds >= currentTimestamp_ - 0.001) {
            break;
        }
        // Otherwise keep popping to catch up (frame skip on slow render)
    }

    // Track underrun only when playing and we got nothing new
    if (state_ == PlaybackState::Playing && !gotNew && lastFrame_.has_value()) {
        // We have a stale frame to display (underrun) — caller notified via tick()
    }
}

}  // namespace video
}  // namespace gopost
