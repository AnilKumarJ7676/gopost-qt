#ifndef GOPOST_VIDEO_DECODER_POOL_HPP
#define GOPOST_VIDEO_DECODER_POOL_HPP

#include "decoder_interface.hpp"
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct GopostEngine;

namespace gopost {
namespace video {

/// Priority levels for decoder requests.
enum class DecoderPriority : int {
    High   = 0,  // Currently visible viewport thumbnails, active preview
    Medium = 1,  // Preview panel decode for active clip
    Low    = 2,  // Off-screen prefetch thumbnails
};

/// Handle returned by acquire(). Caller must call release() when done.
/// RAII wrapper ensures the decoder is returned to the pool even on exception.
class DecoderLease {
public:
    DecoderLease() = default;
    DecoderLease(DecoderLease&& o) noexcept;
    DecoderLease& operator=(DecoderLease&& o) noexcept;
    ~DecoderLease();

    DecoderLease(const DecoderLease&) = delete;
    DecoderLease& operator=(const DecoderLease&) = delete;

    /// The decoder — valid only while this lease is alive.
    IVideoDecoder* decoder() const { return decoder_; }
    explicit operator bool() const { return decoder_ != nullptr; }

private:
    friend class DecoderPool;
    DecoderLease(IVideoDecoder* dec, class DecoderPool* pool, uint32_t slot_id);

    IVideoDecoder* decoder_ = nullptr;
    DecoderPool* pool_ = nullptr;
    uint32_t slot_id_ = 0;
};

/// Manages a bounded pool of FFmpeg/platform video decoders.
///
/// At most `max_decoders` decoders can be alive simultaneously. Requests beyond
/// the limit block in a FIFO queue (ordered by priority) until a slot frees up.
///
/// The pool also maintains a small LRU cache of idle decoders that are already
/// opened on a specific file path, so re-acquiring a decoder for the same file
/// is O(1) without re-opening the container.
class DecoderPool {
public:
    /// @param engine       Engine pointer for frame pool access.
    /// @param max_decoders Maximum number of concurrent decoder instances.
    explicit DecoderPool(GopostEngine* engine, int max_decoders = 2);
    ~DecoderPool();

    DecoderPool(const DecoderPool&) = delete;
    DecoderPool& operator=(const DecoderPool&) = delete;

    /// Acquire a decoder opened on `path`.
    ///
    /// If the pool has a cached idle decoder for this path it is returned
    /// immediately. Otherwise, if a slot is free a new decoder is created and
    /// opened. If all slots are occupied the caller blocks until one is freed.
    ///
    /// @param path     Media file path to open.
    /// @param priority Queue priority (higher priority requests are serviced first).
    /// @return A lease that must stay alive while the decoder is in use.
    ///         The lease's destructor returns the decoder to the pool.
    ///         Returns an empty lease if the pool is shutting down or open failed.
    DecoderLease acquire(const std::string& path, DecoderPriority priority = DecoderPriority::Medium);

    /// Try to acquire without blocking. Returns empty lease if no slot available.
    DecoderLease try_acquire(const std::string& path);

    /// Change the maximum number of concurrent decoders at runtime.
    /// If the new limit is lower than current active count, excess decoders
    /// will be reclaimed as they are released.
    void set_max_decoders(int max_decoders);

    /// Get current max decoder limit.
    int max_decoders() const;

    /// Get the number of currently active (leased out) decoders.
    int active_count() const;

    /// Close all idle cached decoders to free memory.
    void flush_idle();

    /// Shut down the pool. All pending acquire() calls return empty leases.
    void shutdown();

    /// Get approximate memory usage of all decoders (active + idle).
    size_t estimated_memory_bytes() const;

private:
    friend class DecoderLease;

    struct DecoderSlot {
        uint32_t id = 0;
        std::unique_ptr<IVideoDecoder> decoder;
        std::string open_path;
        bool active = false;  // true = leased out, false = idle/cached
    };

    struct WaitEntry {
        DecoderPriority priority;
        std::string path;
        std::condition_variable cv;
        DecoderSlot* granted_slot = nullptr;
        bool cancelled = false;
    };

    void release_slot(uint32_t slot_id);
    DecoderSlot* find_idle_for_path(const std::string& path);
    DecoderSlot* find_or_create_slot(const std::string& path);
    DecoderSlot* evict_idle_slot();

    GopostEngine* engine_;
    int max_decoders_;
    uint32_t next_slot_id_ = 1;
    bool shutting_down_ = false;

    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<DecoderSlot>> slots_;
    std::deque<WaitEntry*> wait_queue_;
};

}  // namespace video
}  // namespace gopost

#endif  // GOPOST_VIDEO_DECODER_POOL_HPP
