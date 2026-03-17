#include "ring_frame_buffer.hpp"

#include <algorithm>
#include <thread>

namespace gopost {
namespace video {

// ============================================================================
// Construction
// ============================================================================

RingFrameBuffer::RingFrameBuffer(size_t capacity)
    : capacity_(capacity < 1 ? 1 : capacity)
    , buffer_(capacity_)
{
    // Indices start at 0; the modular arithmetic in push/pop handles wrapping.
    write_idx_.value.store(0, std::memory_order_relaxed);
    read_idx_.value.store(0, std::memory_order_relaxed);
}

// ============================================================================
// Producer: try_push / push
// ============================================================================

bool RingFrameBuffer::try_push(DecodedFrame&& frame) {
    const size_t w = write_idx_.value.load(std::memory_order_relaxed);
    const size_t r = read_idx_.value.load(std::memory_order_acquire);

    // Full when (write - read) == capacity.
    // Using unsigned subtraction handles index wrapping correctly.
    if (w - r >= capacity_) {
        return false;
    }

    // Move the frame into the slot. The previous occupant (if any) is
    // overwritten — its pixels vector is freed by the move-assignment.
    buffer_[w % capacity_] = std::move(frame);

    // Release-store ensures the frame data written above is visible to
    // the consumer before it observes the incremented write index.
    write_idx_.value.store(w + 1, std::memory_order_release);
    return true;
}

bool RingFrameBuffer::push(DecodedFrame&& frame) {
    while (!shutdown_.load(std::memory_order_relaxed)) {
        if (try_push(std::move(frame))) {
            return true;
        }
        // Yield instead of busy-spinning — lets the consumer thread run
        // on the same core (important on mobile with few cores).
        std::this_thread::yield();
    }
    return false;  // Shutdown requested
}

// ============================================================================
// Consumer: try_pop / pop
// ============================================================================

std::optional<DecodedFrame> RingFrameBuffer::try_pop() {
    const size_t r = read_idx_.value.load(std::memory_order_relaxed);
    const size_t w = write_idx_.value.load(std::memory_order_acquire);

    // Empty when read has caught up to write.
    if (r >= w) {
        return std::nullopt;
    }

    // Move the frame out of the slot.
    DecodedFrame result = std::move(buffer_[r % capacity_]);

    // Release-store ensures the slot is logically freed before the
    // producer observes the incremented read index and reuses the slot.
    read_idx_.value.store(r + 1, std::memory_order_release);
    return result;
}

std::optional<DecodedFrame> RingFrameBuffer::pop() {
    while (!shutdown_.load(std::memory_order_relaxed)) {
        auto frame = try_pop();
        if (frame.has_value()) {
            return frame;
        }
        std::this_thread::yield();
    }
    return std::nullopt;  // Shutdown requested
}

// ============================================================================
// Shared: clear / shutdown / restart
// ============================================================================

void RingFrameBuffer::clear() {
    // Reset the pixel buffers of all occupied slots so memory is freed
    // promptly rather than lingering until the slot is overwritten.
    const size_t r = read_idx_.value.load(std::memory_order_relaxed);
    const size_t w = write_idx_.value.load(std::memory_order_relaxed);
    for (size_t i = r; i < w; ++i) {
        buffer_[i % capacity_] = DecodedFrame{};
    }

    // Reset both indices to zero. Relaxed is fine here because clear()
    // is called only when both threads are paused (e.g. during seek).
    write_idx_.value.store(0, std::memory_order_relaxed);
    read_idx_.value.store(0, std::memory_order_relaxed);
}

void RingFrameBuffer::shutdown() {
    shutdown_.store(true, std::memory_order_release);
}

void RingFrameBuffer::restart() {
    shutdown_.store(false, std::memory_order_release);
}

// ============================================================================
// Queries
// ============================================================================

size_t RingFrameBuffer::size() const {
    const size_t w = write_idx_.value.load(std::memory_order_acquire);
    const size_t r = read_idx_.value.load(std::memory_order_acquire);
    return w - r;
}

bool RingFrameBuffer::full() const {
    return size() >= capacity_;
}

bool RingFrameBuffer::empty() const {
    return size() == 0;
}

}  // namespace video
}  // namespace gopost
