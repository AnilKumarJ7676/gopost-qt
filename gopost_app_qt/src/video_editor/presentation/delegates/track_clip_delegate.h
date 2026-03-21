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

    QList<int> addClipsBatch(int trackIndex,
                             const QList<ClipSourceType>& sourceTypes,
                             const QStringList& sourcePaths,
                             const QStringList& displayNames,
                             const QList<double>& durations);

    void removeClip(int clipId, bool ripple = false);
    void moveClip(int clipId, int toTrackIndex, double toTimelineIn);
    void trimClip(int clipId, double newIn, double newOut);
    void splitClipAtPlayhead();
    void splitClipAtPosition(int clipId, double timelinePosition);
    void rippleDelete(int clipId);
    void duplicateClip(int clipId);

    // ---- selection ---------------------------------------------------------
    void selectClip(int clipId);
    void deselectClip();
    std::optional<int> clipUnderPlayhead() const;
    void autoSelectClipIfNeeded();

    // ---- gap management ----------------------------------------------------
    void closeTrackGaps(int trackIndex);
    void deleteAllGaps();
    void closeGapAt(int trackIndex, double gapStart);
    void insertSlugAt(int trackIndex, double gapStart, double gapDuration);

    // ---- clip update helper ------------------------------------------------
    void updateClipDuration(int clipId, double duration);

    // ---- clip color / label ------------------------------------------------
    void setClipColorTag(int clipId, const QString& colorHex);
    void clearClipColorTag(int clipId);
    void setClipCustomLabel(int clipId, const QString& label);

    // ---- track color -------------------------------------------------------
    void setTrackColor(int trackIndex, const QString& colorHex);
    void clearTrackColor(int trackIndex);

    // Next available clip id (public for splitAllAtPlayhead)
    int nextClipId() const;

private:
    TimelineOperations* ops_;

    // Collision detection
    bool hasCollision(const VideoTrack& track, double inPt, double outPt,
                      int excludeClipId = -1) const;

    // Magnetic snapping
    double magneticSnap(const VideoTrack& track, double position,
                        int excludeClipId = -1) const;

    // Magnetic timeline helpers (public for use by TimelineNotifier)
public:
    static void compactTrack(QList<VideoClip>& clips);
    void moveConnectedClips(int primaryClipId, double deltaTime);
private:
    bool isMagneticPrimaryTrack(int trackIndex) const;
};

} // namespace gopost::video_editor
