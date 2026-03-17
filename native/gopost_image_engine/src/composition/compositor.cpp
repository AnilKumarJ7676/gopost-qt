#include "gopost/canvas.h"
#include "gopost/engine.h"
#include "gopost/types.h"
#include "canvas/canvas_internal.h"

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cmath>

/*
 * S5-04: CPU composition pipeline.
 *
 * All blend modes use gopost_blend_pixel (see blend_modes.cpp).
 * Layers are composited bottom-to-top. The first layer in the vector is
 * the bottom layer; each subsequent layer blends over the accumulator.
 */

static inline uint8_t to_u8(float v) {
    int i = (int)(v * 255.0f + 0.5f);
    return (uint8_t)(i < 0 ? 0 : (i > 255 ? 255 : i));
}

/*
 * Composite a layer's pixels onto the destination buffer.
 * Positioned at (tx, ty) with scale (sx, sy) and opacity.
 * CPU path — rotation handled by GPU in Sprint 6+.
 */
static void composite_layer_onto(
    uint8_t* dst, int32_t dst_w, int32_t dst_h,
    const uint8_t* src, int32_t src_w, int32_t src_h,
    float tx, float ty, float sx, float sy,
    float opacity, GopostBlendMode mode
) {
    if (!src || src_w <= 0 || src_h <= 0) return;
    if (opacity <= 0.0f) return;

    int32_t placed_w = (int32_t)(src_w * sx);
    int32_t placed_h = (int32_t)(src_h * sy);
    if (placed_w <= 0 || placed_h <= 0) return;

    int32_t x0 = (int32_t)tx;
    int32_t y0 = (int32_t)ty;
    int32_t start_y = std::max(y0, 0);
    int32_t end_y   = std::min(y0 + placed_h, dst_h);
    int32_t start_x = std::max(x0, 0);
    int32_t end_x   = std::min(x0 + placed_w, dst_w);

    for (int32_t dy = start_y; dy < end_y; dy++) {
        int32_t sy_i = std::clamp((int32_t)((dy - y0) / sy), 0, src_h - 1);

        for (int32_t dx = start_x; dx < end_x; dx++) {
            int32_t sx_i = std::clamp((int32_t)((dx - x0) / sx), 0, src_w - 1);

            size_t s_off = ((size_t)sy_i * src_w + sx_i) * 4;
            size_t d_off = ((size_t)dy * dst_w + dx) * 4;

            uint8_t sa_mod = (uint8_t)(src[s_off + 3] * opacity + 0.5f);

            gopost_blend_pixel(mode,
                               src[s_off], src[s_off + 1], src[s_off + 2], sa_mod,
                               dst[d_off], dst[d_off + 1], dst[d_off + 2], dst[d_off + 3],
                               &dst[d_off], &dst[d_off + 1], &dst[d_off + 2], &dst[d_off + 3]);
        }
    }
}

extern "C" {

GopostError gopost_canvas_render(GopostCanvas* canvas, GopostFrame** out_frame) {
    if (!canvas || !out_frame) return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t w = gopost_internal_canvas_width(canvas);
    int32_t h = gopost_internal_canvas_height(canvas);
    if (w <= 0 || h <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    size_t stride = (size_t)w * 4;
    size_t buf_size = stride * (size_t)h;

    GopostFrame* frame = nullptr;
    GopostEngine* engine = gopost_internal_canvas_get_engine(canvas);

    if (engine && gopost_frame_acquire(engine, &frame, (uint32_t)w, (uint32_t)h, GOPOST_PIXEL_FORMAT_RGBA8) == GOPOST_OK && frame) {
        /* Frame from pool; stride/size set by engine */
        stride = frame->stride;
        buf_size = frame->data_size;
    } else {
        /* Fallback: malloc when engine is null or pool acquire failed */
        frame = static_cast<GopostFrame*>(malloc(sizeof(GopostFrame)));
        if (!frame) return GOPOST_ERROR_OUT_OF_MEMORY;
        frame->data = static_cast<uint8_t*>(malloc(buf_size));
        if (!frame->data) {
            free(frame);
            return GOPOST_ERROR_OUT_OF_MEMORY;
        }
        frame->width = (uint32_t)w;
        frame->height = (uint32_t)h;
        frame->format = GOPOST_PIXEL_FORMAT_RGBA8;
        frame->data_size = buf_size;
        frame->stride = stride;
    }

    if (gopost_internal_canvas_transparent_bg(canvas)) {
        memset(frame->data, 0, buf_size);
    } else {
        uint8_t br = to_u8(gopost_internal_canvas_bg_r(canvas));
        uint8_t bg = to_u8(gopost_internal_canvas_bg_g(canvas));
        uint8_t bb = to_u8(gopost_internal_canvas_bg_b(canvas));
        uint8_t ba = to_u8(gopost_internal_canvas_bg_a(canvas));
        for (size_t i = 0; i < buf_size; i += 4) {
            frame->data[i]     = br;
            frame->data[i + 1] = bg;
            frame->data[i + 2] = bb;
            frame->data[i + 3] = ba;
        }
    }

    frame->width = (uint32_t)w;
    frame->height = (uint32_t)h;
    frame->format = GOPOST_PIXEL_FORMAT_RGBA8;
    frame->data_size = buf_size;
    frame->stride = stride;

    int32_t layer_count = gopost_internal_canvas_layer_count(canvas);
    GopostLayerInfo info;

    for (int32_t i = 0; i < layer_count; i++) {
        if (gopost_internal_canvas_layer_at(canvas, i, &info) != GOPOST_OK)
            continue;

        if (!info.visible) continue;
        if (info.opacity <= 0.0f) continue;

        const uint8_t* px = nullptr;
        size_t px_size = 0;
        if (gopost_internal_canvas_layer_pixels(canvas, i, &px, &px_size) != GOPOST_OK)
            continue;

        if (!px || px_size == 0 || info.content_w <= 0 || info.content_h <= 0)
            continue;

        composite_layer_onto(
            frame->data, w, h,
            px, info.content_w, info.content_h,
            info.tx, info.ty,
            info.sx, info.sy,
            info.opacity,
            info.blend_mode
        );
    }

    gopost_canvas_invalidate(canvas);
    *out_frame = frame;
    return GOPOST_OK;
}

GopostError gopost_canvas_render_region(
    GopostCanvas* canvas,
    float x, float y, float w, float h,
    GopostFrame** out_frame
) {
    if (!canvas || !out_frame) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (w <= 0 || h <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t iw = (int32_t)w, ih = (int32_t)h;
    if (iw <= 0 || ih <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    size_t stride = (size_t)iw * 4;
    size_t buf_size = stride * (size_t)ih;

    GopostFrame* frame = nullptr;
    GopostEngine* engine = gopost_internal_canvas_get_engine(canvas);

    if (engine && gopost_frame_acquire(engine, &frame, (uint32_t)iw, (uint32_t)ih, GOPOST_PIXEL_FORMAT_RGBA8) == GOPOST_OK && frame) {
        stride = frame->stride;
        buf_size = frame->data_size;
    } else {
        frame = static_cast<GopostFrame*>(malloc(sizeof(GopostFrame)));
        if (!frame) return GOPOST_ERROR_OUT_OF_MEMORY;
        frame->data = static_cast<uint8_t*>(calloc(1, buf_size));
        if (!frame->data) {
            free(frame);
            return GOPOST_ERROR_OUT_OF_MEMORY;
        }
        frame->width = (uint32_t)iw;
        frame->height = (uint32_t)ih;
        frame->format = GOPOST_PIXEL_FORMAT_RGBA8;
        frame->data_size = buf_size;
        frame->stride = stride;
    }

    int32_t layer_count = gopost_internal_canvas_layer_count(canvas);
    GopostLayerInfo info;

    for (int32_t i = 0; i < layer_count; i++) {
        if (gopost_internal_canvas_layer_at(canvas, i, &info) != GOPOST_OK)
            continue;
        if (!info.visible || info.opacity <= 0.0f) continue;

        const uint8_t* px = nullptr;
        size_t px_size = 0;
        if (gopost_internal_canvas_layer_pixels(canvas, i, &px, &px_size) != GOPOST_OK)
            continue;
        if (!px || px_size == 0) continue;

        composite_layer_onto(
            frame->data, iw, ih,
            px, info.content_w, info.content_h,
            info.tx - x, info.ty - y,
            info.sx, info.sy,
            info.opacity,
            info.blend_mode
        );
    }

    *out_frame = frame;
    return GOPOST_OK;
}

GopostError gopost_canvas_invalidate(GopostCanvas* canvas) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    return GOPOST_OK;
}

GopostError gopost_canvas_invalidate_layer(GopostCanvas* canvas, int32_t layer_id) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    (void)layer_id;
    return gopost_canvas_invalidate(canvas);
}

GopostError gopost_canvas_render_gpu(
    GopostCanvas* canvas,
    void** out_texture_handle,
    int32_t* out_width, int32_t* out_height
) {
    if (!canvas || !out_texture_handle || !out_width || !out_height)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    /*
     * S5-05: GPU rendering path.
     * For Sprint 5, this performs a CPU render and returns the pixel buffer.
     * The caller owns the full GopostFrame and must free it via
     * free(data) when done. Real Metal/Vulkan path in Sprint 6+.
     */
    GopostFrame* frame = nullptr;
    GopostError err = gopost_canvas_render(canvas, &frame);
    if (err != GOPOST_OK) return err;

    *out_texture_handle = frame->data;
    *out_width = (int32_t)frame->width;
    *out_height = (int32_t)frame->height;

    /* Transfer pixel data ownership to caller; free only the struct wrapper */
    frame->data = nullptr;
    free(frame);

    return GOPOST_OK;
}

void gopost_render_frame_free(GopostFrame* frame) {
    if (!frame) return;
    if (frame->data) {
        free(frame->data);
        frame->data = nullptr;
    }
    free(frame);
}

} // extern "C"
