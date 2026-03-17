#ifndef GOPOST_EXPORT_H
#define GOPOST_EXPORT_H
#include "gopost/types.h"
#include "gopost/error.h"
#include "gopost/image_codec.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostCanvas GopostCanvas;
typedef struct GopostEngine GopostEngine;

typedef enum {
    GOPOST_EXPORT_RES_ORIGINAL = 0,
    GOPOST_EXPORT_RES_4K       = 1,  /* 3840x2160 */
    GOPOST_EXPORT_RES_1080P    = 2,  /* 1920x1080 */
    GOPOST_EXPORT_RES_720P     = 3,
    GOPOST_EXPORT_RES_INSTAGRAM_SQUARE = 4,  /* 1080x1080 */
    GOPOST_EXPORT_RES_INSTAGRAM_STORY  = 5,  /* 1080x1920 */
    GOPOST_EXPORT_RES_CUSTOM   = 6,
} GopostExportResolution;

typedef struct {
    GopostImageFormat format;        /* JPEG, PNG, WebP */
    int32_t quality;                 /* 1-100, for JPEG/WebP */
    GopostExportResolution resolution;
    int32_t custom_width;            /* only if CUSTOM */
    int32_t custom_height;
    int32_t embed_color_profile;     /* 0 or 1 */
    float dpi;                       /* for print, default 72 */
} GopostExportConfig;

/* Progress callback: progress 0.0-1.0, return 0 to continue, non-zero to cancel */
typedef int32_t (*GopostExportProgressCallback)(float progress, void* user_data);

/* Export rendered canvas to memory buffer */
GopostError gopost_export_to_buffer(
    GopostCanvas* canvas,
    const GopostExportConfig* config,
    GopostExportProgressCallback progress_cb,
    void* user_data,
    uint8_t** out_data, size_t* out_size
);

/* Export rendered canvas to file */
GopostError gopost_export_to_file(
    GopostCanvas* canvas,
    const GopostExportConfig* config,
    const char* output_path,
    GopostExportProgressCallback progress_cb,
    void* user_data
);

/* Get estimated file size in bytes */
GopostError gopost_export_estimate_size(
    GopostCanvas* canvas,
    const GopostExportConfig* config,
    size_t* out_estimated_bytes
);

void gopost_export_free(uint8_t* data);

#ifdef __cplusplus
}
#endif
#endif
