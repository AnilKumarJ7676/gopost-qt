#include "gopost/decoder_pool_api.h"
#include "decoder_pool.hpp"
#include "thumbnail_generator.hpp"
#include <cstring>
#include <new>

// =============================================================================
// Opaque wrappers
// =============================================================================

struct GopostDecoderPool {
    std::unique_ptr<gopost::video::DecoderPool> pool;
};

struct GopostThumbnailGenerator {
    std::unique_ptr<gopost::video::ThumbnailGenerator> gen;
};

// =============================================================================
// Decoder Pool API
// =============================================================================

extern "C" {

GopostError gopost_decoder_pool_create(
    GopostEngine* engine,
    int32_t max_decoders,
    GopostDecoderPool** out_pool) {
    if (!engine || !out_pool || max_decoders <= 0) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    auto* wrapper = new (std::nothrow) GopostDecoderPool{};
    if (!wrapper) return GOPOST_ERROR_OUT_OF_MEMORY;

    wrapper->pool = std::make_unique<gopost::video::DecoderPool>(engine, max_decoders);
    *out_pool = wrapper;
    return GOPOST_OK;
}

void gopost_decoder_pool_destroy(GopostDecoderPool* pool) {
    if (!pool) return;
    pool->pool->shutdown();
    delete pool;
}

GopostError gopost_decoder_pool_set_max(GopostDecoderPool* pool, int32_t max_decoders) {
    if (!pool || max_decoders <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    pool->pool->set_max_decoders(max_decoders);
    return GOPOST_OK;
}

GopostError gopost_decoder_pool_active_count(
    const GopostDecoderPool* pool, int32_t* out_count) {
    if (!pool || !out_count) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_count = pool->pool->active_count();
    return GOPOST_OK;
}

GopostError gopost_decoder_pool_flush_idle(GopostDecoderPool* pool) {
    if (!pool) return GOPOST_ERROR_INVALID_ARGUMENT;
    pool->pool->flush_idle();
    return GOPOST_OK;
}

// =============================================================================
// Thumbnail Generator API
// =============================================================================

GopostError gopost_thumbnail_generator_create(
    GopostDecoderPool* pool,
    GopostEngine* engine,
    GopostThumbnailGenerator** out_gen) {
    if (!pool || !engine || !out_gen) return GOPOST_ERROR_INVALID_ARGUMENT;

    auto* wrapper = new (std::nothrow) GopostThumbnailGenerator{};
    if (!wrapper) return GOPOST_ERROR_OUT_OF_MEMORY;

    wrapper->gen = std::make_unique<gopost::video::ThumbnailGenerator>(
        pool->pool.get(), engine);
    *out_gen = wrapper;
    return GOPOST_OK;
}

void gopost_thumbnail_generator_destroy(GopostThumbnailGenerator* gen) {
    if (!gen) return;
    gen->gen->shutdown();
    delete gen;
}

int32_t gopost_thumbnail_submit(
    GopostThumbnailGenerator* gen,
    const GopostThumbnailRequest* request) {
    if (!gen || !request) return -1;

    gopost::video::ThumbnailRequest req;
    req.source_path = std::string(
        request->source_path,
        strnlen(request->source_path, sizeof(request->source_path)));
    req.source_duration = request->source_duration;
    req.count = request->count;
    req.thumb_width = request->thumb_width > 0 ? request->thumb_width : 160;
    req.thumb_height = request->thumb_height > 0 ? request->thumb_height : 90;
    req.priority = static_cast<gopost::video::DecoderPriority>(request->priority);

    return gen->gen->submit(std::move(req));
}

GopostError gopost_thumbnail_cancel(GopostThumbnailGenerator* gen, int32_t job_id) {
    if (!gen) return GOPOST_ERROR_INVALID_ARGUMENT;
    gen->gen->cancel(job_id);
    return GOPOST_OK;
}

GopostError gopost_thumbnail_cancel_all(GopostThumbnailGenerator* gen) {
    if (!gen) return GOPOST_ERROR_INVALID_ARGUMENT;
    gen->gen->cancel_all();
    return GOPOST_OK;
}

GopostThumbnailJobStatus gopost_thumbnail_job_status(
    const GopostThumbnailGenerator* gen, int32_t job_id) {
    if (!gen) return GOPOST_THUMB_STATUS_FAILED;
    auto status = gen->gen->job_status(job_id);
    return static_cast<GopostThumbnailJobStatus>(static_cast<int>(status));
}

int32_t gopost_thumbnail_result_count(
    const GopostThumbnailGenerator* gen, int32_t job_id) {
    if (!gen) return 0;
    auto results = gen->gen->get_results(job_id);
    return static_cast<int32_t>(results.size());
}

GopostError gopost_thumbnail_get_result(
    const GopostThumbnailGenerator* gen,
    int32_t job_id,
    int32_t index,
    GopostThumbnailResult* out_result) {
    if (!gen || !out_result || index < 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    auto results = gen->gen->get_results(job_id);
    if (index >= static_cast<int32_t>(results.size())) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    const auto& frame = results[static_cast<size_t>(index)];
    out_result->jpeg_data = frame.jpeg_data.data();
    out_result->jpeg_size = static_cast<int32_t>(frame.jpeg_data.size());
    out_result->width = frame.width;
    out_result->height = frame.height;
    out_result->timestamp = frame.timestamp;
    return GOPOST_OK;
}

int32_t gopost_thumbnail_queue_size(const GopostThumbnailGenerator* gen) {
    if (!gen) return 0;
    return gen->gen->queue_size();
}

}  // extern "C"
