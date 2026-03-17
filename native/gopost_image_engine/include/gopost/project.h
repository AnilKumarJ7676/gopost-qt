#ifndef GOPOST_PROJECT_H
#define GOPOST_PROJECT_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostCanvas GopostCanvas;
typedef struct GopostEngine GopostEngine;

#define GOPOST_PROJECT_MAGIC   0x4750494D  /* "GPIM" */
#define GOPOST_PROJECT_VERSION 1

typedef struct {
    uint32_t magic;
    uint32_t version;
    int32_t canvas_width;
    int32_t canvas_height;
    float dpi;
    int32_t color_space;
    float bg_r, bg_g, bg_b, bg_a;
    int32_t transparent_bg;
    int32_t layer_count;
} GopostProjectHeader;

typedef struct {
    int32_t layer_id;
    int32_t layer_type;
    char name[128];
    float opacity;
    int32_t blend_mode;
    int32_t visible;
    int32_t locked;
    float tx, ty, sx, sy, rotation;
    int32_t content_w;
    int32_t content_h;
    uint32_t pixel_data_size;   /* 0 if no pixel data (group layers) */
} GopostProjectLayerEntry;

/* Serialize canvas state to a .gpimg buffer */
GopostError gopost_project_save(
    GopostCanvas* canvas,
    uint8_t** out_data, size_t* out_size
);

/* Deserialize .gpimg buffer and reconstruct canvas */
GopostError gopost_project_load(
    GopostEngine* engine,
    const uint8_t* data, size_t data_size,
    GopostCanvas** out_canvas
);

/* Save to file path */
GopostError gopost_project_save_to_file(
    GopostCanvas* canvas,
    const char* path
);

/* Load from file path */
GopostError gopost_project_load_from_file(
    GopostEngine* engine,
    const char* path,
    GopostCanvas** out_canvas
);

void gopost_project_free(uint8_t* data);

#ifdef __cplusplus
}
#endif

#endif
