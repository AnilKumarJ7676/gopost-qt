#ifndef GOPOST_CANVAS_H
#define GOPOST_CANVAS_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostEngine GopostEngine;
typedef struct GopostCanvas GopostCanvas;

/*--- Enums ---*/

typedef enum {
    GOPOST_LAYER_IMAGE       = 0,
    GOPOST_LAYER_SOLID_COLOR = 1,
    GOPOST_LAYER_TEXT        = 2,
    GOPOST_LAYER_SHAPE       = 3,
    GOPOST_LAYER_GROUP       = 4,
    GOPOST_LAYER_ADJUSTMENT  = 5,
    GOPOST_LAYER_GRADIENT    = 6,
    GOPOST_LAYER_STICKER     = 7,
} GopostLayerType;

typedef enum {
    GOPOST_BLEND_NORMAL      = 0,
    GOPOST_BLEND_MULTIPLY    = 1,
    GOPOST_BLEND_SCREEN      = 2,
    GOPOST_BLEND_OVERLAY     = 3,
    GOPOST_BLEND_DARKEN      = 4,
    GOPOST_BLEND_LIGHTEN     = 5,
    GOPOST_BLEND_COLOR_DODGE = 6,
    GOPOST_BLEND_COLOR_BURN  = 7,
    GOPOST_BLEND_HARD_LIGHT  = 8,
    GOPOST_BLEND_SOFT_LIGHT  = 9,
    GOPOST_BLEND_DIFFERENCE  = 10,
    GOPOST_BLEND_EXCLUSION   = 11,
    GOPOST_BLEND_HUE         = 12,
    GOPOST_BLEND_SATURATION  = 13,
    GOPOST_BLEND_COLOR       = 14,
    GOPOST_BLEND_LUMINOSITY  = 15,
    GOPOST_BLEND_LINEAR_BURN = 16,
    GOPOST_BLEND_LINEAR_DODGE= 17,
    GOPOST_BLEND_VIVID_LIGHT = 18,
    GOPOST_BLEND_LINEAR_LIGHT= 19,
    GOPOST_BLEND_PIN_LIGHT   = 20,
    GOPOST_BLEND_HARD_MIX    = 21,
    GOPOST_BLEND_DISSOLVE    = 22,
    GOPOST_BLEND_DIVIDE      = 23,
    GOPOST_BLEND_SUBTRACT    = 24,
    GOPOST_BLEND_GRAIN_EXTRACT=25,
    GOPOST_BLEND_GRAIN_MERGE = 26,
} GopostBlendMode;

typedef enum {
    GOPOST_COLOR_SPACE_SRGB       = 0,
    GOPOST_COLOR_SPACE_DISPLAY_P3 = 1,
    GOPOST_COLOR_SPACE_ADOBE_RGB  = 2,
} GopostColorSpace;

/*--- Canvas config ---*/

typedef struct {
    int32_t width;
    int32_t height;
    float dpi;
    GopostColorSpace color_space;
    float bg_r, bg_g, bg_b, bg_a;
    int32_t transparent_bg;
} GopostCanvasConfig;

/*--- Layer info (read-only query result) ---*/

typedef struct {
    int32_t id;
    GopostLayerType type;
    char name[128];
    float opacity;
    GopostBlendMode blend_mode;
    int32_t visible;
    int32_t locked;
    float tx, ty;       /* translation */
    float sx, sy;       /* scale */
    float rotation;     /* degrees */
    int32_t content_w;  /* layer content pixel width */
    int32_t content_h;  /* layer content pixel height */
} GopostLayerInfo;

/*--- Canvas lifecycle (S5-03) ---*/

GopostError gopost_canvas_create(
    GopostEngine* engine,
    const GopostCanvasConfig* config,
    GopostCanvas** out_canvas
);

GopostError gopost_canvas_destroy(GopostCanvas* canvas);

GopostError gopost_canvas_get_size(
    GopostCanvas* canvas,
    int32_t* out_width, int32_t* out_height, float* out_dpi
);

GopostError gopost_canvas_resize(
    GopostCanvas* canvas,
    int32_t width, int32_t height
);

/*--- Layer management (S5-03) ---*/

GopostError gopost_canvas_add_image_layer(
    GopostCanvas* canvas,
    const uint8_t* rgba_pixels, int32_t width, int32_t height,
    int32_t index,
    int32_t* out_layer_id
);

GopostError gopost_canvas_add_solid_layer(
    GopostCanvas* canvas,
    float r, float g, float b, float a,
    int32_t width, int32_t height,
    int32_t index,
    int32_t* out_layer_id
);

GopostError gopost_canvas_add_group_layer(
    GopostCanvas* canvas,
    const char* name,
    int32_t index,
    int32_t* out_layer_id
);

GopostError gopost_canvas_remove_layer(GopostCanvas* canvas, int32_t layer_id);
GopostError gopost_canvas_reorder_layer(GopostCanvas* canvas, int32_t layer_id, int32_t new_index);
GopostError gopost_canvas_duplicate_layer(GopostCanvas* canvas, int32_t layer_id, int32_t* out_new_id);

/*--- Layer properties (S5-03) ---*/

GopostError gopost_layer_set_visible(GopostCanvas* canvas, int32_t layer_id, int32_t visible);
GopostError gopost_layer_set_locked(GopostCanvas* canvas, int32_t layer_id, int32_t locked);
GopostError gopost_layer_set_opacity(GopostCanvas* canvas, int32_t layer_id, float opacity);
GopostError gopost_layer_set_blend_mode(GopostCanvas* canvas, int32_t layer_id, GopostBlendMode mode);
GopostError gopost_layer_set_name(GopostCanvas* canvas, int32_t layer_id, const char* name);
GopostError gopost_layer_set_transform(
    GopostCanvas* canvas, int32_t layer_id,
    float tx, float ty,
    float sx, float sy,
    float rotation
);

GopostError gopost_canvas_get_layer_count(GopostCanvas* canvas, int32_t* out_count);
GopostError gopost_canvas_get_layer_info(GopostCanvas* canvas, int32_t layer_id, GopostLayerInfo* out_info);

/* Retrieves ordered layer IDs. out_ids must point to a buffer of at least
   gopost_canvas_get_layer_count() elements. */
GopostError gopost_canvas_get_layer_ids(GopostCanvas* canvas, int32_t* out_ids, int32_t max_count);

/*--- Composition & rendering (S5-04) ---*/

GopostError gopost_canvas_render(
    GopostCanvas* canvas,
    GopostFrame** out_frame
);

GopostError gopost_canvas_render_region(
    GopostCanvas* canvas,
    float x, float y, float w, float h,
    GopostFrame** out_frame
);

GopostError gopost_canvas_invalidate(GopostCanvas* canvas);
GopostError gopost_canvas_invalidate_layer(GopostCanvas* canvas, int32_t layer_id);

/* Free a frame returned by gopost_canvas_render / gopost_canvas_render_region.
 * If the frame was allocated from the engine's frame pool, use gopost_frame_release(engine, frame)
 * instead; get the engine via gopost_canvas_get_engine(canvas). */
void gopost_render_frame_free(GopostFrame* frame);

/* Get the engine that owns this canvas (for frame_release when frame came from pool). */
GopostEngine* gopost_canvas_get_engine(GopostCanvas* canvas);

/*--- GPU texture output (S5-05) ---*/

GopostError gopost_canvas_render_gpu(
    GopostCanvas* canvas,
    void** out_texture_handle,
    int32_t* out_width, int32_t* out_height
);

GopostError gopost_canvas_set_viewport(
    GopostCanvas* canvas,
    float pan_x, float pan_y,
    float zoom, float rotation
);

GopostError gopost_canvas_get_viewport(
    GopostCanvas* canvas,
    float* out_pan_x, float* out_pan_y,
    float* out_zoom, float* out_rotation
);

/*--- Tile configuration (S5-06) ---*/

GopostError gopost_canvas_set_tile_size(GopostCanvas* canvas, int32_t tile_size);
GopostError gopost_canvas_get_dirty_tile_count(GopostCanvas* canvas, int32_t* out_count);

/*--- Frame callback for continuous rendering ---*/

typedef void (*GopostFrameCallback)(
    void* user_data,
    void* texture_handle,
    int32_t width, int32_t height
);

GopostError gopost_canvas_set_frame_callback(
    GopostCanvas* canvas,
    GopostFrameCallback callback,
    void* user_data
);

/*--- Per-pixel blend (for composition) ---*/

void gopost_blend_pixel(GopostBlendMode mode,
                        uint8_t src_r, uint8_t src_g, uint8_t src_b, uint8_t src_a,
                        uint8_t dst_r, uint8_t dst_g, uint8_t dst_b, uint8_t dst_a,
                        uint8_t* out_r, uint8_t* out_g, uint8_t* out_b, uint8_t* out_a);

#ifdef __cplusplus
}
#endif

#endif
