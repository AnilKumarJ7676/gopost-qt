#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>

namespace gopost::memory {

class MemoryPool {
public:
    explicit MemoryPool(uint64_t budget = 0) : budget_(budget) {}

    bool tryAcquire(uint64_t bytes) {
        uint64_t current = usage_.load(std::memory_order_relaxed);
        do {
            if (current + bytes > budget_)
                return false;
        } while (!usage_.compare_exchange_weak(
            current, current + bytes,
            std::memory_order_acq_rel,
            std::memory_order_relaxed));
        return true;
    }

    void release(uint64_t bytes) {
        uint64_t prev = usage_.fetch_sub(bytes, std::memory_order_acq_rel);
        assert(prev >= bytes && "MemoryPool::release underflow");
        (void)prev;
    }

    [[nodiscard]] uint64_t currentUsage() const {
        return usage_.load(std::memory_order_acquire);
    }

    [[nodiscard]] uint64_t budget() const { return budget_; }

private:
    std::atomic<uint64_t> usage_{0};
    uint64_t budget_;
};

} // namespace gopost::memory
