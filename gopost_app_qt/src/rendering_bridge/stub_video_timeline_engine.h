#pragma once

#include "rendering_bridge/engine_api.h"
#include "rendering_bridge/render_decode_thread.h"

#include <map>
#include <memory>
#include <vector>

namespace gopost::rendering {

/// Stub VideoTimelineEngine that tracks clips/tracks in memory when the
/// native library is not available.
class StubVideoTimelineEngine final : public VideoTimelineEngine {
public:
    StubVideoTimelineEngine() = default;
    ~StubVideoTimelineEngine() override = default;

    /// Enable/disable background FFmpeg decode threads.
    /// When disabled (default), renderFrame() uses colored placeholders.
    /// Disable when an external player (e.g. Qt MediaPlayer) handles video display.
    void setDecodeEnabled(bool enabled) { decodeEnabled_ = enabled; }
    bool isDecodeEnabled() const { return decodeEnabled_; }

    // --- TimelineLifecycle ---
    int createTimeline(const TimelineConfig& config) override;
    void destroyTimeline(int timelineId) override;
    TimelineConfig getTimelineConfig(int timelineId) override;
    double getDuration(int timelineId) override;

    // --- TimelineTrackOps ---
    int addTrack(int timelineId, VideoTrackType type) override;
    void removeTrack(int timelineId, int trackIndex) override;
    int getTrackCount(int timelineId) override;
    void reorderTracks(int timelineId, const std::vector<int>& newOrder) override;
    void setTrackSyncLock(int timelineId, int trackIndex, bool locked) override;
    void setTrackHeight(int timelineId, int trackIndex, double heightPx) override;
    double getTrackHeight(int timelineId, int trackIndex) override;

    // --- TimelineClipOps ---
    int addClip(int timelineId, const ClipDescriptor& descriptor) override;
    void removeClip(int timelineId, int clipId) override;
    void trimClip(int timelineId, int clipId, const TimelineRange& newRange,
                  const SourceRange& newSource) override;
    void moveClip(int timelineId, int clipId, int newTrackIndex,
                  double newInTime) override;
    std::optional<int> splitClip(int timelineId, int clipId,
                                  double splitTimeSeconds) override;
    void rippleDelete(int timelineId, int trackIndex, double rangeStart,
                      double rangeEnd) override;
    void moveMultipleClips(int timelineId, const std::vector<int>& clipIds,
                           double deltaTime, int deltaTrack) override;
    void swapClips(int timelineId, int clipIdA, int clipIdB) override;
    int splitAllTracks(int timelineId, double splitTimeSeconds) override;
    void liftDelete(int timelineId, int trackIndex, double rangeStart,
                    double rangeEnd) override;
    int checkOverlap(int timelineId, int trackIndex, double inTime,
                     double outTime, int excludeClipId) override;
    std::vector<int> getOverlappingClips(int timelineId, int trackIndex,
                                          double inTime, double outTime) override;

    // --- TimelinePlayback ---
    void seek(int timelineId, double positionSeconds) override;
    std::optional<DecodedImage> renderFrame(int timelineId) override;

    /// Render at explicit preview dimensions (avoids full 1920x1080 compositing).
    std::optional<DecodedImage> renderFrame(int timelineId,
                                             int previewW, int previewH);
    double getPosition(int timelineId) override;
    void setFrameCacheSizeBytes(int timelineId, int maxBytes) override;
    void invalidateFrameCache(int timelineId) override;

    // --- TimelineNleEdits ---
    int insertEdit(int timelineId, int trackIndex, double atTime,
                   const ClipDescriptor& clip) override;
    int overwriteEdit(int timelineId, int trackIndex, double atTime,
                      const ClipDescriptor& clip) override;
    void rollEdit(int timelineId, int clipId, double deltaSec) override;
    void slipEdit(int timelineId, int clipId, double deltaSec) override;
    void slideEdit(int timelineId, int clipId, double deltaSec) override;
    void rateStretch(int timelineId, int clipId, double newDurationSec) override;
    int duplicateClip(int timelineId, int clipId) override;
    std::vector<double> getSnapPoints(int timelineId, double timeSec,
                                       double thresholdSec) override;

    // --- TimelineMediaProbe ---
    std::optional<MediaInfo> probeMedia(const QString& filePath) override;
    std::optional<MediaInfo> probeMediaFast(const QString& filePath) override;

    // --- VideoTimelineEngine extended ---
    void setClipVolume(int timelineId, int clipId, double volume) override;
    double getClipVolume(int timelineId, int clipId) override;
    void setClipTransitionIn(int timelineId, int clipId, int transitionType,
                             double durationSeconds, int easingCurve) override;
    void setClipTransitionOut(int timelineId, int clipId, int transitionType,
                              double durationSeconds, int easingCurve) override;
    void setClipKeyframe(int timelineId, int clipId, int property, double time,
                         double value, int interpolation) override;
    void removeClipKeyframe(int timelineId, int clipId, int property,
                            double time) override;
    void clearClipKeyframes(int timelineId, int clipId, int property) override;
    void setClipColorGrading(int timelineId, int clipId, double brightness,
        double contrast, double saturation, double exposure,
        double temperature, double tint, double highlights, double shadows,
        double vibrance, double hue, double fade, double vignette) override;
    void clearClipEffects(int timelineId, int clipId) override;
    void setClipPan(int timelineId, int clipId, double pan) override;
    void setClipFadeIn(int timelineId, int clipId, double seconds) override;
    void setClipFadeOut(int timelineId, int clipId, double seconds) override;
    void setTrackVolume(int timelineId, int trackIndex, double volume) override;
    void setTrackPan(int timelineId, int trackIndex, double pan) override;
    void setTrackMute(int timelineId, int trackIndex, bool mute) override;
    void setTrackSolo(int timelineId, int trackIndex, bool solo) override;
    int startExport(int timelineId, const VideoExportConfig& config) override;
    double getExportProgress(int exportJobId) override;
    void cancelExport(int exportJobId) override;
    bool supportsHardwareEncoding() const override;
    std::vector<EngineEffectDef> listEffects(const QString& category) override;
    int addClipEffect(int timelineId, int clipId, const QString& effectDefId) override;
    void removeClipEffect(int timelineId, int clipId, int effectInstanceId) override;
    void reorderClipEffects(int timelineId, int clipId,
                            const std::vector<int>& instanceIds) override;
    void setEffectParam(int timelineId, int clipId, int effectInstanceId,
                        const QString& paramId, double value) override;
    void setEffectEnabled(int timelineId, int clipId, int effectInstanceId,
                          bool enabled) override;
    void setEffectMix(int timelineId, int clipId, int effectInstanceId,
                      double mix) override;
    int addClipMask(int timelineId, int clipId, const MaskData& mask) override;
    void updateClipMask(int timelineId, int clipId, int maskId,
                        const MaskData& mask) override;
    void removeClipMask(int timelineId, int clipId, int maskId) override;
    int startTracking(int timelineId, int clipId, double x, double y,
                      double timeSec) override;
    std::vector<TrackPoint> getTrackingData(int timelineId, int trackerId) override;
    void stabilizeClip(int timelineId, int clipId,
                       const StabilizationConfig& config) override;
    void setClipText(int timelineId, int clipId, const TextLayerData& textData) override;
    int addClipShape(int timelineId, int clipId, const ShapeData& shape) override;
    void updateClipShape(int timelineId, int clipId, int shapeId,
                         const ShapeData& shape) override;
    void removeClipShape(int timelineId, int clipId, int shapeId) override;
    int addAudioEffect(int timelineId, int clipId,
                       const QString& audioEffectDefId) override;
    void setAudioEffectParam(int timelineId, int clipId, int effectInstanceId,
                             const QString& paramId, double value) override;
    void removeAudioEffect(int timelineId, int clipId, int effectInstanceId) override;
    std::vector<AudioEffectDef> listAudioEffects() override;
    int startAiSegmentation(int timelineId, int clipId,
                            const AiSegmentationConfig& config) override;
    double getAiSegmentationProgress(int jobId) override;
    void cancelAiSegmentation(int jobId) override;
    void enableProxyMode(int timelineId, const ProxyConfig& config) override;
    void disableProxyMode(int timelineId) override;
    bool isProxyModeActive(int timelineId) override;
    int createMultiCamClip(int timelineId, int trackIndex,
                           const MultiCamConfig& config) override;
    void switchMultiCamAngle(int timelineId, int clipId, int angleIndex,
                             double atTimeSec) override;
    void flattenMultiCam(int timelineId, int clipId) override;
    int createTextureBridge(int width, int height) override;
    void destroyTextureBridge() override;
    bool renderToTextureBridge(int timelineId) override;
    void resizeTextureBridge(int width, int height) override;
    std::optional<QByteArray> getTextureBridgePixels() override;

    // ---- Data structs for per-clip rendering state -------------------------

    struct ColorGradingParams {
        double brightness = 0, contrast = 0, saturation = 0;
        double exposure = 0, temperature = 0, tint = 0;
        double highlights = 0, shadows = 0, vibrance = 0, hue = 0;
        double fade = 0, vignette = 0;
        bool isDefault() const {
            return brightness == 0 && contrast == 0 && saturation == 0 &&
                   exposure == 0 && temperature == 0 && tint == 0 &&
                   highlights == 0 && shadows == 0 && vibrance == 0 && hue == 0 &&
                   fade == 0 && vignette == 0;
        }
    };

    struct EffectData {
        int type = 0;      // maps to EffectType enum value
        double value = 0;
        bool enabled = true;
        double mix = 1.0;
        int instanceId = 0;
    };

    struct TransitionData {
        int type = 0;           // maps to TransitionType enum
        double durationSec = 0;
        int easing = 0;         // maps to EasingCurve enum
    };

    struct KeyframeData {
        double time = 0;
        double value = 0;
        int interpolation = 0; // 0=Linear,1=Bezier,2=Hold,3=EaseIn,4=EaseOut,5=EaseInOut
    };

    struct AudioParams {
        double volume = 1.0;
        double pan = 0.0;
        double fadeIn = 0.0;
        double fadeOut = 0.0;
        bool muted = false;
    };

    struct TrackAudioParams {
        double volume = 1.0;
        double pan = 0.0;
        bool mute = false;
        bool solo = false;
    };

private:
    struct StubClip {
        int id = 0;
        int trackIndex = 0;
        VideoClipSourceType sourceType = VideoClipSourceType::Video;
        QString sourcePath;
        QString displayName;
        TimelineRange timelineRange;
        SourceRange sourceRange;
        double speed = 1.0;
        double opacity = 1.0;
        int blendMode = 0;
        int effectHash = 0;
    };

    struct StubTrack {
        VideoTrackType type = VideoTrackType::Video;
        std::vector<StubClip> clips;
    };

    // ---- Core helpers ----
    void checkTimeline(int id);
    double computeDuration(int timelineId);
    StubClip* findClip(int timelineId, int clipId);

    // ---- Rendering helpers ----
    void applyColorGrading(QImage& img, int clipId);
    void applyEffects(QImage& img, int clipId);
    void applyTransform(QImage& img, int clipId, double localTime, int w, int h);
    void compositeFrame(QImage& dst, const QImage& src, double opacity, int blendMode);
    void renderTextLayer(QImage& img, const TextLayerData& td, int w, int h);
    void renderClipContent(QImage& clipFrame, const StubClip* clip,
                           double pos, int w, int h);

    // ---- Effect pixel operations ----
    static void applyBoxBlur(QImage& img, double radius);
    static void applyPixelate(QImage& img, int blockSize);
    static void applyGlitch(QImage& img, double amount, int seed);
    static void applyChromatic(QImage& img, double amount);
    static void applyVignette(QImage& img, double amount);
    static void applyGrain(QImage& img, double amount, int seed);
    static void applySepia(QImage& img, double amount);
    static void applyInvert(QImage& img);
    static void applyPosterize(QImage& img, int levels);

    // ---- Keyframe helpers ----
    static double evaluateKeyframes(const std::vector<KeyframeData>& kfs, double time);
    static double applyEasing(double t, int easing);

    // ---- Transition helper ----
    void renderTransition(QImage& dst, const QImage& outgoing, const QImage& incoming,
                          double progress, int type, int easing, int w, int h);

    // ---- Core state ----
    std::map<int, TimelineConfig> configs_;
    std::map<int, std::vector<StubTrack>> tracks_;
    std::map<int, double> positions_;
    int nextId_ = 1;
    int nextClipId_ = 1;
    int nextExportId_ = 1;
    int nextEffectInstanceId_ = 1;
    std::map<int, double> exportProgress_;
    bool decodeEnabled_ = false;

    // ---- Per-clip decode state ----
    std::map<int, std::unique_ptr<RenderDecodeThread>> decoders_;
    struct CachedFrame {
        RenderDecodedFrame frame;
        double seekedPosition = -1.0;
    };
    std::map<int, CachedFrame> cachedFrames_;
    std::map<int, double> lastSeekTime_;

    // ---- Per-clip rendering data ----
    std::map<int, ColorGradingParams> colorGradingParams_;
    std::map<int, std::vector<EffectData>> clipEffects_;
    std::map<int, TransitionData> clipTransitionIn_;
    std::map<int, TransitionData> clipTransitionOut_;
    std::map<int, std::map<int, std::vector<KeyframeData>>> clipKeyframes_;
    std::map<int, TextLayerData> clipTextData_;
    std::map<int, std::vector<std::pair<int, ShapeData>>> clipShapes_;
    std::map<int, AudioParams> clipAudioParams_;

    // ---- Per-track audio state ----
    std::map<int, TrackAudioParams> trackAudioParams_;
};

} // namespace gopost::rendering
