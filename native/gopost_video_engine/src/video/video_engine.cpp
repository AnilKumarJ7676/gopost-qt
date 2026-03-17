#include "gopost/video_engine.h"
#include "gopost/engine.h"
#include "core/frame_pool.h"
#include "timeline_model.hpp"
#include "frame_cache.hpp"
#include "timeline_evaluator.hpp"
#include "video_compositor.hpp"
#include "media_probe.hpp"
#include "audio_mixer.hpp"
#include "decoder_pool.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

static constexpr size_t kDefaultFrameCacheBytes = 64 * 1024 * 1024;  /* 64 MB */

struct GopostTimeline {
    GopostEngine* engine = nullptr;
    std::unique_ptr<gopost::video::TimelineModel> model;
    std::unique_ptr<gopost::video::FrameCache> frame_cache;
    std::unique_ptr<gopost::video::TimelineEvaluator> evaluator;
    std::unique_ptr<gopost::video::AudioMixer> audio_mixer;
    gopost::video::DecoderPool* decoder_pool = nullptr;  // Non-owning; set externally
    size_t frame_cache_max_bytes = kDefaultFrameCacheBytes;
};

namespace gopost {
namespace video {

static TrackType to_track_type(GopostTrackType t) {
    switch (t) {
        case GOPOST_TRACK_AUDIO:  return TrackType::Audio;
        case GOPOST_TRACK_TITLE:  return TrackType::Title;
        case GOPOST_TRACK_EFFECT: return TrackType::Effect;
        case GOPOST_TRACK_SUBTITLE: return TrackType::Subtitle;
        default: return TrackType::Video;
    }
}

static ClipSourceType to_clip_source_type(GopostClipSourceType t) {
    switch (t) {
        case GOPOST_CLIP_SOURCE_IMAGE: return ClipSourceType::Image;
        case GOPOST_CLIP_SOURCE_TITLE:  return ClipSourceType::Title;
        case GOPOST_CLIP_SOURCE_COLOR:  return ClipSourceType::Color;
        default: return ClipSourceType::Video;
    }
}

}  // namespace video
}  // namespace gopost

extern "C" {

GopostError gopost_timeline_create(
    GopostEngine* engine,
    const GopostTimelineConfig* config,
    GopostTimeline** out_timeline) {
    if (!engine || !config || !out_timeline) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (!gopost_engine_get_frame_pool(engine)) {
        return GOPOST_ERROR_NOT_INITIALIZED;
    }

    gopost::video::TimelineConfig cfg;
    cfg.frame_rate = config->frame_rate > 0 ? config->frame_rate : 30.0;
    cfg.width = config->width > 0 ? config->width : 1920;
    cfg.height = config->height > 0 ? config->height : 1080;
    cfg.color_space = config->color_space;

    auto* tl = new (std::nothrow) GopostTimeline{};
    if (!tl) return GOPOST_ERROR_OUT_OF_MEMORY;
    tl->engine = engine;
    tl->model = std::make_unique<gopost::video::TimelineModel>(cfg);

    auto acquire_fn = [engine](uint32_t w, uint32_t h, GopostPixelFormat fmt) -> GopostFrame* {
        GopostFrame* f = nullptr;
        if (gopost_frame_acquire(engine, &f, w, h, fmt) != GOPOST_OK) return nullptr;
        return f;
    };
    auto release_fn = [engine](GopostFrame* f) {
        if (f) gopost_frame_release(engine, f);
    };
    tl->frame_cache = std::make_unique<gopost::video::FrameCache>(
        kDefaultFrameCacheBytes, acquire_fn, release_fn);
    tl->evaluator = std::make_unique<gopost::video::TimelineEvaluator>(
        engine, tl->model.get(), tl->frame_cache.get());
    tl->audio_mixer = std::make_unique<gopost::video::AudioMixer>();

    *out_timeline = tl;
    return GOPOST_OK;
}

void gopost_timeline_destroy(GopostTimeline* timeline) {
    if (!timeline) return;
    timeline->evaluator.reset();
    timeline->frame_cache->invalidate_all();
    timeline->frame_cache.reset();
    timeline->model.reset();
    delete timeline;
}

GopostError gopost_timeline_get_config(
    const GopostTimeline* timeline,
    GopostTimelineConfig* out_config) {
    if (!timeline || !out_config) return GOPOST_ERROR_INVALID_ARGUMENT;
    const auto& c = timeline->model->config();
    out_config->frame_rate = c.frame_rate;
    out_config->width = c.width;
    out_config->height = c.height;
    out_config->color_space = c.color_space;
    return GOPOST_OK;
}

GopostError gopost_timeline_get_duration(
    const GopostTimeline* timeline,
    double* out_duration_seconds) {
    if (!timeline || !out_duration_seconds) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_duration_seconds = timeline->model->duration_seconds();
    return GOPOST_OK;
}

GopostError gopost_timeline_add_track(
    GopostTimeline* timeline,
    GopostTrackType type,
    int32_t* out_track_index) {
    if (!timeline || !out_track_index) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_track_index = timeline->model->add_track(gopost::video::to_track_type(type));
    return GOPOST_OK;
}

GopostError gopost_timeline_remove_track(
    GopostTimeline* timeline,
    int32_t track_index) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->remove_track(track_index)) return GOPOST_ERROR_INVALID_ARGUMENT;
    return GOPOST_OK;
}

GopostError gopost_timeline_get_track_count(
    const GopostTimeline* timeline,
    int32_t* out_count) {
    if (!timeline || !out_count) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_count = timeline->model->track_count();
    return GOPOST_OK;
}

GopostError gopost_timeline_add_clip(
    GopostTimeline* timeline,
    const GopostClipDescriptor* descriptor,
    int32_t* out_clip_id) {
    if (!timeline || !descriptor || !out_clip_id) return GOPOST_ERROR_INVALID_ARGUMENT;
    gopost::video::TimeRange tr;
    tr.in_time = descriptor->timeline_range.in_time;
    tr.out_time = descriptor->timeline_range.out_time;
    gopost::video::SourceRange sr;
    sr.source_in = descriptor->source_range.source_in;
    sr.source_out = descriptor->source_range.source_out;
    std::string path(descriptor->source_path, strnlen(descriptor->source_path, sizeof(descriptor->source_path)));
    int32_t id = timeline->model->add_clip(
        descriptor->track_index,
        gopost::video::to_clip_source_type(descriptor->source_type),
        path,
        tr, sr,
        descriptor->speed > 0 ? descriptor->speed : 1.0,
        descriptor->opacity,
        descriptor->blend_mode,
        descriptor->effect_hash);
    if (id < 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_clip_id = id;
    return GOPOST_OK;
}

GopostError gopost_timeline_remove_clip(
    GopostTimeline* timeline,
    int32_t clip_id) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->remove_clip(clip_id)) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_trim_clip(
    GopostTimeline* timeline,
    int32_t clip_id,
    const GopostTimelineRange* new_range,
    const GopostSourceRange* new_source) {
    if (!timeline || !new_range || !new_source) return GOPOST_ERROR_INVALID_ARGUMENT;
    gopost::video::TimeRange tr;
    tr.in_time = new_range->in_time;
    tr.out_time = new_range->out_time;
    gopost::video::SourceRange sr;
    sr.source_in = new_source->source_in;
    sr.source_out = new_source->source_out;
    if (!timeline->model->trim_clip(clip_id, tr, sr)) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_move_clip(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t new_track_index,
    double new_in_time) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->move_clip(clip_id, new_track_index, new_in_time)) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_split_clip(
    GopostTimeline* timeline,
    int32_t clip_id,
    double split_time_seconds,
    int32_t* out_new_clip_id) {
    if (!timeline || !out_new_clip_id) return GOPOST_ERROR_INVALID_ARGUMENT;
    int32_t new_id = timeline->model->split_clip(clip_id, split_time_seconds);
    if (new_id < 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_new_clip_id = new_id;
    timeline->frame_cache->invalidate_clip(clip_id);
    timeline->frame_cache->invalidate_clip(new_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_ripple_delete(
    GopostTimeline* timeline,
    int32_t track_index,
    double range_start_seconds,
    double range_end_seconds) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->ripple_delete(track_index, range_start_seconds, range_end_seconds)) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    timeline->frame_cache->invalidate_all();
    return GOPOST_OK;
}

GopostError gopost_timeline_seek(
    GopostTimeline* timeline,
    double position_seconds) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->model->set_position_seconds(position_seconds);
    return GOPOST_OK;
}

GopostError gopost_timeline_render_frame(
    GopostTimeline* timeline,
    GopostFrame** out_frame) {
    if (!timeline || !out_frame) return GOPOST_ERROR_INVALID_ARGUMENT;
    int err = timeline->evaluator->render_frame(out_frame);
    return static_cast<GopostError>(err);
}

GopostError gopost_timeline_get_position(
    const GopostTimeline* timeline,
    double* out_position_seconds) {
    if (!timeline || !out_position_seconds) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_position_seconds = timeline->model->position_seconds();
    return GOPOST_OK;
}

GopostError gopost_timeline_set_frame_cache_size_bytes(
    GopostTimeline* timeline,
    size_t max_bytes) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache_max_bytes = max_bytes > 0 ? max_bytes : kDefaultFrameCacheBytes;
    timeline->frame_cache->set_max_bytes(timeline->frame_cache_max_bytes);
    return GOPOST_OK;
}

GopostError gopost_timeline_invalidate_frame_cache(GopostTimeline* timeline) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_all();
    return GOPOST_OK;
}

/* --- Media probe (S8-01) --- */

GopostError gopost_media_probe(
    const char* path,
    GopostMediaInfo* out_info) {
    if (!path || !out_info) return GOPOST_ERROR_INVALID_ARGUMENT;

    gopost::video::MediaProbeInfo info;
    if (!gopost::video::probe_media(std::string(path), info)) {
        return GOPOST_ERROR_IO;
    }

    out_info->duration_seconds = info.duration_seconds;
    out_info->width = info.width;
    out_info->height = info.height;
    out_info->frame_rate = info.frame_rate;
    out_info->frame_count = info.frame_count;
    out_info->has_audio = info.has_audio ? 1 : 0;
    out_info->audio_sample_rate = info.audio_sample_rate;
    out_info->audio_channels = info.audio_channels;
    out_info->audio_duration_seconds = info.audio_duration_seconds;
    return GOPOST_OK;
}

/* --- Audio (S8-10) --- */

GopostError gopost_timeline_set_clip_volume(
    GopostTimeline* timeline,
    int32_t clip_id,
    float volume) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->audio_mixer->set_clip_volume(clip_id, volume);
    return GOPOST_OK;
}

GopostError gopost_timeline_get_clip_volume(
    GopostTimeline* timeline,
    int32_t clip_id,
    float* out_volume) {
    if (!timeline || !out_volume) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_volume = timeline->audio_mixer->get_clip_volume(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_render_audio(
    GopostTimeline* timeline,
    float* buffer,
    int32_t num_frames,
    int32_t num_channels,
    int32_t sample_rate) {
    if (!timeline || !buffer || num_frames <= 0 || num_channels <= 0 || sample_rate <= 0) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    timeline->audio_mixer->render_audio(
        timeline->model.get(),
        timeline->model->position_seconds(),
        buffer, num_frames, num_channels, sample_rate);
    return GOPOST_OK;
}

/* --- Transitions (S10-03) --- */

GopostError gopost_timeline_set_clip_transition_in(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t transition_type,
    double duration_seconds,
    int32_t easing_curve) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto* clip = timeline->model->find_clip(clip_id);
    if (!clip) return GOPOST_ERROR_INVALID_ARGUMENT;
    clip->transition_in.type = static_cast<gopost::video::TransitionType>(transition_type);
    clip->transition_in.duration_seconds = duration_seconds;
    clip->transition_in.easing = static_cast<gopost::video::EasingCurve>(easing_curve);
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_set_clip_transition_out(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t transition_type,
    double duration_seconds,
    int32_t easing_curve) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto* clip = timeline->model->find_clip(clip_id);
    if (!clip) return GOPOST_ERROR_INVALID_ARGUMENT;
    clip->transition_out.type = static_cast<gopost::video::TransitionType>(transition_type);
    clip->transition_out.duration_seconds = duration_seconds;
    clip->transition_out.easing = static_cast<gopost::video::EasingCurve>(easing_curve);
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

/* --- Keyframes (S10-04) --- */

GopostError gopost_timeline_set_clip_keyframe(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t property,
    double time,
    double value,
    int32_t interpolation) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto* clip = timeline->model->find_clip(clip_id);
    if (!clip) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto prop = static_cast<gopost::video::KeyframeProperty>(property);
    auto interp = static_cast<gopost::video::KeyframeInterpolation>(interpolation);
    gopost::video::Keyframe kf{time, value, interp};
    // Find or create track
    gopost::video::KeyframeTrack* target_track = nullptr;
    for (auto& t : clip->keyframes.tracks) {
        if (t.property == prop) { target_track = &t; break; }
    }
    if (!target_track) {
        clip->keyframes.tracks.push_back({prop, {}});
        target_track = &clip->keyframes.tracks.back();
    }
    // Upsert keyframe at time
    bool replaced = false;
    for (auto& k : target_track->keyframes) {
        if (std::abs(k.time - time) < 1e-6) { k = kf; replaced = true; break; }
    }
    if (!replaced) {
        target_track->keyframes.push_back(kf);
        std::sort(target_track->keyframes.begin(), target_track->keyframes.end(),
            [](const auto& a, const auto& b) { return a.time < b.time; });
    }
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_remove_clip_keyframe(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t property,
    double time) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto* clip = timeline->model->find_clip(clip_id);
    if (!clip) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto prop = static_cast<gopost::video::KeyframeProperty>(property);
    for (auto& t : clip->keyframes.tracks) {
        if (t.property == prop) {
            t.keyframes.erase(
                std::remove_if(t.keyframes.begin(), t.keyframes.end(),
                    [time](const auto& k) { return std::abs(k.time - time) < 1e-6; }),
                t.keyframes.end());
            break;
        }
    }
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_clear_clip_keyframes(
    GopostTimeline* timeline,
    int32_t clip_id,
    int32_t property) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto* clip = timeline->model->find_clip(clip_id);
    if (!clip) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto prop = static_cast<gopost::video::KeyframeProperty>(property);
    clip->keyframes.tracks.erase(
        std::remove_if(clip->keyframes.tracks.begin(), clip->keyframes.tracks.end(),
            [prop](const auto& t) { return t.property == prop; }),
        clip->keyframes.tracks.end());
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

/* --- Effects (S10-01, S10-02) --- */

GopostError gopost_timeline_set_clip_color_grading(
    GopostTimeline* timeline,
    int32_t clip_id,
    float brightness, float contrast, float saturation, float exposure,
    float temperature, float tint, float highlights, float shadows,
    float vibrance, float hue) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto* clip = timeline->model->find_clip(clip_id);
    if (!clip) return GOPOST_ERROR_INVALID_ARGUMENT;
    clip->effects.color_grading.brightness = brightness;
    clip->effects.color_grading.contrast = contrast;
    clip->effects.color_grading.saturation = saturation;
    clip->effects.color_grading.exposure = exposure;
    clip->effects.color_grading.temperature = temperature;
    clip->effects.color_grading.tint = tint;
    clip->effects.color_grading.highlights = highlights;
    clip->effects.color_grading.shadows = shadows;
    clip->effects.color_grading.vibrance = vibrance;
    clip->effects.color_grading.hue = hue;
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_clear_clip_effects(
    GopostTimeline* timeline,
    int32_t clip_id) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto* clip = timeline->model->find_clip(clip_id);
    if (!clip) return GOPOST_ERROR_INVALID_ARGUMENT;
    clip->effects = gopost::video::ClipEffectState{};
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

/* --- Audio (S10-05 enhancements) --- */

GopostError gopost_timeline_set_clip_pan(
    GopostTimeline* timeline, int32_t clip_id, float pan) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->audio_mixer->set_clip_pan(clip_id, pan);
    return GOPOST_OK;
}

GopostError gopost_timeline_set_clip_fade_in(
    GopostTimeline* timeline, int32_t clip_id, float seconds) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->audio_mixer->set_clip_fade_in(clip_id, seconds);
    return GOPOST_OK;
}

GopostError gopost_timeline_set_clip_fade_out(
    GopostTimeline* timeline, int32_t clip_id, float seconds) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->audio_mixer->set_clip_fade_out(clip_id, seconds);
    return GOPOST_OK;
}

GopostError gopost_timeline_set_track_volume(
    GopostTimeline* timeline, int32_t track_index, float volume) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->audio_mixer->set_track_volume(track_index, volume);
    return GOPOST_OK;
}

GopostError gopost_timeline_set_track_pan(
    GopostTimeline* timeline, int32_t track_index, float pan) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->audio_mixer->set_track_pan(track_index, pan);
    return GOPOST_OK;
}

GopostError gopost_timeline_set_track_mute(
    GopostTimeline* timeline, int32_t track_index, int32_t mute) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->audio_mixer->set_track_mute(track_index, mute != 0);
    return GOPOST_OK;
}

GopostError gopost_timeline_set_track_solo(
    GopostTimeline* timeline, int32_t track_index, int32_t solo) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->audio_mixer->set_track_solo(track_index, solo != 0);
    return GOPOST_OK;
}

/* =========================================================================
   Phase 2: NLE Edit Operations
   ========================================================================= */

GopostError gopost_timeline_insert_edit(
    GopostTimeline* timeline,
    int32_t track_index,
    double at_time,
    const GopostClipDescriptor* descriptor,
    int32_t* out_clip_id) {
    if (!timeline || !descriptor || !out_clip_id) return GOPOST_ERROR_INVALID_ARGUMENT;
    gopost::video::TimeRange tr;
    tr.in_time = descriptor->timeline_range.in_time;
    tr.out_time = descriptor->timeline_range.out_time;
    gopost::video::SourceRange sr;
    sr.source_in = descriptor->source_range.source_in;
    sr.source_out = descriptor->source_range.source_out;
    std::string path(descriptor->source_path, strnlen(descriptor->source_path, sizeof(descriptor->source_path)));
    int32_t id = timeline->model->insert_edit(
        track_index, at_time,
        gopost::video::to_clip_source_type(descriptor->source_type),
        path, tr, sr,
        descriptor->speed > 0 ? descriptor->speed : 1.0,
        descriptor->opacity,
        descriptor->blend_mode,
        descriptor->effect_hash);
    if (id < 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_clip_id = id;
    return GOPOST_OK;
}

GopostError gopost_timeline_overwrite_edit(
    GopostTimeline* timeline,
    int32_t track_index,
    double at_time,
    const GopostClipDescriptor* descriptor,
    int32_t* out_clip_id) {
    if (!timeline || !descriptor || !out_clip_id) return GOPOST_ERROR_INVALID_ARGUMENT;
    gopost::video::TimeRange tr;
    tr.in_time = descriptor->timeline_range.in_time;
    tr.out_time = descriptor->timeline_range.out_time;
    gopost::video::SourceRange sr;
    sr.source_in = descriptor->source_range.source_in;
    sr.source_out = descriptor->source_range.source_out;
    std::string path(descriptor->source_path, strnlen(descriptor->source_path, sizeof(descriptor->source_path)));
    int32_t id = timeline->model->overwrite_edit(
        track_index, at_time,
        gopost::video::to_clip_source_type(descriptor->source_type),
        path, tr, sr,
        descriptor->speed > 0 ? descriptor->speed : 1.0,
        descriptor->opacity,
        descriptor->blend_mode,
        descriptor->effect_hash);
    if (id < 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_clip_id = id;
    return GOPOST_OK;
}

GopostError gopost_timeline_roll_edit(
    GopostTimeline* timeline, int32_t clip_id, double delta_sec) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->roll_edit(clip_id, delta_sec)) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_slip_edit(
    GopostTimeline* timeline, int32_t clip_id, double delta_sec) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->slip_edit(clip_id, delta_sec)) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_slide_edit(
    GopostTimeline* timeline, int32_t clip_id, double delta_sec) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->slide_edit(clip_id, delta_sec)) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_all();
    return GOPOST_OK;
}

GopostError gopost_timeline_rate_stretch(
    GopostTimeline* timeline, int32_t clip_id, double new_duration_sec) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->rate_stretch(clip_id, new_duration_sec)) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_clip(clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_duplicate_clip(
    GopostTimeline* timeline, int32_t clip_id, int32_t* out_new_clip_id) {
    if (!timeline || !out_new_clip_id) return GOPOST_ERROR_INVALID_ARGUMENT;
    int32_t new_id = timeline->model->duplicate_clip(clip_id);
    if (new_id < 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_new_clip_id = new_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_get_snap_points(
    GopostTimeline* timeline,
    double time_sec, double threshold_sec,
    double* out_points, int32_t max_points, int32_t* out_count) {
    if (!timeline || !out_points || !out_count || max_points < 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::vector<double> points = timeline->model->snap_points(time_sec, threshold_sec);
    *out_count = static_cast<int32_t>(std::min(static_cast<size_t>(max_points), points.size()));
    for (int32_t i = 0; i < *out_count; ++i) {
        out_points[i] = points[static_cast<size_t>(i)];
    }
    return GOPOST_OK;
}

GopostError gopost_timeline_reorder_tracks(
    GopostTimeline* timeline,
    const int32_t* new_order, int32_t count) {
    if (!timeline || !new_order) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::vector<int32_t> order(new_order, new_order + count);
    if (!timeline->model->reorder_tracks(order)) return GOPOST_ERROR_INVALID_ARGUMENT;
    timeline->frame_cache->invalidate_all();
    return GOPOST_OK;
}

/* =========================================================================
   Phase 3: Effect DAG & Registry (stubs)
   ========================================================================= */

GopostError gopost_timeline_add_clip_effect(
    GopostTimeline* timeline, int32_t clip_id,
    const char* effect_def_id, int32_t* out_instance_id) {
    (void)timeline; (void)clip_id; (void)effect_def_id; (void)out_instance_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_remove_clip_effect(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id) {
    (void)timeline; (void)clip_id; (void)instance_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_set_clip_effect_param(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id,
    const char* param_id, float value) {
    (void)timeline; (void)clip_id; (void)instance_id; (void)param_id; (void)value;
    return GOPOST_OK;
}

GopostError gopost_timeline_set_clip_effect_enabled(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id,
    int32_t enabled) {
    (void)timeline; (void)clip_id; (void)instance_id; (void)enabled;
    return GOPOST_OK;
}

GopostError gopost_timeline_set_clip_effect_mix(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id,
    float mix) {
    (void)timeline; (void)clip_id; (void)instance_id; (void)mix;
    return GOPOST_OK;
}

/* =========================================================================
   Phase 4: Masking & Tracking (stubs)
   ========================================================================= */

GopostError gopost_timeline_add_clip_mask(
    GopostTimeline* timeline, int32_t clip_id,
    const GopostMaskDesc* mask, const GopostMaskPoint* points,
    int32_t* out_mask_id) {
    (void)timeline; (void)clip_id; (void)mask; (void)points; (void)out_mask_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_update_clip_mask(
    GopostTimeline* timeline, int32_t clip_id, int32_t mask_id,
    const GopostMaskDesc* mask, const GopostMaskPoint* points) {
    (void)timeline; (void)clip_id; (void)mask_id; (void)mask; (void)points;
    return GOPOST_OK;
}

GopostError gopost_timeline_remove_clip_mask(
    GopostTimeline* timeline, int32_t clip_id, int32_t mask_id) {
    (void)timeline; (void)clip_id; (void)mask_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_start_tracking(
    GopostTimeline* timeline, int32_t clip_id,
    float x, float y, double time_sec,
    int32_t* out_tracker_id) {
    (void)timeline; (void)clip_id; (void)x; (void)y; (void)time_sec; (void)out_tracker_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_stabilize_clip(
    GopostTimeline* timeline, int32_t clip_id,
    int32_t method, float smoothness, int32_t crop_to_stable) {
    (void)timeline; (void)clip_id; (void)method; (void)smoothness; (void)crop_to_stable;
    return GOPOST_OK;
}

/* =========================================================================
   Phase 5: Text Layers, Shapes, Audio Effects (stubs)
   ========================================================================= */

GopostError gopost_timeline_set_clip_text(
    GopostTimeline* timeline, int32_t clip_id,
    const GopostTextLayerDesc* text_data) {
    (void)timeline; (void)clip_id; (void)text_data;
    return GOPOST_OK;
}

GopostError gopost_timeline_add_clip_shape(
    GopostTimeline* timeline, int32_t clip_id,
    const GopostShapeDesc* shape, int32_t* out_shape_id) {
    (void)timeline; (void)clip_id; (void)shape; (void)out_shape_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_update_clip_shape(
    GopostTimeline* timeline, int32_t clip_id, int32_t shape_id,
    const GopostShapeDesc* shape) {
    (void)timeline; (void)clip_id; (void)shape_id; (void)shape;
    return GOPOST_OK;
}

GopostError gopost_timeline_remove_clip_shape(
    GopostTimeline* timeline, int32_t clip_id, int32_t shape_id) {
    (void)timeline; (void)clip_id; (void)shape_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_add_audio_effect(
    GopostTimeline* timeline, int32_t clip_id,
    const char* effect_def_id, int32_t* out_instance_id) {
    (void)timeline; (void)clip_id; (void)effect_def_id; (void)out_instance_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_set_audio_effect_param(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id,
    const char* param_id, float value) {
    (void)timeline; (void)clip_id; (void)instance_id; (void)param_id; (void)value;
    return GOPOST_OK;
}

GopostError gopost_timeline_remove_audio_effect(
    GopostTimeline* timeline, int32_t clip_id, int32_t instance_id) {
    (void)timeline; (void)clip_id; (void)instance_id;
    return GOPOST_OK;
}

/* =========================================================================
   Phase 6: AI, Proxy, Multi-Cam (stubs)
   ========================================================================= */

GopostError gopost_timeline_start_ai_segmentation(
    GopostTimeline* timeline, int32_t clip_id,
    int32_t seg_type, float edge_feather, int32_t refine_edges,
    int32_t* out_job_id) {
    (void)timeline; (void)clip_id; (void)seg_type; (void)edge_feather; (void)refine_edges; (void)out_job_id;
    return GOPOST_OK;
}

double gopost_ai_segmentation_get_progress(int32_t job_id) {
    (void)job_id;
    return 0.0;
}

GopostError gopost_ai_segmentation_cancel(int32_t job_id) {
    (void)job_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_enable_proxy_mode(
    GopostTimeline* timeline,
    int32_t resolution, int32_t video_codec, int32_t bitrate_bps) {
    (void)timeline; (void)resolution; (void)video_codec; (void)bitrate_bps;
    return GOPOST_OK;
}

GopostError gopost_timeline_disable_proxy_mode(GopostTimeline* timeline) {
    (void)timeline;
    return GOPOST_OK;
}

GopostError gopost_timeline_is_proxy_active(
    const GopostTimeline* timeline, int32_t* out_active) {
    if (!timeline || !out_active) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_active = 0;
    return GOPOST_OK;
}

GopostError gopost_timeline_create_multicam_clip(
    GopostTimeline* timeline, int32_t track_index,
    const char* name,
    const GopostCameraAngle* angles, int32_t angle_count,
    double duration_sec,
    int32_t* out_clip_id) {
    (void)timeline; (void)track_index; (void)name; (void)angles; (void)angle_count; (void)duration_sec; (void)out_clip_id;
    return GOPOST_OK;
}

GopostError gopost_timeline_switch_multicam_angle(
    GopostTimeline* timeline, int32_t clip_id,
    int32_t angle_index, double at_time_sec) {
    (void)timeline; (void)clip_id; (void)angle_index; (void)at_time_sec;
    return GOPOST_OK;
}

GopostError gopost_timeline_flatten_multicam(
    GopostTimeline* timeline, int32_t clip_id) {
    (void)timeline; (void)clip_id;
    return GOPOST_OK;
}

/* =========================================================================
   Phase 7: Extended Clip Engine
   ========================================================================= */

GopostError gopost_timeline_move_multiple_clips(
    GopostTimeline* timeline,
    const int32_t* clip_ids, int32_t count,
    double delta_time, int32_t delta_track) {
    if (!timeline || !clip_ids || count <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::vector<int32_t> ids(clip_ids, clip_ids + count);
    if (!timeline->model->move_multiple_clips(ids, delta_time, delta_track)) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    timeline->frame_cache->invalidate_all();
    return GOPOST_OK;
}

GopostError gopost_timeline_swap_clips(
    GopostTimeline* timeline,
    int32_t clip_id_a, int32_t clip_id_b) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->swap_clips(clip_id_a, clip_id_b)) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    timeline->frame_cache->invalidate_all();
    return GOPOST_OK;
}

GopostError gopost_timeline_split_all_tracks(
    GopostTimeline* timeline,
    double split_time_seconds,
    int32_t* out_new_clip_count) {
    if (!timeline || !out_new_clip_count) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_new_clip_count = timeline->model->split_all_tracks(split_time_seconds);
    timeline->frame_cache->invalidate_all();
    return GOPOST_OK;
}

GopostError gopost_timeline_lift_delete(
    GopostTimeline* timeline,
    int32_t track_index,
    double range_start_seconds,
    double range_end_seconds) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->lift_delete(track_index, range_start_seconds, range_end_seconds)) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    timeline->frame_cache->invalidate_all();
    return GOPOST_OK;
}

GopostError gopost_timeline_check_overlap(
    GopostTimeline* timeline,
    int32_t track_index,
    double in_time, double out_time,
    int32_t exclude_clip_id,
    int32_t* out_result) {
    if (!timeline || !out_result) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_result = timeline->model->check_overlap(track_index, in_time, out_time, exclude_clip_id);
    return GOPOST_OK;
}

GopostError gopost_timeline_get_overlapping_clips(
    GopostTimeline* timeline,
    int32_t track_index,
    double in_time, double out_time,
    int32_t* out_clip_ids, int32_t max_ids,
    int32_t* out_count) {
    if (!timeline || !out_clip_ids || !out_count || max_ids < 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto ids = timeline->model->get_overlapping_clips(track_index, in_time, out_time);
    *out_count = static_cast<int32_t>(std::min(static_cast<size_t>(max_ids), ids.size()));
    for (int32_t i = 0; i < *out_count; ++i) {
        out_clip_ids[i] = ids[static_cast<size_t>(i)];
    }
    return GOPOST_OK;
}

GopostError gopost_timeline_set_track_sync_lock(
    GopostTimeline* timeline,
    int32_t track_index,
    int32_t locked) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->set_track_sync_lock(track_index, locked != 0)) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    return GOPOST_OK;
}

GopostError gopost_timeline_set_track_height(
    GopostTimeline* timeline,
    int32_t track_index,
    float height_px) {
    if (!timeline) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!timeline->model->set_track_height(track_index, height_px)) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    return GOPOST_OK;
}

GopostError gopost_timeline_get_track_height(
    const GopostTimeline* timeline,
    int32_t track_index,
    float* out_height_px) {
    if (!timeline || !out_height_px) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_height_px = timeline->model->get_track_height(track_index);
    return GOPOST_OK;
}

}  // extern "C"
