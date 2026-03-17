#ifndef GOPOST_VIDEO_RING_FRAME_BUFFER_HPP
#define GOPOST_VIDEO_RING_FRAME_BUFFER_HPP

#include "decoder_interface.hpp"

#include <atomic>
#include <cstddef>
#include <new>
#include <optional>
#include <vector>

namespace gopost {
namespace video {

// ============================================================================
// RingFrameBuffer — Lock-free SPSC (Single Producer, Single Consumer)
// ring buffer for pre-decoded video frames.
//
// Design:
//   - One thread calls push() (decoder/producer thread)
//   - One thread calls try_pop() (render/consumer thread)
//   - clear() must be called when BOTH threads are quiesced (e.g. on seek)
//
// Memory ordering:
//   - write_idx_ uses release on store so the consumer sees the written frame
//   - read_idx_ uses release on store so the producer sees the freed slot
//   - Loads of the OTHER thread's index use acquire to synchronize
//
// Cache-line padding prevents false sharing between producer and consumer
// indices on architectures where cache lines are <= 128 bytes.
// ============================================================================

class RingFrameBuffer {
public:
    static constexpr size_t kDefaultCapacity = 30;

    explicit RingFrameBuffer(size_t capacity = kDefaultCapacity);
    ~RingFrameBuffer() = default;

    // Non-copyable, non-movable (atomics + internal buffer)
    RingFrameBuffer(const RingFrameBuffer&) = delete;
    RingFrameBuffer& operator=(const RingFrameBuffer&) = delete;
    RingFrameBuffer(RingFrameBuffer&&) = delete;
    RingFrameBuffer& operator=(RingFrameBuffer&&) = delete;

    // ── Producer API (single writer thread) ─────────────────────────

    /// Non-blocking push. Returns true if the frame was enqueued,
    /// false if the buffer is full (caller should back-off or drop).
    bool try_push(DecodedFrame&& frame);

    /// Blocking push. Spins (with yield) until a slot opens or the
    /// buffer is shut down. Returns false only if shutdown was requested.
    bool push(DecodedFrame&& frame);

    // ── Consumer API (single reader thread) ──────────────────────────

    /// Non-blocking pop. Returns nullopt if the buffer is empty.
    std::optional<DecodedFrame> try_pop();

    /// Blocking pop. Spins (with yield) until a frame is available or
    /// the buffer is shut down. Returns nullopt only on shutdown.
    std::optional<DecodedFrame> pop();

    // ── Shared API (call only when BOTH threads are quiesced) ────────

    /// Discard all buffered frames and reset indices.
    /// MUST be called only when neither producer nor consumer is active
    /// (e.g. during a seek pause).
    void clear();

    /// Signal the buffer to unblock any spinning push/pop.
    /// After shutdown, push returns false and pop returns nullopt.
    void shutdown();

    /// Re-arm after shutdown (e.g. after seek completes).
    void restart();

    // ── Queries (safe to call from any thread) ───────────────────────

    /// Number of frames currently buffered.
    size_t size() const;

    /// True when size() == capacity.
    bool full() const;

    /// True when size() == 0.
    bool empty() const;

    /// Fixed capacity.
    size_t capacity() const { return capacity_; }

private:
    // Pad atomics to separate cache lines and prevent false sharing.
    // Hardware_destructive_interference_size is C++17 but not universally
    // available, so we use a conservative 128-byte alignment.
    static constexpr size_t kCacheLineSize = 128;

    struct alignas(kCacheLineSize) AlignedIndex {
        std::atomic<size_t> value{0};
    };

    size_t capacity_;
    std::vector<DecodedFrame> buffer_;

    AlignedIndex write_idx_;   // Only modified by producer
    AlignedIndex read_idx_;    // Only modified by consumer
    std::atomic<bool> shutdown_{false};
};

}  // namespace video
}  // namespace gopost

#endif  // GOPOST_VIDEO_RING_FRAME_BUFFER_HPP
