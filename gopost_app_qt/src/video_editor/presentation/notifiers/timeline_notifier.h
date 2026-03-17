#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
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
// Converted 1:1 from timeline_notifier.dart's TimelineNotifier.
// ---------------------------------------------------------------------------
class TimelineNotifier : public QObject, public TimelineOperations {
    Q_OBJECT

    // --- top-level properties -----------------------------------------------
    Q_PROPERTY(int    phase            READ phaseInt        NOTIFY stateChanged)
    Q_PROPERTY(bool   isReady          READ isReady         NOTIFY stateChanged)
    Q_PROPERTY(bool   isPlaying        READ isPlaying       NOTIFY stateChanged)
    Q_PROPERTY(double position         READ position        NOTIFY stateChanged)
    Q_PROPERTY(int    selectedClipId   READ selectedClipIdProp NOTIFY stateChanged)
    Q_PROPERTY(double pixelsPerSecond  READ pixelsPerSecondProp NOTIFY stateChanged)
    Q_PROPERTY(double scrollOffset     READ scrollOffsetProp NOTIFY stateChanged)
    Q_PROPERTY(bool   canUndo          READ canUndo         NOTIFY stateChanged)
    Q_PROPERTY(bool   canRedo          READ canRedo         NOTIFY stateChanged)
    Q_PROPERTY(int    activePanel      READ activePanelInt  NOTIFY stateChanged)
    Q_PROPERTY(bool   useProxyPlayback READ useProxyPlayback NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage    READ errorMessage    NOTIFY stateChanged)
    Q_PROPERTY(double totalDuration    READ totalDuration   NOTIFY stateChanged)
    Q_PROPERTY(int    trackCount       READ trackCount      NOTIFY stateChanged)
    Q_PROPERTY(int    projectWidth     READ projectWidth    NOTIFY stateChanged)
    Q_PROPERTY(int    projectHeight    READ projectHeight   NOTIFY stateChanged)
    Q_PROPERTY(double clipVolume      READ clipVolume      NOTIFY stateChanged)
    Q_PROPERTY(double clipPan         READ clipPan         NOTIFY stateChanged)
    Q_PROPERTY(double clipFadeIn      READ clipFadeIn      NOTIFY stateChanged)
    Q_PROPERTY(double clipFadeOut     READ clipFadeOut     NOTIFY stateChanged)
    Q_PROPERTY(bool   clipIsMuted     READ clipIsMuted     NOTIFY stateChanged)
    Q_PROPERTY(QVariantList tracks    READ tracksVariant   NOTIFY stateChanged)
    Q_PROPERTY(int    markerCount     READ markerCount     NOTIFY stateChanged)
    Q_PROPERTY(QVariantList markers   READ markersVariant  NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap selectedClip READ selectedClipVariant NOTIFY stateChanged)
    Q_PROPERTY(int frameVersion READ frameVersion NOTIFY frameReady)
    Q_PROPERTY(QString activeClipSource READ activeClipSource NOTIFY stateChanged)
    Q_PROPERTY(double activeClipOffset READ activeClipOffset NOTIFY stateChanged)
    Q_PROPERTY(double inPoint READ inPoint NOTIFY stateChanged)
    Q_PROPERTY(double outPoint READ outPoint NOTIFY stateChanged)
    Q_PROPERTY(bool hasInPoint READ hasInPoint NOTIFY stateChanged)
    Q_PROPERTY(bool hasOutPoint READ hasOutPoint NOTIFY stateChanged)

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
    bool    canRedo()           const { return state_.canRedo; }
    int     activePanelInt()    const { return static_cast<int>(state_.activePanel); }
    bool    useProxyPlayback()  const { return state_.useProxyPlayback; }
    QString errorMessage()      const { return state_.errorMessage; }
    double  totalDuration()     const { return state_.project ? state_.project->duration() : 0.0; }
    int     trackCount()        const { return state_.project ? state_.project->tracks.size() : 0; }
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
    double  inPoint()    const { return state_.playback.inPoint.value_or(-1.0); }
    double  outPoint()   const { return state_.playback.outPoint.value_or(-1.0); }
    bool    hasInPoint() const { return state_.playback.inPoint.has_value(); }
    bool    hasOutPoint() const { return state_.playback.outPoint.has_value(); }

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
    Q_INVOKABLE void moveClip(int clipId, int toTrack, double toTime);
    Q_INVOKABLE void trimClip(int clipId, double newIn, double newOut);
    Q_INVOKABLE void splitClipAtPlayhead();
    Q_INVOKABLE void rippleDelete(int clipId);
    Q_INVOKABLE void selectClip(int clipId);
    Q_INVOKABLE void deselectClip();
    Q_INVOKABLE void updateClipDuration(int clipId, double dur);
    Q_INVOKABLE void updateClipDisplayName(int clipId, const QString& name);

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
    Q_INVOKABLE void removeMarker(int markerId);
    Q_INVOKABLE void updateMarker(int markerId, const QString& label);
    Q_INVOKABLE void navigateToMarker(int markerId);
    Q_INVOKABLE void navigateToNextMarker();
    Q_INVOKABLE void navigateToPreviousMarker();
    Q_INVOKABLE void setProjectDimensions(int w, int h);
    Q_INVOKABLE void generateProxyForClip(int clipId);
    Q_INVOKABLE void toggleProxyPlayback();
    Q_INVOKABLE void applySpeedRamp(int clipId, int presetIndex);

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
    void stateChanged();
    void frameReady();

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

    // Debounce timer for render
    QTimer renderDebounce_;

    // Engine & services (owned)
    std::unique_ptr<rendering::StubVideoTimelineEngine> stubEngine_;
    int engineTimelineId_ = -1;
    video_editor::ProxyGenerationService* proxyService_ = nullptr;
};

} // namespace gopost::video_editor
