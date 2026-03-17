#ifndef GOPOST_ENGINE_H
#define GOPOST_ENGINE_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostEngine GopostEngine;

typedef struct {
    uint32_t thread_count;
    size_t frame_pool_size_mb;
    int enable_gpu;
    int log_level; // 0=error, 1=warn, 2=info, 3=debug
} GopostEngineConfig;

GopostError gopost_engine_create(GopostEngine** engine, const GopostEngineConfig* config);
GopostError gopost_engine_destroy(GopostEngine* engine);

GopostError gopost_engine_get_version(uint32_t* major, uint32_t* minor, uint32_t* patch);

// Frame pool
GopostError gopost_frame_acquire(GopostEngine* engine, GopostFrame** frame, uint32_t width, uint32_t height, GopostPixelFormat format);
GopostError gopost_frame_release(GopostEngine* engine, GopostFrame* frame);

struct GopostFramePool;
GopostFramePool* gopost_engine_get_frame_pool(GopostEngine* engine);

#ifdef __cplusplus
}
#endif

#endif
