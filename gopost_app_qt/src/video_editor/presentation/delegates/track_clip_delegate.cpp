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
    state.trackGeneration++;
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

    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::toggleTrackVisibility(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    auto before = *state.project;
    tracks[trackIndex].isVisible = !tracks[trackIndex].isVisible;
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::toggleTrackLock(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    auto before = *state.project;
    tracks[trackIndex].isLocked = !tracks[trackIndex].isLocked;
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::toggleTrackMute(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    auto before = *state.project;
    tracks[trackIndex].isMuted = !tracks[trackIndex].isMuted;
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::toggleTrackSolo(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    auto before = *state.project;
    tracks[trackIndex].isSolo = !tracks[trackIndex].isSolo;
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
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
    state.trackGeneration++;
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

            state.trackGeneration++;
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

    // ---- Track-type compatibility enforcement ----
    // Determine which track types are compatible with this clip's source type.
    // Video/Image clips → Video tracks only
    // Audio clips → Audio tracks only
    // Title clips → Title or Video tracks
    // Color/Adjustment clips → Effect tracks
    auto compatibleTrackType = [](ClipSourceType src) -> TrackType {
        switch (src) {
        case ClipSourceType::Video:
        case ClipSourceType::Image:      return TrackType::Video;
        case ClipSourceType::Audio:      return TrackType::Audio;
        case ClipSourceType::Title:      return TrackType::Title;
        case ClipSourceType::Color:
        case ClipSourceType::Adjustment: return TrackType::Effect;
        default:                         return TrackType::Video;
        }
    };

    TrackType requiredType = compatibleTrackType(moving.sourceType);
    // Title clips can also go on Video tracks
    bool titleOnVideo = (moving.sourceType == ClipSourceType::Title);

    auto isCompatible = [&](int trackIdx) {
        if (trackIdx < 0 || trackIdx >= static_cast<int>(tracks.size())) return false;
        TrackType tt = tracks[trackIdx].type;
        if (tt == requiredType) return true;
        if (titleOnVideo && tt == TrackType::Video) return true;
        return false;
    };

    int resolvedTrackIdx = toTrackIndex;

    if (!isCompatible(toTrackIndex)) {
        // Find the nearest compatible track
        int bestTrack = -1;
        int bestDist = 9999;
        for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
            if (isCompatible(i)) {
                int dist = std::abs(i - toTrackIndex);
                if (dist < bestDist) { bestDist = dist; bestTrack = i; }
            }
        }

        if (bestTrack >= 0) {
            resolvedTrackIdx = bestTrack;
        } else {
            // No compatible track exists — create one
            VideoTrack newTrack;
            newTrack.type  = requiredType;
            newTrack.index = static_cast<int>(tracks.size());
            auto typeStr = [](TrackType t) -> QString {
                switch (t) {
                case TrackType::Video:  return QStringLiteral("Video");
                case TrackType::Audio:  return QStringLiteral("Audio");
                case TrackType::Title:  return QStringLiteral("Title");
                case TrackType::Effect: return QStringLiteral("Effect");
                default:                return QStringLiteral("Track");
                }
            };
            newTrack.label = typeStr(requiredType) + QStringLiteral(" ") +
                             QString::number(newTrack.index + 1);
            tracks.push_back(std::move(newTrack));
            resolvedTrackIdx = static_cast<int>(tracks.size()) - 1;
            qDebug() << "[TrackClipDelegate] moveClip: created new"
                     << typeStr(requiredType) << "track at index" << resolvedTrackIdx;
        }
    }

    // Apply magnetic snap
    double snapped = magneticSnap(tracks[resolvedTrackIdx], toTimelineIn, clipId);
    snapped = std::max(0.0, snapped);

    const double clipDur = moving.duration();

    // Resolve collisions: find nearest gap that fits this clip
    auto& destTrack = tracks[resolvedTrackIdx];
    double bestPos = snapped;

    if (hasCollision(destTrack, snapped, snapped + clipDur, clipId)) {
        // Sort clips by timelineIn to find gaps
        std::vector<std::pair<double, double>> occupied; // (in, out) ranges
        for (const auto& c : destTrack.clips) {
            if (c.id == clipId) continue;
            occupied.push_back({c.timelineIn, c.timelineOut});
        }
        std::sort(occupied.begin(), occupied.end());

        // Strategy: try snapping to the nearest gap edge
        double closestValid = -1;
        double closestDist  = 1e12;

        // Try placing right after each clip
        for (const auto& [oIn, oOut] : occupied) {
            double candidate = oOut;
            if (!hasCollision(destTrack, candidate, candidate + clipDur, clipId)) {
                double dist = std::abs(candidate - snapped);
                if (dist < closestDist) { closestDist = dist; closestValid = candidate; }
            }
        }
        // Try placing right before each clip
        for (const auto& [oIn, oOut] : occupied) {
            double candidate = oIn - clipDur;
            if (candidate >= 0 && !hasCollision(destTrack, candidate, candidate + clipDur, clipId)) {
                double dist = std::abs(candidate - snapped);
                if (dist < closestDist) { closestDist = dist; closestValid = candidate; }
            }
        }
        // Try start of timeline
        if (!hasCollision(destTrack, 0, clipDur, clipId)) {
            double dist = std::abs(0.0 - snapped);
            if (dist < closestDist) { closestDist = dist; closestValid = 0.0; }
        }
        // Try end of timeline (after last clip)
        if (!occupied.empty()) {
            double candidate = occupied.back().second;
            double dist = std::abs(candidate - snapped);
            if (dist < closestDist) { closestDist = dist; closestValid = candidate; }
        }

        if (closestValid >= 0) {
            bestPos = closestValid;
        } else {
            // No valid position found — revert (shouldn't happen)
            qDebug() << "[TrackClipDelegate] moveClip: no valid position found, reverting";
            return;
        }
    }

    moving.trackIndex  = resolvedTrackIdx;
    moving.timelineIn  = bestPos;
    moving.timelineOut = bestPos + clipDur;

    destTrack.clips.push_back(std::move(moving));
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
    ops_->syncNativeToProject();
}

void TrackClipDelegate::trimClip(int clipId, double newIn, double newOut) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    // Find the clip to compute delta for ripple
    const auto* origClip = state.project->findClip(clipId);
    if (!origClip) return;
    const double oldOut = origClip->timelineOut;
    const double oldIn  = origClip->timelineIn;
    const int trackIdx  = origClip->trackIndex;

    ops_->updateClip(clipId, [&](const VideoClip& c) {
        VideoClip updated = c;
        updated.timelineIn  = newIn;
        updated.timelineOut = newOut;
        updated.sourceIn    = c.sourceIn + (newIn - c.timelineIn);
        updated.sourceOut   = c.sourceOut - (c.timelineOut - newOut);
        return updated;
    });

    // Ripple-on-trim: if ripple mode is on, shift downstream clips
    if (state.rippleMode) {
        // Right trim shortened → shift downstream left
        double rightDelta = newOut - oldOut; // negative when shortened
        // Left trim changed → shift downstream by the inverse
        double leftDelta = newIn - oldIn;   // positive when trimmed from left

        double shiftAmount = 0;
        double shiftAfter = 0;

        if (std::abs(rightDelta) > 0.001 && std::abs(leftDelta) < 0.001) {
            // Right edge trimmed
            shiftAmount = rightDelta;
            shiftAfter = oldOut; // shift clips that were after the old out point
        } else if (std::abs(leftDelta) > 0.001 && std::abs(rightDelta) < 0.001) {
            // Left edge trimmed — no downstream ripple needed
            shiftAmount = 0;
        }

        if (std::abs(shiftAmount) > 0.001 && trackIdx >= 0 &&
            trackIdx < static_cast<int>(state.project->tracks.size())) {
            auto& track = state.project->tracks[trackIdx];
            for (auto& c : track.clips) {
                if (c.id == clipId) continue;
                if (c.timelineIn >= shiftAfter - 0.01) {
                    c.timelineIn  += shiftAmount;
                    c.timelineOut += shiftAmount;
                }
            }
            state.trackGeneration++;
            ops_->setState(std::move(state));
        }
    }

    ops_->pushUndo(before);
}

void TrackClipDelegate::splitClipAtPlayhead() {
    auto state = ops_->currentState();
    if (!state.project) return;
    const double pos = state.playback.positionSeconds;

    // Find clip under playhead (search all tracks)
    for (auto& track : state.project->tracks) {
        for (size_t i = 0; i < track.clips.size(); ++i) {
            auto& clip = track.clips[i];
            if (pos > clip.timelineIn + 0.001 && pos < clip.timelineOut - 0.001) {
                auto before = *state.project;

                // Calculate source split point accounting for speed
                const double timelineElapsed = pos - clip.timelineIn;
                const double sourceElapsed   = timelineElapsed * clip.speed;

                VideoClip secondHalf    = clip;
                secondHalf.id           = nextClipId();
                secondHalf.timelineIn   = pos;
                secondHalf.sourceIn     = clip.sourceIn + sourceElapsed;
                // secondHalf keeps original timelineOut, sourceOut, and all properties

                clip.timelineOut = pos;
                clip.sourceOut   = clip.sourceIn + sourceElapsed;

                const int newClipId = secondHalf.id;
                track.clips.insert(track.clips.begin() + static_cast<int>(i) + 1,
                                   std::move(secondHalf));

                qDebug() << "[TrackClipDelegate] split clip" << clip.id
                         << "at" << pos << "→ new clip" << newClipId
                         << "sourceElapsed:" << sourceElapsed
                         << "speed:" << clip.speed;

                // Auto-select the second half so the user can immediately
                // delete/move it (matches Premiere/Resolve behavior)
                state.selectedClipId = newClipId;
                state.trackGeneration++;
                state.selectionGeneration++;
                ops_->setState(std::move(state));
                ops_->pushUndo(before);
                ops_->syncNativeToProject();
                ops_->renderCurrentFrame();
                return;
            }
        }
    }
    qDebug() << "[TrackClipDelegate] splitClipAtPlayhead: no clip under playhead at" << pos;
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

                state.trackGeneration++;
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
    state.selectionGeneration++;
    ops_->setState(std::move(state));
}

void TrackClipDelegate::deselectClip() {
    auto state = ops_->currentState();
    state.selectedClipId.reset();
    state.selectionGeneration++;
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

    state.trackGeneration++;
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
    // Use 0.01s tolerance to prevent false positives at snap edges
    // where floating-point imprecision can cause adjacent clips to
    // appear to overlap by sub-millisecond amounts.
    constexpr double kTolerance = 0.01;
    for (const auto& clip : track.clips) {
        if (clip.id == excludeClipId) continue;
        if (inPt < clip.timelineOut - kTolerance && outPt > clip.timelineIn + kTolerance)
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

// ---------------------------------------------------------------------------
// Clip color tag / custom label
// ---------------------------------------------------------------------------

void TrackClipDelegate::setClipColorTag(int clipId, const QString& colorHex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&colorHex](const VideoClip& c) {
        return c.copyWith({.colorTag = colorHex});
    });
    state = ops_->currentState();
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::clearClipColorTag(int clipId) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [](const VideoClip& c) {
        return c.copyWith({.clearColorTag = true});
    });
    state = ops_->currentState();
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::setClipCustomLabel(int clipId, const QString& label) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&label](const VideoClip& c) {
        return c.copyWith({.customLabel = label});
    });
    state = ops_->currentState();
    state.selectionGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

// ---------------------------------------------------------------------------
// Track color
// ---------------------------------------------------------------------------

void TrackClipDelegate::setTrackColor(int trackIndex, const QString& colorHex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    if (trackIndex < 0 || trackIndex >= state.project->tracks.size()) return;
    auto before = *state.project;

    state.project->tracks[trackIndex] =
        state.project->tracks[trackIndex].copyWith({.color = colorHex});
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void TrackClipDelegate::clearTrackColor(int trackIndex) {
    auto state = ops_->currentState();
    if (!state.project) return;
    if (trackIndex < 0 || trackIndex >= state.project->tracks.size()) return;
    auto before = *state.project;

    state.project->tracks[trackIndex] =
        state.project->tracks[trackIndex].copyWith({.clearColor = true});
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

} // namespace gopost::video_editor
