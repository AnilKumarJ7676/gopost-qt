#ifndef GOPOST_CORE_FRAME_POOL_H
#define GOPOST_CORE_FRAME_POOL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque frame pool (backed by FramePoolAllocator). */
typedef struct GopostFramePool GopostFramePool;

GopostFramePool* gopost_frame_pool_create(size_t pool_size_mb);
void gopost_frame_pool_destroy(GopostFramePool* pool);
void* gopost_frame_pool_acquire(GopostFramePool* pool, size_t size);
void gopost_frame_pool_release(GopostFramePool* pool, void* ptr);

#ifdef __cplusplus
}
#endif

#endif
