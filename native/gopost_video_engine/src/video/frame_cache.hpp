#ifndef GOPOST_VIDEO_FRAME_CACHE_HPP
#define GOPOST_VIDEO_FRAME_CACHE_HPP

#include "gopost/types.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

struct GopostEngine;
struct GopostFramePool;

namespace gopost {
namespace video {

/** Cache key: timeline position (frame index) + clip id + effect hash. */
struct FrameCacheKey {
    int64_t frame_index = 0;
    int32_t clip_id = 0;
    uint32_t effect_hash = 0;
    bool operator==(const FrameCacheKey& o) const {
        return frame_index == o.frame_index && clip_id == o.clip_id && effect_hash == o.effect_hash;
    }
};

struct FrameCacheKeyHash {
    size_t operator()(const FrameCacheKey& k) const {
        size_t h = static_cast<size_t>(k.frame_index);
        h ^= static_cast<size_t>(k.clip_id) << 20;
        h ^= static_cast<size_t>(k.effect_hash) << 10;
        return h;
    }
};

/** LRU frame cache. Entries are GopostFrame*; cache does not own frames, but tracks byte size for eviction. */
class FrameCache {
public:
    /** acquire_frame: (width, height, format) -> GopostFrame* from pool. release_frame: return to pool. */
    using FrameAcquireFn = std::function<GopostFrame*(uint32_t w, uint32_t h, GopostPixelFormat fmt)>;
    using FrameReleaseFn = std::function<void(GopostFrame*)>;

    explicit FrameCache(size_t max_bytes, FrameAcquireFn acquire, FrameReleaseFn release);
    ~FrameCache();

    void set_max_bytes(size_t max_bytes) { max_bytes_ = max_bytes; }
    size_t max_bytes() const { return max_bytes_; }
    size_t current_bytes() const { return current_bytes_; }

    /** Get cached frame or nullptr. If found, entry is moved to most recently used. */
    GopostFrame* get(const FrameCacheKey& key);

    /** Put frame into cache. May evict oldest entries. Caller must not use frame after put (cache owns reference). */
    void put(const FrameCacheKey& key, GopostFrame* frame);

    /** Invalidate all entries (release all frames to pool). */
    void invalidate_all();

    /** Remove entries for a clip (e.g. after clip trim). */
    void invalidate_clip(int32_t clip_id);

private:
    struct Entry {
        FrameCacheKey key;
        GopostFrame* frame = nullptr;
        size_t byte_size = 0;
    };
    using ListIter = std::list<Entry>::iterator;

    void evict_until(size_t target_bytes);
    static size_t frame_byte_size(const GopostFrame* f);

    size_t max_bytes_;
    size_t current_bytes_ = 0;
    FrameAcquireFn acquire_fn_;
    FrameReleaseFn release_fn_;
    std::list<Entry> lru_list_;
    std::unordered_map<FrameCacheKey, ListIter, FrameCacheKeyHash> map_;
    mutable std::mutex mutex_;
};

}  // namespace video
}  // namespace gopost

#endif
