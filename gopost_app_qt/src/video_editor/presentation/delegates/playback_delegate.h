#pragma once

#include <QTimer>
#include <chrono>
#include <deque>
#include <functional>

#include "video_editor/presentation/delegates/timeline_operations.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// PlaybackDelegate — play/pause, seek, shuttle, zoom, snap, render pipeline
//
// Converted 1:1 from playback_delegate.dart.
// Plain C++ class (not QObject); timer callbacks are forwarded from the
// owning TimelineNotifier which *is* a QObject.
// ---------------------------------------------------------------------------
class PlaybackDelegate {
public:
    // Constants
    static constexpr double kMinZoom        = 0.01;
    static constexpr double kMaxZoom        = 400.0;
    static constexpr int    kPlaybackTickMs = 33;   // ~30 fps

    explicit PlaybackDelegate(TimelineOperations* ops);
    ~PlaybackDelegate();

    // ---- playback ----------------------------------------------------------
    void play();
    void pause();
    void togglePlayPause();
    void seek(double positionSeconds);
    void stepForward();
    void stepBackward();
    void jumpToStart();
    void jumpToEnd();

    // ---- JKL shuttle -------------------------------------------------------
    void shuttleForward();
    void shuttleReverse();
    void shuttleStop();

    // ---- in / out points ---------------------------------------------------
    void setInPoint();
    void clearInPoint();
    void setOutPoint();
    void clearOutPoint();

    // ---- scrub with serialised seek queue ----------------------------------
    void scrubTo(double positionSeconds);

    // ---- snap points -------------------------------------------------------
    double snapPosition(double rawPos) const;
    void   setSnapEnabled(bool enabled);
    bool   isSnapEnabled() const { return snapEnabled_; }
    void   jumpToPreviousSnapPoint();
    void   jumpToNextSnapPoint();

    // ---- zoom / scroll -----------------------------------------------------
    void   setPixelsPerSecond(double pps);
    void   setScrollOffset(double offset);
    void   zoomAtPosition(double factor, double anchorX);

    // ---- render pipeline ---------------------------------------------------
    void requestRender();

    // ---- tick (called by TimelineNotifier timer) ---------------------------
    void onPlaybackTick();

private:
    TimelineOperations* ops_;

    // Shuttle state
    int shuttleSpeed_ = 0;   // -3..-1, 0, 1..3

    // Snap
    bool snapEnabled_ = true;

    // Serialised seek queue
    struct SeekRequest { double position; };
    std::deque<SeekRequest> seekQueue_;
    bool isSeeking_ = false;
    void processSeekQueue();

    // Render coalescing
    bool isRendering_     = false;
    bool renderRequested_ = false;
    void executeRender();
};

} // namespace gopost::video_editor
