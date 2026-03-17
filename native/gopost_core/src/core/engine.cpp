#include "gopost/engine.h"
#include "gopost/version.h"
#include "core/frame_pool.h"
#include <cstdlib>
#include <cstring>
#include <new>

struct GopostEngine {
    GopostEngineConfig config;
    bool initialized;
    GopostFramePool* frame_pool;
};

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

static size_t frame_stride(uint32_t width, GopostPixelFormat format) {
    switch (format) {
        case GOPOST_PIXEL_FORMAT_RGBA8:
        case GOPOST_PIXEL_FORMAT_BGRA8:
            return (size_t)width * 4;
        case GOPOST_PIXEL_FORMAT_NV12:
            return (size_t)width;
        case GOPOST_PIXEL_FORMAT_YUV420P:
            return (size_t)width;
        default:
            return (size_t)width * 4;
    }
}

extern "C" {

GopostError gopost_engine_create(GopostEngine** engine, const GopostEngineConfig* config) {
    if (!engine || !config) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    auto* e = new (std::nothrow) GopostEngine{};
    if (!e) {
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    e->config = *config;
    e->initialized = true;
    e->frame_pool = gopost_frame_pool_create(config->frame_pool_size_mb > 0 ? config->frame_pool_size_mb : 64);
    if (!e->frame_pool) {
        delete e;
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    *engine = e;
    return GOPOST_OK;
}

GopostError gopost_engine_destroy(GopostEngine* engine) {
    if (!engine) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    if (engine->frame_pool) {
        gopost_frame_pool_destroy(engine->frame_pool);
        engine->frame_pool = nullptr;
    }
    engine->initialized = false;
    delete engine;
    return GOPOST_OK;
}

GopostFramePool* gopost_engine_get_frame_pool(GopostEngine* engine) {
    if (!engine) return nullptr;
    return engine->frame_pool;
}

GopostError gopost_engine_get_version(uint32_t* major, uint32_t* minor, uint32_t* patch) {
    if (!major || !minor || !patch) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    *major = GOPOST_VERSION_MAJOR;
    *minor = GOPOST_VERSION_MINOR;
    *patch = GOPOST_VERSION_PATCH;
    return GOPOST_OK;
}

const char* gopost_error_string(GopostError error) {
    switch (error) {
        case GOPOST_OK:                       return "OK";
        case GOPOST_ERROR_INVALID_ARGUMENT:   return "Invalid argument";
        case GOPOST_ERROR_OUT_OF_MEMORY:      return "Out of memory";
        case GOPOST_ERROR_NOT_INITIALIZED:    return "Not initialized";
        case GOPOST_ERROR_ALREADY_INITIALIZED: return "Already initialized";
        case GOPOST_ERROR_GPU_NOT_AVAILABLE:  return "GPU not available";
        case GOPOST_ERROR_CODEC_NOT_FOUND:    return "Codec not found";
        case GOPOST_ERROR_IO:                 return "I/O error";
        case GOPOST_ERROR_CRYPTO:             return "Crypto error";
        case GOPOST_ERROR_DECODER:            return "Decoder error";
        case GOPOST_ERROR_UNSUPPORTED_FORMAT: return "Unsupported format";
        case GOPOST_ERROR_UNKNOWN:
        default:                              return "Unknown error";
    }
}

GopostError gopost_frame_acquire(GopostEngine* engine, GopostFrame** frame, uint32_t width, uint32_t height, GopostPixelFormat format) {
    if (!engine || !frame || width == 0 || height == 0) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (!engine->frame_pool) {
        return GOPOST_ERROR_NOT_INITIALIZED;
    }

    size_t buf_size = frame_buffer_size(width, height, format);
    void* data = gopost_frame_pool_acquire(engine->frame_pool, buf_size);
    if (!data) {
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    GopostFrame* f = static_cast<GopostFrame*>(malloc(sizeof(GopostFrame)));
    if (!f) {
        gopost_frame_pool_release(engine->frame_pool, data);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    f->width = width;
    f->height = height;
    f->format = format;
    f->data = static_cast<uint8_t*>(data);
    f->data_size = buf_size;
    f->stride = frame_stride(width, format);

    *frame = f;
    return GOPOST_OK;
}

GopostError gopost_frame_release(GopostEngine* engine, GopostFrame* frame) {
    if (!engine || !frame) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (!engine->frame_pool) {
        return GOPOST_ERROR_NOT_INITIALIZED;
    }

    if (frame->data) {
        gopost_frame_pool_release(engine->frame_pool, frame->data);
        frame->data = nullptr;
        frame->data_size = 0;
    }
    free(frame);
    return GOPOST_OK;
}

}
