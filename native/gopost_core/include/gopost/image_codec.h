#ifndef GOPOST_IMAGE_CODEC_H
#define GOPOST_IMAGE_CODEC_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostEngine GopostEngine;

typedef enum {
    GOPOST_IMAGE_FORMAT_UNKNOWN = 0,
    GOPOST_IMAGE_FORMAT_JPEG    = 1,
    GOPOST_IMAGE_FORMAT_PNG     = 2,
    GOPOST_IMAGE_FORMAT_WEBP    = 3,
    GOPOST_IMAGE_FORMAT_HEIC    = 4,
    GOPOST_IMAGE_FORMAT_BMP     = 5,
    GOPOST_IMAGE_FORMAT_GIF     = 6,
    GOPOST_IMAGE_FORMAT_TIFF    = 7,
    GOPOST_IMAGE_FORMAT_TGA     = 8,
} GopostImageFormat;

typedef enum {
    GOPOST_ORIENTATION_NORMAL      = 1,
    GOPOST_ORIENTATION_FLIP_H      = 2,
    GOPOST_ORIENTATION_ROTATE_180  = 3,
    GOPOST_ORIENTATION_FLIP_V      = 4,
    GOPOST_ORIENTATION_TRANSPOSE   = 5,
    GOPOST_ORIENTATION_ROTATE_90   = 6,
    GOPOST_ORIENTATION_TRANSVERSE  = 7,
    GOPOST_ORIENTATION_ROTATE_270  = 8,
} GopostExifOrientation;

typedef struct {
    int32_t width;
    int32_t height;
    int32_t channels;
    GopostImageFormat format;
    GopostExifOrientation orientation;
    int32_t has_alpha;
    int32_t color_space;  /* 0=sRGB, 1=DisplayP3, 2=AdobeRGB */
} GopostImageInfo;

/*--- Decoder (S5-01) ---*/

GopostError gopost_image_probe(
    const uint8_t* data, size_t data_len,
    GopostImageInfo* out_info
);

GopostError gopost_image_decode(
    GopostEngine* engine,
    const uint8_t* data, size_t data_len,
    GopostFrame** out_frame
);

GopostError gopost_image_decode_file(
    GopostEngine* engine,
    const char* path,
    GopostFrame** out_frame
);

GopostError gopost_image_decode_resize(
    GopostEngine* engine,
    const uint8_t* data, size_t data_len,
    int32_t max_width, int32_t max_height,
    GopostFrame** out_frame
);

/*--- Encoder (S5-02) ---*/

typedef struct {
    int32_t quality;      /* 1-100, default 85 */
    int32_t progressive;  /* 0 or 1 */
} GopostJpegEncodeOpts;

typedef struct {
    int32_t compression_level;  /* 0-9, default 6 */
} GopostPngEncodeOpts;

typedef struct {
    int32_t quality;    /* 1-100, default 80 */
    int32_t lossless;   /* 0 or 1 */
} GopostWebpEncodeOpts;

GopostError gopost_image_encode_jpeg(
    GopostFrame* frame,
    const GopostJpegEncodeOpts* opts,
    uint8_t** out_data, size_t* out_size
);

GopostError gopost_image_encode_png(
    GopostFrame* frame,
    const GopostPngEncodeOpts* opts,
    uint8_t** out_data, size_t* out_size
);

GopostError gopost_image_encode_webp(
    GopostFrame* frame,
    const GopostWebpEncodeOpts* opts,
    uint8_t** out_data, size_t* out_size
);

typedef struct {
    int32_t quality;    /* 1-100, default 80 */
} GopostHeicEncodeOpts;

GopostError gopost_image_encode_heic(
    GopostFrame* frame,
    const GopostHeicEncodeOpts* opts,
    uint8_t** out_data, size_t* out_size
);

GopostError gopost_image_encode_bmp(
    GopostFrame* frame,
    uint8_t** out_data, size_t* out_size
);

GopostError gopost_image_encode_to_file(
    GopostFrame* frame,
    const char* path,
    GopostImageFormat format,
    int32_t quality
);

void gopost_image_encode_free(uint8_t* data);

#ifdef __cplusplus
}
#endif

#endif
