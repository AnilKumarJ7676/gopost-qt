#include "frame_cache.hpp"
#include "gopost/types.h"
#include <algorithm>
#include <cstring>

namespace gopost {
namespace video {

static size_t frame_buffer_size(uint32_t width, uint32_t height, GopostPixelFormat format) {
    switch (format) {
        case GOPOST_PIXEL_FORMAT_RGBA8:
        case GOPOST_PIXEL_FORMAT_BGRA8:
            return (size_t)width * height * 4;
        case GOPOST_PIXEL_FORMAT_NV12:
        case GOPOST_PIXEL_FORMAT_YUV420P:
            return (size_t)width * height + (size_t)((width + 1) / 2) * ((height + 1) / 2) * 2;
        default:
            return (size_t)width * height * 4;
    }
}

size_t FrameCache::frame_byte_size(const GopostFrame* f) {
    if (!f) return 0;
    return frame_buffer_size(f->width, f->height, f->format);
}

FrameCache::FrameCache(size_t max_bytes, FrameAcquireFn acquire, FrameReleaseFn release)
    : max_bytes_(max_bytes)
    , acquire_fn_(std::move(acquire))
    , release_fn_(std::move(release)) {}

FrameCache::~FrameCache() {
    invalidate_all();
}

GopostFrame* FrameCache::get(const FrameCacheKey& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(key);
    if (it == map_.end()) return nullptr;
    GopostFrame* f = it->second->frame;
    if (!f) return nullptr;
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
    return f;
}

void FrameCache::put(const FrameCacheKey& key, GopostFrame* frame) {
    if (!frame) return;
    std::lock_guard<std::mutex> lock(mutex_);
    size_t sz = frame_byte_size(frame);
    evict_until(max_bytes_ > sz ? max_bytes_ - sz : 0);
    Entry e;
    e.key = key;
    e.frame = frame;
    e.byte_size = sz;
    lru_list_.push_front(std::move(e));
    auto list_it = lru_list_.begin();
    list_it->frame = frame;
    list_it->byte_size = sz;
    map_[key] = list_it;
    current_bytes_ += sz;
}

void FrameCache::invalidate_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& e : lru_list_) {
        if (e.frame) {
            release_fn_(e.frame);
            e.frame = nullptr;
        }
    }
    lru_list_.clear();
    map_.clear();
    current_bytes_ = 0;
}

void FrameCache::invalidate_clip(int32_t clip_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<FrameCacheKey> to_remove;
    for (const auto& kv : map_) {
        if (kv.first.clip_id == clip_id) {
            to_remove.push_back(kv.first);
        }
    }
    for (const auto& key : to_remove) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            if (it->second->frame) {
                release_fn_(it->second->frame);
                current_bytes_ -= it->second->byte_size;
            }
            lru_list_.erase(it->second);
            map_.erase(it);
        }
    }
}

void FrameCache::evict_until(size_t target_bytes) {
    while (current_bytes_ > target_bytes && !lru_list_.empty()) {
        Entry& e = lru_list_.back();
        if (e.frame) {
            release_fn_(e.frame);
            current_bytes_ -= e.byte_size;
            map_.erase(e.key);
        }
        lru_list_.pop_back();
    }
}

}  // namespace video
}  // namespace gopost
