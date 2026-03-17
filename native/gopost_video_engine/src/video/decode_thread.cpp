#include "decode_thread.hpp"

#include <chrono>
#include <sstream>

namespace gopost {
namespace video {

// ============================================================================
// Construction / Destruction
// ============================================================================

DecodeThread::DecodeThread(std::unique_ptr<IVideoDecoder> decoder,
                           RingFrameBuffer& ring)
    : decoder_(std::move(decoder))
    , ring_(ring)
{
}

DecodeThread::~DecodeThread() {
    stop();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool DecodeThread::open(const std::string& path) {
    if (running_.load(std::memory_order_relaxed)) {
        report_error(ErrorSeverity::Warning, "Cannot open while running");
        return false;
    }
    if (!decoder_) {
        report_error(ErrorSeverity::Fatal, "No decoder instance");
        return false;
    }

    // Close previous file if open
    if (decoder_->is_open()) {
        decoder_->close();
    }

    if (!decoder_->open(path)) {
        std::ostringstream oss;
        oss << "Failed to open: " << path;
        report_error(ErrorSeverity::Fatal, oss.str());
        return false;
    }
    return true;
}

bool DecodeThread::start() {
    if (running_.load(std::memory_order_relaxed)) {
        return false;  // Already running
    }
    if (!decoder_ || !decoder_->is_open()) {
        report_error(ErrorSeverity::Fatal, "Decoder not open — call open() first");
        return false;
    }

    // Reset state for a fresh run
    stop_flag_.store(false, std::memory_order_relaxed);
    seek_requested_.store(false, std::memory_order_relaxed);
    eof_.store(false, std::memory_order_relaxed);
    frames_decoded_.store(0, std::memory_order_relaxed);
    frames_dropped_.store(0, std::memory_order_relaxed);
    ring_.restart();

    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&DecodeThread::decode_loop, this);
    return true;
}

void DecodeThread::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    // Signal stop
    stop_flag_.store(true, std::memory_order_release);

    // Wake the thread in case it's sleeping on the condition variable
    {
        std::lock_guard<std::mutex> lock(wake_mutex_);
        wake_cv_.notify_one();
    }

    // Shut down the ring buffer to unblock any spinning push()
    ring_.shutdown();

    // Join the thread
    if (thread_.joinable()) {
        thread_.join();
    }

    running_.store(false, std::memory_order_release);
}

bool DecodeThread::is_running() const {
    return running_.load(std::memory_order_acquire);
}

// ============================================================================
// Seek
// ============================================================================

void DecodeThread::seek_to(double timestamp_seconds) {
    // Store the target first, then set the flag.
    // The decode loop reads in reverse order (flag then target) so
    // it always sees a valid target when the flag is set.
    seek_target_.store(timestamp_seconds, std::memory_order_relaxed);
    seek_requested_.store(true, std::memory_order_release);

    // Reset EOF — we're seeking to a new position
    eof_.store(false, std::memory_order_relaxed);

    // Wake the thread if it's sleeping (EOF or ring-full backoff)
    {
        std::lock_guard<std::mutex> lock(wake_mutex_);
        wake_cv_.notify_one();
    }
}

// ============================================================================
// State Queries
// ============================================================================

bool DecodeThread::is_eof() const {
    return eof_.load(std::memory_order_acquire);
}

uint64_t DecodeThread::frames_decoded() const {
    return frames_decoded_.load(std::memory_order_relaxed);
}

uint64_t DecodeThread::frames_dropped() const {
    return frames_dropped_.load(std::memory_order_relaxed);
}

// ============================================================================
// Configuration
// ============================================================================

void DecodeThread::set_error_callback(ErrorCallback cb) {
    error_cb_ = std::move(cb);
}

void DecodeThread::set_backoff_us(int microseconds) {
    backoff_us_.store(microseconds > 0 ? microseconds : 1,
                      std::memory_order_relaxed);
}

// ============================================================================
// Decode Loop (runs on the decode thread)
// ============================================================================

void DecodeThread::decode_loop() {
    while (!stop_flag_.load(std::memory_order_acquire)) {

        // ── Priority 1: Handle pending seek ──────────────────────────
        if (handle_seek()) {
            continue;  // Re-check stop flag after seek
        }

        // ── Priority 2: If EOF, sleep until seek or stop ─────────────
        if (eof_.load(std::memory_order_relaxed)) {
            std::unique_lock<std::mutex> lock(wake_mutex_);
            wake_cv_.wait_for(lock,
                std::chrono::milliseconds(50),
                [this] {
                    return stop_flag_.load(std::memory_order_relaxed) ||
                           seek_requested_.load(std::memory_order_relaxed);
                });
            continue;
        }

        // ── Priority 3: If ring is full, back off ────────────────────
        if (ring_.full()) {
            std::unique_lock<std::mutex> lock(wake_mutex_);
            wake_cv_.wait_for(lock,
                std::chrono::microseconds(backoff_us_.load(std::memory_order_relaxed)),
                [this] {
                    return stop_flag_.load(std::memory_order_relaxed) ||
                           seek_requested_.load(std::memory_order_relaxed) ||
                           !ring_.full();
                });
            continue;
        }

        // ── Priority 4: Decode next frame ────────────────────────────
        auto frame = decoder_->decode_next_frame();

        if (!frame.has_value()) {
            if (decoder_->is_eof()) {
                eof_.store(true, std::memory_order_release);
            } else {
                // Decode error — skip this frame
                frames_dropped_.fetch_add(1, std::memory_order_relaxed);
                report_error(ErrorSeverity::Warning, "Frame decode failed, skipping");
            }
            continue;
        }

        // Push to ring buffer (non-blocking — we checked !full() above,
        // but another check via try_push handles the race gracefully)
        if (ring_.try_push(std::move(*frame))) {
            frames_decoded_.fetch_add(1, std::memory_order_relaxed);
        } else {
            // Ring became full between our check and push — rare, not an error.
            // The frame is lost but the decoder will produce the next one.
            // This is acceptable for real-time playback (frame skip).
            frames_dropped_.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

bool DecodeThread::handle_seek() {
    if (!seek_requested_.load(std::memory_order_acquire)) {
        return false;
    }

    // Consume the seek request
    double target = seek_target_.load(std::memory_order_relaxed);
    seek_requested_.store(false, std::memory_order_release);

    // 1. Seek the decoder
    if (!decoder_->seek_to(target)) {
        std::ostringstream oss;
        oss << "Seek failed to " << target << "s";
        report_error(ErrorSeverity::Warning, oss.str());
        // Don't clear the ring on failed seek — keep existing frames
        return true;
    }

    // 2. Clear the ring buffer (discard stale pre-decoded frames)
    ring_.clear();

    // 3. Reset EOF
    eof_.store(false, std::memory_order_relaxed);

    // 4. Reset frame counters for the new segment
    frames_decoded_.store(0, std::memory_order_relaxed);
    frames_dropped_.store(0, std::memory_order_relaxed);

    return true;
}

void DecodeThread::report_error(ErrorSeverity severity, const std::string& msg) {
    if (error_cb_) {
        error_cb_(severity, msg);
    }
}

}  // namespace video
}  // namespace gopost
