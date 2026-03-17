#ifndef GOPOST_VIDEO_DECODE_THREAD_HPP
#define GOPOST_VIDEO_DECODE_THREAD_HPP

#include "decoder_interface.hpp"
#include "ring_frame_buffer.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace gopost {
namespace video {

// ============================================================================
// DecodeThread — Dedicated background thread that sequentially decodes frames
// from an IVideoDecoder into a RingFrameBuffer.
//
// Typical lifecycle:
//   1. Construct with decoder + ring buffer
//   2. start()  — spawns the thread and begins decoding
//   3. seek_to() — interrupts decoding, flushes ring, resumes from new pos
//   4. stop()   — signals shutdown, joins thread
//
// Thread model:
//   - The decode thread is the sole PRODUCER for the ring buffer
//   - The render/playback thread is the sole CONSUMER (calls ring.try_pop())
//   - seek_to() can be called from any thread (render thread, UI thread, etc.)
//
// Error handling:
//   - Decode errors on individual frames are skipped (logged via callback)
//   - File-level errors (decoder not open) prevent start
//   - EOF is signalled via atomic flag; consumer polls is_eof()
// ============================================================================

class DecodeThread {
public:
    /// Error severity for the error callback.
    enum class ErrorSeverity {
        Warning,   // Frame skipped, decoding continues
        Fatal,     // Decode loop cannot continue (e.g. decoder closed)
    };

    /// Callback for decode errors. Called on the decode thread.
    using ErrorCallback = std::function<void(ErrorSeverity severity,
                                             const std::string& message)>;

    /// @param decoder    Decoder instance (DecodeThread takes ownership).
    /// @param ring       Ring buffer to push decoded frames into (not owned).
    DecodeThread(std::unique_ptr<IVideoDecoder> decoder,
                 RingFrameBuffer& ring);

    ~DecodeThread();

    // Non-copyable, non-movable (owns a running thread)
    DecodeThread(const DecodeThread&) = delete;
    DecodeThread& operator=(const DecodeThread&) = delete;
    DecodeThread(DecodeThread&&) = delete;
    DecodeThread& operator=(DecodeThread&&) = delete;

    // ── Lifecycle ────────────────────────────────────────────────────

    /// Open the decoder on the given file path. Must be called before start().
    /// Returns false if the decoder fails to open the file.
    bool open(const std::string& path);

    /// Spawn the decode thread. The decoder must be open.
    /// Returns false if already running or decoder is not open.
    bool start();

    /// Signal the decode thread to stop and block until it joins.
    /// Safe to call even if not running (no-op).
    void stop();

    /// True if the decode thread is running.
    bool is_running() const;

    // ── Seek ─────────────────────────────────────────────────────────

    /// Request a seek to the given timestamp. Thread-safe.
    ///
    /// The decode thread will:
    ///   1. Pause decoding
    ///   2. Call decoder->seek_to(timestamp)
    ///   3. Clear the ring buffer (discard stale frames)
    ///   4. Reset EOF flag
    ///   5. Resume decoding from the new position
    ///
    /// The seek is asynchronous — this method returns immediately.
    /// The next frame popped from the ring will be at or after the target.
    void seek_to(double timestamp_seconds);

    // ── State queries (thread-safe) ──────────────────────────────────

    /// True if the decoder has reached end-of-file and all decoded frames
    /// have been pushed to the ring buffer.
    bool is_eof() const;

    /// Number of frames decoded since last start() or seek_to().
    uint64_t frames_decoded() const;

    /// Number of frames that failed to decode (skipped).
    uint64_t frames_dropped() const;

    // ── Configuration ────────────────────────────────────────────────

    /// Set the error callback. Must be called before start().
    void set_error_callback(ErrorCallback cb);

    /// Set how long the thread sleeps when the ring buffer is full.
    /// Default: 1ms. Lower = more responsive but more CPU. Higher = less CPU.
    void set_backoff_us(int microseconds);

private:
    void decode_loop();
    bool handle_seek();
    void report_error(ErrorSeverity severity, const std::string& msg);

    // Owned decoder
    std::unique_ptr<IVideoDecoder> decoder_;

    // Ring buffer (not owned — lifetime managed by caller)
    RingFrameBuffer& ring_;

    // Thread
    std::thread thread_;
    std::atomic<bool> running_{false};

    // Stop signal
    std::atomic<bool> stop_flag_{false};

    // Seek request (lock-free signalling)
    std::atomic<bool> seek_requested_{false};
    std::atomic<double> seek_target_{0.0};

    // State
    std::atomic<bool> eof_{false};
    std::atomic<uint64_t> frames_decoded_{0};
    std::atomic<uint64_t> frames_dropped_{0};

    // Configuration
    ErrorCallback error_cb_;
    std::atomic<int> backoff_us_{1000};  // 1ms default

    // Condition variable to wake the thread when it's sleeping (on EOF or full)
    std::mutex wake_mutex_;
    std::condition_variable wake_cv_;
};

}  // namespace video
}  // namespace gopost

#endif  // GOPOST_VIDEO_DECODE_THREAD_HPP
