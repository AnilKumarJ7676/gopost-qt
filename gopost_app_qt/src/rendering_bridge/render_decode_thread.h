#pragma once

#include "rendering_bridge/qprocess_video_decoder.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

namespace gopost::rendering {

// ============================================================================
// RenderDecodeThread — Background thread that decodes video frames via
// QProcessVideoDecoder and buffers them for consumption by the render thread.
//
// This is the Qt-side equivalent of the native DecodeThread + RingFrameBuffer.
// Uses a mutex-guarded deque (not lock-free SPSC) because the QProcess reads
// are the bottleneck, not the queue throughput.
//
// Thread safety:
//   - start/stop/seek: call from any thread (UI or render)
//   - popFrame: call from render thread
// ============================================================================

class RenderDecodeThread {
public:
    explicit RenderDecodeThread(int bufferCapacity = 24);
    ~RenderDecodeThread();

    RenderDecodeThread(const RenderDecodeThread&) = delete;
    RenderDecodeThread& operator=(const RenderDecodeThread&) = delete;

    /// Open a video file and start decoding from the beginning.
    bool open(const QString& path);

    /// Stop decoding and release the decoder.
    void close();

    bool isOpen() const { return open_.load(std::memory_order_acquire); }

    /// Seek to a timestamp. Clears buffered frames and restarts decode.
    void seekTo(double timestampSeconds);

    /// Pop the next decoded frame (FIFO). Returns nullopt if buffer is empty.
    std::optional<RenderDecodedFrame> popFrame();

    /// Peek at the timestamp of the front frame without removing it.
    std::optional<double> peekTimestamp() const;

    /// Number of frames currently buffered.
    int bufferedFrameCount() const;

    /// Metadata passthrough
    int width() const;
    int height() const;
    double frameRate() const;
    double duration() const;
    bool isEof() const { return eof_.load(std::memory_order_acquire); }

private:
    void decodeLoop();

    int bufferCapacity_;
    std::atomic<bool> open_{false};
    std::atomic<bool> eof_{false};
    std::atomic<bool> stopFlag_{false};
    std::atomic<bool> seekRequested_{false};
    std::atomic<double> seekTarget_{0.0};

    // Decoder (owned, accessed only on decode thread after open)
    std::unique_ptr<QProcessVideoDecoder> decoder_;

    // Frame buffer (mutex-guarded)
    mutable std::mutex bufferMutex_;
    std::deque<RenderDecodedFrame> buffer_;
    std::condition_variable bufferCV_;  // wakes decode thread when buffer has space

    // Decode thread
    std::thread thread_;
    std::mutex wakeMutex_;
    std::condition_variable wakeCV_;

    // Cached metadata (set during open, immutable after)
    int width_ = 0;
    int height_ = 0;
    double fps_ = 30.0;
    double duration_ = 0.0;
};

} // namespace gopost::rendering
