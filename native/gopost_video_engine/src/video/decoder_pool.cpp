#include "decoder_pool.hpp"
#include <algorithm>
#include <cassert>

namespace gopost {
namespace video {

// =============================================================================
// DecoderLease
// =============================================================================

DecoderLease::DecoderLease(IVideoDecoder* dec, DecoderPool* pool, uint32_t slot_id)
    : decoder_(dec), pool_(pool), slot_id_(slot_id) {}

DecoderLease::DecoderLease(DecoderLease&& o) noexcept
    : decoder_(o.decoder_), pool_(o.pool_), slot_id_(o.slot_id_) {
    o.decoder_ = nullptr;
    o.pool_ = nullptr;
    o.slot_id_ = 0;
}

DecoderLease& DecoderLease::operator=(DecoderLease&& o) noexcept {
    if (this != &o) {
        // Release current lease if any
        if (decoder_ && pool_) {
            pool_->release_slot(slot_id_);
        }
        decoder_ = o.decoder_;
        pool_ = o.pool_;
        slot_id_ = o.slot_id_;
        o.decoder_ = nullptr;
        o.pool_ = nullptr;
        o.slot_id_ = 0;
    }
    return *this;
}

DecoderLease::~DecoderLease() {
    if (decoder_ && pool_) {
        pool_->release_slot(slot_id_);
    }
}

// =============================================================================
// DecoderPool
// =============================================================================

DecoderPool::DecoderPool(GopostEngine* engine, int max_decoders)
    : engine_(engine)
    , max_decoders_(max_decoders > 0 ? max_decoders : 1) {}

DecoderPool::~DecoderPool() {
    shutdown();
}

DecoderLease DecoderPool::acquire(const std::string& path, DecoderPriority priority) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (shutting_down_) return {};

    // 1. Check for an idle decoder already opened on this path
    DecoderSlot* slot = find_idle_for_path(path);
    if (slot) {
        slot->active = true;
        return DecoderLease(slot->decoder.get(), this, slot->id);
    }

    // 2. Try to create/reclaim a slot
    slot = find_or_create_slot(path);
    if (slot) {
        // Open the decoder on the requested path
        if (!slot->open_path.empty() && slot->open_path != path) {
            slot->decoder->close();
            slot->open_path.clear();
        }
        if (slot->open_path.empty()) {
            if (!slot->decoder->open(path)) {
                // Open failed — return slot to idle, return empty lease
                slot->active = false;
                return {};
            }
            slot->open_path = path;
        }
        slot->active = true;
        return DecoderLease(slot->decoder.get(), this, slot->id);
    }

    // 3. All slots occupied — wait in priority queue
    WaitEntry entry;
    entry.priority = priority;
    entry.path = path;

    // Insert sorted by priority (lower enum value = higher priority)
    auto insert_pos = wait_queue_.begin();
    for (auto it = wait_queue_.begin(); it != wait_queue_.end(); ++it) {
        if (static_cast<int>((*it)->priority) > static_cast<int>(priority)) {
            insert_pos = it;
            break;
        }
        insert_pos = it + 1;
    }
    wait_queue_.insert(insert_pos, &entry);

    entry.cv.wait(lock, [&] {
        return entry.granted_slot != nullptr || entry.cancelled || shutting_down_;
    });

    // Remove from queue (may already be removed by release_slot)
    wait_queue_.erase(
        std::remove(wait_queue_.begin(), wait_queue_.end(), &entry),
        wait_queue_.end());

    if (entry.cancelled || shutting_down_ || !entry.granted_slot) {
        return {};
    }

    slot = entry.granted_slot;

    // Open on requested path if needed
    if (!slot->open_path.empty() && slot->open_path != path) {
        slot->decoder->close();
        slot->open_path.clear();
    }
    if (slot->open_path.empty()) {
        if (!slot->decoder->open(path)) {
            slot->active = false;
            return {};
        }
        slot->open_path = path;
    }
    slot->active = true;
    return DecoderLease(slot->decoder.get(), this, slot->id);
}

DecoderLease DecoderPool::try_acquire(const std::string& path) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (shutting_down_) return {};

    DecoderSlot* slot = find_idle_for_path(path);
    if (slot) {
        slot->active = true;
        return DecoderLease(slot->decoder.get(), this, slot->id);
    }

    slot = find_or_create_slot(path);
    if (!slot) return {};

    if (!slot->open_path.empty() && slot->open_path != path) {
        slot->decoder->close();
        slot->open_path.clear();
    }
    if (slot->open_path.empty()) {
        if (!slot->decoder->open(path)) {
            slot->active = false;
            return {};
        }
        slot->open_path = path;
    }
    slot->active = true;
    return DecoderLease(slot->decoder.get(), this, slot->id);
}

void DecoderPool::release_slot(uint32_t slot_id) {
    std::unique_lock<std::mutex> lock(mutex_);

    DecoderSlot* slot = nullptr;
    for (auto& s : slots_) {
        if (s->id == slot_id) { slot = s.get(); break; }
    }
    if (!slot) return;

    slot->active = false;

    // If over capacity, close and remove the slot
    int active = 0;
    int total = static_cast<int>(slots_.size());
    for (auto& s : slots_) {
        if (s->active) active++;
    }
    if (total > max_decoders_ && !slot->active) {
        slot->decoder->close();
        slots_.erase(
            std::remove_if(slots_.begin(), slots_.end(),
                [slot_id](const auto& s) { return s->id == slot_id; }),
            slots_.end());
        // Don't grant to waiters — we just freed capacity
    }

    // Grant to highest-priority waiter if any
    if (!wait_queue_.empty()) {
        // Find the slot to grant (prefer one matching the waiter's path)
        for (auto it = wait_queue_.begin(); it != wait_queue_.end(); ++it) {
            WaitEntry* waiter = *it;

            // Find a free slot for this waiter
            DecoderSlot* free_slot = find_idle_for_path(waiter->path);
            if (!free_slot) {
                // Check if we can use ANY idle slot
                for (auto& s : slots_) {
                    if (!s->active) { free_slot = s.get(); break; }
                }
            }
            if (free_slot) {
                free_slot->active = true;
                waiter->granted_slot = free_slot;
                wait_queue_.erase(it);
                waiter->cv.notify_one();
                return;
            }
            break;  // No free slot available for any waiter
        }
    }
}

void DecoderPool::set_max_decoders(int max_decoders) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_decoders_ = max_decoders > 0 ? max_decoders : 1;
    // Excess idle slots will be reclaimed on next release
}

int DecoderPool::max_decoders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return max_decoders_;
}

int DecoderPool::active_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;
    for (auto& s : slots_) {
        if (s->active) count++;
    }
    return count;
}

void DecoderPool::flush_idle() {
    std::lock_guard<std::mutex> lock(mutex_);
    slots_.erase(
        std::remove_if(slots_.begin(), slots_.end(),
            [](const auto& s) {
                if (!s->active) {
                    s->decoder->close();
                    return true;
                }
                return false;
            }),
        slots_.end());
}

void DecoderPool::shutdown() {
    std::unique_lock<std::mutex> lock(mutex_);
    shutting_down_ = true;

    // Cancel all waiters
    for (auto* w : wait_queue_) {
        w->cancelled = true;
        w->cv.notify_one();
    }
    wait_queue_.clear();

    // Close all decoders
    for (auto& s : slots_) {
        if (s->decoder && s->decoder->is_open()) {
            s->decoder->close();
        }
    }
    slots_.clear();
}

size_t DecoderPool::estimated_memory_bytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Each open decoder holds ~5-20MB depending on resolution and codec.
    // Rough estimate: 10MB per open decoder.
    size_t count = 0;
    for (auto& s : slots_) {
        if (s->decoder && s->decoder->is_open()) {
            const auto& i = s->decoder->info();
            // Estimate: format context + codec context + a few decoded frames
            count += static_cast<size_t>(i.width) * i.height * 4 * 3 + (2 * 1024 * 1024);
        }
    }
    return count;
}

// =============================================================================
// Private helpers
// =============================================================================

DecoderPool::DecoderSlot* DecoderPool::find_idle_for_path(const std::string& path) {
    for (auto& s : slots_) {
        if (!s->active && s->open_path == path && s->decoder && s->decoder->is_open()) {
            return s.get();
        }
    }
    return nullptr;
}

DecoderPool::DecoderSlot* DecoderPool::find_or_create_slot(const std::string& path) {
    // Count active slots
    int active = 0;
    DecoderSlot* idle_slot = nullptr;
    for (auto& s : slots_) {
        if (s->active) {
            active++;
        } else if (!idle_slot) {
            idle_slot = s.get();
        }
    }

    // If we have an idle slot, reuse it
    if (idle_slot) {
        return idle_slot;
    }

    // If under capacity, create a new slot
    if (static_cast<int>(slots_.size()) < max_decoders_) {
        auto slot = std::make_unique<DecoderSlot>();
        slot->id = next_slot_id_++;
        slot->decoder = create_video_decoder(engine_);
        slot->active = false;
        DecoderSlot* raw = slot.get();
        slots_.push_back(std::move(slot));
        return raw;
    }

    // All slots are active — caller must wait
    return nullptr;
}

DecoderPool::DecoderSlot* DecoderPool::evict_idle_slot() {
    // Evict the least recently used idle slot
    for (auto& s : slots_) {
        if (!s->active) {
            s->decoder->close();
            s->open_path.clear();
            return s.get();
        }
    }
    return nullptr;
}

}  // namespace video
}  // namespace gopost
