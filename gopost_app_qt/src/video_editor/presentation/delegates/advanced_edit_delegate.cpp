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
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
}

void AdvancedEditDelegate::overwriteEdit(int clipId, double atTime) {
    Q_UNUSED(clipId); Q_UNUSED(atTime);
    // Overwrite replaces content at atTime without shifting
    // TODO: Full implementation requires removing overlapping portions
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
    if (speed <= 0.0) return;
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [speed](const VideoClip& c) {
        VideoClip u = c;
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
        u.speed = -c.speed; // Negative speed indicates reverse
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
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&preset](const VideoClip& c) {
        VideoClip u = c;
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

    TimelineMarker marker;
    marker.id              = nextMarkerId();
    marker.type            = type;
    marker.label           = label;
    marker.positionSeconds = state.playback.positionSeconds;
    state.project->markers.append(std::move(marker));
    ops_->setState(std::move(state));
}

void AdvancedEditDelegate::removeMarker(int markerId) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& markers = state.project->markers;
    markers.removeIf([markerId](const TimelineMarker& m) {
        return m.id == markerId;
    });
    ops_->setState(std::move(state));
}

void AdvancedEditDelegate::updateMarker(int markerId, const QString& label) {
    auto state = ops_->currentState();
    if (!state.project) return;
    for (auto& m : state.project->markers) {
        if (m.id == markerId) { m.label = label; break; }
    }
    ops_->setState(std::move(state));
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
