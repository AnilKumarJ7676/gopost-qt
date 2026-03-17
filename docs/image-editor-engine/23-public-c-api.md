
## IE-23. Public C API (FFI Boundary)

### 23.1 Complete C API Header

```c
#ifndef GOPOST_IE_API_H
#define GOPOST_IE_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
    #define GP_API __declspec(dllexport)
#else
    #define GP_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t GpError;
typedef struct GpEngine GpEngine;
typedef struct GpCanvas GpCanvas;
typedef struct GpExportSession GpExportSession;
typedef struct GpAISession GpAISession;
typedef struct GpBrushSession GpBrushSession;

/* ─── Error Codes ─────────────────────────────── */
#define GP_OK                    0
#define GP_ERR_INVALID_ARG      -1
#define GP_ERR_OUT_OF_MEMORY    -2
#define GP_ERR_GPU_ERROR        -3
#define GP_ERR_DECODE_ERROR     -4
#define GP_ERR_ENCODE_ERROR     -5
#define GP_ERR_FILE_NOT_FOUND   -6
#define GP_ERR_UNSUPPORTED      -7
#define GP_ERR_CANCELLED        -8
#define GP_ERR_LAYER_NOT_FOUND  -9
#define GP_ERR_TEMPLATE_ERROR   -10
#define GP_ERR_INTERNAL         -99

GP_API const char* gp_ie_error_string(GpError error);

/* ─── Engine Lifecycle ──────────────────────────── */
typedef struct {
    int gpu_backend;            /* 0=auto, 1=metal, 2=vulkan, 3=gles, 4=dx12 */
    int thread_count;           /* 0=auto */
    size_t memory_budget;       /* bytes, 0=auto */
    const char* cache_dir;
    const char* temp_dir;
    void* native_window;        /* Platform window handle for GPU init */
} GpIEEngineConfig;

GP_API GpError gp_ie_engine_create(const GpIEEngineConfig* config, GpEngine** out);
GP_API void    gp_ie_engine_destroy(GpEngine* engine);
GP_API GpError gp_ie_engine_gpu_info(GpEngine* engine, char* out_json, size_t max_len);

/* ─── Canvas ────────────────────────────────────── */
typedef struct {
    int width, height;
    float dpi;
    int color_space;            /* 0=sRGB, 1=DisplayP3, 2=AdobeRGB, 3=ProPhotoRGB */
    int bit_depth;              /* 0=uint8, 1=uint16, 2=float16, 3=float32 */
    float bg_r, bg_g, bg_b, bg_a;
    int transparent_bg;
} GpCanvasConfig;

GP_API GpError gp_canvas_create(GpEngine* engine, const GpCanvasConfig* config,
                                 GpCanvas** out);
GP_API void    gp_canvas_destroy(GpCanvas* canvas);
GP_API GpError gp_canvas_resize(GpCanvas* canvas, int width, int height);
GP_API GpError gp_canvas_get_size(GpCanvas* canvas, int* out_w, int* out_h, float* out_dpi);

/* ─── Viewport ──────────────────────────────────── */
GP_API GpError gp_canvas_set_viewport(GpCanvas* canvas, float pan_x, float pan_y,
                                       float zoom, float rotation);
GP_API GpError gp_canvas_get_viewport(GpCanvas* canvas, float* pan_x, float* pan_y,
                                       float* zoom, float* rotation);

/* ─── Media Import ──────────────────────────────── */
GP_API GpError gp_media_import(GpEngine* engine, const char* path, int32_t* out_media_id);
GP_API GpError gp_media_import_buffer(GpEngine* engine, const uint8_t* data, size_t size,
                                       const char* format_hint, int32_t* out_media_id);
GP_API GpError gp_media_info(GpEngine* engine, int32_t media_id,
                              char* out_json, size_t max_len);
GP_API GpError gp_media_thumbnail(GpEngine* engine, int32_t media_id, int max_width,
                                   uint8_t* out_rgba, int* out_w, int* out_h);
GP_API void    gp_media_remove(GpEngine* engine, int32_t media_id);

/* ─── Layers ────────────────────────────────────── */
GP_API GpError gp_layer_add_image(GpCanvas* canvas, int32_t media_id,
                                   int32_t index, int32_t* out_layer_id);
GP_API GpError gp_layer_add_text(GpCanvas* canvas, const char* text_json,
                                  int32_t index, int32_t* out_layer_id);
GP_API GpError gp_layer_add_shape(GpCanvas* canvas, const char* shape_json,
                                   int32_t index, int32_t* out_layer_id);
GP_API GpError gp_layer_add_solid(GpCanvas* canvas, float r, float g, float b, float a,
                                   int width, int height, int32_t index, int32_t* out_layer_id);
GP_API GpError gp_layer_add_gradient(GpCanvas* canvas, const char* gradient_json,
                                      int32_t index, int32_t* out_layer_id);
GP_API GpError gp_layer_add_group(GpCanvas* canvas, const char* name,
                                   int32_t index, int32_t* out_layer_id);
GP_API GpError gp_layer_add_adjustment(GpCanvas* canvas, const char* adj_json,
                                        int32_t index, int32_t* out_layer_id);
GP_API GpError gp_layer_add_sticker(GpCanvas* canvas, const char* sticker_id,
                                     int32_t index, int32_t* out_layer_id);

GP_API GpError gp_layer_remove(GpCanvas* canvas, int32_t layer_id);
GP_API GpError gp_layer_duplicate(GpCanvas* canvas, int32_t layer_id, int32_t* out_new_id);
GP_API GpError gp_layer_reorder(GpCanvas* canvas, int32_t layer_id, int32_t new_index);
GP_API GpError gp_layer_merge_down(GpCanvas* canvas, int32_t layer_id);
GP_API GpError gp_layer_flatten_all(GpCanvas* canvas);

/* ─── Layer Properties ──────────────────────────── */
GP_API GpError gp_layer_set_visible(GpCanvas* canvas, int32_t layer_id, int visible);
GP_API GpError gp_layer_set_locked(GpCanvas* canvas, int32_t layer_id, int locked);
GP_API GpError gp_layer_set_opacity(GpCanvas* canvas, int32_t layer_id, float opacity);
GP_API GpError gp_layer_set_blend_mode(GpCanvas* canvas, int32_t layer_id, int blend_mode);
GP_API GpError gp_layer_set_name(GpCanvas* canvas, int32_t layer_id, const char* name);

/* ─── Layer Transform ───────────────────────────── */
GP_API GpError gp_layer_set_transform(GpCanvas* canvas, int32_t layer_id,
                                       const char* transform_json);
GP_API GpError gp_layer_get_transform(GpCanvas* canvas, int32_t layer_id,
                                       char* out_json, size_t max_len);
GP_API GpError gp_layer_flip_horizontal(GpCanvas* canvas, int32_t layer_id);
GP_API GpError gp_layer_flip_vertical(GpCanvas* canvas, int32_t layer_id);
GP_API GpError gp_layer_align(GpCanvas* canvas, const int32_t* layer_ids, int count,
                               int alignment);

/* ─── Effects ───────────────────────────────────── */
GP_API GpError gp_effect_list(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_layer_add_effect(GpCanvas* canvas, int32_t layer_id,
                                    const char* effect_id, int32_t* out_effect_instance_id);
GP_API GpError gp_layer_remove_effect(GpCanvas* canvas, int32_t layer_id,
                                       int32_t effect_instance_id);
GP_API GpError gp_effect_set_param(GpCanvas* canvas, int32_t layer_id,
                                    int32_t effect_instance_id,
                                    const char* param_id, const char* value_json);

/* ─── Layer Style ───────────────────────────────── */
GP_API GpError gp_layer_set_style(GpCanvas* canvas, int32_t layer_id,
                                   const char* style_json);
GP_API GpError gp_layer_get_style(GpCanvas* canvas, int32_t layer_id,
                                   char* out_json, size_t max_len);

/* ─── Filters / Presets ─────────────────────────── */
GP_API GpError gp_filter_list(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_filter_apply(GpCanvas* canvas, int32_t layer_id,
                                const char* filter_id, float intensity);
GP_API GpError gp_filter_preview(GpCanvas* canvas, int32_t layer_id,
                                  const char* filter_id, float intensity);

/* ─── Text ──────────────────────────────────────── */
GP_API GpError gp_text_update(GpCanvas* canvas, int32_t layer_id, const char* text_json);
GP_API GpError gp_text_set_content(GpCanvas* canvas, int32_t layer_id, const char* text);
GP_API GpError gp_font_list(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_font_load(GpEngine* engine, const uint8_t* data, size_t size,
                             const char* family, const char* style);

/* ─── Selection ─────────────────────────────────── */
GP_API GpError gp_select_all(GpCanvas* canvas);
GP_API GpError gp_select_none(GpCanvas* canvas);
GP_API GpError gp_select_invert(GpCanvas* canvas);
GP_API GpError gp_select_rect(GpCanvas* canvas, float x, float y, float w, float h, int mode);
GP_API GpError gp_select_ellipse(GpCanvas* canvas, float x, float y, float w, float h, int mode);
GP_API GpError gp_select_magic_wand(GpCanvas* canvas, float x, float y,
                                     int tolerance, int contiguous, int mode);
GP_API GpError gp_select_color_range(GpCanvas* canvas, float r, float g, float b,
                                      float fuzziness);
GP_API GpError gp_select_feather(GpCanvas* canvas, float radius);
GP_API GpError gp_select_expand(GpCanvas* canvas, float pixels);
GP_API GpError gp_select_contract(GpCanvas* canvas, float pixels);

/* ─── Mask ──────────────────────────────────────── */
GP_API GpError gp_mask_create_from_selection(GpCanvas* canvas, int32_t layer_id);
GP_API GpError gp_mask_delete(GpCanvas* canvas, int32_t layer_id);
GP_API GpError gp_mask_set_enabled(GpCanvas* canvas, int32_t layer_id, int enabled);
GP_API GpError gp_mask_invert(GpCanvas* canvas, int32_t layer_id);

/* ─── Brush ─────────────────────────────────────── */
GP_API GpError gp_brush_list(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_brush_begin_stroke(GpCanvas* canvas, int32_t layer_id,
                                      const char* brush_json,
                                      float x, float y, float pressure,
                                      GpBrushSession** out);
GP_API GpError gp_brush_continue_stroke(GpBrushSession* session,
                                         float x, float y, float pressure);
GP_API GpError gp_brush_end_stroke(GpBrushSession* session);
GP_API void    gp_brush_cancel_stroke(GpBrushSession* session);
GP_API void    gp_brush_destroy_session(GpBrushSession* session);

/* ─── Transform / Warp ──────────────────────────── */
GP_API GpError gp_transform_warp(GpCanvas* canvas, int32_t layer_id,
                                  const char* warp_json);
GP_API GpError gp_transform_liquify_begin(GpCanvas* canvas, int32_t layer_id);
GP_API GpError gp_transform_liquify_brush(GpCanvas* canvas, int32_t layer_id,
                                           int brush_type, float x, float y,
                                           float radius, float pressure);
GP_API GpError gp_transform_liquify_end(GpCanvas* canvas, int32_t layer_id);
GP_API GpError gp_transform_perspective(GpCanvas* canvas, int32_t layer_id,
                                         float tl_x, float tl_y, float tr_x, float tr_y,
                                         float bl_x, float bl_y, float br_x, float br_y);

/* ─── AI Features ───────────────────────────────── */
GP_API GpError gp_ai_remove_background(GpEngine* engine, GpCanvas* canvas,
                                        int32_t layer_id, int quality,
                                        GpAISession** out_session);
GP_API GpError gp_ai_remove_object(GpEngine* engine, GpCanvas* canvas,
                                    int32_t layer_id, const uint8_t* mask,
                                    int mask_w, int mask_h,
                                    GpAISession** out_session);
GP_API GpError gp_ai_upscale(GpEngine* engine, GpCanvas* canvas,
                              int32_t layer_id, int scale_factor,
                              GpAISession** out_session);
GP_API GpError gp_ai_style_transfer(GpEngine* engine, GpCanvas* canvas,
                                     int32_t layer_id, int style_id,
                                     float strength, GpAISession** out_session);
GP_API GpError gp_ai_auto_enhance(GpEngine* engine, GpCanvas* canvas,
                                    int32_t layer_id, GpAISession** out_session);
GP_API GpError gp_ai_session_progress(GpAISession* session, float* out_progress);
GP_API GpError gp_ai_session_result(GpAISession* session, char* out_json, size_t max_len);
GP_API void    gp_ai_session_cancel(GpAISession* session);
GP_API void    gp_ai_session_destroy(GpAISession* session);

/* ─── Template ──────────────────────────────────── */
GP_API GpError gp_template_load(GpEngine* engine, const uint8_t* encrypted_data,
                                 size_t data_size, const uint8_t* session_key,
                                 size_t key_size, GpCanvas** out_canvas);
GP_API GpError gp_template_get_placeholders(GpCanvas* canvas,
                                             char* out_json, size_t max_len);
GP_API GpError gp_template_set_text(GpCanvas* canvas, const char* placeholder_id,
                                     const char* text);
GP_API GpError gp_template_set_image(GpCanvas* canvas, const char* placeholder_id,
                                      int32_t media_id);
GP_API GpError gp_template_set_color(GpCanvas* canvas, const char* placeholder_id,
                                      float r, float g, float b, float a);
GP_API GpError gp_template_set_font(GpCanvas* canvas, const char* placeholder_id,
                                     const char* font_family, const char* font_style);
GP_API GpError gp_template_get_palette(GpCanvas* canvas, char* out_json, size_t max_len);
GP_API GpError gp_template_set_palette(GpCanvas* canvas, const char* palette_json);

/* ─── Export ────────────────────────────────────── */
GP_API GpError gp_export_presets(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_export_start(GpCanvas* canvas, const char* config_json,
                                const char* output_path, GpExportSession** out);
GP_API GpError gp_export_to_buffer(GpCanvas* canvas, const char* config_json,
                                    uint8_t** out_data, size_t* out_size);
GP_API GpError gp_export_progress(GpExportSession* session, float* out_progress);
GP_API void    gp_export_cancel(GpExportSession* session);
GP_API void    gp_export_destroy(GpExportSession* session);

/* ─── Rendering ─────────────────────────────────── */
GP_API GpError gp_canvas_render(GpCanvas* canvas, void** out_texture_handle,
                                 int* out_width, int* out_height);
GP_API GpError gp_canvas_invalidate(GpCanvas* canvas);
GP_API GpError gp_canvas_invalidate_layer(GpCanvas* canvas, int32_t layer_id);

/* ─── Project ───────────────────────────────────── */
GP_API GpError gp_project_save(GpCanvas* canvas, const char* path);
GP_API GpError gp_project_load(GpEngine* engine, const char* path, GpCanvas** out);
GP_API GpError gp_project_has_recovery(const char* path, int* out_has);
GP_API GpError gp_project_recover(GpEngine* engine, const char* path, GpCanvas** out);

/* ─── Undo/Redo ─────────────────────────────────── */
GP_API GpError gp_ie_undo(GpCanvas* canvas);
GP_API GpError gp_ie_redo(GpCanvas* canvas);
GP_API int     gp_ie_can_undo(GpCanvas* canvas);
GP_API int     gp_ie_can_redo(GpCanvas* canvas);
GP_API GpError gp_ie_undo_description(GpCanvas* canvas, char* out, size_t max_len);
GP_API GpError gp_ie_history_list(GpCanvas* canvas, char* out_json, size_t max_len);
GP_API GpError gp_ie_history_goto(GpCanvas* canvas, int32_t state_index);

/* ─── Callbacks ─────────────────────────────────── */
typedef void (*GpFrameCallback)(void* user_data, void* texture_handle, int width, int height);
typedef void (*GpProgressCallback)(void* user_data, float progress);
typedef void (*GpErrorCallback)(void* user_data, GpError error, const char* message);
typedef void (*GpLayerCallback)(void* user_data, int32_t layer_id, int event_type);

GP_API GpError gp_ie_set_frame_callback(GpCanvas* canvas, GpFrameCallback cb, void* user_data);
GP_API GpError gp_ie_set_error_callback(GpEngine* engine, GpErrorCallback cb, void* user_data);
GP_API GpError gp_ie_set_layer_callback(GpCanvas* canvas, GpLayerCallback cb, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* GOPOST_IE_API_H */
```

### 23.2 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 3 | Wk 5-6 | IE-289 to IE-291 | Engine lifecycle, canvas, media import |
| Sprint 6 | Wk 11-12 | IE-292 to IE-294 | Layer CRUD, properties, transform |
| Sprint 10 | Wk 19-20 | IE-295 to IE-296 | Effects, text |
| Sprint 13 | Wk 25-26 | IE-297 to IE-298 | Selection, brush |
| Sprint 17 | Wk 33-34 | IE-299 to IE-300 | Template, export |
| Sprint 22 | Wk 43-44 | IE-301 to IE-303 | Undo/redo, callbacks, rendering |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-289 | As a Flutter developer, I want engine lifecycle API (create/destroy/gpu_info) so that I can initialize and query the engine | - gp_ie_engine_create, gp_ie_engine_destroy work correctly<br/>- gp_ie_engine_gpu_info returns JSON with GPU details<br/>- Config options (thread_count, memory_budget, cache_dir) applied | 3 | Sprint 3 | — |
| IE-290 | As a Flutter developer, I want canvas API (create/destroy/resize/viewport) so that I can manage canvases | - gp_canvas_create, gp_canvas_destroy, gp_canvas_resize, gp_canvas_get_size<br/>- gp_canvas_set_viewport, gp_canvas_get_viewport for pan/zoom/rotation<br/>- All return appropriate GpError | 5 | Sprint 3 | IE-289 |
| IE-291 | As a Flutter developer, I want media import API (import/info/thumbnail/remove) so that I can load and inspect images | - gp_media_import, gp_media_import_buffer, gp_media_info, gp_media_thumbnail, gp_media_remove<br/>- Thumbnail returns RGBA buffer<br/>- Format hint supported for buffer import | 5 | Sprint 3 | IE-289 |
| IE-292 | As a Flutter developer, I want layer API (add/remove/duplicate/reorder/merge/flatten) so that I can manage the layer stack | - gp_layer_add_* for image, text, shape, solid, gradient, group, adjustment, sticker<br/>- gp_layer_remove, gp_layer_duplicate, gp_layer_reorder, gp_layer_merge_down, gp_layer_flatten_all<br/>- All operations update canvas correctly | 8 | Sprint 6 | IE-290 |
| IE-293 | As a Flutter developer, I want layer property API (visible/locked/opacity/blend/name) so that I can control layer appearance | - gp_layer_set_visible, gp_layer_set_locked, gp_layer_set_opacity, gp_layer_set_blend_mode, gp_layer_set_name<br/>- Changes reflected in render | 3 | Sprint 6 | IE-292 |
| IE-294 | As a Flutter developer, I want layer transform API (set/get/flip/align) so that I can position and transform layers | - gp_layer_set_transform, gp_layer_get_transform (JSON)<br/>- gp_layer_flip_horizontal, gp_layer_flip_vertical<br/>- gp_layer_align for multiple layers | 5 | Sprint 6 | IE-292 |
| IE-295 | As a Flutter developer, I want effect API (list/add/remove/set_param) so that I can apply and configure effects | - gp_effect_list returns JSON of available effects<br/>- gp_layer_add_effect, gp_layer_remove_effect, gp_effect_set_param<br/>- Param changes apply non-destructively | 5 | Sprint 10 | IE-292 |
| IE-296 | As a Flutter developer, I want text API (update/set_content/font_list/font_load) so that I can edit text layers and manage fonts | - gp_text_update, gp_text_set_content for text layers<br/>- gp_font_list returns available fonts<br/>- gp_font_load for custom font data | 5 | Sprint 10 | IE-292 |
| IE-297 | As a Flutter developer, I want selection API (all/none/invert/rect/ellipse/magic_wand) so that I can create and modify selections | - gp_select_all, gp_select_none, gp_select_invert<br/>- gp_select_rect, gp_select_ellipse, gp_select_magic_wand with mode<br/>- Selection affects subsequent operations | 5 | Sprint 13 | IE-290 |
| IE-298 | As a Flutter developer, I want brush API (begin/continue/end/cancel) so that I can paint on layers | - gp_brush_begin_stroke, gp_brush_continue_stroke, gp_brush_end_stroke, gp_brush_cancel_stroke<br/>- GpBrushSession lifecycle managed correctly<br/>- Strokes render with correct brush params | 5 | Sprint 13 | IE-292 |
| IE-299 | As a Flutter developer, I want template API (load/placeholders/set_text/set_image/set_color) so that I can load and customize templates | - gp_template_load with encrypted .gpit and session key<br/>- gp_template_get_placeholders, gp_template_set_text, gp_template_set_image, gp_template_set_color<br/>- Placeholder replacement updates canvas | 8 | Sprint 17 | IE-290 |
| IE-300 | As a Flutter developer, I want export API (presets/start/to_buffer/progress/cancel) so that I can export the canvas | - gp_export_presets returns available presets<br/>- gp_export_start, gp_export_to_buffer with config JSON<br/>- gp_export_progress, gp_export_cancel for async export | 5 | Sprint 17 | IE-290 |
| IE-301 | As a Flutter developer, I want undo/redo API (undo/redo/can_undo/can_redo/history) so that I can support undo/redo in the UI | - gp_ie_undo, gp_ie_redo, gp_ie_can_undo, gp_ie_can_redo<br/>- gp_ie_undo_description, gp_ie_history_list, gp_ie_history_goto<br/>- History consistent with command pattern | 5 | Sprint 22 | IE-290 |
| IE-302 | As a Flutter developer, I want callback API (frame/error/layer callbacks) so that I can react to engine events | - gp_ie_set_frame_callback for render completion<br/>- gp_ie_set_error_callback for errors<br/>- gp_ie_set_layer_callback for layer events | 3 | Sprint 22 | IE-289, IE-290 |
| IE-303 | As a Flutter developer, I want rendering API (render/invalidate) so that I can trigger and control canvas rendering | - gp_canvas_render returns texture handle and dimensions<br/>- gp_canvas_invalidate, gp_canvas_invalidate_layer for dirty marking<br/>- Render produces correct output | 5 | Sprint 22 | IE-290 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1-2: Core Foundation + Layer System |
| **Sprint(s)** | IE-Sprint 3 + IE-Sprint 6 (progressive) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [20-platform-integration](20-platform-integration.md) |
| **Successor** | [18-project-serialization](18-project-serialization.md) |
| **Story Points Total** | 105 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-336 | As a developer, I want IE error codes and gp_ie_error_string so that errors are reportable | - GP_OK, GP_ERR_* defines<br/>- gp_ie_error_string(error) returns message<br/>- All error paths covered | 2 | P0 | — |
| IE-337 | As a developer, I want GpIEEngineConfig + gp_ie_engine_create/destroy so that the engine lifecycle is managed | - GpIEEngineConfig struct<br/>- gp_ie_engine_create(), gp_ie_engine_destroy()<br/>- Config: gpu_backend, thread_count, memory_budget | 5 | P0 | IE-336 |
| IE-338 | As a developer, I want canvas operations (create/destroy/settings/resize) so that canvases are manageable | - gp_canvas_create(), gp_canvas_destroy()<br/>- gp_canvas_resize(), gp_canvas_get_size()<br/>- GpCanvasConfig with all settings | 5 | P0 | IE-337 |
| IE-339 | As a developer, I want layer operations (add/remove/reorder/properties/visibility/opacity/blend) so that layers are manageable | - gp_layer_add_*, gp_layer_remove, gp_layer_reorder<br/>- gp_layer_set_visible, set_opacity, set_blend_mode<br/>- All layer types supported | 8 | P0 | IE-338 |
| IE-340 | As a developer, I want effect operations (add/remove/set_param per layer) so that effects are configurable | - gp_layer_add_effect, gp_layer_remove_effect<br/>- gp_effect_set_param with JSON value<br/>- gp_effect_list for available effects | 5 | P0 | IE-339 |
| IE-341 | As a developer, I want text operations (create/update/style) so that text layers are editable | - gp_layer_add_text, gp_text_update, gp_text_set_content<br/>- gp_font_list, gp_font_load<br/>- Style via JSON | 5 | P0 | IE-339 |
| IE-342 | As a developer, I want shape operations (create/modify/boolean) so that vector shapes are editable | - gp_layer_add_shape with shape_json<br/>- Modify and boolean ops<br/>- Clipper2 integration | 5 | P0 | IE-339 |
| IE-343 | As a developer, I want selection operations (create/modify/feather/invert/to_mask) so that selections work | - gp_select_all, gp_select_none, gp_select_invert<br/>- gp_select_rect, gp_select_ellipse, gp_select_magic_wand<br/>- gp_select_feather, gp_select_expand/contract | 5 | P0 | IE-338 |
| IE-344 | As a developer, I want brush operations (set_brush/begin_stroke/continue_stroke/end_stroke) so that painting works | - gp_brush_begin_stroke, continue_stroke, end_stroke<br/>- GpBrushSession lifecycle<br/>- brush_json for preset | 5 | P0 | IE-339 |
| IE-345 | As a developer, I want transform operations (free_transform/perspective/warp/liquify) so that transforms work | - gp_transform_warp, gp_transform_perspective<br/>- gp_transform_liquify_begin/brush/end<br/>- JSON params for warp | 5 | P0 | IE-339 |
| IE-346 | As a developer, I want template operations (load/placeholder_list/replace_placeholder) so that templates work | - gp_template_load with encrypted .gpit<br/>- gp_template_get_placeholders<br/>- gp_template_set_text, set_image, set_color | 8 | P0 | IE-338 |
| IE-347 | As a developer, I want export operations (configure/start/progress/cancel) so that export works | - gp_export_presets, gp_export_start, gp_export_to_buffer<br/>- gp_export_progress, gp_export_cancel<br/>- config_json for format options | 5 | P0 | IE-338 |
| IE-348 | As a developer, I want undo/redo operations so that history is exposed | - gp_ie_undo, gp_ie_redo<br/>- gp_ie_can_undo, gp_ie_can_redo<br/>- gp_ie_undo_description, gp_ie_history_list, gp_ie_history_goto | 5 | P0 | IE-338 |
| IE-349 | As a developer, I want AI operations (bg_remove/object_remove/upscale/style_transfer/session) so that AI features are exposed | - gp_ai_remove_background, gp_ai_remove_object<br/>- gp_ai_upscale, gp_ai_style_transfer, gp_ai_auto_enhance<br/>- GpAISession progress/result/cancel | 8 | P0 | IE-337 |
| IE-350 | As a developer, I want callback registration (render/progress/error) so that the app can react to engine events | - gp_ie_set_frame_callback<br/>- gp_ie_set_error_callback<br/>- gp_ie_set_layer_callback | 3 | P0 | IE-337 |
| IE-351 | As a developer, I want canvas render to Flutter texture handle so that the canvas displays in Flutter | - gp_canvas_render returns texture handle<br/>- Platform-specific texture registry<br/>- Zero-copy where possible | 5 | P0 | IE-338 |
| IE-352 | As a developer, I want media import (image files) so that images can be loaded | - gp_media_import, gp_media_import_buffer<br/>- Format hint for buffer<br/>- Media pool management | 5 | P0 | IE-337 |
| IE-353 | As a developer, I want media pool management so that media lifecycle is controlled | - gp_media_info, gp_media_thumbnail<br/>- gp_media_remove<br/>- Media refs in layers | 3 | P0 | IE-352 |
| IE-354 | As a developer, I want histogram data query so that levels/curves can be driven | - Histogram API for luminance/RGB<br/>- Per-channel or combined<br/>- GPU or CPU path | 3 | P0 | IE-338 |
| IE-355 | As a developer, I want Flutter FFI Dart bindings generation so that Dart can call the C API | - ffigen configuration for gopost_ie_api.h<br/>- Dart bindings generated<br/>- All public APIs exposed | 5 | P0 | IE-337 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
