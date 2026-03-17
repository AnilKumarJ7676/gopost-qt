#ifndef GOPOST_TEXT_ENGINE_H
#define GOPOST_TEXT_ENGINE_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostEngine GopostEngine;
typedef struct GopostCanvas GopostCanvas;

/*--- Text alignment ---*/

typedef enum {
    GOPOST_TEXT_ALIGN_LEFT   = 0,
    GOPOST_TEXT_ALIGN_CENTER = 1,
    GOPOST_TEXT_ALIGN_RIGHT  = 2,
} GopostTextAlign;

/*--- Font style ---*/

typedef enum {
    GOPOST_FONT_STYLE_NORMAL      = 0,
    GOPOST_FONT_STYLE_BOLD        = 1,
    GOPOST_FONT_STYLE_ITALIC      = 2,
    GOPOST_FONT_STYLE_BOLD_ITALIC = 3,
} GopostFontStyle;

/*--- Text layer configuration ---*/

typedef struct {
    const char* text;
    const char* font_family;
    GopostFontStyle style;
    float font_size;
    GopostColor color;
    GopostTextAlign alignment;
    float line_height;       /* multiplier, 1.0 = normal */
    float letter_spacing;    /* extra pixels between glyphs */

    int32_t has_shadow;
    GopostColor shadow_color;
    float shadow_offset_x;
    float shadow_offset_y;
    float shadow_blur;

    int32_t has_outline;
    GopostColor outline_color;
    float outline_width;
} GopostTextConfig;

/*--- Text engine lifecycle ---*/

GopostError gopost_text_init(GopostEngine* engine);
GopostError gopost_text_shutdown(GopostEngine* engine);

/*--- Font management ---*/

GopostError gopost_text_register_font(GopostEngine* engine,
                                      const char* family, const char* style_name,
                                      const uint8_t* data, size_t data_size);

GopostError gopost_text_get_font_count(GopostEngine* engine, int32_t* out_count);
GopostError gopost_text_get_font_family(GopostEngine* engine, int32_t index,
                                        char* out_family, int32_t buf_len);

/*--- Text rasterization ---*/

GopostError gopost_text_rasterize(GopostEngine* engine,
                                  const GopostTextConfig* config,
                                  int32_t max_width,
                                  GopostFrame** out_frame);

/*--- Canvas integration: add text as a layer ---*/

GopostError gopost_canvas_add_text_layer(GopostCanvas* canvas,
                                         const GopostTextConfig* config,
                                         int32_t max_width,
                                         int32_t index,
                                         int32_t* out_layer_id);

GopostError gopost_canvas_update_text_layer(GopostCanvas* canvas,
                                            int32_t layer_id,
                                            const GopostTextConfig* config,
                                            int32_t max_width);

#ifdef __cplusplus
}
#endif

#endif
