#pragma once

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <chrono>
#include <memory>
#include <optional>

#include "video_editor/presentation/notifiers/timeline_state.h"
#include "video_editor/presentation/delegates/timeline_operations.h"
#include "video_editor/presentation/delegates/playback_delegate.h"
#include "video_editor/presentation/delegates/track_clip_delegate.h"
#include "video_editor/presentation/delegates/effect_color_delegate.h"
#include "video_editor/presentation/delegates/keyframe_audio_delegate.h"
#include "video_editor/presentation/delegates/advanced_edit_delegate.h"

namespace gopost::rendering { class StubVideoTimelineEngine; }
namespace gopost::video_editor { class ProxyGenerationService; class ThumbnailService; class FFmpegExportService; }

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// TimelineNotifier — QObject facade wrapping all five delegates
//
// Performance-optimised signal architecture: properties are grouped into
// five NOTIFY categories so that a 30fps playback tick only causes QML
// to re-evaluate the handful of playback-related bindings, not all 30+.
// ---------------------------------------------------------------------------
class TimelineNotifier : public QObject, public TimelineOperations {
    Q_OBJECT

    // --- playback properties (change every tick during playback) -------------
    Q_PROPERTY(bool   isPlaying        READ isPlaying       NOTIFY playbackChanged)
    Q_PROPERTY(double position         READ position        NOTIFY playbackChanged)
    Q_PROPERTY(double totalDuration    READ totalDuration   NOTIFY playbackChanged)
    Q_PROPERTY(QString activeClipSource READ activeClipSource NOTIFY playbackChanged)
    Q_PROPERTY(double activeClipOffset READ activeClipOffset NOTIFY playbackChanged)
    Q_PROPERTY(QString activeAudioSource READ activeAudioSource NOTIFY playbackChanged)
    Q_PROPERTY(double activeAudioOffset READ activeAudioOffset NOTIFY playbackChanged)
    Q_PROPERTY(double activeClipSpeed  READ activeClipSpeed  NOTIFY playbackChanged)
    Q_PROPERTY(bool   activeClipReversed READ activeClipReversed NOTIFY playbackChanged)
    Q_PROPERTY(double activeClipVolume READ activeClipVolume NOTIFY playbackChanged)
    Q_PROPERTY(bool   activeClipMuted  READ activeClipMuted  NOTIFY playbackChanged)
    Q_PROPERTY(double activeAudioVolume READ activeAudioVolume NOTIFY playbackChanged)
    Q_PROPERTY(bool   activeAudioMuted  READ activeAudioMuted  NOTIFY playbackChanged)
    Q_PROPERTY(double inPoint READ inPoint NOTIFY playbackChanged)
    Q_PROPERTY(double outPoint READ outPoint NOTIFY playbackChanged)
    Q_PROPERTY(bool hasInPoint READ hasInPoint NOTIFY playbackChanged)
    Q_PROPERTY(bool hasOutPoint READ hasOutPoint NOTIFY playbackChanged)

    // --- track/clip structure (change on add/remove/move/split) --------------
    Q_PROPERTY(QVariantList tracks    READ tracksVariant   NOTIFY tracksChanged)
    Q_PROPERTY(int    trackCount       READ trackCount      NOTIFY tracksChanged)
    Q_PROPERTY(int    trackGeneration  READ trackGeneration NOTIFY tracksChanged)
    Q_PROPERTY(int    markerCount     READ markerCount     NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList markers   READ markersVariant  NOTIFY tracksChanged)

    // --- selection & clip properties (change on select/deselect/edit) --------
    Q_PROPERTY(int    selectedClipId   READ selectedClipIdProp NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectedClip READ selectedClipVariant NOTIFY selectionChanged)
    Q_PROPERTY(double clipVolume      READ clipVolume      NOTIFY selectionChanged)
    Q_PROPERTY(double clipPan         READ clipPan         NOTIFY selectionChanged)
    Q_PROPERTY(double clipFadeIn      READ clipFadeIn      NOTIFY selectionChanged)
    Q_PROPERTY(double clipFadeOut     READ clipFadeOut     NOTIFY selectionChanged)
    Q_PROPERTY(bool   clipIsMuted     READ clipIsMuted     NOTIFY selectionChanged)

    // --- zoom/scroll (change on zoom/scroll gestures) -----------------------
    Q_PROPERTY(double pixelsPerSecond  READ pixelsPerSecondProp NOTIFY zoomChanged)
    Q_PROPERTY(double scrollOffset     READ scrollOffsetProp NOTIFY zoomChanged)

    // --- frame preview (change on each rendered frame) ----------------------
    Q_PROPERTY(int frameVersion READ frameVersion NOTIFY frameReady)

    // --- meta/lifecycle (change rarely: init, undo/redo, panel switch) ------
    Q_PROPERTY(int    phase            READ phaseInt        NOTIFY stateChanged)
    Q_PROPERTY(bool   isReady          READ isReady         NOTIFY stateChanged)
    Q_PROPERTY(bool   canUndo          READ canUndo         NOTIFY stateChanged)
    Q_PROPERTY(bool   canRedo          READ canRedo         NOTIFY stateChanged)
    Q_PROPERTY(int    activePanel      READ activePanelInt  NOTIFY stateChanged)
    Q_PROPERTY(bool   useProxyPlayback READ useProxyPlayback NOTIFY stateChanged)
    Q_PROPERTY(bool   rippleMode       READ rippleMode      NOTIFY stateChanged)
    Q_PROPERTY(bool   insertMode       READ insertMode      NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage    READ errorMessage    NOTIFY stateChanged)
    Q_PROPERTY(int    projectWidth     READ projectWidth    NOTIFY stateChanged)
    Q_PROPERTY(int    projectHeight    READ projectHeight   NOTIFY stateChanged)
    Q_PROPERTY(bool   waveformEnabled  READ waveformEnabled NOTIFY stateChanged)
    Q_PROPERTY(bool   waveformStereo   READ waveformStereo  NOTIFY stateChanged)

    // --- multi-select (Feature 13) -------------------------------------------
    Q_PROPERTY(QVariantList selectedClipIds READ selectedClipIdsVariant NOTIFY selectionChanged)
    Q_PROPERTY(int  selectedClipCount READ selectedClipCount NOTIFY selectionChanged)

    // --- zoom percentage (Feature 11) ----------------------------------------
    Q_PROPERTY(double zoomPercentage READ zoomPercentage NOTIFY zoomChanged)

    // --- transitions (Feature 9) ---------------------------------------------
    Q_PROPERTY(QVariantList transitions READ transitionsVariant NOTIFY tracksChanged)

public:
    explicit TimelineNotifier(QObject* parent = nullptr);
    ~TimelineNotifier() override;

    // ---- property accessors ------------------------------------------------
    int     phaseInt()          const { return static_cast<int>(state_.phase); }
    bool    isReady()           const { return state_.isReady(); }
    bool    isPlaying()         const { return state_.playback.isPlaying; }
    double  position()          const { return state_.playback.positionSeconds; }
    int     selectedClipIdProp() const { return state_.selectedClipId.value_or(-1); }
    double  pixelsPerSecondProp() const { return state_.pixelsPerSecond; }
    double  scrollOffsetProp()  const { return state_.scrollOffset; }
    bool    canUndo()           const { return state_.canUndo; }
    bool    waveformEnabled()  const { return state_.showWaveforms; }
    bool    waveformStereo()   const { return state_.waveformStereo; }
    bool    canRedo()           const { return state_.canRedo; }
    int     activePanelInt()    const { return static_cast<int>(state_.activePanel); }
    bool    useProxyPlayback()  const { return state_.useProxyPlayback; }
    bool    rippleMode()        const { return state_.rippleMode; }
    bool    insertMode()        const { return state_.insertMode; }
    QString errorMessage()      const { return state_.errorMessage; }
    double  totalDuration()     const { return state_.project ? state_.project->duration() : 0.0; }
    int     trackCount()        const { return state_.project ? state_.project->tracks.size() : 0; }
    int     trackGeneration()   const { return state_.trackGeneration; }
    int     projectWidth()      const { return state_.project ? state_.project->width : 0; }
    int     projectHeight()     const { return state_.project ? state_.project->height : 0; }
    double  clipVolume()        const;
    double  clipPan()           const;
    double  clipFadeIn()        const;
    double  clipFadeOut()       const;
    bool    clipIsMuted()       const;
    QVariantList tracksVariant() const;
    int     markerCount()       const;
    QVariantList markersVariant() const;
    QVariantMap selectedClipVariant() const;
    int frameVersion() const { return frameVersion_; }
    QString activeClipSource() const;
    double activeClipOffset() const;
    QString activeAudioSource() const;
    double activeAudioOffset() const;
    double activeClipSpeed() const;
    bool   activeClipReversed() const;
    double activeClipVolume() const;
    bool   activeClipMuted() const;
    double activeAudioVolume() const;
    bool   activeAudioMuted() const;
    double  inPoint()    const { return state_.playback.inPoint.value_or(-1.0); }
    double  outPoint()   const { return state_.playback.outPoint.value_or(-1.0); }
    bool    hasInPoint() const { return state_.playback.inPoint.has_value(); }
    bool    hasOutPoint() const { return state_.playback.outPoint.has_value(); }

    // Feature 13: multi-select
    QVariantList selectedClipIdsVariant() const;
    int  selectedClipCount() const { return state_.selectedClipIds.size(); }

    // Feature 11: zoom percentage
    double zoomPercentage() const { return (state_.pixelsPerSecond / 80.0) * 100.0; }

    // Feature 9: transitions
    QVariantList transitionsVariant() const;

    /// Set the video frame provider (not owned — registered with QML engine).
    void setFrameProvider(class VideoFrameProvider* provider) { frameProvider_ = provider; }

    // ---- lifecycle ---------------------------------------------------------
    Q_INVOKABLE void initTimeline();
    Q_INVOKABLE void loadProject(const QString& projectJson);

    // ---- delegate facades (all Q_INVOKABLE for QML) ------------------------

    // Playback
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void seek(double pos);
    Q_INVOKABLE void stepForward();
    Q_INVOKABLE void stepBackward();
    Q_INVOKABLE void jumpToStart();
    Q_INVOKABLE void jumpToEnd();
    Q_INVOKABLE void shuttleForward();
    Q_INVOKABLE void shuttleReverse();
    Q_INVOKABLE void shuttleStop();
    Q_INVOKABLE void setInPoint();
    Q_INVOKABLE void clearInPoint();
    Q_INVOKABLE void setOutPoint();
    Q_INVOKABLE void clearOutPoint();
    Q_INVOKABLE void scrubTo(double pos);
    Q_INVOKABLE void setPixelsPerSecond(double pps);
    Q_INVOKABLE void setScrollOffset(double offset);
    Q_INVOKABLE void zoomAtPosition(double factor, double anchorX);
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void stepForwardN(int n);
    Q_INVOKABLE void stepBackwardN(int n);
    Q_INVOKABLE void clearInOutPoints();
    Q_INVOKABLE void jumpToPreviousSnapPoint();
    Q_INVOKABLE void jumpToNextSnapPoint();

    // Track / clip
    Q_INVOKABLE void addTrack(int trackType);
    Q_INVOKABLE void removeTrack(int trackIndex);
    Q_INVOKABLE void toggleTrackVisibility(int trackIndex);
    Q_INVOKABLE void toggleTrackLock(int trackIndex);
    Q_INVOKABLE void toggleTrackMute(int trackIndex);
    Q_INVOKABLE void toggleTrackSolo(int trackIndex);
    Q_INVOKABLE int  addClip(int trackIndex, int sourceType,
                             const QString& sourcePath,
                             const QString& displayName, double duration);
    Q_INVOKABLE int  clipCountForTrack(int trackIndex) const;
    Q_INVOKABLE QVariantMap clipData(int trackIndex, int clipIndex) const;
    Q_INVOKABLE void removeClip(int clipId);
    Q_INVOKABLE void toggleRippleMode();
    Q_INVOKABLE void toggleInsertMode();
    Q_INVOKABLE void reorderClipInsert(int clipId, int trackIndex, double atTime);
    Q_INVOKABLE void reorderClipOverwrite(int clipId, int trackIndex, double atTime);
    Q_INVOKABLE void moveClip(int clipId, int toTrack, double toTime);
    Q_INVOKABLE void trimClip(int clipId, double newIn, double newOut);
    Q_INVOKABLE void splitClipAtPlayhead();
    Q_INVOKABLE void rippleDelete(int clipId);
    Q_INVOKABLE void selectClip(int clipId);
    Q_INVOKABLE void deselectClip();
    Q_INVOKABLE void updateClipDuration(int clipId, double dur);
    Q_INVOKABLE void updateClipDisplayName(int clipId, const QString& name);

    // Clip color tag / custom label
    Q_INVOKABLE void setClipColorTag(int clipId, const QString& colorHex);
    Q_INVOKABLE void clearClipColorTag(int clipId);
    Q_INVOKABLE void setClipCustomLabel(int clipId, const QString& label);
    Q_INVOKABLE int  clipColorPresetCount() const;
    Q_INVOKABLE QString clipColorPresetHex(int index) const;
    Q_INVOKABLE QString clipColorPresetName(int index) const;
    Q_INVOKABLE QString autoColorForClipSourceType(int sourceType) const;

    // Track color
    Q_INVOKABLE void setTrackColor(int trackIndex, const QString& colorHex);
    Q_INVOKABLE void clearTrackColor(int trackIndex);

    // Effects / color
    Q_INVOKABLE void addEffect(int clipId, int effectType, double value);
    Q_INVOKABLE void removeEffect(int clipId, int effectType);
    Q_INVOKABLE void toggleEffect(int clipId, int effectType);
    Q_INVOKABLE void updateEffect(int clipId, int effectType, double value);
    Q_INVOKABLE void clearEffects(int clipId);
    Q_INVOKABLE void applyPresetFilter(int clipId, int presetId);
    Q_INVOKABLE void resetColorGrading(int clipId);
    Q_INVOKABLE void setColorGradingProperty(int clipId, const QString& prop, double value);
    Q_INVOKABLE void setClipOpacity(int clipId, double opacity);
    Q_INVOKABLE void setClipBlendMode(int clipId, int mode);

    // Transitions
    Q_INVOKABLE void setTransitionIn(int clipId, int transType, double dur, int easing);
    Q_INVOKABLE void setTransitionOut(int clipId, int transType, double dur, int easing);
    Q_INVOKABLE void removeTransitionIn(int clipId);
    Q_INVOKABLE void removeTransitionOut(int clipId);

    // Keyframe / audio
    Q_INVOKABLE void addKeyframe(int clipId, int prop, double time, double value);
    Q_INVOKABLE void removeKeyframe(int clipId, int prop, double time);
    Q_INVOKABLE void clearKeyframes(int clipId, int prop);
    Q_INVOKABLE void setClipVolume(int clipId, double vol);
    Q_INVOKABLE void setClipPan(int clipId, double pan);
    Q_INVOKABLE void setClipFadeIn(int clipId, double sec);
    Q_INVOKABLE void setClipFadeOut(int clipId, double sec);
    Q_INVOKABLE void setClipMuted(int clipId, bool muted);
    Q_INVOKABLE void setTrackVolume(int trackIndex, double vol);
    Q_INVOKABLE void setTrackPan(int trackIndex, double pan);

    // Advanced
    Q_INVOKABLE void setClipSpeed(int clipId, double speed);
    Q_INVOKABLE void reverseClip(int clipId);
    Q_INVOKABLE void freezeFrame(int clipId);
    Q_INVOKABLE void duplicateClip(int clipId);
    Q_INVOKABLE void rateStretch(int clipId, double newDuration);
    Q_INVOKABLE void addMarker();
    Q_INVOKABLE void addMarkerWithType(int type, const QString& label);
    Q_INVOKABLE void addMarkerAtPosition(double positionSeconds, int type,
                                          const QString& label, const QString& color = {});
    Q_INVOKABLE void addClipMarker(int clipId, int type, const QString& label);
    Q_INVOKABLE void addMarkerRegion(double startSeconds, double endSeconds,
                                      int type, const QString& label);
    Q_INVOKABLE void removeMarker(int markerId);
    Q_INVOKABLE void updateMarker(int markerId, const QString& label);
    Q_INVOKABLE void updateMarkerFull(int markerId, const QString& label,
                                       const QString& notes, const QString& color, int type);
    Q_INVOKABLE void updateMarkerPosition(int markerId, double positionSeconds);
    Q_INVOKABLE void navigateToMarker(int markerId);
    Q_INVOKABLE void navigateToNextMarker();
    Q_INVOKABLE void navigateToPreviousMarker();
    Q_INVOKABLE QVariantMap markerById(int markerId) const;
    Q_INVOKABLE void setProjectDimensions(int w, int h);
    Q_INVOKABLE void generateProxyForClip(int clipId);
    Q_INVOKABLE void toggleProxyPlayback();
    Q_INVOKABLE void toggleWaveform();
    Q_INVOKABLE void toggleWaveformStereo();
    Q_INVOKABLE void applySpeedRamp(int clipId, int presetIndex);
    Q_INVOKABLE void addSpeedRampPoint(int clipId, double normalizedTime, double speed);
    Q_INVOKABLE void removeSpeedRampPoint(int clipId, double time);
    Q_INVOKABLE void moveSpeedRampPoint(int clipId, double oldTime, double newTime, double newSpeed);
    Q_INVOKABLE void clearSpeedRamp(int clipId);
    Q_INVOKABLE bool clipHasSpeedRamp(int clipId) const;
    Q_INVOKABLE bool clipIsReversed(int clipId) const;
    Q_INVOKABLE QVariantList clipSpeedRampPoints(int clipId) const;

    // Transform / Crop stubs
    Q_INVOKABLE void setKeyframe(int clipId, int prop, double value);
    Q_INVOKABLE void resetTransform(int clipId);
    Q_INVOKABLE void applyKenBurns(int clipId, double startScale, double endScale);
    Q_INVOKABLE void applyKenBurnsPan(int clipId, double startX, double startY, double endX, double endY);

    // Adjustment layer stubs
    Q_INVOKABLE void createAdjustmentClip();
    Q_INVOKABLE void editMarker(int markerId);

    // Clip data accessors for QML panels
    Q_INVOKABLE QVariantMap clipColorGrading(int clipId) const;
    Q_INVOKABLE QVariantList clipEffects(int clipId) const;
    Q_INVOKABLE double clipSpeed(int clipId) const;
    Q_INVOKABLE double clipOpacity(int clipId) const;
    Q_INVOKABLE QVariantList clipKeyframeTimes(int clipId, int prop) const;
    Q_INVOKABLE QVariantMap clipTextData(int clipId) const;

    // Panel
    Q_INVOKABLE void setActivePanel(int panel);

    // Undo / Redo
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // ---- Feature 9: Auto-Crossfade on Overlap ------------------------------
    Q_INVOKABLE void addCrossfade(int clipAId, int clipBId, double duration, int type);
    Q_INVOKABLE void removeCrossfade(int clipAId, int clipBId);
    Q_INVOKABLE void changeCrossfadeType(int clipAId, int clipBId, int newType);
    Q_INVOKABLE QVariantMap crossfadeAt(int clipAId, int clipBId) const;

    // ---- Feature 10: Linked Audio/Video Movement ---------------------------
    Q_INVOKABLE void linkClips(int clipAId, int clipBId);
    Q_INVOKABLE void unlinkClip(int clipId);
    Q_INVOKABLE int  linkedClipId(int clipId) const;
    Q_INVOKABLE bool isClipLinked(int clipId) const;

    // ---- Feature 11: Timeline Zoom (enhanced) ------------------------------
    Q_INVOKABLE void setZoomPercentage(double percent);

    // ---- Feature 12: Split Clip (enhanced - all unlocked tracks) -----------
    Q_INVOKABLE void splitAllAtPlayhead();

    // ---- Feature 13: Multi-Select and Group Drag ---------------------------
    Q_INVOKABLE void addToSelection(int clipId);
    Q_INVOKABLE void removeFromSelection(int clipId);
    Q_INVOKABLE void toggleClipSelection(int clipId);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void deselectAll();
    Q_INVOKABLE bool isClipSelected(int clipId) const;
    Q_INVOKABLE void moveSelectedClips(double deltaTime, int deltaTrack);
    Q_INVOKABLE void deleteSelectedClips();

    // ---- Feature 15: Track header editing ----------------------------------
    Q_INVOKABLE void renameTrack(int trackIndex, const QString& name);
    Q_INVOKABLE QVariantMap trackInfo(int trackIndex) const;

    // ---- TimelineOperations interface --------------------------------------
    const TimelineState& currentState() const override { return state_; }
    void              setState(TimelineState state) override;
    NativeVideoEngine*      engine()       override;
    ProxyGenerationService* proxyService() override;
    UndoRedoStack*          undoRedo()     override;
    bool isMounted() const override { return true; }
    void pushUndo(const VideoProject& before) override;
    void syncUndoRedoState() override;
    void updateClip(int clipId,
                    std::function<VideoClip(const VideoClip&)> mutator) override;
    void renderCurrentFrame()   override;
    void debouncedRenderFrame() override;
    void throttledRenderFrame() override;
    void updateActiveVideo()    override;
    QString resolvePlaybackPath(const VideoClip& clip) const override;
    int  toEngineSourceType(ClipSourceType t) const override;
    void syncNativeToProject()  override;
    void restoreClipS10State(const VideoClip& clip) override;
    void syncEffectsToEngine(int clipId) override;

signals:
    void stateChanged();       // meta/lifecycle properties only
    void playbackChanged();    // position, isPlaying, in/out points
    void tracksChanged();      // track list, clip list, markers
    void selectionChanged();   // selected clip, clip audio props
    void zoomChanged();        // pixelsPerSecond, scrollOffset
    void frameReady();         // new preview frame available

private:
    TimelineState state_;
    int frameVersion_ = 0;
    class VideoFrameProvider* frameProvider_ = nullptr;
    QTimer playbackTimer_;

    // Delegates
    std::unique_ptr<PlaybackDelegate>     playback_;
    std::unique_ptr<TrackClipDelegate>    trackClip_;
    std::unique_ptr<EffectColorDelegate>  effectColor_;
    std::unique_ptr<KeyframeAudioDelegate> keyframeAudio_;
    std::unique_ptr<AdvancedEditDelegate>  advancedEdit_;

    // Undo/redo stack (simple project snapshots)
    struct UndoStack;
    std::unique_ptr<UndoStack> undoStack_;

    // --- Render throttle (Phase 3) ------------------------------------------
    std::chrono::steady_clock::time_point lastRenderTime_;
    QTimer renderThrottleTimer_;
    static constexpr int kRenderIntervalMs = 33;

    // --- Double-buffered preview frames (Phase 2B) --------------------------
    QImage frameBuffers_[2];
    int currentBuffer_ = 0;

    // --- Previous-state snapshots for diffing (Phase 1B) --------------------
    double prevPosition_      = 0.0;
    bool   prevIsPlaying_     = false;
    std::optional<double> prevInPoint_;
    std::optional<double> prevOutPoint_;
    int    prevTrackGen_      = -1;
    int    prevSelectionGen_  = -1;
    std::optional<int> prevSelectedClipId_;
    double prevPps_           = 80.0;
    double prevScrollOffset_  = 0.0;

    // --- Cached QVariant accessors (Phase 4) --------------------------------
    mutable QVariantList cachedTracks_;
    mutable QVariantList cachedMarkers_;
    mutable QVariantMap  cachedSelectedClip_;
    mutable int cachedTracksGen_    = -1;
    mutable int cachedMarkersGen_   = -1;
    mutable int cachedSelectionGen_ = -1;

    // Engine & services (owned)
    std::unique_ptr<rendering::StubVideoTimelineEngine> stubEngine_;
    int engineTimelineId_ = -1;
    video_editor::ProxyGenerationService* proxyService_ = nullptr;
};

} // namespace gopost::video_editor
