#ifndef GOPOST_VIDEO_ENGINE_H
#define GOPOST_VIDEO_ENGINE_H

#include "gopost/types.h"
#include "gopost/error.h"
#include "gopost/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Timeline & track types (S8-02) --- */

typedef struct GopostTimeline GopostTimeline;

typedef enum {
    GOPOST_TRACK_VIDEO  = 0,
    GOPOST_TRACK_AUDIO  = 1,
    GOPOST_TRACK_TITLE  = 2,
    GOPOST_TRACK_EFFECT = 3,
    GOPOST_TRACK_SUBTITLE = 4,
} GopostTrackType;

typedef enum {
    GOPOST_CLIP_SOURCE_VIDEO = 0,
    GOPOST_CLIP_SOURCE_IMAGE = 1,
    GOPOST_CLIP_SOURCE_TITLE = 2,
    GOPOST_CLIP_SOURCE_COLOR = 3,
} GopostClipSourceType;

/** Timeline config: frame rate, dimensions, color space. */
typedef struct {
    double frame_rate;       /* e.g. 30.0, 29.97 */
    int32_t width;
    int32_t height;
    int32_t color_space;     /* 0 = sRGB, 1 = Display P3, 2 = Adobe RGB */
} GopostTimelineConfig;

/** In/out range on timeline (in seconds). */
typedef struct {
    double in_time;   /* start on timeline */
    double out_time;  /* end on timeline */
} GopostTimelineRange;

/** Source in/out (which part of source media is used). */
typedef struct {
    double source_in;   /* start in source (seconds) */
    double source_out;  /* end in source (seconds) */
} GopostSourceRange;

/** Clip descriptor for add_clip. */
typedef struct {
    int32_t track_index;           /* index of track */
    GopostClipSourceType source_type;
    char source_path[1024];        /* file path (video/image) or empty for title/color */
    GopostTimelineRange timeline_range;
    GopostSourceRange source_range;
    double speed;                  /* 1.0 = normal */
    float opacity;                 /* 0.0 - 1.0 */
    int32_t blend_mode;            /* 0 = Normal, 1 = Multiply, 2 = Screen, 3 = Overlay, 4 = Add */
    uint32_t effect_hash;         /* for cache key; 0 = no effects */
} GopostClipDescriptor;

/** Timeline lifecycle (S8-02, S8-05) --- */

GopostError gopost_timeline_create(
    GopostEngine* engine,
    const GopostTimelineConfig* config,
    GopostTimeline** out_timeline);

void gopost_timeline_destroy(GopostTimeline* timeline);

GopostError gopost_timeline_get_config(
    const GopostTimeline* timeline,
    GopostTimelineConfig* out_config);

/** Total duration in seconds. */
GopostError gopost_timeline_get_duration(
    const GopostTimeline* timeline,
    double* out_duration_seconds);

/** Track operations --- */

GopostError gopost_timeline_add_track(
    GopostTimeline* timeline,
    GopostTrackType type,
    int32_t* out_track_index);

GopostError gopost_timeline_remove_track(
    GopostTimeline* timeline,
    int32_t track_index);

GopostError gopost_timeline_get_track_count(
    const GopostTimeline* timeline,
    int32_t* out_count);

/** Clip operations --- */

GopostError gopost_timeline_add_clip(
    GopostTimeline* timeline,
    const GopostClipDescriptor* descriptor,
    int32_t* out_clip_id);

GopostError gopost_timeline_remove_clip(
    GopostTimeline* timeline,
    int32_t clip_id);

GopostError gopost_timeline_trim_clip(
    GopostTimeline* timeline,
    int32_t clip_id,
    const GopostTimelineRange* new_range,
    const GopostSourceRange* new_source);

GopostError gopost_timeline_move_clip(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t new_track_index,
    double new_in_time);

/** Split clip at split_time_seconds (must be between clip in and out). Returns new clip id or negative on failure. */
GopostError gopost_timeline_split_clip(
    GopostTimeline* timeline,
    int32_t clip_id,
    double split_time_seconds,
    int32_t* out_new_clip_id);

/** Ripple delete: remove clips on track in [range_start, range_end] and shift later clips left. */
GopostError gopost_timeline_ripple_delete(
    GopostTimeline* timeline,
    int32_t track_index,
    double range_start_seconds,
    double range_end_seconds);

/** Playback & render (S8-03, S8-04, S8-05) --- */

/** Seek playhead to position (seconds). */
GopostError gopost_timeline_seek(
    GopostTimeline* timeline,
    double position_seconds);

/** Render frame at current playhead into a frame from engine's pool. Caller must release with gopost_frame_release. */
GopostError gopost_timeline_render_frame(
    GopostTimeline* timeline,
    GopostFrame** out_frame);

/** Get current playhead position (seconds). */
GopostError gopost_timeline_get_position(
    const GopostTimeline* timeline,
    double* out_position_seconds);

/** Frame cache control (S8-09). Optional: set max cache size in bytes (0 = use engine default). */
GopostError gopost_timeline_set_frame_cache_size_bytes(
    GopostTimeline* timeline,
    size_t max_bytes);

GopostError gopost_timeline_invalidate_frame_cache(GopostTimeline* timeline);

/* --- Media probe (S8-01) --- */

typedef struct {
    double duration_seconds;
    int32_t width;
    int32_t height;
    double frame_rate;
    int64_t frame_count;
    int32_t has_audio;       /* 0 = no, 1 = yes */
    int32_t audio_sample_rate;
    int32_t audio_channels;
    double audio_duration_seconds;
} GopostMediaInfo;

/** Probe a media file for duration, dimensions, frame rate, and audio info. */
GopostError gopost_media_probe(
    const char* path,
    GopostMediaInfo* out_info);

/* --- Audio (S8-10) --- */

/** Set per-clip volume (0.0 = mute, 1.0 = normal, 2.0 = max boost). */
GopostError gopost_timeline_set_clip_volume(
    GopostTimeline* timeline,
    int32_t clip_id,
    float volume);

GopostError gopost_timeline_get_clip_volume(
    GopostTimeline* timeline,
    int32_t clip_id,
    float* out_volume);

/**
 * Render mixed audio at current playhead into interleaved float32 PCM buffer.
 * `buffer` must hold at least num_frames * num_channels floats.
 * Returns GOPOST_OK on success.
 */
GopostError gopost_timeline_render_audio(
    GopostTimeline* timeline,
    float* buffer,
    int32_t num_frames,
    int32_t num_channels,
    int32_t sample_rate);

/* --- Transitions (S10-03) --- */

GopostError gopost_timeline_set_clip_transition_in(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t transition_type,
    double duration_seconds,
    int32_t easing_curve);

GopostError gopost_timeline_set_clip_transition_out(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t transition_type,
    double duration_seconds,
    int32_t easing_curve);

/* --- Keyframes (S10-04) --- */

GopostError gopost_timeline_set_clip_keyframe(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t property,
    double time,
    double value,
    int32_t interpolation);

GopostError gopost_timeline_remove_clip_keyframe(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t property,
    double time);

GopostError gopost_timeline_clear_clip_keyframes(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t property);

/* --- Effects (S10-01, S10-02) --- */

/** Set color grading parameters for a clip (10 floats). */
GopostError gopost_timeline_set_clip_color_grading(
    GopostTimeline* timeline,
    int32_t clip_id,
    float brightness, float contrast, float saturation, float exposure,
    float temperature, float tint, float highlights, float shadows,
    float vibrance, float hue);

/** Clear all effects and color grading from a clip. */
GopostError gopost_timeline_clear_clip_effects(
    GopostTimeline* timeline,
    int32_t clip_id);

/* --- Audio (S10-05 enhancements) --- */

GopostError gopost_timeline_set_clip_pan(
    GopostTimeline* timeline, int32_t clip_id, float pan);

GopostError gopost_timeline_set_clip_fade_in(
    GopostTimeline* timeline, int32_t clip_id, float seconds);

GopostError gopost_timeline_set_clip_fade_out(
    GopostTimeline* timeline, int32_t clip_id, float seconds);

GopostError gopost_timeline_set_track_volume(
    GopostTimeline* timeline, int32_t track_index, float volume);

GopostError gopost_timeline_set_track_pan(
    GopostTimeline* timeline, int32_t track_index, float pan);

GopostError gopost_timeline_set_track_mute(
    GopostTimeline* timeline, int32_t track_index, int32_t mute);

GopostError gopost_timeline_set_track_solo(
    GopostTimeline* timeline, int32_t track_index, int32_t solo);

/* --------------- Export pipeline (S11) --------------- */

/// Start an async video export. Returns a job ID (≥0) or negative on failure.
int32_t gopost_timeline_start_export(
    GopostTimeline* timeline,
    int32_t width, int32_t height,
    double frame_rate,
    int32_t video_codec, int32_t video_bitrate_bps,
    int32_t audio_codec, int32_t audio_bitrate_kbps,
    int32_t container_format,
    const char* output_path);

/// Get progress of an export job (0.0 – 1.0). Returns -1 if unknown job.
double gopost_export_get_progress(int32_t job_id);

/// Cancel a running export job.
GopostError gopost_export_cancel(int32_t job_id);

/* =========================================================================
   Phase 2: NLE Edit Operations
   ========================================================================= */

GopostError gopost_timeline_insert_edit(
    GopostTimeline* timeline,
    int32_t track_index,
    double at_time,
    const GopostClipDescriptor* descriptor,
    int32_t* out_clip_id);

GopostError gopost_timeline_overwrite_edit(
    GopostTimeline* timeline,
    int32_t track_index,
    double at_time,
    const GopostClipDescriptor* descriptor,
    int32_t* out_clip_id);

GopostError gopost_timeline_roll_edit(
    GopostTimeline* timeline, int32_t clip_id, double delta_sec);

GopostError gopost_timeline_slip_edit(
    GopostTimeline* timeline, int32_t clip_id, double delta_sec);

GopostError gopost_timeline_slide_edit(
    GopostTimeline* timeline, int32_t clip_id, double delta_sec);

GopostError gopost_timeline_rate_stretch(
    GopostTimeline* timeline, int32_t clip_id, double new_duration_sec);

GopostError gopost_timeline_duplicate_clip(
    GopostTimeline* timeline, int32_t clip_id, int32_t* out_new_clip_id);

GopostError gopost_timeline_get_snap_points(
    GopostTimeline* timeline,
    double time_sec, double threshold_sec,
    double* out_points, int32_t max_points, int32_t* out_count);

GopostError gopost_timeline_reorder_tracks(
    GopostTimeline* timeline,
    const int32_t* new_order, int32_t count);

/* =========================================================================
   Phase 3: Effect DAG & Registry (video clip effects)
   ========================================================================= */

GopostError gopost_timeline_add_clip_effect(
    GopostTimeline* timeline, int32_t clip_id,
    const char* effect_def_id, int32_t* out_instance_id);

GopostError gopost_timeline_remove_clip_effect(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id);

GopostError gopost_timeline_set_clip_effect_param(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id,
    const char* param_id, float value);

GopostError gopost_timeline_set_clip_effect_enabled(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id,
    int32_t enabled);

GopostError gopost_timeline_set_clip_effect_mix(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id,
    float mix);

/* =========================================================================
   Phase 4: Masking & Tracking
   ========================================================================= */

typedef struct {
    int32_t type;       /* 0=rect, 1=ellipse, 2=bezier, 3=freehand */
    float feather;
    float opacity;
    int32_t inverted;
    float expansion;
    int32_t point_count;
    /* points follow in a separate array */
} GopostMaskDesc;

typedef struct {
    float x, y;
    float handle_in_x, handle_in_y;
    float handle_out_x, handle_out_y;
} GopostMaskPoint;

GopostError gopost_timeline_add_clip_mask(
    GopostTimeline* timeline, int32_t clip_id,
    const GopostMaskDesc* mask, const GopostMaskPoint* points,
    int32_t* out_mask_id);

GopostError gopost_timeline_update_clip_mask(
    GopostTimeline* timeline, int32_t clip_id, int32_t mask_id,
    const GopostMaskDesc* mask, const GopostMaskPoint* points);

GopostError gopost_timeline_remove_clip_mask(
    GopostTimeline* timeline, int32_t clip_id, int32_t mask_id);

GopostError gopost_timeline_start_tracking(
    GopostTimeline* timeline, int32_t clip_id,
    float x, float y, double time_sec,
    int32_t* out_tracker_id);

GopostError gopost_timeline_stabilize_clip(
    GopostTimeline* timeline, int32_t clip_id,
    int32_t method, float smoothness, int32_t crop_to_stable);

/* =========================================================================
   Phase 5: Text Layers, Shapes, Audio Effects
   ========================================================================= */

typedef struct {
    char text[512];
    char font_family[128];
    char font_style[64];
    float font_size;
    uint32_t fill_color;
    int32_t fill_enabled;
    uint32_t stroke_color;
    float stroke_width;
    int32_t stroke_enabled;
    int32_t alignment;
    float tracking;
    float leading;
    float position_x, position_y;
    float rotation;
    float scale_x, scale_y;
} GopostTextLayerDesc;

GopostError gopost_timeline_set_clip_text(
    GopostTimeline* timeline, int32_t clip_id,
    const GopostTextLayerDesc* text_data);

typedef struct {
    int32_t type;       /* ShapeType enum */
    float x, y, width, height;
    float rotation;
    uint32_t fill_color;
    int32_t fill_enabled;
    uint32_t stroke_color;
    float stroke_width;
    int32_t stroke_enabled;
    float corner_radius;
    int32_t sides;
    float inner_radius;
} GopostShapeDesc;

GopostError gopost_timeline_add_clip_shape(
    GopostTimeline* timeline, int32_t clip_id,
    const GopostShapeDesc* shape, int32_t* out_shape_id);

GopostError gopost_timeline_update_clip_shape(
    GopostTimeline* timeline, int32_t clip_id, int32_t shape_id,
    const GopostShapeDesc* shape);

GopostError gopost_timeline_remove_clip_shape(
    GopostTimeline* timeline, int32_t clip_id, int32_t shape_id);

GopostError gopost_timeline_add_audio_effect(
    GopostTimeline* timeline, int32_t clip_id,
    const char* effect_def_id, int32_t* out_instance_id);

GopostError gopost_timeline_set_audio_effect_param(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id,
    const char* param_id, float value);

GopostError gopost_timeline_remove_audio_effect(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id);

/* =========================================================================
   Phase 6: AI, Proxy, Multi-Cam
   ========================================================================= */

GopostError gopost_timeline_start_ai_segmentation(
    GopostTimeline* timeline, int32_t clip_id,
    int32_t seg_type, float edge_feather, int32_t refine_edges,
    int32_t* out_job_id);

double gopost_ai_segmentation_get_progress(int32_t job_id);

GopostError gopost_ai_segmentation_cancel(int32_t job_id);

GopostError gopost_timeline_enable_proxy_mode(
    GopostTimeline* timeline,
    int32_t resolution, int32_t video_codec, int32_t bitrate_bps);

GopostError gopost_timeline_disable_proxy_mode(GopostTimeline* timeline);

GopostError gopost_timeline_is_proxy_active(
    const GopostTimeline* timeline, int32_t* out_active);

typedef struct {
    char name[128];
    char source_path[1024];
    double sync_offset;
} GopostCameraAngle;

GopostError gopost_timeline_create_multicam_clip(
    GopostTimeline* timeline, int32_t track_index,
    const char* name,
    const GopostCameraAngle* angles, int32_t angle_count,
    double duration_sec,
    int32_t* out_clip_id);

GopostError gopost_timeline_switch_multicam_angle(
    GopostTimeline* timeline, int32_t clip_id,
    int32_t angle_index, double at_time_sec);

GopostError gopost_timeline_flatten_multicam(
    GopostTimeline* timeline, int32_t clip_id);

/* =========================================================================
   Phase 7: Extended Clip Engine — multi-clip, collision, sync-lock
   ========================================================================= */

/** Move multiple clips simultaneously. */
GopostError gopost_timeline_move_multiple_clips(
    GopostTimeline* timeline,
    const int32_t* clip_ids, int32_t count,
    double delta_time, int32_t delta_track);

/** Swap two clips on the timeline. */
GopostError gopost_timeline_swap_clips(
    GopostTimeline* timeline,
    int32_t clip_id_a, int32_t clip_id_b);

/** Split all tracks at the given time. Returns count of new clips created. */
GopostError gopost_timeline_split_all_tracks(
    GopostTimeline* timeline,
    double split_time_seconds,
    int32_t* out_new_clip_count);

/** Lift delete: remove clips in range without closing gap. */
GopostError gopost_timeline_lift_delete(
    GopostTimeline* timeline,
    int32_t track_index,
    double range_start_seconds,
    double range_end_seconds);

/** Collision detection: check if a region on a track overlaps existing clips.
 *  out_result: 0 = CLEAR, 1 = OVERLAP, 2 = ADJACENT. */
GopostError gopost_timeline_check_overlap(
    GopostTimeline* timeline,
    int32_t track_index,
    double in_time, double out_time,
    int32_t exclude_clip_id,
    int32_t* out_result);

/** Get IDs of clips overlapping a region on a track. */
GopostError gopost_timeline_get_overlapping_clips(
    GopostTimeline* timeline,
    int32_t track_index,
    double in_time, double out_time,
    int32_t* out_clip_ids, int32_t max_ids,
    int32_t* out_count);

/** Set sync-lock on a track. When enabled, moving clips on one track
 *  ripples all sync-locked tracks. */
GopostError gopost_timeline_set_track_sync_lock(
    GopostTimeline* timeline,
    int32_t track_index,
    int32_t locked);

/** Set track height (persisted per project). */
GopostError gopost_timeline_set_track_height(
    GopostTimeline* timeline,
    int32_t track_index,
    float height_px);

/** Get track height. */
GopostError gopost_timeline_get_track_height(
    const GopostTimeline* timeline,
    int32_t track_index,
    float* out_height_px);

#ifdef __cplusplus
}
#endif

#endif /* GOPOST_VIDEO_ENGINE_H */
