#ifndef GOPOST_CANVAS_INTERNAL_H
#define GOPOST_CANVAS_INTERNAL_H

/*
 * Engine-internal interface for accessing canvas layer pixel data.
 * NOT part of the public C API. Used only by compositor and tile renderer
 * within the same shared library.
 */

#include "gopost/canvas.h"
#include "gopost/types.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t gopost_internal_canvas_layer_count(GopostCanvas* canvas);

GopostError gopost_internal_canvas_layer_at(
    GopostCanvas* canvas, int32_t index, GopostLayerInfo* out_info
);

GopostError gopost_internal_canvas_layer_pixels(
    GopostCanvas* canvas, int32_t index,
    const uint8_t** out_data, size_t* out_data_size
);

int32_t gopost_internal_canvas_width(GopostCanvas* canvas);
int32_t gopost_internal_canvas_height(GopostCanvas* canvas);
float gopost_internal_canvas_bg_r(GopostCanvas* canvas);
float gopost_internal_canvas_bg_g(GopostCanvas* canvas);
float gopost_internal_canvas_bg_b(GopostCanvas* canvas);
float gopost_internal_canvas_bg_a(GopostCanvas* canvas);
int32_t gopost_internal_canvas_transparent_bg(GopostCanvas* canvas);

GopostEngine* gopost_internal_canvas_get_engine(GopostCanvas* canvas);

/* Update layer pixel content (used by text layer update) */
GopostError gopost_internal_canvas_update_layer_pixels(
    GopostCanvas* canvas, int32_t layer_id,
    const uint8_t* rgba_pixels, int32_t width, int32_t height
);

#ifdef __cplusplus
}
#endif

#endif
