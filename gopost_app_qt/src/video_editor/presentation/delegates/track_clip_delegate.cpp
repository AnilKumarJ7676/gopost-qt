#include "video_editor/presentation/delegates/track_clip_delegate.h"

#include <QDebug>
#include <algorithm>
#include <cmath>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
TrackClipDelegate::TrackClipDelegate(TimelineOperations* ops) : ops_(ops) {}
TrackClipDelegate::~TrackClipDelegate() = default;

// ---------------------------------------------------------------------------
// Track management
// ---------------------------------------------------------------------------

void TrackClipDelegate::addTrack(TrackType type) {
    auto state = ops_->currentState();
    if (!state.project) return;

    auto before = *state.project;
    VideoTrack track;
    track.type    = type;
    track.index   = static_cast<int>(state.project->tracks.size());
    auto typeStr = [](TrackType t) -> QString {
        switch (t) {
        case TrackType::Video:    return QStringLiteral("Video");
        case TrackType::Audio:    return QStringLiteral("Audio");
        case TrackType::Title:    return QStringLiteral("Title");
        case TrackType::Effect:   return QStringLiteral("Effect");
        case TrackType::Subtitle: return QStringLiteral("Subtitle");
        }
        return QStringLiteral("Track");
    };
    track.label = typeStr(type) + QStringLiteral(" ") +
                  QString::number(track.index + 1);
    state.project->tracks.push_back(std::move(track));
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::removeTrack(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;

    auto before = *state.project;
    tracks.erase(tracks.begin() + trackIndex);

    // Re-index
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i)
        tracks[i].index = i;

    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::toggleTrackVisibility(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    tracks[trackIndex].isVisible = !tracks[trackIndex].isVisible;
    ops_->setState(std::move(state));
}

void TrackClipDelegate::toggleTrackLock(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    tracks[trackIndex].isLocked = !tracks[trackIndex].isLocked;
    ops_->setState(std::move(state));
}

void TrackClipDelegate::toggleTrackMute(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    tracks[trackIndex].isMuted = !tracks[trackIndex].isMuted;
    ops_->setState(std::move(state));
}

void TrackClipDelegate::toggleTrackSolo(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    tracks[trackIndex].isSolo = !tracks[trackIndex].isSolo;
    ops_->setState(std::move(state));
}

// ---------------------------------------------------------------------------
// Clip management
// ---------------------------------------------------------------------------

int TrackClipDelegate::nextClipId() const {
    int maxId = 0;
    const auto& state = ops_->currentState();
    if (!state.project) return 1;
    for (const auto& track : state.project->tracks)
        for (const auto& clip : track.clips)
            maxId = std::max(maxId, clip.id);
    return maxId + 1;
}

int TrackClipDelegate::addClip(int trackIndex,
                               ClipSourceType sourceType,
                               const QString& sourcePath,
                               const QString& displayName,
                               double duration) {
    qDebug() << "[TrackClipDelegate] addClip: track=" << trackIndex
             << "sourceType=" << static_cast<int>(sourceType)
             << "path=" << sourcePath << "name=" << displayName
             << "duration=" << duration;
    auto state = ops_->currentState();
    if (!state.project) {
        qWarning() << "[TrackClipDelegate] addClip FAILED: no project!";
        return -1;
    }
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
        qWarning() << "[TrackClipDelegate] addClip FAILED: trackIndex" << trackIndex
                   << "out of range (tracks:" << tracks.size() << ")";
        return -1;
    }

    auto before = *state.project;
    auto& track = tracks[trackIndex];

    // Place after last clip on track
    double timelineIn = 0.0;
    for (const auto& c : track.clips)
        timelineIn = std::max(timelineIn, c.timelineOut);

    VideoClip clip;
    clip.id          = nextClipId();
    clip.trackIndex  = trackIndex;
    clip.sourceType  = sourceType;
    clip.sourcePath  = sourcePath;
    clip.displayName = displayName;
    clip.sourceIn    = 0.0;
    clip.sourceOut   = duration;
    clip.timelineIn  = timelineIn;
    clip.timelineOut = timelineIn + duration;
    clip.speed       = 1.0;
    clip.opacity     = 1.0;
    clip.blendMode   = 0;

    const int newClipId = clip.id;
    track.clips.push_back(std::move(clip));
    qDebug() << "[TrackClipDelegate] clip added: id=" << newClipId
             << "track=" << trackIndex << "timelineIn=" << timelineIn
             << "timelineOut=" << (timelineIn + duration)
             << "total clips on track=" << tracks[trackIndex].clips.size();
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
    ops_->syncNativeToProject();
    ops_->debouncedRenderFrame();  // render preview for newly added clip

    return newClipId;
}

void TrackClipDelegate::removeClip(int clipId, bool ripple) {
    auto state = ops_->currentState();
    if (!state.project) return;

    auto before = *state.project;

    for (auto& track : state.project->tracks) {
        auto it = std::find_if(track.clips.begin(), track.clips.end(),
                               [clipId](const VideoClip& c) { return c.id == clipId; });
        if (it != track.clips.end()) {
            const double gapStart = it->timelineIn;
            const double gapLen   = it->duration();
            track.clips.erase(it);

            if (ripple) {
                for (auto& c : track.clips) {
                    if (c.timelineIn >= gapStart) {
                        c.timelineIn  -= gapLen;
                        c.timelineOut -= gapLen;
                    }
                }
            }

            if (state.selectedClipId == clipId)
                state.selectedClipId.reset();

            ops_->setState(std::move(state));
            ops_->pushUndo(before);
            ops_->syncNativeToProject();
            return;
        }
    }
}

void TrackClipDelegate::moveClip(int clipId, int toTrackIndex, double toTimelineIn) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (toTrackIndex < 0 || toTrackIndex >= static_cast<int>(tracks.size())) return;

    auto before = *state.project;

    // Find and remove from old track
    VideoClip moving;
    bool found = false;
    for (auto& track : tracks) {
        auto it = std::find_if(track.clips.begin(), track.clips.end(),
                               [clipId](const VideoClip& c) { return c.id == clipId; });
        if (it != track.clips.end()) {
            moving = *it;
            track.clips.erase(it);
            found = true;
            break;
        }
    }
    if (!found) return;

    // Apply magnetic snap
    double snapped = magneticSnap(tracks[toTrackIndex], toTimelineIn, clipId);
    snapped = std::max(0.0, snapped);

    moving.trackIndex  = toTrackIndex;
    moving.timelineIn  = snapped;
    moving.timelineOut = snapped + moving.duration();

    // Collision check — if collision, revert
    if (hasCollision(tracks[toTrackIndex], moving.timelineIn, moving.timelineOut, clipId)) {
        // Put it back (find original track by moving.trackIndex)
        return; // state not committed
    }

    tracks[toTrackIndex].clips.push_back(std::move(moving));
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
    ops_->syncNativeToProject();
}

void TrackClipDelegate::trimClip(int clipId, double newIn, double newOut) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&](const VideoClip& c) {
        VideoClip updated = c;
        updated.timelineIn  = newIn;
        updated.timelineOut = newOut;
        updated.sourceIn    = c.sourceIn + (newIn - c.timelineIn);
        updated.sourceOut   = c.sourceOut - (c.timelineOut - newOut);
        return updated;
    });

    ops_->pushUndo(before);
}

void TrackClipDelegate::splitClipAtPlayhead() {
    auto state = ops_->currentState();
    if (!state.project) return;
    const double pos = state.playback.positionSeconds;

    // Find clip under playhead
    for (auto& track : state.project->tracks) {
        for (size_t i = 0; i < track.clips.size(); ++i) {
            auto& clip = track.clips[i];
            if (pos > clip.timelineIn && pos < clip.timelineOut) {
                auto before = *state.project;

                const double splitPoint = pos - clip.timelineIn;
                VideoClip secondHalf = clip;
                secondHalf.id         = nextClipId();
                secondHalf.timelineIn = pos;
                secondHalf.sourceIn   = clip.sourceIn + splitPoint;

                clip.timelineOut = pos;
                clip.sourceOut   = clip.sourceIn + splitPoint;

                track.clips.insert(track.clips.begin() + static_cast<int>(i) + 1, secondHalf);

                ops_->setState(std::move(state));
                ops_->pushUndo(before);
                ops_->syncNativeToProject();
                return;
            }
        }
    }
}

void TrackClipDelegate::rippleDelete(int clipId) {
    removeClip(clipId, /*ripple=*/true);
}

void TrackClipDelegate::duplicateClip(int clipId) {
    auto state = ops_->currentState();
    if (!state.project) return;

    for (const auto& track : state.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.id == clipId) {
                auto before = *state.project;
                VideoClip dup = clip;
                dup.id = nextClipId();
                dup.timelineIn  = clip.timelineOut;
                dup.timelineOut = dup.timelineIn + clip.duration();

                auto& destTrack = state.project->tracks[clip.trackIndex];
                destTrack.clips.push_back(std::move(dup));

                ops_->setState(std::move(state));
                ops_->pushUndo(before);
                ops_->syncNativeToProject();
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

void TrackClipDelegate::selectClip(int clipId) {
    auto state = ops_->currentState();
    state.selectedClipId = clipId;
    ops_->setState(std::move(state));
}

void TrackClipDelegate::deselectClip() {
    auto state = ops_->currentState();
    state.selectedClipId.reset();
    ops_->setState(std::move(state));
}

std::optional<int> TrackClipDelegate::clipUnderPlayhead() const {
    const auto& state = ops_->currentState();
    if (!state.project) return std::nullopt;
    const double pos = state.playback.positionSeconds;

    for (const auto& track : state.project->tracks) {
        if (!track.isVisible) continue;
        for (const auto& clip : track.clips) {
            if (pos >= clip.timelineIn && pos < clip.timelineOut)
                return clip.id;
        }
    }
    return std::nullopt;
}

void TrackClipDelegate::autoSelectClipIfNeeded() {
    if (ops_->currentState().selectedClipId.has_value()) return;
    auto id = clipUnderPlayhead();
    if (id) selectClip(*id);
}

// ---------------------------------------------------------------------------
// Gap management
// ---------------------------------------------------------------------------

void TrackClipDelegate::closeTrackGaps(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;

    auto before = *state.project;
    auto& clips = tracks[trackIndex].clips;

    std::sort(clips.begin(), clips.end(),
              [](const VideoClip& a, const VideoClip& b) {
                  return a.timelineIn < b.timelineIn;
              });

    double cursor = 0.0;
    for (auto& clip : clips) {
        clip.timelineIn  = cursor;
        clip.timelineOut = cursor + clip.duration();
        cursor = clip.timelineOut;
    }

    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::updateClipDuration(int clipId, double duration) {
    ops_->updateClip(clipId, [duration](const VideoClip& c) {
        VideoClip updated = c;
        updated.sourceOut   = c.sourceIn + duration;
        updated.timelineOut = c.timelineIn + duration;
        return updated;
    });
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool TrackClipDelegate::hasCollision(const VideoTrack& track, double inPt,
                                     double outPt, int excludeClipId) const {
    for (const auto& clip : track.clips) {
        if (clip.id == excludeClipId) continue;
        if (inPt < clip.timelineOut && outPt > clip.timelineIn)
            return true;
    }
    return false;
}

double TrackClipDelegate::magneticSnap(const VideoTrack& track, double position,
                                       int excludeClipId) const {
    double best = position;
    double bestDist = kMagneticSnapThreshold;

    for (const auto& clip : track.clips) {
        if (clip.id == excludeClipId) continue;
        for (double edge : {clip.timelineIn, clip.timelineOut}) {
            double dist = std::abs(position - edge);
            if (dist < bestDist) {
                bestDist = dist;
                best = edge;
            }
        }
    }
    return best;
}

} // namespace gopost::video_editor
