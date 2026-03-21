#pragma once

#include <QObject>
#include <QSet>
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

enum class TrimEditMode : int { Normal = 0, Ripple, Roll, Slip, Slide };
Q_ENUM_NS(TrimEditMode)

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
    QSet<int> selectedClipIds;        // Feature 13: multi-select clip IDs
    double pixelsPerSecond = 80.0;
    double scrollOffset    = 0.0;
    double trackHeight     = 68.0;
    QString errorMessage;
    bool canUndo           = false;
    bool canRedo           = false;
    BottomPanelTab activePanel = BottomPanelTab::timeline;
    bool useProxyPlayback  = true;
    bool autoFitEnabled    = true;

    // Ripple mode: when enabled, deleting or trimming clips auto-closes gaps
    bool rippleMode = false;

    // Insert mode: true=insert (push clips right), false=overwrite (replace underneath)
    bool insertMode = true;

    // Trim/edit mode (Normal, Ripple, Roll, Slip, Slide)
    TrimEditMode trimEditMode = TrimEditMode::Normal;

    // Waveform display
    bool showWaveforms = true;
    bool waveformStereo = false;  // false=mono, true=stereo L/R

    // Duplicate frame detection overlay
    bool showDuplicateFrames = false;

    // Razor/Blade tool mode
    bool razorModeEnabled = false;

    // Magnetic timeline (FCP-style): auto-ripple on primary storyline
    bool magneticTimelineEnabled = true;
    bool positionOverrideActive = false;  // hold P key to allow free placement

    // Generation counters for fine-grained change tracking.
    // Delegates bump these before calling setState() so the notifier
    // can diff and emit only the signals whose category actually changed.
    int trackGeneration     = 0;   // tracks, clips, markers structure
    int selectionGeneration = 0;   // selected clip property edits

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
