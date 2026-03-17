#ifndef GOPOST_VIDEO_TIMELINE_MODEL_HPP
#define GOPOST_VIDEO_TIMELINE_MODEL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

#include "video_effects.hpp"
#include "transition_renderer.hpp"
#include "keyframe_evaluator.hpp"

namespace gopost {
namespace video {

enum class TrackType { Video, Audio, Title, Effect, Subtitle };
enum class ClipSourceType { Video, Image, Title, Color };

struct TimeRange {
    double in_time = 0;
    double out_time = 0;
};

struct SourceRange {
    double source_in = 0;
    double source_out = 0;
};

struct ClipAudioConfig {
    float volume = 1.0f;
    float pan = 0.0f;
    float fade_in_seconds = 0.0f;
    float fade_out_seconds = 0.0f;
    bool is_muted = false;
};

struct Clip {
    int32_t id = 0;
    int32_t track_index = 0;
    ClipSourceType source_type = ClipSourceType::Video;
    std::string source_path;
    TimeRange timeline_range;
    SourceRange source_range;
    double speed = 1.0;
    float opacity = 1.0f;
    int32_t blend_mode = 0;  /* 0=Normal, 1=Multiply, 2=Screen, 3=Overlay, 4=Add */
    uint32_t effect_hash = 0;
    ClipEffectState effects;
    TransitionDesc transition_in;
    TransitionDesc transition_out;
    ClipKeyframeState keyframes;
    ClipAudioConfig audio;
};

struct TrackAudioConfig {
    float volume = 1.0f;
    float pan = 0.0f;
    bool is_muted = false;
    bool is_solo = false;
};

struct Track {
    TrackType type = TrackType::Video;
    std::vector<Clip> clips;
    TrackAudioConfig audio;
    bool sync_locked = false;
    float height = 68.0f;
};

struct TimelineConfig {
    double frame_rate = 30.0;
    int32_t width = 1920;
    int32_t height = 1080;
    int32_t color_space = 0;
};

/** In-memory timeline model. Thread-safe for read; mutate under lock. */
class TimelineModel {
public:
    explicit TimelineModel(const TimelineConfig& config);
    ~TimelineModel() = default;

    const TimelineConfig& config() const { return config_; }
    double duration_seconds() const;
    double position_seconds() const { return position_seconds_; }
    void set_position_seconds(double t) { position_seconds_ = t; }

    std::mutex& mutex() { return mutex_; }

    int32_t add_track(TrackType type);
    bool remove_track(int32_t track_index);
    int32_t track_count() const { return static_cast<int32_t>(tracks_.size()); }

    int32_t add_clip(int32_t track_index, ClipSourceType source_type,
                     const std::string& source_path,
                     const TimeRange& timeline_range,
                     const SourceRange& source_range,
                     double speed, float opacity, int32_t blend_mode,
                     uint32_t effect_hash);
    bool remove_clip(int32_t clip_id);
    bool trim_clip(int32_t clip_id, const TimeRange& new_timeline, const SourceRange& new_source);
    bool move_clip(int32_t clip_id, int32_t new_track_index, double new_in_time);

    /**
     * Split clip at split_time_seconds (must be strictly between clip in and out).
     * Original clip is trimmed to [in, split]; new clip is [split, out] with same effects/audio/keyframes.
     * Returns new clip id, or -1 on failure.
     */
    int32_t split_clip(int32_t clip_id, double split_time_seconds);

    /**
     * Ripple delete: remove all clips on the given track that overlap [range_start, range_end],
     * then shift all later clips on that track left by the deleted duration.
     * Returns true if any change was made.
     */
    bool ripple_delete(int32_t track_index, double range_start_seconds, double range_end_seconds);

    /** Clips that are active at current playhead (position_seconds_), ordered by track (bottom to top). */
    std::vector<std::pair<int32_t, const Clip*>> active_clips_at_position() const;

    const std::vector<Track>& tracks() const { return tracks_; }
    Track* track_at(int32_t index);
    const Track* track_at(int32_t index) const;
    Clip* find_clip(int32_t clip_id);
    const Clip* find_clip(int32_t clip_id) const;

    /* Phase 2: NLE edit operations */
    int32_t insert_edit(int32_t track_index, double at_time,
                        ClipSourceType source_type, const std::string& source_path,
                        const TimeRange& timeline_range, const SourceRange& source_range,
                        double speed, float opacity, int32_t blend_mode, uint32_t effect_hash);
    int32_t overwrite_edit(int32_t track_index, double at_time,
                           ClipSourceType source_type, const std::string& source_path,
                           const TimeRange& timeline_range, const SourceRange& source_range,
                           double speed, float opacity, int32_t blend_mode, uint32_t effect_hash);
    bool roll_edit(int32_t clip_id, double delta_sec);
    bool slip_edit(int32_t clip_id, double delta_sec);
    bool slide_edit(int32_t clip_id, double delta_sec);
    bool rate_stretch(int32_t clip_id, double new_duration_sec);
    int32_t duplicate_clip(int32_t clip_id);
    std::vector<double> snap_points(double time_sec, double threshold_sec) const;
    bool reorder_tracks(const std::vector<int32_t>& new_order);

    /* Phase 7: Extended clip engine operations */
    bool move_multiple_clips(const std::vector<int32_t>& clip_ids, double delta_time, int32_t delta_track);
    bool swap_clips(int32_t clip_id_a, int32_t clip_id_b);
    int32_t split_all_tracks(double split_time);
    bool lift_delete(int32_t track_index, double range_start, double range_end);

    /** Collision detection: 0=CLEAR, 1=OVERLAP, 2=ADJACENT */
    int32_t check_overlap(int32_t track_index, double in_time, double out_time,
                          int32_t exclude_clip_id = -1) const;
    std::vector<int32_t> get_overlapping_clips(int32_t track_index,
                                                double in_time, double out_time) const;

    bool set_track_sync_lock(int32_t track_index, bool locked);
    bool set_track_height(int32_t track_index, float height_px);
    float get_track_height(int32_t track_index) const;

private:
    TimelineConfig config_;
    double position_seconds_ = 0;
    std::vector<Track> tracks_;
    int32_t next_clip_id_ = 1;
    mutable std::mutex mutex_;
};

}  // namespace video
}  // namespace gopost

#endif
