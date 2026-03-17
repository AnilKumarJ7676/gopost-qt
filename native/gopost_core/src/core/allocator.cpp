#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <sys/mman.h>
#elif defined(__linux__) || defined(__ANDROID__)
#include <sys/mman.h>
#endif

#include <string.h>

namespace gopost {

static constexpr size_t kAlignment = 64;

static void* aligned_alloc_impl(size_t size) {
    void* ptr = nullptr;
#if defined(_WIN32)
    ptr = _aligned_malloc(size, kAlignment);
#else
    if (posix_memalign(&ptr, kAlignment, size) != 0) {
        ptr = nullptr;
    }
#endif
    return ptr;
}

static void aligned_free_impl(void* ptr) {
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

class FramePoolAllocator {
public:
    explicit FramePoolAllocator(size_t pool_size_mb)
        : pool_size_(pool_size_mb * 1024 * 1024) {}

    ~FramePoolAllocator() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& block : free_blocks_) {
            secure_free(block.ptr, block.size);
        }
        for (auto& block : used_blocks_) {
            secure_free(block.ptr, block.size);
        }
    }

    void* acquire(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);

        size = align_size(size);

        for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
            if (it->size >= size) {
                void* ptr = it->ptr;
                used_blocks_.push_back({ptr, it->size});
                free_blocks_.erase(it);
                return ptr;
            }
        }

        void* ptr = aligned_alloc_impl(size);
        if (!ptr) return nullptr;

#if defined(__APPLE__) || defined(__linux__) || defined(__ANDROID__)
        mlock(ptr, size);
#endif

        used_blocks_.push_back({ptr, size});
        allocated_total_ += size;
        return ptr;
    }

    void release(void* ptr) {
        if (!ptr) return;
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = used_blocks_.begin(); it != used_blocks_.end(); ++it) {
            if (it->ptr == ptr) {
#if defined(_WIN32)
                SecureZeroMemory(it->ptr, it->size);
#elif defined(__linux__) || defined(__ANDROID__)
                explicit_bzero(it->ptr, it->size);
#else
                volatile unsigned char* p = static_cast<volatile unsigned char*>(it->ptr);
                for (size_t i = 0; i < it->size; ++i) p[i] = 0;
#endif
                free_blocks_.push_back(*it);
                used_blocks_.erase(it);
                return;
            }
        }
    }

    size_t allocated_bytes() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return allocated_total_;
    }

    size_t free_blocks_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return free_blocks_.size();
    }

private:
    struct Block {
        void* ptr;
        size_t size;
    };

    static size_t align_size(size_t size) {
        return (size + kAlignment - 1) & ~(kAlignment - 1);
    }

    void secure_free(void* ptr, size_t size) {
#if defined(_WIN32)
        SecureZeroMemory(ptr, size);
#elif defined(__linux__) || defined(__ANDROID__)
        explicit_bzero(ptr, size);
        munlock(ptr, size);
#elif defined(__APPLE__)
        {
            volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
            for (size_t i = 0; i < size; ++i) p[i] = 0;
        }
        munlock(ptr, size);
#endif
        aligned_free_impl(ptr);
    }

    size_t pool_size_;
    size_t allocated_total_ = 0;
    mutable std::mutex mutex_;
    std::vector<Block> free_blocks_;
    std::vector<Block> used_blocks_;
};

}  // namespace gopost

#include "core/frame_pool.h"

extern "C" {

struct GopostFramePool {
    gopost::FramePoolAllocator* allocator;
};

GopostFramePool* gopost_frame_pool_create(size_t pool_size_mb) {
    auto* alloc = new (std::nothrow) gopost::FramePoolAllocator(pool_size_mb);
    if (!alloc) return nullptr;
    auto* pool = new (std::nothrow) GopostFramePool{alloc};
    if (!pool) {
        delete alloc;
        return nullptr;
    }
    return pool;
}

void gopost_frame_pool_destroy(GopostFramePool* pool) {
    if (!pool) return;
    if (pool->allocator) {
        delete pool->allocator;
        pool->allocator = nullptr;
    }
    delete pool;
}

void* gopost_frame_pool_acquire(GopostFramePool* pool, size_t size) {
    if (!pool || !pool->allocator) return nullptr;
    return pool->allocator->acquire(size);
}

void gopost_frame_pool_release(GopostFramePool* pool, void* ptr) {
    if (!pool || !pool->allocator) return;
    pool->allocator->release(ptr);
}

}
