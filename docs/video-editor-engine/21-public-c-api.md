## VE-21. Public C API (FFI Boundary — Extended)

### 21.1 Complete C API Header

```c
#ifndef GOPOST_VE_API_H
#define GOPOST_VE_API_H

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
typedef struct GpTimeline GpTimeline;
typedef struct GpComposition GpComposition;
typedef struct GpExportSession GpExportSession;
typedef struct GpAISession GpAISession;

// ─── Error Codes ───────────────────────────────
#define GP_OK                    0
#define GP_ERR_INVALID_ARG      -1
#define GP_ERR_OUT_OF_MEMORY    -2
#define GP_ERR_GPU_ERROR        -3
#define GP_ERR_DECODE_ERROR     -4
#define GP_ERR_ENCODE_ERROR     -5
#define GP_ERR_FILE_NOT_FOUND   -6
#define GP_ERR_UNSUPPORTED      -7
#define GP_ERR_CANCELLED        -8
#define GP_ERR_INTERNAL         -99

GP_API const char* gp_error_string(GpError error);

// ─── Engine Lifecycle ──────────────────────────
typedef struct {
    int gpu_backend;          // 0=auto, 1=metal, 2=vulkan, 3=gles, 4=dx12
    int thread_count;         // 0=auto
    size_t memory_budget;     // bytes, 0=auto
    const char* cache_dir;
    const char* temp_dir;
    void* native_window;      // Platform window handle for GPU init
} GpEngineConfig;

GP_API GpError gp_engine_create(const GpEngineConfig* config, GpEngine** out);
GP_API void    gp_engine_destroy(GpEngine* engine);
GP_API GpError gp_engine_gpu_info(GpEngine* engine, char* out_json, size_t max_len);

// ─── Media Import ──────────────────────────────
GP_API GpError gp_media_import(GpEngine* engine, const char* path, int32_t* out_media_id);
GP_API GpError gp_media_info(GpEngine* engine, int32_t media_id, char* out_json, size_t max_len);
GP_API GpError gp_media_thumbnail(GpEngine* engine, int32_t media_id,
                                   int64_t time_us, int max_width,
                                   uint8_t* out_rgba, int* out_w, int* out_h);
GP_API void    gp_media_remove(GpEngine* engine, int32_t media_id);

// ─── Timeline ──────────────────────────────────
typedef struct {
    int width, height;
    int fps_num, fps_den;
    int sample_rate;
    int audio_channels;
} GpTimelineConfig;

GP_API GpError gp_timeline_create(GpEngine* engine, const GpTimelineConfig* config,
                                   GpTimeline** out);
GP_API void    gp_timeline_destroy(GpTimeline* tl);

// Track operations
GP_API GpError gp_timeline_add_track(GpTimeline* tl, int type, const char* name, int32_t* out_id);
GP_API GpError gp_timeline_remove_track(GpTimeline* tl, int32_t track_id);
GP_API GpError gp_timeline_set_track_visible(GpTimeline* tl, int32_t track_id, int visible);
GP_API GpError gp_timeline_set_track_muted(GpTimeline* tl, int32_t track_id, int muted);

// Clip operations
GP_API GpError gp_timeline_add_clip(GpTimeline* tl, int32_t track_id,
                                     const char* clip_json, int32_t* out_clip_id);
GP_API GpError gp_timeline_remove_clip(GpTimeline* tl, int32_t clip_id);
GP_API GpError gp_timeline_move_clip(GpTimeline* tl, int32_t clip_id,
                                      int32_t target_track, int64_t target_time_us);
GP_API GpError gp_timeline_trim_clip(GpTimeline* tl, int32_t clip_id,
                                      int64_t in_delta_us, int64_t out_delta_us);
GP_API GpError gp_timeline_split_clip(GpTimeline* tl, int32_t clip_id,
                                       int64_t split_time_us, int32_t* out_new_clip_id);
GP_API GpError gp_timeline_set_clip_speed(GpTimeline* tl, int32_t clip_id, float speed);

// Playback
GP_API GpError gp_timeline_play(GpTimeline* tl);
GP_API GpError gp_timeline_pause(GpTimeline* tl);
GP_API GpError gp_timeline_seek(GpTimeline* tl, int64_t time_us);
GP_API GpError gp_timeline_get_position(GpTimeline* tl, int64_t* out_time_us);
GP_API GpError gp_timeline_get_duration(GpTimeline* tl, int64_t* out_duration_us);

// Frame rendering (returns GPU texture handle for Flutter)
GP_API GpError gp_timeline_render_frame(GpTimeline* tl, void** out_texture_handle,
                                         int* out_width, int* out_height);

// ─── Effects ───────────────────────────────────
GP_API GpError gp_effect_list(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_clip_add_effect(GpTimeline* tl, int32_t clip_id,
                                   const char* effect_id, int32_t* out_effect_instance_id);
GP_API GpError gp_clip_remove_effect(GpTimeline* tl, int32_t clip_id, int32_t effect_instance_id);
GP_API GpError gp_effect_set_param(GpTimeline* tl, int32_t clip_id,
                                    int32_t effect_instance_id,
                                    const char* param_id, const char* value_json);

// ─── Keyframes ─────────────────────────────────
GP_API GpError gp_keyframe_add(GpTimeline* tl, int32_t clip_id,
                                const char* property_path, const char* keyframe_json);
GP_API GpError gp_keyframe_remove(GpTimeline* tl, int32_t clip_id,
                                   const char* property_path, int32_t keyframe_index);
GP_API GpError gp_keyframe_modify(GpTimeline* tl, int32_t clip_id,
                                   const char* property_path, int32_t keyframe_index,
                                   const char* keyframe_json);

// ─── Transitions ───────────────────────────────
GP_API GpError gp_transition_list(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_clip_add_transition(GpTimeline* tl, int32_t clip_id,
                                       int edge,  // 0=in, 1=out
                                       const char* transition_id,
                                       int64_t duration_us);
GP_API GpError gp_clip_remove_transition(GpTimeline* tl, int32_t clip_id, int edge);

// ─── Audio ─────────────────────────────────────
GP_API GpError gp_audio_get_waveform(GpEngine* engine, int32_t media_id,
                                      int target_width, float* out_min, float* out_max,
                                      int* out_count);
GP_API GpError gp_audio_get_levels(GpTimeline* tl, float* out_left, float* out_right);
GP_API GpError gp_clip_set_volume(GpTimeline* tl, int32_t clip_id, float volume);
GP_API GpError gp_clip_set_pan(GpTimeline* tl, int32_t clip_id, float pan);
GP_API GpError gp_clip_add_audio_effect(GpTimeline* tl, int32_t clip_id,
                                         const char* effect_id, int32_t* out_id);

// ─── Text ──────────────────────────────────────
GP_API GpError gp_text_create_clip(GpTimeline* tl, int32_t track_id,
                                    const char* text_json, int32_t* out_clip_id);
GP_API GpError gp_text_update(GpTimeline* tl, int32_t clip_id, const char* text_json);
GP_API GpError gp_font_list(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_font_load(GpEngine* engine, const uint8_t* data, size_t size,
                             const char* family, const char* style);

// ─── AI Features ───────────────────────────────
GP_API GpError gp_ai_background_remove(GpEngine* engine, int32_t clip_id,
                                        GpAISession** out_session);
GP_API GpError gp_ai_auto_captions(GpEngine* engine, GpTimeline* tl,
                                     const char* language, GpAISession** out_session);
GP_API GpError gp_ai_scene_detect(GpEngine* engine, int32_t media_id,
                                    GpAISession** out_session);
GP_API GpError gp_ai_session_progress(GpAISession* session, float* out_progress);
GP_API GpError gp_ai_session_result(GpAISession* session, char* out_json, size_t max_len);
GP_API void    gp_ai_session_cancel(GpAISession* session);
GP_API void    gp_ai_session_destroy(GpAISession* session);

// ─── Export ────────────────────────────────────
GP_API GpError gp_export_presets(GpEngine* engine, char* out_json, size_t max_len);
GP_API GpError gp_export_start(GpTimeline* tl, const char* config_json,
                                const char* output_path, GpExportSession** out);
GP_API GpError gp_export_progress(GpExportSession* session, float* out_progress);
GP_API GpError gp_export_pause(GpExportSession* session);
GP_API GpError gp_export_resume(GpExportSession* session);
GP_API void    gp_export_cancel(GpExportSession* session);
GP_API void    gp_export_destroy(GpExportSession* session);

// ─── Project ───────────────────────────────────
GP_API GpError gp_project_save(GpTimeline* tl, const char* path);
GP_API GpError gp_project_load(GpEngine* engine, const char* path, GpTimeline** out);
GP_API GpError gp_project_has_recovery(const char* path, int* out_has);
GP_API GpError gp_project_recover(GpEngine* engine, const char* path, GpTimeline** out);

// ─── Undo/Redo ─────────────────────────────────
GP_API GpError gp_undo(GpTimeline* tl);
GP_API GpError gp_redo(GpTimeline* tl);
GP_API int     gp_can_undo(GpTimeline* tl);
GP_API int     gp_can_redo(GpTimeline* tl);
GP_API GpError gp_undo_description(GpTimeline* tl, char* out, size_t max_len);

// ─── Callbacks ─────────────────────────────────
typedef void (*GpFrameCallback)(void* user_data, void* texture_handle, int width, int height);
typedef void (*GpProgressCallback)(void* user_data, float progress);
typedef void (*GpErrorCallback)(void* user_data, GpError error, const char* message);

GP_API GpError gp_set_frame_callback(GpTimeline* tl, GpFrameCallback cb, void* user_data);
GP_API GpError gp_set_error_callback(GpEngine* engine, GpErrorCallback cb, void* user_data);

#ifdef __cplusplus
}
#endif

#endif // GOPOST_VE_API_H
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1-2: Core Foundation & Timeline Engine |
| **Sprint(s)** | VE-Sprint 3 + VE-Sprint 6 (progressive) |
| **Team** | C/C++ Engine Developer (2), Tech Lead, Flutter Developer |
| **Predecessor** | [20-build-system](20-build-system.md) |
| **Successor** | [22-testing-validation](22-testing-validation.md) |
| **Story Points Total** | 95 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-331 | As a Flutter developer, I want error code enum and gp_error_string so that errors are debuggable | - GpError enum with GP_OK, GP_ERR_*<br/>- gp_error_string(error) returns human-readable string<br/>- All error paths use enum | 2 | P0 | — |
| VE-332 | As a Flutter developer, I want GpEngineConfig + gp_engine_create/destroy so that the engine lifecycle is managed | - GpEngineConfig struct (gpu_backend, thread_count, memory_budget, cache_dir, etc.)<br/>- gp_engine_create(), gp_engine_destroy()<br/>- Null checks, cleanup on destroy | 5 | P0 | — |
| VE-333 | As a Flutter developer, I want gp_engine_gpu_info (JSON output) so that GPU capabilities are known | - gp_engine_gpu_info(engine, out_json, max_len)<br/>- JSON with backend, vendor, device name<br/>- Safe buffer handling | 2 | P1 | VE-332 |
| VE-334 | As a Flutter developer, I want gp_media_import + gp_media_info + gp_media_thumbnail + gp_media_remove so that media can be managed | - gp_media_import returns media_id<br/>- gp_media_info returns JSON metadata<br/>- gp_media_thumbnail, gp_media_remove | 5 | P0 | — |
| VE-335 | As a Flutter developer, I want GpTimelineConfig + gp_timeline_create/destroy so that timelines can be created | - GpTimelineConfig (width, height, fps, sample_rate, channels)<br/>- gp_timeline_create(), gp_timeline_destroy()<br/>- Ownership semantics clear | 5 | P0 | VE-332 |
| VE-336 | As a Flutter developer, I want track operations (add/remove/visible/muted) so that tracks can be managed | - gp_timeline_add_track, gp_timeline_remove_track<br/>- gp_timeline_set_track_visible, gp_timeline_set_track_muted<br/>- Track type support | 3 | P0 | VE-335 |
| VE-337 | As a Flutter developer, I want clip operations (add/remove/move/trim/split/speed) so that clips can be edited | - gp_timeline_add_clip, gp_timeline_remove_clip<br/>- gp_timeline_move_clip, gp_timeline_trim_clip<br/>- gp_timeline_split_clip, gp_timeline_set_clip_speed | 5 | P0 | VE-335 |
| VE-338 | As a Flutter developer, I want playback (play/pause/seek/position/duration) so that timeline playback works | - gp_timeline_play, gp_timeline_pause, gp_timeline_seek<br/>- gp_timeline_get_position, gp_timeline_get_duration<br/>- Thread-safe, callback-driven | 5 | P0 | VE-335 |
| VE-339 | As a Flutter developer, I want gp_timeline_render_frame→texture handle so that frames display in Flutter | - gp_timeline_render_frame returns texture handle<br/>- Width, height output<br/>- Flutter Texture interop (IOSurface, AHardwareBuffer, etc.) | 8 | P0 | VE-335 |
| VE-340 | As a Flutter developer, I want effect operations (list/add/remove/set_param) so that effects can be applied | - gp_effect_list returns JSON catalog<br/>- gp_clip_add_effect, gp_clip_remove_effect<br/>- gp_effect_set_param with value_json | 5 | P0 | VE-337 |
| VE-341 | As a Flutter developer, I want keyframe operations (add/remove/modify) so that animations can be created | - gp_keyframe_add, gp_keyframe_remove, gp_keyframe_modify<br/>- property_path, keyframe_json format<br/>- Interpolation support | 5 | P0 | VE-337 |
| VE-342 | As a Flutter developer, I want transition operations (list/add/remove) so that transitions can be used | - gp_transition_list returns JSON<br/>- gp_clip_add_transition (edge, id, duration)<br/>- gp_clip_remove_transition | 3 | P0 | VE-337 |
| VE-343 | As a Flutter developer, I want audio operations (waveform/levels/volume/pan/effect) so that audio is controllable | - gp_audio_get_waveform, gp_audio_get_levels<br/>- gp_clip_set_volume, gp_clip_set_pan<br/>- gp_clip_add_audio_effect | 5 | P0 | VE-335 |
| VE-344 | As a Flutter developer, I want text operations (create/update/font_list/font_load) so that text clips work | - gp_text_create_clip, gp_text_update<br/>- gp_font_list, gp_font_load<br/>- Text JSON schema | 5 | P0 | VE-337 |
| VE-345 | As a Flutter developer, I want AI session operations (background_remove/captions/scene_detect/progress/result/cancel) so that AI features are accessible | - gp_ai_background_remove, gp_ai_auto_captions, gp_ai_scene_detect<br/>- gp_ai_session_progress, gp_ai_session_result<br/>- gp_ai_session_cancel, gp_ai_session_destroy | 8 | P0 | — |
| VE-346 | As a Flutter developer, I want export operations (presets/start/progress/pause/resume/cancel) so that export works from Flutter | - gp_export_presets returns JSON<br/>- gp_export_start, gp_export_progress<br/>- gp_export_pause, gp_export_resume, gp_export_cancel | 5 | P0 | — |
| VE-347 | As a Flutter developer, I want project operations (save/load/recovery) so that projects persist | - gp_project_save, gp_project_load<br/>- gp_project_has_recovery, gp_project_recover<br/>- Path handling, error codes | 5 | P0 | — |
| VE-348 | As a Flutter developer, I want undo/redo (undo/redo/can_undo/can_redo/description) so that undo works from UI | - gp_undo, gp_redo<br/>- gp_can_undo, gp_can_redo<br/>- gp_undo_description for UI label | 3 | P0 | VE-335 |
| VE-349 | As a Flutter developer, I want callback registration (frame/progress/error) so that async events are received | - gp_set_frame_callback for rendered frames<br/>- gp_set_error_callback for errors<br/>- user_data pointer, thread safety | 5 | P0 | VE-338 |
| VE-350 | As a Flutter developer, I want Flutter FFI Dart bindings generation via ffigen so that the C API is callable from Dart | - ffigen config for gopost_ve_api.h<br/>- Generated Dart bindings<br/>- Null safety, async where needed | 5 | P0 | VE-332 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
