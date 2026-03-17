#include "rendering_bridge/render_decode_thread.h"

#include <QDebug>
#include <algorithm>
#include <chrono>

namespace gopost::rendering {

RenderDecodeThread::RenderDecodeThread(int bufferCapacity)
    : bufferCapacity_(std::max(bufferCapacity, 2))
{
}

RenderDecodeThread::~RenderDecodeThread() {
    close();
}

// ============================================================================
// Open / Close
// ============================================================================

bool RenderDecodeThread::open(const QString& path) {
    close();  // Ensure clean state

    // Only probe metadata on the main thread — don't start ffmpeg process here.
    // The actual QProcess for decoding will be created lazily on the decode thread.
    decoder_ = std::make_unique<QProcessVideoDecoder>();
    if (!decoder_->open(path)) {
        decoder_.reset();
        return false;
    }

    // Cache metadata
    width_    = decoder_->width();
    height_   = decoder_->height();
    fps_      = decoder_->frameRate();
    duration_ = decoder_->duration();

    // Start decode thread — the decoder is moved to the thread
    stopFlag_.store(false, std::memory_order_relaxed);
    seekRequested_.store(false, std::memory_order_relaxed);
    eof_.store(false, std::memory_order_relaxed);
    open_.store(true, std::memory_order_release);

    thread_ = std::thread(&RenderDecodeThread::decodeLoop, this);
    return true;
}

void RenderDecodeThread::close() {
    if (!open_.load(std::memory_order_acquire) && !thread_.joinable()) return;

    // Signal stop
    stopFlag_.store(true, std::memory_order_release);

    // Wake decode thread so it can exit and clean up its QProcess
    {
        std::lock_guard<std::mutex> lock(wakeMutex_);
        wakeCV_.notify_one();
    }
    {
        std::lock_guard<std::mutex> lock(bufferMutex_);
        bufferCV_.notify_one();
    }

    // Wait for the decode thread to finish — it will destroy the QProcess
    // on its own thread, avoiding the cross-thread QProcess assert.
    if (thread_.joinable()) thread_.join();

    // Clean up buffer (safe from main thread — no QProcess involved)
    {
        std::lock_guard<std::mutex> lock(bufferMutex_);
        buffer_.clear();
    }

    // The decoder is now safe to reset — the QProcess was already destroyed
    // by the decode thread in decodeLoop() cleanup.
    decoder_.reset();

    open_.store(false, std::memory_order_release);
    eof_.store(false, std::memory_order_relaxed);
}

// ============================================================================
// Seek
// ============================================================================

void RenderDecodeThread::seekTo(double timestampSeconds) {
    seekTarget_.store(timestampSeconds, std::memory_order_relaxed);
    seekRequested_.store(true, std::memory_order_release);
    eof_.store(false, std::memory_order_relaxed);

    // Wake decode thread
    {
        std::lock_guard<std::mutex> lock(wakeMutex_);
        wakeCV_.notify_one();
    }
}

// ============================================================================
// Consumer API
// ============================================================================

std::optional<RenderDecodedFrame> RenderDecodeThread::popFrame() {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    if (buffer_.empty()) return std::nullopt;

    RenderDecodedFrame frame = std::move(buffer_.front());
    buffer_.pop_front();

    // Notify decode thread that buffer has space
    bufferCV_.notify_one();
    return frame;
}

std::optional<double> RenderDecodeThread::peekTimestamp() const {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    if (buffer_.empty()) return std::nullopt;
    return buffer_.front().timestamp_seconds;
}

int RenderDecodeThread::bufferedFrameCount() const {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    return static_cast<int>(buffer_.size());
}

// ============================================================================
// Metadata
// ============================================================================

int RenderDecodeThread::width() const { return width_; }
int RenderDecodeThread::height() const { return height_; }
double RenderDecodeThread::frameRate() const { return fps_; }
double RenderDecodeThread::duration() const { return duration_; }

// ============================================================================
// Decode loop (runs on background thread)
// ============================================================================

void RenderDecodeThread::decodeLoop() {
    while (!stopFlag_.load(std::memory_order_acquire)) {

        // ── Handle seek request ──────────────────────────────────────
        if (seekRequested_.load(std::memory_order_acquire)) {
            double target = seekTarget_.load(std::memory_order_relaxed);
            seekRequested_.store(false, std::memory_order_release);

            // Clear buffer
            {
                std::lock_guard<std::mutex> lock(bufferMutex_);
                buffer_.clear();
            }

            // Restart decoder at new position
            if (decoder_) {
                decoder_->seekTo(target);
            }
            eof_.store(false, std::memory_order_relaxed);
            continue;
        }

        // ── If EOF, sleep until seek or stop ─────────────────────────
        if (eof_.load(std::memory_order_relaxed)) {
            std::unique_lock<std::mutex> lock(wakeMutex_);
            wakeCV_.wait_for(lock, std::chrono::milliseconds(50), [this] {
                return stopFlag_.load(std::memory_order_relaxed) ||
                       seekRequested_.load(std::memory_order_relaxed);
            });
            continue;
        }

        // ── If buffer is full, wait for consumer ─────────────────────
        {
            std::unique_lock<std::mutex> lock(bufferMutex_);
            if (static_cast<int>(buffer_.size()) >= bufferCapacity_) {
                bufferCV_.wait_for(lock, std::chrono::milliseconds(10), [this] {
                    return stopFlag_.load(std::memory_order_relaxed) ||
                           seekRequested_.load(std::memory_order_relaxed) ||
                           static_cast<int>(buffer_.size()) < bufferCapacity_;
                });
                continue;
            }
        }

        // ── Decode next frame ────────────────────────────────────────
        if (!decoder_) {
            eof_.store(true, std::memory_order_release);
            continue;
        }

        auto frame = decoder_->decodeNextFrame();
        if (!frame.has_value()) {
            if (decoder_->isEof()) {
                eof_.store(true, std::memory_order_release);
            }
            continue;
        }

        // Push to buffer
        {
            std::lock_guard<std::mutex> lock(bufferMutex_);
            buffer_.push_back(std::move(*frame));
        }
    }

    // ── Thread cleanup: destroy QProcess on this thread ──────────────
    // The QProcess was created on this thread (via lazy-start in
    // decodeNextFrame/seekTo), so it MUST be destroyed here to avoid
    // Qt's cross-thread assert.
    if (decoder_) {
        qDebug() << "[RenderDecodeThread] cleaning up decoder on decode thread";
        decoder_->close();
    }
}

} // namespace gopost::rendering
