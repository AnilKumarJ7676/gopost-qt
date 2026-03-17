#pragma once

#include <functional>
#include <optional>
#include <QString>

#include "video_editor/presentation/delegates/timeline_operations.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// TrackClipDelegate — track CRUD, clip add/remove/move/trim/split, selection
//
// Converted 1:1 from track_clip_delegate.dart.
// Plain C++ class (not QObject).
// ---------------------------------------------------------------------------
class TrackClipDelegate {
public:
    // Magnetic snap threshold (seconds)
    static constexpr double kMagneticSnapThreshold = 0.1;

    explicit TrackClipDelegate(TimelineOperations* ops);
    ~TrackClipDelegate();

    // ---- track management --------------------------------------------------
    void addTrack(TrackType type);
    void removeTrack(int trackIndex);
    void toggleTrackVisibility(int trackIndex);
    void toggleTrackLock(int trackIndex);
    void toggleTrackMute(int trackIndex);
    void toggleTrackSolo(int trackIndex);

    // ---- clip management ---------------------------------------------------
    /// Returns the new clip id, or -1 on failure.
    int addClip(int trackIndex,
                ClipSourceType sourceType,
                const QString& sourcePath,
                const QString& displayName,
                double duration);

    void removeClip(int clipId, bool ripple = false);
    void moveClip(int clipId, int toTrackIndex, double toTimelineIn);
    void trimClip(int clipId, double newIn, double newOut);
    void splitClipAtPlayhead();
    void rippleDelete(int clipId);
    void duplicateClip(int clipId);

    // ---- selection ---------------------------------------------------------
    void selectClip(int clipId);
    void deselectClip();
    std::optional<int> clipUnderPlayhead() const;
    void autoSelectClipIfNeeded();

    // ---- gap management ----------------------------------------------------
    void closeTrackGaps(int trackIndex);

    // ---- clip update helper ------------------------------------------------
    void updateClipDuration(int clipId, double duration);

private:
    TimelineOperations* ops_;

    // Collision detection
    bool hasCollision(const VideoTrack& track, double inPt, double outPt,
                      int excludeClipId = -1) const;

    // Magnetic snapping
    double magneticSnap(const VideoTrack& track, double position,
                        int excludeClipId = -1) const;

    // Next available clip id
    int nextClipId() const;
};

} // namespace gopost::video_editor
