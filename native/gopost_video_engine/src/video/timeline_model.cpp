#include "timeline_model.hpp"
#include <algorithm>
#include <cmath>

namespace gopost {
namespace video {

TimelineModel::TimelineModel(const TimelineConfig& config)
    : config_(config) {}

double TimelineModel::duration_seconds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    double end = 0;
    for (const auto& track : tracks_) {
        for (const auto& clip : track.clips) {
            if (clip.timeline_range.out_time > end) {
                end = clip.timeline_range.out_time;
            }
        }
    }
    return end;
}

int32_t TimelineModel::add_track(TrackType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    Track t;
    t.type = type;
    t.clips = {};
    tracks_.push_back(std::move(t));
    return static_cast<int32_t>(tracks_.size() - 1);
}

bool TimelineModel::remove_track(int32_t track_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (track_index < 0 || track_index >= static_cast<int32_t>(tracks_.size())) {
        return false;
    }
    tracks_.erase(tracks_.begin() + track_index);
    return true;
}

int32_t TimelineModel::add_clip(int32_t track_index, ClipSourceType source_type,
                                const std::string& source_path,
                                const TimeRange& timeline_range,
                                const SourceRange& source_range,
                                double speed, float opacity, int32_t blend_mode,
                                uint32_t effect_hash) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (track_index < 0 || track_index >= static_cast<int32_t>(tracks_.size())) {
        return -1;
    }
    Clip c;
    c.id = next_clip_id_++;
    c.track_index = track_index;
    c.source_type = source_type;
    c.source_path = source_path;
    c.timeline_range = timeline_range;
    c.source_range = source_range;
    c.speed = speed;
    c.opacity = opacity;
    c.blend_mode = blend_mode;
    c.effect_hash = effect_hash;
    tracks_[static_cast<size_t>(track_index)].clips.push_back(c);
    return c.id;
}

bool TimelineModel::remove_clip(int32_t clip_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& track : tracks_) {
        auto it = std::find_if(track.clips.begin(), track.clips.end(),
                               [clip_id](const Clip& c) { return c.id == clip_id; });
        if (it != track.clips.end()) {
            track.clips.erase(it);
            return true;
        }
    }
    return false;
}

bool TimelineModel::trim_clip(int32_t clip_id, const TimeRange& new_timeline, const SourceRange& new_source) {
    std::lock_guard<std::mutex> lock(mutex_);
    Clip* c = find_clip(clip_id);
    if (!c) return false;
    c->timeline_range = new_timeline;
    c->source_range = new_source;
    return true;
}

bool TimelineModel::move_clip(int32_t clip_id, int32_t new_track_index, double new_in_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (new_track_index < 0 || new_track_index >= static_cast<int32_t>(tracks_.size())) {
        return false;
    }
    Clip* c = find_clip(clip_id);
    if (!c) return false;
    const int32_t old_track_index = c->track_index;
    const double duration = c->timeline_range.out_time - c->timeline_range.in_time;
    Clip copy = *c;
    copy.track_index = new_track_index;
    copy.timeline_range.in_time = new_in_time;
    copy.timeline_range.out_time = new_in_time + duration;

    auto& old_track = tracks_[static_cast<size_t>(old_track_index)];
    old_track.clips.erase(
        std::remove_if(old_track.clips.begin(), old_track.clips.end(),
                       [clip_id](const Clip& x) { return x.id == clip_id; }),
        old_track.clips.end());

    tracks_[static_cast<size_t>(new_track_index)].clips.push_back(std::move(copy));
    return true;
}

int32_t TimelineModel::split_clip(int32_t clip_id, double split_time_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    Clip* c = find_clip(clip_id);
    if (!c) return -1;
    const double in_t = c->timeline_range.in_time;
    const double out_t = c->timeline_range.out_time;
    if (split_time_seconds <= in_t || split_time_seconds >= out_t) return -1;

    const double clip_duration = out_t - in_t;
    const double source_duration = c->source_range.source_out - c->source_range.source_in;
    const double t_ratio = clip_duration > 0 ? (split_time_seconds - in_t) / clip_duration : 0;
    const double source_at_split = c->source_range.source_in + t_ratio * source_duration;

    const int32_t new_id = next_clip_id_++;
    Clip right = *c;
    right.id = new_id;
    right.timeline_range.in_time = split_time_seconds;
    right.timeline_range.out_time = out_t;
    right.source_range.source_in = source_at_split;
    right.source_range.source_out = c->source_range.source_out;

    c->timeline_range.out_time = split_time_seconds;
    c->source_range.source_out = source_at_split;

    Track& track = tracks_[static_cast<size_t>(c->track_index)];
    track.clips.push_back(std::move(right));
    std::sort(track.clips.begin(), track.clips.end(),
              [](const Clip& a, const Clip& b) {
                  return a.timeline_range.in_time < b.timeline_range.in_time;
              });
    return new_id;
}

bool TimelineModel::ripple_delete(int32_t track_index, double range_start_seconds, double range_end_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (track_index < 0 || track_index >= static_cast<int32_t>(tracks_.size())) return false;
    Track& track = tracks_[static_cast<size_t>(track_index)];
    if (range_end_seconds <= range_start_seconds) return false;

    const double delete_duration = range_end_seconds - range_start_seconds;
    std::vector<Clip> kept;
    kept.reserve(track.clips.size());
    bool any_removed = false;
    for (Clip& clip : track.clips) {
        const double clip_in = clip.timeline_range.in_time;
        const double clip_out = clip.timeline_range.out_time;
        const bool overlaps = clip_in < range_end_seconds && clip_out > range_start_seconds;
        if (overlaps) {
            any_removed = true;
            continue;
        }
        if (clip_out > range_end_seconds) {
            clip.timeline_range.in_time -= delete_duration;
            clip.timeline_range.out_time -= delete_duration;
        }
        kept.push_back(std::move(clip));
    }
    if (!any_removed) return false;
    track.clips = std::move(kept);
    return true;
}

std::vector<std::pair<int32_t, const Clip*>> TimelineModel::active_clips_at_position() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<int32_t, const Clip*>> out;
    const double t = position_seconds_;
    for (int32_t ti = 0; ti < static_cast<int32_t>(tracks_.size()); ++ti) {
        const auto& track = tracks_[ti];
        for (const auto& clip : track.clips) {
            if (t >= clip.timeline_range.in_time && t < clip.timeline_range.out_time) {
                out.emplace_back(ti, &clip);
            }
        }
    }
    return out;
}

Track* TimelineModel::track_at(int32_t index) {
    if (index < 0 || index >= static_cast<int32_t>(tracks_.size())) return nullptr;
    return &tracks_[static_cast<size_t>(index)];
}

const Track* TimelineModel::track_at(int32_t index) const {
    if (index < 0 || index >= static_cast<int32_t>(tracks_.size())) return nullptr;
    return &tracks_[static_cast<size_t>(index)];
}

Clip* TimelineModel::find_clip(int32_t clip_id) {
    for (auto& track : tracks_) {
        for (auto& clip : track.clips) {
            if (clip.id == clip_id) return &clip;
        }
    }
    return nullptr;
}

const Clip* TimelineModel::find_clip(int32_t clip_id) const {
    for (const auto& track : tracks_) {
        for (const auto& clip : track.clips) {
            if (clip.id == clip_id) return &clip;
        }
    }
    return nullptr;
}

/* Phase 2: NLE edit operations */

int32_t TimelineModel::insert_edit(int32_t track_index, double at_time,
                                   ClipSourceType source_type, const std::string& source_path,
                                   const TimeRange& timeline_range, const SourceRange& source_range,
                                   double speed, float opacity, int32_t blend_mode, uint32_t effect_hash) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (track_index < 0 || track_index >= static_cast<int32_t>(tracks_.size())) return -1;
    const double duration = timeline_range.out_time - timeline_range.in_time;
    if (duration <= 0) return -1;

    TimeRange placed_range;
    placed_range.in_time = at_time;
    placed_range.out_time = at_time + duration;

    Clip c;
    c.id = next_clip_id_++;
    c.track_index = track_index;
    c.source_type = source_type;
    c.source_path = source_path;
    c.timeline_range = placed_range;
    c.source_range = source_range;
    c.speed = speed;
    c.opacity = opacity;
    c.blend_mode = blend_mode;
    c.effect_hash = effect_hash;
    const int32_t id = c.id;
    tracks_[static_cast<size_t>(track_index)].clips.push_back(std::move(c));

    Track& track = tracks_[static_cast<size_t>(track_index)];
    for (auto& clip : track.clips) {
        if (clip.id != id && clip.timeline_range.in_time >= at_time) {
            clip.timeline_range.in_time += duration;
            clip.timeline_range.out_time += duration;
        }
    }
    std::sort(track.clips.begin(), track.clips.end(),
              [](const Clip& a, const Clip& b) {
                  return a.timeline_range.in_time < b.timeline_range.in_time;
              });
    return id;
}

int32_t TimelineModel::overwrite_edit(int32_t track_index, double at_time,
                                      ClipSourceType source_type, const std::string& source_path,
                                      const TimeRange& timeline_range, const SourceRange& source_range,
                                      double speed, float opacity, int32_t blend_mode, uint32_t effect_hash) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (track_index < 0 || track_index >= static_cast<int32_t>(tracks_.size())) return -1;
    const double duration = timeline_range.out_time - timeline_range.in_time;
    if (duration <= 0) return -1;

    const double range_start = at_time;
    const double range_end = at_time + duration;

    Track& track = tracks_[static_cast<size_t>(track_index)];
    std::vector<Clip> kept;
    kept.reserve(track.clips.size());
    for (Clip& clip : track.clips) {
        const double clip_in = clip.timeline_range.in_time;
        const double clip_out = clip.timeline_range.out_time;
        const bool overlaps = clip_in < range_end && clip_out > range_start;
        if (overlaps) {
            continue;
        }
        kept.push_back(std::move(clip));
    }
    track.clips = std::move(kept);

    TimeRange placed_range;
    placed_range.in_time = at_time;
    placed_range.out_time = range_end;

    Clip c;
    c.id = next_clip_id_++;
    c.track_index = track_index;
    c.source_type = source_type;
    c.source_path = source_path;
    c.timeline_range = placed_range;
    c.source_range = source_range;
    c.speed = speed;
    c.opacity = opacity;
    c.blend_mode = blend_mode;
    c.effect_hash = effect_hash;
    const int32_t new_id = c.id;
    track.clips.push_back(std::move(c));
    return new_id;
}

bool TimelineModel::roll_edit(int32_t clip_id, double delta_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    Clip* c = find_clip(clip_id);
    if (!c) return false;
    Track& track = tracks_[static_cast<size_t>(c->track_index)];

    Clip* next_clip = nullptr;
    for (auto& clip : track.clips) {
        if (clip.timeline_range.in_time >= c->timeline_range.out_time - 1e-9) {
            if (!next_clip || clip.timeline_range.in_time < next_clip->timeline_range.in_time) {
                next_clip = &clip;
            }
        }
    }
    if (!next_clip) return false;

    const double shared_point = c->timeline_range.out_time;
    const double new_out = shared_point + delta_sec;
    const double new_next_in = shared_point + delta_sec;

    if (new_out <= c->timeline_range.in_time) return false;
    const double source_duration = c->source_range.source_out - c->source_range.source_in;
    const double clip_duration = c->timeline_range.out_time - c->timeline_range.in_time;
    if (clip_duration <= 0) return false;
    const double new_source_out = c->source_range.source_in +
        (new_out - c->timeline_range.in_time) / clip_duration * source_duration;
    if (new_source_out < c->source_range.source_in) return false;

    const double next_source_duration = next_clip->source_range.source_out - next_clip->source_range.source_in;
    const double next_clip_duration = next_clip->timeline_range.out_time - next_clip->timeline_range.in_time;
    if (next_clip_duration <= 0) return false;
    const double shorten_by = delta_sec;
    const double new_next_source_in = next_clip->source_range.source_in +
        shorten_by / next_clip_duration * next_source_duration;
    if (new_next_source_in >= next_clip->source_range.source_out || new_next_source_in < next_clip->source_range.source_in) return false;

    c->timeline_range.out_time = new_out;
    c->source_range.source_out = new_source_out;
    next_clip->timeline_range.in_time = new_next_in;
    next_clip->source_range.source_in = new_next_source_in;
    return true;
}

bool TimelineModel::slip_edit(int32_t clip_id, double delta_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    Clip* c = find_clip(clip_id);
    if (!c) return false;
    double new_in = c->source_range.source_in + delta_sec;
    double new_out = c->source_range.source_out + delta_sec;
    if (new_in < 0) {
        new_out += (-new_in);
        new_in = 0;
    }
    if (new_out <= new_in) return false;
    c->source_range.source_in = new_in;
    c->source_range.source_out = new_out;
    return true;
}

bool TimelineModel::slide_edit(int32_t clip_id, double delta_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    Clip* c = find_clip(clip_id);
    if (!c) return false;
    const double duration = c->timeline_range.out_time - c->timeline_range.in_time;
    const double new_in = c->timeline_range.in_time + delta_sec;
    const double new_out = new_in + duration;
    if (new_in < 0) return false;

    Track& track = tracks_[static_cast<size_t>(c->track_index)];
    Clip* prev_clip = nullptr;
    Clip* next_clip = nullptr;
    for (auto& clip : track.clips) {
        if (clip.id == c->id) continue;
        if (clip.timeline_range.out_time <= c->timeline_range.in_time + 1e-9) {
            if (!prev_clip || clip.timeline_range.out_time > prev_clip->timeline_range.out_time) {
                prev_clip = &clip;
            }
        }
        if (clip.timeline_range.in_time >= c->timeline_range.out_time - 1e-9) {
            if (!next_clip || clip.timeline_range.in_time < next_clip->timeline_range.in_time) {
                next_clip = &clip;
            }
        }
    }
    if (prev_clip && new_in < prev_clip->timeline_range.out_time) return false;
    if (next_clip && new_out > next_clip->timeline_range.in_time) return false;

    c->timeline_range.in_time = new_in;
    c->timeline_range.out_time = new_out;

    if (prev_clip) {
        prev_clip->timeline_range.out_time = new_in;
    }
    if (next_clip) {
        next_clip->timeline_range.in_time = new_out;
    }
    return true;
}

bool TimelineModel::rate_stretch(int32_t clip_id, double new_duration_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    Clip* c = find_clip(clip_id);
    if (!c) return false;
    const double old_duration = c->timeline_range.out_time - c->timeline_range.in_time;
    if (old_duration <= 0 || new_duration_sec <= 0) return false;
    c->speed = c->speed * old_duration / new_duration_sec;
    c->timeline_range.out_time = c->timeline_range.in_time + new_duration_sec;
    return true;
}

int32_t TimelineModel::duplicate_clip(int32_t clip_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const Clip* src = find_clip(clip_id);
    if (!src) return -1;
    Clip dup = *src;
    dup.id = next_clip_id_++;
    const double duration = src->timeline_range.out_time - src->timeline_range.in_time;
    dup.timeline_range.in_time = src->timeline_range.out_time;
    dup.timeline_range.out_time = dup.timeline_range.in_time + duration;
    const int32_t new_id = dup.id;
    tracks_[static_cast<size_t>(src->track_index)].clips.push_back(std::move(dup));
    Track& track = tracks_[static_cast<size_t>(src->track_index)];
    std::sort(track.clips.begin(), track.clips.end(),
              [](const Clip& a, const Clip& b) {
                  return a.timeline_range.in_time < b.timeline_range.in_time;
              });
    return new_id;
}

std::vector<double> TimelineModel::snap_points(double time_sec, double threshold_sec) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<double> points;
    for (const auto& track : tracks_) {
        for (const auto& clip : track.clips) {
            const double in_t = clip.timeline_range.in_time;
            const double out_t = clip.timeline_range.out_time;
            if (std::abs(in_t - time_sec) <= threshold_sec) points.push_back(in_t);
            if (std::abs(out_t - time_sec) <= threshold_sec) points.push_back(out_t);
        }
    }
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    return points;
}

bool TimelineModel::reorder_tracks(const std::vector<int32_t>& new_order) {
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t n = tracks_.size();
    if (new_order.size() != n) return false;
    std::vector<bool> used(n, false);
    for (int32_t idx : new_order) {
        if (idx < 0 || idx >= static_cast<int32_t>(n)) return false;
        if (used[static_cast<size_t>(idx)]) return false;
        used[static_cast<size_t>(idx)] = true;
    }
    std::vector<Track> reordered;
    reordered.reserve(n);
    for (int32_t idx : new_order) {
        reordered.push_back(std::move(tracks_[static_cast<size_t>(idx)]));
    }
    tracks_ = std::move(reordered);
    for (size_t i = 0; i < tracks_.size(); ++i) {
        for (auto& clip : tracks_[i].clips) {
            clip.track_index = static_cast<int32_t>(i);
        }
    }
    return true;
}

// =========================================================================
// Phase 7: Extended clip engine operations
// =========================================================================

bool TimelineModel::move_multiple_clips(const std::vector<int32_t>& clip_ids,
                                         double delta_time, int32_t delta_track) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Gather clip pointers first, validate all exist.
    std::vector<Clip*> clips;
    clips.reserve(clip_ids.size());
    for (int32_t id : clip_ids) {
        Clip* c = find_clip(id);
        if (!c) return false;
        clips.push_back(c);
    }

    // Apply deltas.
    for (Clip* c : clips) {
        c->timeline_range.in_time += delta_time;
        c->timeline_range.out_time += delta_time;

        if (delta_track != 0) {
            int32_t new_track = c->track_index + delta_track;
            if (new_track < 0 || new_track >= static_cast<int32_t>(tracks_.size())) continue;

            // Remove from old track.
            auto& old_clips = tracks_[static_cast<size_t>(c->track_index)].clips;
            Clip copy = *c;
            copy.track_index = new_track;
            old_clips.erase(
                std::remove_if(old_clips.begin(), old_clips.end(),
                    [c](const Clip& cl) { return cl.id == c->id; }),
                old_clips.end());

            // Insert into new track.
            tracks_[static_cast<size_t>(new_track)].clips.push_back(std::move(copy));
        }
    }
    return true;
}

bool TimelineModel::swap_clips(int32_t clip_id_a, int32_t clip_id_b) {
    std::lock_guard<std::mutex> lock(mutex_);
    Clip* a = find_clip(clip_id_a);
    Clip* b = find_clip(clip_id_b);
    if (!a || !b) return false;

    // Swap timeline positions, keep source content intact.
    std::swap(a->timeline_range, b->timeline_range);
    std::swap(a->track_index, b->track_index);
    return true;
}

int32_t TimelineModel::split_all_tracks(double split_time) {
    // Don't lock here — split_clip acquires the lock internally per call.
    // Collect clip IDs first, then split each.
    std::vector<int32_t> to_split;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& track : tracks_) {
            for (const auto& clip : track.clips) {
                if (split_time > clip.timeline_range.in_time + 0.001 &&
                    split_time < clip.timeline_range.out_time - 0.001) {
                    to_split.push_back(clip.id);
                }
            }
        }
    }

    int32_t count = 0;
    for (int32_t id : to_split) {
        if (split_clip(id, split_time) >= 0) ++count;
    }
    return count;
}

bool TimelineModel::lift_delete(int32_t track_index, double range_start, double range_end) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* track = track_at(track_index);
    if (!track) return false;

    const size_t before = track->clips.size();
    track->clips.erase(
        std::remove_if(track->clips.begin(), track->clips.end(),
            [range_start, range_end](const Clip& c) {
                return c.timeline_range.in_time >= range_start &&
                       c.timeline_range.out_time <= range_end;
            }),
        track->clips.end());
    // Note: no shifting — gap remains (lift vs ripple).
    return track->clips.size() != before;
}

int32_t TimelineModel::check_overlap(int32_t track_index, double in_time,
                                      double out_time, int32_t exclude_clip_id) const {
    const Track* track = track_at(track_index);
    if (!track) return 0; // CLEAR

    constexpr double eps = 0.001;
    bool has_adjacent = false;

    for (const auto& clip : track->clips) {
        if (clip.id == exclude_clip_id) continue;
        const double ci = clip.timeline_range.in_time;
        const double co = clip.timeline_range.out_time;

        // Check overlap (intervals intersect if ci < out_time && co > in_time).
        if (ci < out_time - eps && co > in_time + eps) {
            return 1; // OVERLAP
        }
        // Check adjacency.
        if (std::abs(ci - out_time) < eps || std::abs(co - in_time) < eps) {
            has_adjacent = true;
        }
    }
    return has_adjacent ? 2 : 0; // ADJACENT or CLEAR
}

std::vector<int32_t> TimelineModel::get_overlapping_clips(int32_t track_index,
                                                           double in_time, double out_time) const {
    std::vector<int32_t> result;
    const Track* track = track_at(track_index);
    if (!track) return result;

    constexpr double eps = 0.001;
    for (const auto& clip : track->clips) {
        const double ci = clip.timeline_range.in_time;
        const double co = clip.timeline_range.out_time;
        if (ci < out_time - eps && co > in_time + eps) {
            result.push_back(clip.id);
        }
    }
    return result;
}

bool TimelineModel::set_track_sync_lock(int32_t track_index, bool locked) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* track = track_at(track_index);
    if (!track) return false;
    track->sync_locked = locked;
    return true;
}

bool TimelineModel::set_track_height(int32_t track_index, float height_px) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* track = track_at(track_index);
    if (!track) return false;
    track->height = height_px;
    return true;
}

float TimelineModel::get_track_height(int32_t track_index) const {
    const auto* track = track_at(track_index);
    return track ? track->height : 68.0f;
}

}  // namespace video
}  // namespace gopost
