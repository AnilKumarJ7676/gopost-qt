#pragma once

#include <QTimer>
#include <chrono>
#include <deque>
#include <functional>

#include "video_editor/presentation/delegates/timeline_operations.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// PlaybackState enum — explicit state machine for the playback controller.
// ---------------------------------------------------------------------------
enum class PlaybackStatus { Stopped, Playing, Paused, Seeking };

// ---------------------------------------------------------------------------
// PlaybackDelegate — play/pause, seek, shuttle, zoom, snap, render pipeline
//
// Acts as the PlaybackController for the timeline engine.
// Uses real elapsed time (steady_clock) for frame-accurate playback that
// compensates for timer jitter and variable frame times.
// ---------------------------------------------------------------------------
class PlaybackDelegate {
public:
    // Constants
    static constexpr double kMinZoom        = 0.01;
    static constexpr double kMaxZoom        = 400.0;
    static constexpr int    kPlaybackTickMs = 33;   // ~30 fps target

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

    // ---- state queries -----------------------------------------------------
    PlaybackStatus status() const { return status_; }
    double playbackRate() const;
    double getProgress() const;
    double getDuration() const;

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

    // Playback state machine
    PlaybackStatus status_ = PlaybackStatus::Stopped;
    bool wasPlayingBeforeSeek_ = false;

    // Real-time tick tracking (compensates for timer jitter)
    std::chrono::steady_clock::time_point lastTickTime_;

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
