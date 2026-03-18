#include "video_editor/presentation/delegates/advanced_edit_delegate.h"
#include "video_editor/data/services/proxy_generation_service.h"

#include <QDebug>
#include <algorithm>
#include <cmath>

namespace gopost::video_editor {

AdvancedEditDelegate::AdvancedEditDelegate(TimelineOperations* ops) : ops_(ops) {}
AdvancedEditDelegate::~AdvancedEditDelegate() = default;

// ---------------------------------------------------------------------------
// NLE edits
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::insertEdit(int clipId, double atTime) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    // Find the clip being inserted so we know its duration
    const VideoClip* srcClip = state.project->findClip(clipId);
    if (!srcClip) return;
    const double insertDur = srcClip->duration();

    for (auto& track : state.project->tracks) {
        for (auto& clip : track.clips) {
            if (clip.id != clipId && clip.timelineIn >= atTime) {
                clip.timelineIn  += insertDur;
                clip.timelineOut += insertDur;
            }
        }
    }
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::overwriteEdit(int clipId, double atTime) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    const VideoClip* srcClip = state.project->findClip(clipId);
    if (!srcClip) return;
    const double insertDur = srcClip->duration();
    const double rangeEnd = atTime + insertDur;
    const int trackIdx = srcClip->trackIndex;

    if (trackIdx < 0 || trackIdx >= static_cast<int>(state.project->tracks.size()))
        return;

    auto& track = state.project->tracks[trackIdx];
    QList<VideoClip> kept;
    int maxId = 0;
    for (const auto& t : state.project->tracks)
        for (const auto& c : t.clips) maxId = std::max(maxId, c.id);

    for (auto& clip : track.clips) {
        if (clip.id == clipId) { kept.append(clip); continue; }

        // Fully inside overwrite range → remove
        if (clip.timelineIn >= atTime && clip.timelineOut <= rangeEnd) continue;

        // Overlaps from left
        if (clip.timelineIn < atTime && clip.timelineOut > atTime && clip.timelineOut <= rangeEnd) {
            clip.sourceOut = clip.sourceIn + (atTime - clip.timelineIn) * clip.speed;
            clip.timelineOut = atTime;
            kept.append(clip); continue;
        }

        // Overlaps from right
        if (clip.timelineIn >= atTime && clip.timelineIn < rangeEnd && clip.timelineOut > rangeEnd) {
            double trimAmt = rangeEnd - clip.timelineIn;
            clip.sourceIn += trimAmt * clip.speed;
            clip.timelineIn = rangeEnd;
            kept.append(clip); continue;
        }

        // Straddles entire range → split into two
        if (clip.timelineIn < atTime && clip.timelineOut > rangeEnd) {
            VideoClip left = clip;
            left.sourceOut = left.sourceIn + (atTime - left.timelineIn) * left.speed;
            left.timelineOut = atTime;
            kept.append(left);

            VideoClip right = clip;
            right.id = ++maxId;
            right.sourceIn += (rangeEnd - clip.timelineIn) * right.speed;
            right.timelineIn = rangeEnd;
            kept.append(right);
            continue;
        }

        // No overlap
        kept.append(clip);
    }
    track.clips = kept;

    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
    ops_->syncNativeToProject();
}

void AdvancedEditDelegate::rollEdit(int clipId, bool leftEdge, double delta) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [leftEdge, delta](const VideoClip& c) {
        VideoClip u = c;
        if (leftEdge) {
            u.timelineIn += delta;
            u.sourceIn   += delta;
        } else {
            u.timelineOut += delta;
            u.sourceOut   += delta;
        }
        return u;
    });
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::slipEdit(int clipId, double delta) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [delta](const VideoClip& c) {
        VideoClip u = c;
        u.sourceIn  += delta;
        u.sourceOut += delta;
        return u;
    });
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::slideEdit(int clipId, double delta) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [delta](const VideoClip& c) {
        VideoClip u = c;
        u.timelineIn  += delta;
        u.timelineOut += delta;
        return u;
    });
    ops_->pushUndo(before);
}

// ---------------------------------------------------------------------------
// Rate stretch
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::rateStretch(int clipId, double newDuration) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [newDuration](const VideoClip& c) {
        VideoClip u = c;
        u.speed       = c.duration() / newDuration;
        u.timelineOut = u.timelineIn + newDuration;
        return u;
    });
    ops_->pushUndo(before);
}

// ---------------------------------------------------------------------------
// Duplicate
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::duplicateClip(int clipId) {
    auto state = ops_->currentState();
    if (!state.project) return;

    for (const auto& track : state.project->tracks) {
        for (const auto& clip : track.clips) {
            if (clip.id == clipId) {
                auto before = *state.project;
                VideoClip dup = clip;

                // Next id
                int maxId = 0;
                for (const auto& t : state.project->tracks)
                    for (const auto& c : t.clips)
                        maxId = std::max(maxId, c.id);
                dup.id = maxId + 1;
                dup.timelineIn  = clip.timelineOut;
                dup.timelineOut = dup.timelineIn + clip.duration();

                state.project->tracks[clip.trackIndex].clips.append(std::move(dup));
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
// Speed controls
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::setClipSpeed(int clipId, double speed) {
    qDebug() << "[AdvancedEdit] setClipSpeed: clipId=" << clipId << "speed=" << speed;
    if (speed <= 0.0) return;
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [speed](const VideoClip& c) {
        VideoClip u = c;
        qDebug() << "[AdvancedEdit] setClipSpeed: oldSpeed=" << c.speed
                 << "oldDuration=" << c.duration() << "newSpeed=" << speed;
        const double origDuration = c.duration() * c.speed; // source duration
        u.speed    = speed;
        u.timelineOut = u.timelineIn + (origDuration / speed);
        return u;
    });
    ops_->pushUndo(before);
    ops_->syncNativeToProject();
}

void AdvancedEditDelegate::reverseClip(int clipId) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [](const VideoClip& c) {
        VideoClip u = c;
        u.isReversed = !c.isReversed;
        return u;
    });
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::freezeFrame(int clipId) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [](const VideoClip& c) {
        // Simple freeze: set speed to near-zero (engine interprets as freeze)
        VideoClip u = c;
        u.speed = 0.001;
        return u;
    });
    ops_->pushUndo(before);
}

// ---------------------------------------------------------------------------
// Speed ramp
// ---------------------------------------------------------------------------

std::vector<SpeedRampPreset> AdvancedEditDelegate::speedRampPresets() {
    return {
        {QStringLiteral("Smooth Slow-Mo"),  {{0.0, 1.0}, {0.3, 0.25}, {0.7, 0.25}, {1.0, 1.0}}},
        {QStringLiteral("Speed Up"),        {{0.0, 0.5}, {1.0, 3.0}}},
        {QStringLiteral("Slow Down"),       {{0.0, 3.0}, {1.0, 0.5}}},
        {QStringLiteral("Pulse"),           {{0.0, 1.0}, {0.25, 0.3}, {0.5, 1.0}, {0.75, 0.3}, {1.0, 1.0}}},
        {QStringLiteral("Dramatic Impact"), {{0.0, 1.0}, {0.4, 1.0}, {0.5, 0.1}, {0.6, 0.1}, {0.7, 2.0}, {1.0, 1.0}}},
    };
}

void AdvancedEditDelegate::applySpeedRamp(int clipId, const SpeedRampPreset& preset) {
    qDebug() << "[AdvancedEdit] applySpeedRamp: clipId=" << clipId << "preset=" << preset.name;
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&preset](const VideoClip& c) {
        VideoClip u = c;
        qDebug() << "[AdvancedEdit] applySpeedRamp: clipDuration=" << c.duration()
                 << "keyframeCount=" << preset.keyframes.size();
        // Build a keyframe track for the Speed property from the preset
        KeyframeTrack speedTrack;
        speedTrack.property = KeyframeProperty::Speed;
        for (const auto& [normTime, speed] : preset.keyframes) {
            speedTrack.keyframes.append(Keyframe{normTime * c.duration(), speed});
        }
        u.keyframes = u.keyframes.updateTrack(speedTrack);
        return u;
    });
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::addSpeedRampPoint(int clipId, double normalizedTime, double speed) {
    if (speed <= 0.0) speed = 0.01;
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [normalizedTime, speed](const VideoClip& c) {
        VideoClip u = c;
        double absTime = normalizedTime * c.duration();
        auto* existing = u.keyframes.trackFor(KeyframeProperty::Speed);
        KeyframeTrack track = existing
            ? existing->addKeyframe({absTime, speed, KeyframeInterpolation::EaseInOut})
            : KeyframeTrack{KeyframeProperty::Speed, {{absTime, speed, KeyframeInterpolation::EaseInOut}}};
        if (!existing) {
            // Ensure we have start and end points for a valid ramp
            if (absTime > 0.01)
                track = track.addKeyframe({0.0, c.speed, KeyframeInterpolation::EaseInOut});
            if (absTime < c.duration() - 0.01)
                track = track.addKeyframe({c.duration(), c.speed, KeyframeInterpolation::EaseInOut});
        }
        u.keyframes = u.keyframes.updateTrack(track);
        return u;
    });
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::removeSpeedRampPoint(int clipId, double time) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [time](const VideoClip& c) {
        VideoClip u = c;
        const auto* track = u.keyframes.trackFor(KeyframeProperty::Speed);
        if (track) {
            auto updated = track->removeKeyframeAt(time);
            u.keyframes = u.keyframes.updateTrack(updated);
        }
        return u;
    });
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::moveSpeedRampPoint(int clipId, double oldTime, double newTime, double newSpeed) {
    if (newSpeed <= 0.0) newSpeed = 0.01;
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [oldTime, newTime, newSpeed](const VideoClip& c) {
        VideoClip u = c;
        const auto* track = u.keyframes.trackFor(KeyframeProperty::Speed);
        if (track) {
            auto updated = track->moveKeyframe(oldTime, newTime, newSpeed);
            u.keyframes = u.keyframes.updateTrack(updated);
        }
        return u;
    });
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::clearSpeedRamp(int clipId) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [](const VideoClip& c) {
        VideoClip u = c;
        u.keyframes = u.keyframes.removeTrack(KeyframeProperty::Speed);
        return u;
    });
    ops_->pushUndo(before);
}

// ---------------------------------------------------------------------------
// Markers
// ---------------------------------------------------------------------------

int AdvancedEditDelegate::nextMarkerId() const {
    const auto& state = ops_->currentState();
    if (!state.project) return 1;
    int maxId = 0;
    for (const auto& m : state.project->markers)
        maxId = std::max(maxId, m.id);
    return maxId + 1;
}

void AdvancedEditDelegate::addMarker(MarkerType type, const QString& label) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    TimelineMarker marker;
    marker.id              = nextMarkerId();
    marker.type            = type;
    marker.label           = label.isEmpty() ? QStringLiteral("Marker") : label;
    marker.positionSeconds = state.playback.positionSeconds;
    marker.color           = TimelineMarker::defaultColorForType(type);
    state.project->markers.append(std::move(marker));
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::addMarkerAtPosition(double positionSeconds, MarkerType type,
                                                const QString& label, const QString& color) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    TimelineMarker marker;
    marker.id              = nextMarkerId();
    marker.type            = type;
    marker.label           = label.isEmpty() ? QStringLiteral("Marker") : label;
    marker.positionSeconds = positionSeconds;
    marker.color           = color.isEmpty() ? TimelineMarker::defaultColorForType(type) : color;
    state.project->markers.append(std::move(marker));
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::addClipMarker(int clipId, MarkerType type, const QString& label) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    const auto* clip = state.project->findClip(clipId);
    if (!clip) return;

    TimelineMarker marker;
    marker.id              = nextMarkerId();
    marker.type            = type;
    marker.label           = label.isEmpty() ? QStringLiteral("Clip Marker") : label;
    marker.positionSeconds = state.playback.positionSeconds;
    marker.color           = TimelineMarker::defaultColorForType(type);
    marker.clipId          = clipId;
    state.project->markers.append(std::move(marker));
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::addMarkerRegion(double startSeconds, double endSeconds,
                                            MarkerType type, const QString& label) {
    auto state = ops_->currentState();
    if (!state.project) return;
    if (endSeconds <= startSeconds) return;
    auto before = *state.project;

    TimelineMarker marker;
    marker.id                 = nextMarkerId();
    marker.type               = type;
    marker.label              = label.isEmpty() ? QStringLiteral("Region") : label;
    marker.positionSeconds    = startSeconds;
    marker.endPositionSeconds = endSeconds;
    marker.color              = TimelineMarker::defaultColorForType(type);
    state.project->markers.append(std::move(marker));
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::removeMarker(int markerId) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;
    auto& markers = state.project->markers;
    markers.removeIf([markerId](const TimelineMarker& m) {
        return m.id == markerId;
    });
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::updateMarker(int markerId, const QString& label) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;
    for (auto& m : state.project->markers) {
        if (m.id == markerId) { m.label = label; break; }
    }
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::updateMarkerFull(int markerId, const QString& label,
                                             const QString& notes, const QString& color, int type) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;
    for (auto& m : state.project->markers) {
        if (m.id == markerId) {
            if (!label.isEmpty()) m.label = label;
            m.notes = notes;
            if (!color.isEmpty()) m.color = color;
            if (type >= 0 && type <= 3) m.type = static_cast<MarkerType>(type);
            break;
        }
    }
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::updateMarkerPosition(int markerId, double positionSeconds) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;
    for (auto& m : state.project->markers) {
        if (m.id == markerId) {
            double delta = positionSeconds - m.positionSeconds;
            m.positionSeconds = positionSeconds;
            if (m.endPositionSeconds.has_value())
                m.endPositionSeconds = *m.endPositionSeconds + delta;
            break;
        }
    }
    state.trackGeneration++;
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::navigateToMarker(int markerId) {
    const auto& state = ops_->currentState();
    if (!state.project) return;
    for (const auto& m : state.project->markers) {
        if (m.id == markerId) {
            auto s = state;
            s.playback.positionSeconds = m.positionSeconds;
            ops_->setState(std::move(s));
            ops_->updateActiveVideo();
            ops_->renderCurrentFrame();
            return;
        }
    }
}

void AdvancedEditDelegate::navigateToNextMarker() {
    const auto& state = ops_->currentState();
    if (!state.project) return;
    const double pos = state.playback.positionSeconds;

    double nearest = -1;
    for (const auto& m : state.project->markers) {
        if (m.positionSeconds > pos + 0.01) {
            if (nearest < 0 || m.positionSeconds < nearest)
                nearest = m.positionSeconds;
        }
    }
    if (nearest >= 0) {
        auto s = state;
        s.playback.positionSeconds = nearest;
        ops_->setState(std::move(s));
        ops_->updateActiveVideo();
        ops_->renderCurrentFrame();
    }
}

void AdvancedEditDelegate::navigateToPreviousMarker() {
    const auto& state = ops_->currentState();
    if (!state.project) return;
    const double pos = state.playback.positionSeconds;

    double nearest = -1;
    for (const auto& m : state.project->markers) {
        if (m.positionSeconds < pos - 0.01) {
            if (nearest < 0 || m.positionSeconds > nearest)
                nearest = m.positionSeconds;
        }
    }
    if (nearest >= 0) {
        auto s = state;
        s.playback.positionSeconds = nearest;
        ops_->setState(std::move(s));
        ops_->updateActiveVideo();
        ops_->renderCurrentFrame();
    }
}

// ---------------------------------------------------------------------------
// Project dimensions
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::setProjectDimensions(int width, int height) {
    auto state = ops_->currentState();
    if (!state.project) return;
    state.project->width  = width;
    state.project->height = height;
    ops_->setState(std::move(state));
}

// ---------------------------------------------------------------------------
// Engine pass-through — log + sync effects to engine
//
// These methods receive JSON from QML UI panels and forward to the native
// engine via syncEffectsToEngine(). The clip's effect/color data is already
// stored in VideoClip::effects and VideoClip::colorGrading. These methods
// serve as the trigger to push that data to the engine.
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::setEngineEffect(int clipId, const QString& effectJson) {
    qDebug() << "[AdvancedEdit] setEngineEffect: clip=" << clipId
             << "payload=" << effectJson.left(120);
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void AdvancedEditDelegate::setEngineMask(int clipId, const QString& maskJson) {
    qDebug() << "[AdvancedEdit] setEngineMask: clip=" << clipId
             << "payload=" << maskJson.left(120);
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void AdvancedEditDelegate::setEngineTracking(int clipId, const QString& trackingJson) {
    qDebug() << "[AdvancedEdit] setEngineTracking: clip=" << clipId
             << "payload=" << trackingJson.left(120);
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void AdvancedEditDelegate::setEngineStabilization(int clipId, bool enabled) {
    qDebug() << "[AdvancedEdit] setEngineStabilization: clip=" << clipId
             << "enabled=" << enabled;
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void AdvancedEditDelegate::setEngineText(int clipId, const QString& textJson) {
    qDebug() << "[AdvancedEdit] setEngineText: clip=" << clipId
             << "payload=" << textJson.left(120);
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void AdvancedEditDelegate::setEngineShape(int clipId, const QString& shapeJson) {
    qDebug() << "[AdvancedEdit] setEngineShape: clip=" << clipId
             << "payload=" << shapeJson.left(120);
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void AdvancedEditDelegate::setEngineAudioEffect(int clipId, const QString& audioFxJson) {
    qDebug() << "[AdvancedEdit] setEngineAudioEffect: clip=" << clipId
             << "payload=" << audioFxJson.left(120);
    ops_->syncEffectsToEngine(clipId);
}

// ---------------------------------------------------------------------------
// AI segmentation
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::runAiSegmentation(int clipId) {
    qDebug() << "[AdvancedEdit] runAiSegmentation: clip=" << clipId;
    // Future: delegate to engine AI segmentation API
}

// ---------------------------------------------------------------------------
// Proxy management
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::generateProxyForClip(int clipId) {
    auto* proxy = ops_->proxyService();
    if (!proxy) {
        qDebug() << "[AdvancedEditDelegate] No proxy service available";
        return;
    }

    const auto& state = ops_->currentState();
    if (!state.project) return;
    const auto* clip = state.project->findClip(clipId);
    if (!clip || clip->sourcePath.isEmpty()) return;

    qDebug() << "[AdvancedEditDelegate] Generating proxy for clip" << clipId
             << "source:" << clip->sourcePath;

    auto result = proxy->generateProxy(clip->sourcePath);
    if (result.has_value()) {
        ops_->updateClip(clipId, [&result](const VideoClip& c) {
            VideoClip u = c;
            u.proxyPath = *result;
            return u;
        });
        qDebug() << "[AdvancedEditDelegate] Proxy stored:" << *result;
    } else {
        qWarning() << "[AdvancedEditDelegate] Proxy generation failed for clip" << clipId;
    }
}

void AdvancedEditDelegate::toggleProxyPlayback() {
    auto state = ops_->currentState();
    state.useProxyPlayback = !state.useProxyPlayback;
    ops_->setState(std::move(state));
}

// ---------------------------------------------------------------------------
// Multi-cam
// ---------------------------------------------------------------------------

void AdvancedEditDelegate::enableMultiCam(const std::vector<int>& clipIds) {
    Q_UNUSED(clipIds);
    // TODO: Multi-cam setup
}

void AdvancedEditDelegate::switchMultiCamAngle(int angleIndex) {
    Q_UNUSED(angleIndex);
    // TODO: Multi-cam angle switch
}

} // namespace gopost::video_editor
