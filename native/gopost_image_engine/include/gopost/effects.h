#ifndef GOPOST_EFFECTS_H
#define GOPOST_EFFECTS_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostEngine GopostEngine;
typedef struct GopostCanvas GopostCanvas;

/*--- Effect parameter types ---*/

typedef enum {
    GOPOST_PARAM_FLOAT  = 0,
    GOPOST_PARAM_INT    = 1,
    GOPOST_PARAM_BOOL   = 2,
    GOPOST_PARAM_COLOR  = 3,
} GopostParamType;

/*--- Effect categories ---*/

typedef enum {
    GOPOST_EFFECT_CAT_ADJUSTMENT  = 0,
    GOPOST_EFFECT_CAT_COLOR       = 1,
    GOPOST_EFFECT_CAT_BLUR        = 2,
    GOPOST_EFFECT_CAT_STYLIZE     = 3,
    GOPOST_EFFECT_CAT_DISTORT     = 4,
    GOPOST_EFFECT_CAT_PRESET      = 5,
} GopostEffectCategory;

/*--- Parameter definition ---*/

typedef struct {
    char id[64];
    char display_name[64];
    GopostParamType type;
    float default_val;
    float min_val;
    float max_val;
} GopostParamDef;

#define GOPOST_MAX_EFFECT_PARAMS 16

/*--- Effect definition (immutable descriptor) ---*/

typedef struct {
    char id[64];
    char display_name[64];
    GopostEffectCategory category;
    int32_t param_count;
    GopostParamDef params[GOPOST_MAX_EFFECT_PARAMS];
    int32_t gpu_accelerated;
} GopostEffectDef;

/*--- Effect instance (mutable, per-layer application) ---*/

typedef struct {
    int32_t instance_id;
    char effect_id[64];
    int32_t enabled;
    float mix;              /* 0.0 = no effect, 1.0 = full */
    float param_values[GOPOST_MAX_EFFECT_PARAMS];
} GopostEffectInstance;

/*--- Registry: register, query, list ---*/

GopostError gopost_effects_init(GopostEngine* engine);
GopostError gopost_effects_shutdown(GopostEngine* engine);

GopostError gopost_effects_get_count(GopostEngine* engine, int32_t* out_count);
GopostError gopost_effects_get_def(GopostEngine* engine, int32_t index,
                                   GopostEffectDef* out_def);
GopostError gopost_effects_find(GopostEngine* engine, const char* effect_id,
                                GopostEffectDef* out_def);

GopostError gopost_effects_list_category(GopostEngine* engine,
                                         GopostEffectCategory category,
                                         GopostEffectDef* out_defs,
                                         int32_t max_count, int32_t* out_count);

/*--- Apply / remove effects on a layer ---*/

GopostError gopost_layer_add_effect(GopostCanvas* canvas, int32_t layer_id,
                                    const char* effect_id,
                                    int32_t* out_instance_id);

GopostError gopost_layer_remove_effect(GopostCanvas* canvas, int32_t layer_id,
                                       int32_t instance_id);

GopostError gopost_layer_get_effect_count(GopostCanvas* canvas, int32_t layer_id,
                                          int32_t* out_count);

GopostError gopost_layer_get_effect(GopostCanvas* canvas, int32_t layer_id,
                                    int32_t index,
                                    GopostEffectInstance* out_instance);

GopostError gopost_effect_set_enabled(GopostCanvas* canvas, int32_t layer_id,
                                      int32_t instance_id, int32_t enabled);

GopostError gopost_effect_set_mix(GopostCanvas* canvas, int32_t layer_id,
                                  int32_t instance_id, float mix);

GopostError gopost_effect_set_param(GopostCanvas* canvas, int32_t layer_id,
                                    int32_t instance_id,
                                    const char* param_id, float value);

GopostError gopost_effect_get_param(GopostCanvas* canvas, int32_t layer_id,
                                    int32_t instance_id,
                                    const char* param_id, float* out_value);

/*--- Process effects on a frame (CPU path) ---*/

GopostError gopost_effects_process(GopostEngine* engine,
                                   const GopostEffectInstance* instance,
                                   GopostFrame* in_frame,
                                   GopostFrame* out_frame);

/*--- Preset filter helpers ---*/

GopostError gopost_preset_get_count(GopostEngine* engine, int32_t* out_count);
GopostError gopost_preset_get_info(GopostEngine* engine, int32_t index,
                                   char* out_name, int32_t name_len,
                                   char* out_category, int32_t cat_len);
GopostError gopost_preset_apply(GopostEngine* engine,
                                GopostFrame* frame,
                                int32_t preset_index, float intensity);

/*--- Blur / sharpen effects (CPU, operate on GopostFrame* in-place) ---*/

GopostError gopost_effect_gaussian_blur(GopostFrame* frame, float radius);
GopostError gopost_effect_box_blur(GopostFrame* frame, float radius);
GopostError gopost_effect_unsharp_mask(GopostFrame* frame, float amount, float radius, float threshold);
GopostError gopost_effect_radial_blur(GopostFrame* frame, float center_x, float center_y, float amount);
GopostError gopost_effect_tilt_shift(GopostFrame* frame, float focus_y, float blur_radius, float transition_size);

/*--- Artistic filters (CPU, operate on GopostFrame* in-place) ---*/

GopostError gopost_effect_oil_paint(GopostFrame* frame, float radius, float intensity);
GopostError gopost_effect_watercolor(GopostFrame* frame, float brush_size, float intensity);
GopostError gopost_effect_sketch(GopostFrame* frame, float threshold, float intensity);
GopostError gopost_effect_pixelate(GopostFrame* frame, int block_size);
GopostError gopost_effect_glitch(GopostFrame* frame, float amount, float seed);
GopostError gopost_effect_halftone(GopostFrame* frame, float dot_size, float angle, float contrast);

/*--- GPU artistic filter shader source (Metal compute kernels) ---*/

const char* gopost_artistic_compute_shader_source(void);

#ifdef __cplusplus
}
#endif

#endif
