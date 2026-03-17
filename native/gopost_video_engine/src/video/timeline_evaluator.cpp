#include "timeline_evaluator.hpp"
#include "video_effects.hpp"
#include "keyframe_evaluator.hpp"
#include "gopost/engine.h"
#include "gopost/error.h"
#include <cmath>
#include <mutex>

namespace gopost {
namespace video {

static GopostVideoBlendMode blend_mode_from_int(int32_t mode) {
    switch (mode) {
        case 1: return GOPOST_VIDEO_BLEND_MULTIPLY;
        case 2: return GOPOST_VIDEO_BLEND_SCREEN;
        case 3: return GOPOST_VIDEO_BLEND_OVERLAY;
        case 4: return GOPOST_VIDEO_BLEND_ADD;
        default: return GOPOST_VIDEO_BLEND_NORMAL;
    }
}

TimelineEvaluator::TimelineEvaluator(GopostEngine* engine,
                                     TimelineModel* model,
                                     FrameCache* frame_cache)
    : engine_(engine)
    , model_(model)
    , frame_cache_(frame_cache) {}

TimelineEvaluator::~TimelineEvaluator() {
    release_all_leases();
}

GopostVideoBlendMode TimelineEvaluator::to_blend_mode(int32_t mode) const {
    return blend_mode_from_int(mode);
}

void TimelineEvaluator::set_decoder_pool(DecoderPool* pool) {
    release_all_leases();
    decoder_pool_ = pool;
    // Clear legacy decoders when switching to pool mode
    if (pool) {
        decoders_.clear();
    }
}

void TimelineEvaluator::release_all_leases() {
    active_leases_.clear();
    prev_active_path_.clear();
}

IVideoDecoder* TimelineEvaluator::get_decoder_for(const std::string& path) {
    if (path.empty()) return nullptr;

    // Pool mode: acquire from decoder pool
    if (decoder_pool_) {
        auto it = active_leases_.find(path);
        if (it != active_leases_.end() && it->second) {
            return it->second.decoder();
        }

        // Acquire from pool (High priority for active playback)
        DecoderLease lease = decoder_pool_->acquire(path, DecoderPriority::High);
        if (!lease) return nullptr;
        IVideoDecoder* dec = lease.decoder();
        active_leases_[path] = std::move(lease);
        return dec;
    }

    // Legacy mode: unbounded decoder map
    auto it = decoders_.find(path);
    if (it != decoders_.end()) return it->second.get();
    auto dec = create_video_decoder(engine_);
    if (!dec || !dec->open(path)) return nullptr;
    IVideoDecoder* p = dec.get();
    decoders_[path] = std::move(dec);
    return p;
}

GopostFrame* TimelineEvaluator::get_clip_frame_at(const Clip* clip, double timeline_time, int64_t frame_index) {
    const double clip_time = timeline_time - clip->timeline_range.in_time;
    const double source_time = clip->source_range.source_in + clip_time * clip->speed;
    FrameCacheKey key;
    key.frame_index = frame_index;
    key.clip_id = clip->id;
    key.effect_hash = clip->effect_hash;

    GopostFrame* cached = frame_cache_->get(key);
    if (cached) return cached;

    if (clip->source_type != ClipSourceType::Video && clip->source_type != ClipSourceType::Image) {
        return nullptr;
    }
    IVideoDecoder* dec = get_decoder_for(clip->source_path);
    if (!dec) return nullptr;
    GopostFrame* frame = dec->decode_frame_at(source_time);
    if (frame) {
        // Apply per-clip effects (S10-01, S10-02)
        apply_clip_effects(frame, clip->effects);

        frame_cache_->put(key, frame);
    }
    return frame;
}

int TimelineEvaluator::render_frame(GopostFrame** output) {
    if (!engine_ || !model_ || !frame_cache_ || !output) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    const TimelineConfig& cfg = model_->config();
    const uint32_t w = static_cast<uint32_t>(cfg.width);
    const uint32_t h = static_cast<uint32_t>(cfg.height);
    const double fps = cfg.frame_rate > 0 ? cfg.frame_rate : 30.0;
    const double pos = model_->position_seconds();
    const int64_t frame_index = static_cast<int64_t>(std::floor(pos * fps));

    GopostFrame* out_frame = nullptr;
    if (gopost_frame_acquire(engine_, &out_frame, w, h, GOPOST_PIXEL_FORMAT_RGBA8) != GOPOST_OK) {
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }
    if (!out_frame || !out_frame->data) {
        if (out_frame) gopost_frame_release(engine_, out_frame);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }
    gopost_video_frame_clear(out_frame);

    std::vector<std::pair<int32_t, const Clip*>> active = model_->active_clips_at_position();

    // Collect paths needed for this frame, release leases no longer needed
    if (decoder_pool_) {
        std::unordered_map<std::string, bool> needed_paths;
        for (const auto& p : active) {
            if (p.second->source_type == ClipSourceType::Video ||
                p.second->source_type == ClipSourceType::Image) {
                needed_paths[p.second->source_path] = true;
            }
        }
        // Release leases for paths no longer active
        for (auto it = active_leases_.begin(); it != active_leases_.end(); ) {
            if (needed_paths.find(it->first) == needed_paths.end()) {
                it = active_leases_.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (const auto& p : active) {
        const Clip* clip = p.second;
        GopostFrame* layer_frame = get_clip_frame_at(clip, pos, frame_index);
        if (!layer_frame) continue;

        // Evaluate keyframe-animated properties (S10-04)
        float final_opacity = clip->opacity;
        if (!clip->keyframes.tracks.empty()) {
            double clip_local = pos - clip->timeline_range.in_time;
            EvaluatedProperties kf_props = evaluate_keyframes(clip->keyframes, clip_local);
            final_opacity = static_cast<float>(clip->opacity * kf_props.opacity);
        }

        int err = gopost_video_composite_blend(
            out_frame, layer_frame,
            final_opacity,
            to_blend_mode(clip->blend_mode));
        if (err != GOPOST_OK) {
            gopost_frame_release(engine_, out_frame);
            return err;
        }
    }

    *output = out_frame;
    return GOPOST_OK;
}

}  // namespace video
}  // namespace gopost
