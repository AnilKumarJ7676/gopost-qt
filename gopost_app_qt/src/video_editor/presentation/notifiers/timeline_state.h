#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <optional>

#include "video_editor/domain/models/video_project.h"

namespace gopost::video_editor {
Q_NAMESPACE

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

enum class TimelinePhase { idle, initializing, ready, error };
Q_ENUM_NS(TimelinePhase)

enum class BottomPanelTab {
    timeline, inspector, text, effects, colorGrading,
    transitions, transform, speed, keyframes, audio,
    markers, adjustmentLayers
};
Q_ENUM_NS(BottomPanelTab)

// ---------------------------------------------------------------------------
// PlaybackState (sub-struct exposed as grouped Q_PROPERTYs)
// ---------------------------------------------------------------------------
struct PlaybackState {
    bool   isPlaying        = false;
    double positionSeconds  = 0.0;
    std::optional<double> inPoint;
    std::optional<double> outPoint;
};

// ---------------------------------------------------------------------------
// TimelineState
// ---------------------------------------------------------------------------
struct TimelineState {
    TimelinePhase phase = TimelinePhase::idle;
    std::shared_ptr<VideoProject> project;
    PlaybackState playback;
    std::optional<int> selectedClipId;
    double pixelsPerSecond = 80.0;
    double scrollOffset    = 0.0;
    double trackHeight     = 68.0;
    QString errorMessage;
    bool canUndo           = false;
    bool canRedo           = false;
    BottomPanelTab activePanel = BottomPanelTab::timeline;
    bool useProxyPlayback  = true;
    bool autoFitEnabled    = true;

    // Convenience
    bool isReady() const { return phase == TimelinePhase::ready; }
    const VideoClip* selectedClip() const;
    const QList<VideoTrack>& tracks() const;
};

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
inline constexpr int    kPreviewWidth       = 640;
inline constexpr int    kPreviewHeight      = 360;
inline constexpr double kDefaultFps         = 30.0;
inline constexpr double kMinTrackHeight     = 24.0;
inline constexpr double kMaxTrackHeight     = 200.0;
inline constexpr double kDefaultTrackHeight = 68.0;

} // namespace gopost::video_editor
