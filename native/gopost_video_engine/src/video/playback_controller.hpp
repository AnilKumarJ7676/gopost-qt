#ifndef GOPOST_VIDEO_PLAYBACK_CONTROLLER_HPP
#define GOPOST_VIDEO_PLAYBACK_CONTROLLER_HPP

#include "decode_thread.hpp"
#include "ring_frame_buffer.hpp"

#include <chrono>
#include <cstdint>
#include <optional>

namespace gopost {
namespace video {

// ============================================================================
// PlaybackController — Drives real-time video playback by advancing a clock
// and pulling decoded frames from a RingFrameBuffer.
//
// Typical lifecycle:
//   1. Construct with a DecodeThread + RingFrameBuffer + duration
//   2. play()   — begin advancing the clock
//   3. tick()   — called every frame (vsync) to advance time and pop frames
//   4. pause()  — freeze the clock
//   5. seekTo() — jump to a new position
//
// Thread model:
//   - All public methods are called from the render/UI thread (single-threaded)
//   - The DecodeThread runs separately, producing frames into the ring buffer
//   - tick() is the consumer of the ring buffer (SPSC consumer side)
//
// Clock:
//   - Uses std::chrono::steady_clock for monotonic, drift-free timing
//   - playbackRate scales elapsed time (1.0 = normal, 2.0 = double speed)
//   - On buffer underrun, the last successfully decoded frame is retained
// ============================================================================

enum class PlaybackState : uint8_t {
    Stopped,
    Playing,
    Paused,
    Seeking,
};

class PlaybackController {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    /// @param decodeThread  Decode thread (not owned — caller manages lifetime).
    /// @param ring          Ring buffer shared with the decode thread (not owned).
    /// @param totalDuration Total video duration in seconds.
    PlaybackController(DecodeThread& decodeThread,
                       RingFrameBuffer& ring,
                       double totalDuration);

    ~PlaybackController() = default;

    // Non-copyable, non-movable (references external objects)
    PlaybackController(const PlaybackController&) = delete;
    PlaybackController& operator=(const PlaybackController&) = delete;

    // ── Transport controls ──────────────────────────────────────────

    /// Start or resume playback. Starts the decode thread if not running.
    void play();

    /// Pause playback. Clock stops advancing.
    void pause();

    /// Stop playback. Resets position to 0.
    void stop();

    /// Seek to a timestamp (seconds). Clears the ring buffer and
    /// repositions the decode thread. If playback was active, it resumes
    /// after the seek completes.
    void seekTo(double timestampSeconds);

    // ── Per-frame update ────────────────────────────────────────────

    /// Called every frame (vsync). Advances the clock if playing,
    /// pops the best frame from the ring buffer, and returns it.
    ///
    /// Returns the frame to display:
    ///   - A freshly popped frame if one is available at or before currentTimestamp
    ///   - The last displayed frame on buffer underrun
    ///   - std::nullopt if no frame has ever been decoded
    ///
    /// @param underrun  If non-null, set to true when no new frame was available.
    std::optional<DecodedFrame> tick(bool* underrun = nullptr);

    // ── Queries ─────────────────────────────────────────────────────

    PlaybackState state() const { return state_; }
    double currentTimestamp() const { return currentTimestamp_; }
    double playbackRate() const { return playbackRate_; }
    double duration() const { return totalDuration_; }

    /// Progress as a fraction [0.0, 1.0].
    double getProgress() const;

    /// True if the decode thread has reached EOF and all buffered frames
    /// have been consumed.
    bool isFinished() const;

    // ── Configuration ───────────────────────────────────────────────

    /// Set playback rate. 1.0 = normal, 0.5 = half speed, 2.0 = double.
    /// Clamped to [0.1, 16.0].
    void setPlaybackRate(double rate);

    /// Set total duration (e.g. after loading a new file).
    void setDuration(double seconds);

    // ── Statistics ──────────────────────────────────────────────────

    uint64_t underrunCount() const { return underrunCount_; }

private:
    void advanceClock();
    void popBestFrame();

    // External references (not owned)
    DecodeThread& decodeThread_;
    RingFrameBuffer& ring_;

    // State
    PlaybackState state_ = PlaybackState::Stopped;
    bool wasPlayingBeforeSeek_ = false;

    // Clock
    double currentTimestamp_ = 0.0;
    double playbackRate_ = 1.0;
    double totalDuration_ = 0.0;
    TimePoint lastTickTime_{};

    // Frame cache (last displayed frame — retained on underrun)
    std::optional<DecodedFrame> lastFrame_;

    // Stats
    uint64_t underrunCount_ = 0;
};

}  // namespace video
}  // namespace gopost

#endif  // GOPOST_VIDEO_PLAYBACK_CONTROLLER_HPP
