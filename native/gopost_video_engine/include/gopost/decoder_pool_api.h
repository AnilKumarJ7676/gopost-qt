#ifndef GOPOST_DECODER_POOL_API_H
#define GOPOST_DECODER_POOL_API_H

#include "gopost/types.h"
#include "gopost/error.h"
#include "gopost/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
   Decoder Pool — bounded pool of video decoders with FIFO queue
   ========================================================================= */

typedef struct GopostDecoderPool GopostDecoderPool;

/// Priority for decoder acquire requests.
typedef enum {
    GOPOST_DECODER_PRIORITY_HIGH   = 0,
    GOPOST_DECODER_PRIORITY_MEDIUM = 1,
    GOPOST_DECODER_PRIORITY_LOW    = 2,
} GopostDecoderPriority;

/// Create a decoder pool. `max_decoders` is the max concurrent decoders (e.g. 2-3).
GopostError gopost_decoder_pool_create(
    GopostEngine* engine,
    int32_t max_decoders,
    GopostDecoderPool** out_pool);

/// Destroy pool and release all decoders.
void gopost_decoder_pool_destroy(GopostDecoderPool* pool);

/// Change max concurrent decoders at runtime.
GopostError gopost_decoder_pool_set_max(GopostDecoderPool* pool, int32_t max_decoders);

/// Get current active decoder count.
GopostError gopost_decoder_pool_active_count(
    const GopostDecoderPool* pool, int32_t* out_count);

/// Flush idle cached decoders to free memory.
GopostError gopost_decoder_pool_flush_idle(GopostDecoderPool* pool);

/* =========================================================================
   Thumbnail Generator — sequential thumbnail extraction
   ========================================================================= */

typedef struct GopostThumbnailGenerator GopostThumbnailGenerator;

/// Thumbnail job status.
typedef enum {
    GOPOST_THUMB_STATUS_QUEUED      = 0,
    GOPOST_THUMB_STATUS_IN_PROGRESS = 1,
    GOPOST_THUMB_STATUS_COMPLETED   = 2,
    GOPOST_THUMB_STATUS_FAILED      = 3,
    GOPOST_THUMB_STATUS_CANCELLED   = 4,
} GopostThumbnailJobStatus;

/// Thumbnail extraction request.
typedef struct {
    char source_path[1024];
    double source_duration;
    int32_t count;
    int32_t thumb_width;
    int32_t thumb_height;
    int32_t priority;  /* GopostDecoderPriority */
} GopostThumbnailRequest;

/// A single thumbnail result.
typedef struct {
    const uint8_t* jpeg_data;
    int32_t jpeg_size;
    int32_t width;
    int32_t height;
    double timestamp;
} GopostThumbnailResult;

/// Create a thumbnail generator backed by a decoder pool.
GopostError gopost_thumbnail_generator_create(
    GopostDecoderPool* pool,
    GopostEngine* engine,
    GopostThumbnailGenerator** out_gen);

/// Destroy the thumbnail generator (cancels pending jobs).
void gopost_thumbnail_generator_destroy(GopostThumbnailGenerator* gen);

/// Submit a thumbnail extraction job. Returns job_id >= 0, or < 0 on error.
int32_t gopost_thumbnail_submit(
    GopostThumbnailGenerator* gen,
    const GopostThumbnailRequest* request);

/// Cancel a specific job.
GopostError gopost_thumbnail_cancel(GopostThumbnailGenerator* gen, int32_t job_id);

/// Cancel all pending jobs.
GopostError gopost_thumbnail_cancel_all(GopostThumbnailGenerator* gen);

/// Query job status.
GopostThumbnailJobStatus gopost_thumbnail_job_status(
    const GopostThumbnailGenerator* gen, int32_t job_id);

/// Get number of completed thumbnails for a job.
int32_t gopost_thumbnail_result_count(
    const GopostThumbnailGenerator* gen, int32_t job_id);

/// Get a specific thumbnail result. Index must be < result_count.
/// The returned pointer is valid until the job is removed or generator destroyed.
GopostError gopost_thumbnail_get_result(
    const GopostThumbnailGenerator* gen,
    int32_t job_id,
    int32_t index,
    GopostThumbnailResult* out_result);

/// Get number of jobs in the queue.
int32_t gopost_thumbnail_queue_size(const GopostThumbnailGenerator* gen);

#ifdef __cplusplus
}
#endif

#endif /* GOPOST_DECODER_POOL_API_H */
