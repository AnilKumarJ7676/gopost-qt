#ifndef GOPOST_MASK_H
#define GOPOST_MASK_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostCanvas GopostCanvas;

typedef enum {
    GOPOST_MASK_NONE     = 0,
    GOPOST_MASK_RASTER   = 1,  /* Grayscale bitmap: white=visible, black=hidden */
    GOPOST_MASK_VECTOR   = 2,  /* Path-based clipping (future) */
    GOPOST_MASK_CLIPPING = 3,  /* Clip to layer below */
} GopostMaskType;

typedef enum {
    GOPOST_MASK_BRUSH_PAINT = 0,  /* Paint white (reveal) */
    GOPOST_MASK_BRUSH_ERASE = 1,  /* Paint black (hide) */
} GopostMaskBrushMode;

/* Add a raster mask to a layer. Creates a white (fully visible) mask by default. */
GopostError gopost_layer_add_mask(GopostCanvas* canvas, int32_t layer_id, GopostMaskType type);

/* Remove mask from layer */
GopostError gopost_layer_remove_mask(GopostCanvas* canvas, int32_t layer_id);

/* Check if layer has a mask */
GopostError gopost_layer_has_mask(GopostCanvas* canvas, int32_t layer_id, int32_t* out_has_mask);

/* Get mask type */
GopostError gopost_layer_get_mask_type(GopostCanvas* canvas, int32_t layer_id, GopostMaskType* out_type);

/* Invert the mask */
GopostError gopost_layer_invert_mask(GopostCanvas* canvas, int32_t layer_id);

/* Enable/disable the mask (without removing it) */
GopostError gopost_layer_set_mask_enabled(GopostCanvas* canvas, int32_t layer_id, int32_t enabled);

/* Get mask data as grayscale buffer (w x h, 1 byte per pixel) */
GopostError gopost_layer_get_mask_data(GopostCanvas* canvas, int32_t layer_id,
                                       uint8_t** out_data, int32_t* out_w, int32_t* out_h);

/* Set mask data from grayscale buffer */
GopostError gopost_layer_set_mask_data(GopostCanvas* canvas, int32_t layer_id,
                                       const uint8_t* data, int32_t w, int32_t h);

/* Paint on mask with a circular brush */
GopostError gopost_layer_mask_paint(GopostCanvas* canvas, int32_t layer_id,
                                    float center_x, float center_y,
                                    float radius, float hardness,
                                    GopostMaskBrushMode mode, float opacity);

/* Fill entire mask with a value (0=black/hidden, 255=white/visible) */
GopostError gopost_layer_mask_fill(GopostCanvas* canvas, int32_t layer_id, uint8_t value);

/* Apply mask to produce final composited layer pixels */
GopostError gopost_layer_apply_mask(GopostCanvas* canvas, int32_t layer_id);

#ifdef __cplusplus
}
#endif

#endif
