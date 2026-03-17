#ifndef GOPOST_VIDEO_TIMELINE_EVALUATOR_HPP
#define GOPOST_VIDEO_TIMELINE_EVALUATOR_HPP

#include "decoder_interface.hpp"
#include "decoder_pool.hpp"
#include "frame_cache.hpp"
#include "timeline_model.hpp"
#include "video_compositor.hpp"
#include "gopost/types.h"
#include <memory>
#include <string>
#include <unordered_map>

struct GopostEngine;
struct GopostFramePool;

namespace gopost {
namespace video {

class FrameCache;

/** Renders the timeline at current position: gather active clips, get/cache frames, composite. */
class TimelineEvaluator {
public:
    TimelineEvaluator(GopostEngine* engine,
                     TimelineModel* model,
                     FrameCache* frame_cache);
    ~TimelineEvaluator();

    /**
     * Render frame at current timeline position into output (from engine pool).
     * Caller must gopost_frame_release(output).
     */
    int render_frame(GopostFrame** output);

    /** Get or create decoder for a source path (cached per path). */
    IVideoDecoder* get_decoder_for(const std::string& path);

    /** Set an external decoder pool. When set, decoders are acquired/released
     *  through the pool instead of the internal unbounded map. */
    void set_decoder_pool(DecoderPool* pool);

private:
    GopostFrame* get_clip_frame_at(const Clip* clip, double timeline_time, int64_t frame_index);
    GopostVideoBlendMode to_blend_mode(int32_t mode) const;
    void release_all_leases();

    GopostEngine* engine_ = nullptr;
    TimelineModel* model_ = nullptr;
    FrameCache* frame_cache_ = nullptr;

    // Legacy: unbounded decoder map (used when no DecoderPool is set)
    std::unordered_map<std::string, std::unique_ptr<IVideoDecoder>> decoders_;

    // Pooled mode: decoders acquired from the pool for the current render pass.
    // Primary + secondary for double-buffer transition support.
    DecoderPool* decoder_pool_ = nullptr;
    std::unordered_map<std::string, DecoderLease> active_leases_;
    std::string prev_active_path_;  // For transition pre-open
};

}  // namespace video
}  // namespace gopost

#endif
