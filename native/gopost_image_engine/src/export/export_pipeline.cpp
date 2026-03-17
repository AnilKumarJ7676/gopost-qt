#include "gopost/export.h"
#include "gopost/canvas.h"
#include "gopost/image_codec.h"
#include "gopost/engine.h"
#include "canvas/canvas_internal.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

/* Resolve target dimensions from canvas size and export resolution */
static void resolve_dimensions(
    int32_t canvas_w, int32_t canvas_h,
    GopostExportResolution resolution,
    int32_t custom_width, int32_t custom_height,
    int32_t* out_w, int32_t* out_h
) {
    switch (resolution) {
        case GOPOST_EXPORT_RES_ORIGINAL:
            *out_w = canvas_w;
            *out_h = canvas_h;
            break;
        case GOPOST_EXPORT_RES_4K: {
            const int32_t max_w = 3840, max_h = 2160;
            if (canvas_w <= max_w && canvas_h <= max_h) {
                *out_w = canvas_w;
                *out_h = canvas_h;
            } else {
                float scale = std::min((float)max_w / canvas_w, (float)max_h / canvas_h);
                *out_w = (int32_t)std::round(canvas_w * scale);
                *out_h = (int32_t)std::round(canvas_h * scale);
            }
            break;
        }
        case GOPOST_EXPORT_RES_1080P: {
            const int32_t max_w = 1920, max_h = 1080;
            if (canvas_w <= max_w && canvas_h <= max_h) {
                *out_w = canvas_w;
                *out_h = canvas_h;
            } else {
                float scale = std::min((float)max_w / canvas_w, (float)max_h / canvas_h);
                *out_w = (int32_t)std::round(canvas_w * scale);
                *out_h = (int32_t)std::round(canvas_h * scale);
            }
            break;
        }
        case GOPOST_EXPORT_RES_720P: {
            const int32_t max_w = 1280, max_h = 720;
            if (canvas_w <= max_w && canvas_h <= max_h) {
                *out_w = canvas_w;
                *out_h = canvas_h;
            } else {
                float scale = std::min((float)max_w / canvas_w, (float)max_h / canvas_h);
                *out_w = (int32_t)std::round(canvas_w * scale);
                *out_h = (int32_t)std::round(canvas_h * scale);
            }
            break;
        }
        case GOPOST_EXPORT_RES_INSTAGRAM_SQUARE: {
            const int32_t max_w = 1080, max_h = 1080;
            float scale = std::min((float)max_w / canvas_w, (float)max_h / canvas_h);
            *out_w = (int32_t)std::round(canvas_w * scale);
            *out_h = (int32_t)std::round(canvas_h * scale);
            break;
        }
        case GOPOST_EXPORT_RES_INSTAGRAM_STORY: {
            const int32_t max_w = 1080, max_h = 1920;
            float scale = std::min((float)max_w / canvas_w, (float)max_h / canvas_h);
            *out_w = (int32_t)std::round(canvas_w * scale);
            *out_h = (int32_t)std::round(canvas_h * scale);
            break;
        }
        case GOPOST_EXPORT_RES_CUSTOM:
            *out_w = custom_width > 0 ? custom_width : canvas_w;
            *out_h = custom_height > 0 ? custom_height : canvas_h;
            break;
        default:
            *out_w = canvas_w;
            *out_h = canvas_h;
            break;
    }
}

/* Bilinear interpolation for RGBA8 resize. Caller must free returned frame. */
static GopostFrame* resize_frame(GopostFrame* src, int32_t target_w, int32_t target_h) {
    if (!src || !src->data || target_w <= 0 || target_h <= 0) return nullptr;
    if (src->format != GOPOST_PIXEL_FORMAT_RGBA8) return nullptr;

    auto* dst = static_cast<GopostFrame*>(malloc(sizeof(GopostFrame)));
    if (!dst) return nullptr;

    size_t stride = (size_t)target_w * 4;
    size_t buf_size = stride * (size_t)target_h;
    dst->data = static_cast<uint8_t*>(malloc(buf_size));
    if (!dst->data) {
        free(dst);
        return nullptr;
    }

    dst->width = (uint32_t)target_w;
    dst->height = (uint32_t)target_h;
    dst->format = GOPOST_PIXEL_FORMAT_RGBA8;
    dst->data_size = buf_size;
    dst->stride = stride;

    const int32_t src_w = (int32_t)src->width;
    const int32_t src_h = (int32_t)src->height;
    const uint8_t* src_px = src->data;
    const size_t src_stride = src->stride;
    uint8_t* dst_px = dst->data;

    const float x_scale = (src_w > 1) ? (float)(src_w - 1) / target_w : 0.0f;
    const float y_scale = (src_h > 1) ? (float)(src_h - 1) / target_h : 0.0f;

    for (int32_t dy = 0; dy < target_h; dy++) {
        float sy = dy * y_scale;
        int32_t y0 = (int32_t)std::floor(sy);
        int32_t y1 = std::min(y0 + 1, src_h - 1);
        float fy = sy - y0;

        for (int32_t dx = 0; dx < target_w; dx++) {
            float sx = dx * x_scale;
            int32_t x0 = (int32_t)std::floor(sx);
            int32_t x1 = std::min(x0 + 1, src_w - 1);
            float fx = sx - x0;

            const uint8_t* p00 = src_px + y0 * src_stride + x0 * 4;
            const uint8_t* p10 = src_px + y0 * src_stride + x1 * 4;
            const uint8_t* p01 = src_px + y1 * src_stride + x0 * 4;
            const uint8_t* p11 = src_px + y1 * src_stride + x1 * 4;

            for (int c = 0; c < 4; c++) {
                float v00 = p00[c], v10 = p10[c], v01 = p01[c], v11 = p11[c];
                float v0 = v00 * (1.0f - fx) + v10 * fx;
                float v1 = v01 * (1.0f - fx) + v11 * fx;
                float v = v0 * (1.0f - fy) + v1 * fy;
                dst_px[dx * 4 + c] = (uint8_t)std::max(0.0f, std::min(255.0f, std::round(v)));
            }
        }
        dst_px += stride;
    }

    return dst;
}

static void release_frame(GopostEngine* engine, GopostFrame* frame) {
    if (!frame) return;
    if (engine) {
        gopost_frame_release(engine, frame);
    } else {
        free(frame->data);
        free(frame);
    }
}

static void free_frame(GopostFrame* frame) {
    if (frame) {
        free(frame->data);
        free(frame);
    }
}

extern "C" {

GopostError gopost_export_to_buffer(
    GopostCanvas* canvas,
    const GopostExportConfig* config,
    GopostExportProgressCallback progress_cb,
    void* user_data,
    uint8_t** out_data, size_t* out_size
) {
    if (!canvas || !config || !out_data || !out_size)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    *out_data = nullptr;
    *out_size = 0;

    if (config->format != GOPOST_IMAGE_FORMAT_JPEG &&
        config->format != GOPOST_IMAGE_FORMAT_PNG &&
        config->format != GOPOST_IMAGE_FORMAT_WEBP)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    if (progress_cb && progress_cb(0.1f, user_data) != 0)
        return GOPOST_ERROR_UNKNOWN;

    GopostEngine* engine = gopost_internal_canvas_get_engine(canvas);

    GopostFrame* frame = nullptr;
    GopostError err = gopost_canvas_render(canvas, &frame);
    if (err != GOPOST_OK) return err;
    if (!frame || !frame->data) {
        release_frame(engine, frame);
        return GOPOST_ERROR_UNKNOWN;
    }

    int32_t canvas_w = 0, canvas_h = 0;
    float dpi = 72.0f;
    gopost_canvas_get_size(canvas, &canvas_w, &canvas_h, &dpi);

    int32_t target_w = 0, target_h = 0;
    resolve_dimensions(
        canvas_w, canvas_h,
        config->resolution,
        config->custom_width, config->custom_height,
        &target_w, &target_h
    );

    GopostFrame* work_frame = frame;
    bool work_is_resized = false;
    if (config->resolution != GOPOST_EXPORT_RES_ORIGINAL ||
        (int32_t)frame->width != target_w || (int32_t)frame->height != target_h) {
        GopostFrame* resized = resize_frame(frame, target_w, target_h);
        release_frame(engine, frame);
        if (!resized) return GOPOST_ERROR_OUT_OF_MEMORY;
        work_frame = resized;
        work_is_resized = true;
    }

    if (progress_cb && progress_cb(0.5f, user_data) != 0) {
        if (work_is_resized) free_frame(work_frame);
        else release_frame(engine, work_frame);
        return GOPOST_ERROR_UNKNOWN;
    }

    uint8_t* encoded = nullptr;
    size_t encoded_size = 0;
    int32_t quality = (config->quality > 0 && config->quality <= 100) ? config->quality : 85;

    switch (config->format) {
        case GOPOST_IMAGE_FORMAT_JPEG: {
            GopostJpegEncodeOpts opts = { quality, 0 };
            err = gopost_image_encode_jpeg(work_frame, &opts, &encoded, &encoded_size);
            break;
        }
        case GOPOST_IMAGE_FORMAT_PNG: {
            GopostPngEncodeOpts opts = { 6 };
            err = gopost_image_encode_png(work_frame, &opts, &encoded, &encoded_size);
            break;
        }
        case GOPOST_IMAGE_FORMAT_WEBP: {
            GopostWebpEncodeOpts opts = { quality, 0 };
            err = gopost_image_encode_webp(work_frame, &opts, &encoded, &encoded_size);
            break;
        }
        default:
            err = GOPOST_ERROR_INVALID_ARGUMENT;
            break;
    }

    if (work_is_resized) free_frame(work_frame);
    else release_frame(engine, work_frame);

    if (err != GOPOST_OK) return err;

    if (progress_cb && progress_cb(1.0f, user_data) != 0) {
        gopost_export_free(encoded);
        return GOPOST_ERROR_UNKNOWN;
    }

    *out_data = encoded;
    *out_size = encoded_size;
    return GOPOST_OK;
}

GopostError gopost_export_to_file(
    GopostCanvas* canvas,
    const GopostExportConfig* config,
    const char* output_path,
    GopostExportProgressCallback progress_cb,
    void* user_data
) {
    if (!canvas || !config || !output_path)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    uint8_t* data = nullptr;
    size_t size = 0;
    GopostError err = gopost_export_to_buffer(
        canvas, config, progress_cb, user_data, &data, &size
    );
    if (err != GOPOST_OK) return err;

    FILE* f = fopen(output_path, "wb");
    if (!f) {
        gopost_export_free(data);
        return GOPOST_ERROR_IO;
    }

    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    gopost_export_free(data);

    return (written == size) ? GOPOST_OK : GOPOST_ERROR_IO;
}

GopostError gopost_export_estimate_size(
    GopostCanvas* canvas,
    const GopostExportConfig* config,
    size_t* out_estimated_bytes
) {
    if (!canvas || !config || !out_estimated_bytes)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t canvas_w = 0, canvas_h = 0;
    float dpi = 72.0f;
    gopost_canvas_get_size(canvas, &canvas_w, &canvas_h, &dpi);

    int32_t w = 0, h = 0;
    resolve_dimensions(
        canvas_w, canvas_h,
        config->resolution,
        config->custom_width, config->custom_height,
        &w, &h
    );

    int32_t quality = (config->quality > 0 && config->quality <= 100) ? config->quality : 85;
    size_t est = 0;

    switch (config->format) {
        case GOPOST_IMAGE_FORMAT_JPEG:
            est = (size_t)(w * h * 3 * quality / 100 / 10);
            break;
        case GOPOST_IMAGE_FORMAT_PNG:
            est = (size_t)(w * h * 4 * 0.6);
            break;
        case GOPOST_IMAGE_FORMAT_WEBP:
            est = (size_t)(w * h * 3 * quality / 100 / 8);
            break;
        default:
            return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    *out_estimated_bytes = est > 0 ? est : 1;
    return GOPOST_OK;
}

void gopost_export_free(uint8_t* data) {
    free(data);
}

} /* extern "C" */
