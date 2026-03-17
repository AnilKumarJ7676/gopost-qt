#pragma once

#include "rendering_bridge/engine_types.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace gopost::rendering {

// =========================================================================
// ISP: Core engine lifecycle operations
// =========================================================================

class EngineLifecycle {
public:
    virtual ~EngineLifecycle() = default;

    virtual void initialize(const EngineConfig& config) = 0;
    virtual void dispose() = 0;
    [[nodiscard]] virtual bool isInitialized() const = 0;
    [[nodiscard]] virtual QString getVersion() = 0;
};

// =========================================================================
// ISP: GPU-related queries
// =========================================================================

class GpuQueryable {
public:
    virtual ~GpuQueryable() = default;

    virtual GpuCapabilities queryGpuCapabilities() = 0;
    virtual HwDecoderInfo queryHwDecoder() = 0;
};

// =========================================================================
// ISP: Template load/unload
// =========================================================================

class TemplateLoader {
public:
    virtual ~TemplateLoader() = default;

    virtual TemplateMetadata loadTemplate(const QByteArray& encryptedBlob,
                                          const QByteArray& sessionKey) = 0;
    virtual void unloadTemplate(const QString& templateId) = 0;
};

// =========================================================================
// ISP: Canvas lifecycle
// =========================================================================

class CanvasManager {
public:
    virtual ~CanvasManager() = default;

    virtual int createCanvas(const CanvasConfig& config) = 0;
    virtual void destroyCanvas(int canvasId) = 0;
    virtual CanvasSize getCanvasSize(int canvasId) = 0;
    virtual void resizeCanvas(int canvasId, int width, int height) = 0;
};

// =========================================================================
// ISP: Layer management
// =========================================================================

class LayerManager {
public:
    virtual ~LayerManager() = default;

    virtual int addImageLayer(int canvasId, const QByteArray& rgbaPixels,
                              int width, int height, int index = -1) = 0;
    virtual int addSolidLayer(int canvasId, double r, double g, double b,
                              double a, int width, int height,
                              int index = -1) = 0;
    virtual int addGroupLayer(int canvasId, const QString& name,
                              int index = -1) = 0;
    virtual void removeLayer(int canvasId, int layerId) = 0;
    virtual void reorderLayer(int canvasId, int layerId, int newIndex) = 0;
    virtual int duplicateLayer(int canvasId, int layerId) = 0;
    virtual int getLayerCount(int canvasId) = 0;
    virtual LayerInfo getLayerInfo(int canvasId, int layerId) = 0;
    virtual std::vector<int> getLayerIds(int canvasId) = 0;
};

// =========================================================================
// ISP: Layer property setters
// =========================================================================

class LayerPropertyEditor {
public:
    virtual ~LayerPropertyEditor() = default;

    virtual void setLayerVisible(int canvasId, int layerId, bool visible) = 0;
    virtual void setLayerLocked(int canvasId, int layerId, bool locked) = 0;
    virtual void setLayerOpacity(int canvasId, int layerId, double opacity) = 0;
    virtual void setLayerBlendMode(int canvasId, int layerId,
                                   BlendMode blendMode) = 0;
    virtual void setLayerName(int canvasId, int layerId,
                              const QString& name) = 0;
    virtual void setLayerTransform(int canvasId, int layerId, double tx = 0,
                                   double ty = 0, double sx = 1, double sy = 1,
                                   double rotation = 0) = 0;
};

// =========================================================================
// ISP: Canvas rendering
// =========================================================================

class CanvasRenderer {
public:
    virtual ~CanvasRenderer() = default;

    virtual DecodedImage renderCanvas(int canvasId) = 0;
    virtual void invalidateCanvas(int canvasId) = 0;
    virtual void invalidateLayer(int canvasId, int layerId) = 0;
};

// =========================================================================
// ISP: GPU texture output
// =========================================================================

class GpuTextureOutput {
public:
    virtual ~GpuTextureOutput() = default;

    virtual GpuTextureResult renderToGpuTexture(int canvasId) = 0;
    virtual void setViewport(int canvasId, const ViewportState& viewport) = 0;
    virtual ViewportState getViewport(int canvasId) = 0;
};

// =========================================================================
// ISP: Image decoding
// =========================================================================

class ImageDecoder {
public:
    virtual ~ImageDecoder() = default;

    virtual ImageInfo probeImage(const QByteArray& data) = 0;
    virtual DecodedImage decodeImage(const QByteArray& data) = 0;
    virtual DecodedImage decodeImageResized(const QByteArray& data,
                                            int maxWidth, int maxHeight) = 0;
    virtual DecodedImage decodeImageFile(const QString& path) = 0;
};

// =========================================================================
// ISP: Image encoding
// =========================================================================

class ImageEncoder {
public:
    virtual ~ImageEncoder() = default;

    virtual QByteArray encodeJpeg(const DecodedImage& image,
                                  int quality = 85) = 0;
    virtual QByteArray encodePng(const DecodedImage& image) = 0;
    virtual QByteArray encodeWebp(const DecodedImage& image, int quality = 80,
                                  bool lossless = false) = 0;
    virtual void encodeToFile(const DecodedImage& image, const QString& path,
                              ImageFormat format, int quality = 85) = 0;
};

// =========================================================================
// ISP: Effect registry queries
// =========================================================================

class EffectRegistry {
public:
    virtual ~EffectRegistry() = default;

    virtual void initEffects() = 0;
    virtual void shutdownEffects() = 0;
    virtual std::vector<EffectDef> getAllEffects() = 0;
    virtual std::optional<EffectDef> findEffect(const QString& effectId) = 0;
    virtual std::vector<EffectDef> getEffectsByCategory(
        EffectCategory category) = 0;
};

// =========================================================================
// ISP: Layer effect management
// =========================================================================

class LayerEffectManager {
public:
    virtual ~LayerEffectManager() = default;

    virtual int addEffectToLayer(int canvasId, int layerId,
                                 const QString& effectId) = 0;
    virtual void removeEffectFromLayer(int canvasId, int layerId,
                                       int instanceId) = 0;
    virtual std::vector<EffectInstance> getLayerEffects(int canvasId,
                                                        int layerId) = 0;
    virtual void setEffectEnabled(int canvasId, int layerId, int instanceId,
                                  bool enabled) = 0;
    virtual void setEffectMix(int canvasId, int layerId, int instanceId,
                              double mix) = 0;
    virtual void setEffectParam(int canvasId, int layerId, int instanceId,
                                const QString& paramId, double value) = 0;
    virtual double getEffectParam(int canvasId, int layerId, int instanceId,
                                  const QString& paramId) = 0;
};

// =========================================================================
// ISP: Preset filter operations
// =========================================================================

class PresetFilterEngine {
public:
    virtual ~PresetFilterEngine() = default;

    virtual std::vector<PresetFilterInfo> getPresetFilters() = 0;
    virtual std::optional<DecodedImage> applyPreset(int canvasId, int layerId,
                                                     int presetIndex,
                                                     double intensity) = 0;
};

// =========================================================================
// ISP: Text engine
// =========================================================================

class TextManager {
public:
    virtual ~TextManager() = default;

    virtual void initText() = 0;
    virtual void shutdownText() = 0;
    virtual std::vector<QString> getAvailableFonts() = 0;
    virtual int addTextLayer(int canvasId, const TextConfig& config,
                             int maxWidth, int index = -1) = 0;
    virtual void updateTextLayer(int canvasId, int layerId,
                                 const TextConfig& config, int maxWidth) = 0;
};

// =========================================================================
// ISP: Image export
// =========================================================================

class ImageExporter {
public:
    virtual ~ImageExporter() = default;

    virtual ExportResult exportToFile(
        int canvasId, const ExportConfig& config, const QString& outputPath,
        std::function<void(double)> onProgress = nullptr) = 0;
    virtual int estimateFileSize(int canvasId, const ExportConfig& config) = 0;
};

// =========================================================================
// ISP: Layer mask management
// =========================================================================

class MaskManager {
public:
    virtual ~MaskManager() = default;

    virtual void addMask(int canvasId, int layerId, MaskType type) = 0;
    virtual void removeMask(int canvasId, int layerId) = 0;
    virtual bool hasMask(int canvasId, int layerId) = 0;
    virtual void invertMask(int canvasId, int layerId) = 0;
    virtual void setMaskEnabled(int canvasId, int layerId, bool enabled) = 0;
    virtual void maskPaint(int canvasId, int layerId, double cx, double cy,
                           double radius, double hardness,
                           MaskBrushMode mode, double opacity) = 0;
    virtual void maskFill(int canvasId, int layerId, int value) = 0;
};

// =========================================================================
// ISP: Project persistence
// =========================================================================

class ProjectManager {
public:
    virtual ~ProjectManager() = default;

    virtual void saveProject(int canvasId, const QString& filePath) = 0;
    virtual int loadProject(const QString& filePath) = 0;
};

// =========================================================================
// Composite: GopostEngine (core engine)
// =========================================================================

class GopostEngine : public EngineLifecycle,
                     public GpuQueryable,
                     public TemplateLoader {
public:
    ~GopostEngine() override = default;
};

// =========================================================================
// Composite: ImageEditorEngine
// =========================================================================

class ImageEditorEngine : public CanvasManager,
                          public LayerManager,
                          public LayerPropertyEditor,
                          public CanvasRenderer,
                          public GpuTextureOutput,
                          public ImageDecoder,
                          public ImageEncoder,
                          public EffectRegistry,
                          public LayerEffectManager,
                          public PresetFilterEngine,
                          public TextManager,
                          public ImageExporter,
                          public MaskManager,
                          public ProjectManager {
public:
    ~ImageEditorEngine() override = default;
};

// =========================================================================
// ISP: Video timeline lifecycle
// =========================================================================

class TimelineLifecycle {
public:
    virtual ~TimelineLifecycle() = default;

    virtual int createTimeline(const TimelineConfig& config) = 0;
    virtual void destroyTimeline(int timelineId) = 0;
    virtual TimelineConfig getTimelineConfig(int timelineId) = 0;
    virtual double getDuration(int timelineId) = 0;
};

// =========================================================================
// ISP: Track CRUD
// =========================================================================

class TimelineTrackOps {
public:
    virtual ~TimelineTrackOps() = default;

    virtual int addTrack(int timelineId, VideoTrackType type) = 0;
    virtual void removeTrack(int timelineId, int trackIndex) = 0;
    virtual int getTrackCount(int timelineId) = 0;
    virtual void reorderTracks(int timelineId,
                               const std::vector<int>& newOrder) = 0;
    virtual void setTrackSyncLock(int timelineId, int trackIndex,
                                  bool locked) = 0;
    virtual void setTrackHeight(int timelineId, int trackIndex,
                                double heightPx) = 0;
    virtual double getTrackHeight(int timelineId, int trackIndex) = 0;
};

// =========================================================================
// ISP: Clip CRUD, move, trim, split, collision
// =========================================================================

class TimelineClipOps {
public:
    virtual ~TimelineClipOps() = default;

    virtual int addClip(int timelineId, const ClipDescriptor& descriptor) = 0;
    virtual void removeClip(int timelineId, int clipId) = 0;
    virtual void trimClip(int timelineId, int clipId,
                          const TimelineRange& newRange,
                          const SourceRange& newSource) = 0;
    virtual void moveClip(int timelineId, int clipId, int newTrackIndex,
                          double newInTime) = 0;
    virtual std::optional<int> splitClip(int timelineId, int clipId,
                                         double splitTimeSeconds) = 0;
    virtual void rippleDelete(int timelineId, int trackIndex,
                              double rangeStartSeconds,
                              double rangeEndSeconds) = 0;
    virtual void moveMultipleClips(int timelineId,
                                   const std::vector<int>& clipIds,
                                   double deltaTime, int deltaTrack) = 0;
    virtual void swapClips(int timelineId, int clipIdA, int clipIdB) = 0;
    virtual int splitAllTracks(int timelineId,
                               double splitTimeSeconds) = 0;
    virtual void liftDelete(int timelineId, int trackIndex,
                            double rangeStartSeconds,
                            double rangeEndSeconds) = 0;
    /// Collision detection: 0=CLEAR, 1=OVERLAP, 2=ADJACENT.
    virtual int checkOverlap(int timelineId, int trackIndex, double inTime,
                             double outTime, int excludeClipId = -1) = 0;
    virtual std::vector<int> getOverlappingClips(int timelineId,
                                                  int trackIndex,
                                                  double inTime,
                                                  double outTime) = 0;
};

// =========================================================================
// ISP: Playback, seek, render, frame cache
// =========================================================================

class TimelinePlayback {
public:
    virtual ~TimelinePlayback() = default;

    virtual void seek(int timelineId, double positionSeconds) = 0;
    virtual std::optional<DecodedImage> renderFrame(int timelineId) = 0;
    virtual double getPosition(int timelineId) = 0;
    virtual void setFrameCacheSizeBytes(int timelineId, int maxBytes) = 0;
    virtual void invalidateFrameCache(int timelineId) = 0;
};

// =========================================================================
// ISP: NLE edit operations
// =========================================================================

class TimelineNleEdits {
public:
    virtual ~TimelineNleEdits() = default;

    virtual int insertEdit(int timelineId, int trackIndex, double atTime,
                           const ClipDescriptor& clip) = 0;
    virtual int overwriteEdit(int timelineId, int trackIndex, double atTime,
                              const ClipDescriptor& clip) = 0;
    virtual void rollEdit(int timelineId, int clipId, double deltaSec) = 0;
    virtual void slipEdit(int timelineId, int clipId, double deltaSec) = 0;
    virtual void slideEdit(int timelineId, int clipId, double deltaSec) = 0;
    virtual void rateStretch(int timelineId, int clipId,
                             double newDurationSec) = 0;
    virtual int duplicateClip(int timelineId, int clipId) = 0;
    virtual std::vector<double> getSnapPoints(int timelineId, double timeSec,
                                               double thresholdSec) = 0;
};

// =========================================================================
// ISP: Media probing
// =========================================================================

class TimelineMediaProbe {
public:
    virtual ~TimelineMediaProbe() = default;

    virtual std::optional<MediaInfo> probeMedia(const QString& filePath) = 0;
    virtual std::optional<MediaInfo> probeMediaFast(
        const QString& filePath) = 0;
};

// =========================================================================
// Composite: VideoTimelineEngine
// =========================================================================

class VideoTimelineEngine : public TimelineLifecycle,
                            public TimelineTrackOps,
                            public TimelineClipOps,
                            public TimelinePlayback,
                            public TimelineNleEdits,
                            public TimelineMediaProbe {
public:
    ~VideoTimelineEngine() override = default;

    // --- Clip audio ---
    virtual void setClipVolume(int timelineId, int clipId,
                               double volume) = 0;
    virtual double getClipVolume(int timelineId, int clipId) = 0;

    // --- Transitions (S10) ---
    virtual void setClipTransitionIn(int timelineId, int clipId,
                                     int transitionType,
                                     double durationSeconds,
                                     int easingCurve) = 0;
    virtual void setClipTransitionOut(int timelineId, int clipId,
                                      int transitionType,
                                      double durationSeconds,
                                      int easingCurve) = 0;

    // --- Keyframes (S10) ---
    virtual void setClipKeyframe(int timelineId, int clipId, int property,
                                 double time, double value,
                                 int interpolation) = 0;
    virtual void removeClipKeyframe(int timelineId, int clipId, int property,
                                    double time) = 0;
    virtual void clearClipKeyframes(int timelineId, int clipId,
                                    int property) = 0;

    // --- Effects (S10) ---
    virtual void setClipColorGrading(
        int timelineId, int clipId, double brightness = 0,
        double contrast = 0, double saturation = 0, double exposure = 0,
        double temperature = 0, double tint = 0, double highlights = 0,
        double shadows = 0, double vibrance = 0, double hue = 0) = 0;
    virtual void clearClipEffects(int timelineId, int clipId) = 0;

    // --- Audio enhancements (S10) ---
    virtual void setClipPan(int timelineId, int clipId, double pan) = 0;
    virtual void setClipFadeIn(int timelineId, int clipId,
                               double seconds) = 0;
    virtual void setClipFadeOut(int timelineId, int clipId,
                                double seconds) = 0;
    virtual void setTrackVolume(int timelineId, int trackIndex,
                                double volume) = 0;
    virtual void setTrackPan(int timelineId, int trackIndex,
                             double pan) = 0;
    virtual void setTrackMute(int timelineId, int trackIndex,
                              bool mute) = 0;
    virtual void setTrackSolo(int timelineId, int trackIndex,
                              bool solo) = 0;

    // --- Export pipeline (S11) ---
    virtual int startExport(int timelineId,
                            const VideoExportConfig& config) = 0;
    virtual double getExportProgress(int exportJobId) = 0;
    virtual void cancelExport(int exportJobId) = 0;
    [[nodiscard]] virtual bool supportsHardwareEncoding() const = 0;

    // --- Phase 3: Effect DAG & Registry ---
    virtual std::vector<EngineEffectDef> listEffects(
        const QString& category = {}) = 0;
    virtual int addClipEffect(int timelineId, int clipId,
                              const QString& effectDefId) = 0;
    virtual void removeClipEffect(int timelineId, int clipId,
                                  int effectInstanceId) = 0;
    virtual void reorderClipEffects(int timelineId, int clipId,
                                    const std::vector<int>& instanceIds) = 0;
    virtual void setEffectParam(int timelineId, int clipId,
                                int effectInstanceId, const QString& paramId,
                                double value) = 0;
    virtual void setEffectEnabled(int timelineId, int clipId,
                                  int effectInstanceId, bool enabled) = 0;
    virtual void setEffectMix(int timelineId, int clipId,
                              int effectInstanceId, double mix) = 0;

    // --- Phase 4: Masking & Tracking ---
    virtual int addClipMask(int timelineId, int clipId,
                            const MaskData& mask) = 0;
    virtual void updateClipMask(int timelineId, int clipId, int maskId,
                                const MaskData& mask) = 0;
    virtual void removeClipMask(int timelineId, int clipId,
                                int maskId) = 0;
    virtual int startTracking(int timelineId, int clipId, double x, double y,
                              double timeSec) = 0;
    virtual std::vector<TrackPoint> getTrackingData(int timelineId,
                                                     int trackerId) = 0;
    virtual void stabilizeClip(int timelineId, int clipId,
                               const StabilizationConfig& config) = 0;

    // --- Phase 5: Text, Shapes, Audio Effects ---
    virtual void setClipText(int timelineId, int clipId,
                             const TextLayerData& textData) = 0;
    virtual int addClipShape(int timelineId, int clipId,
                             const ShapeData& shape) = 0;
    virtual void updateClipShape(int timelineId, int clipId, int shapeId,
                                 const ShapeData& shape) = 0;
    virtual void removeClipShape(int timelineId, int clipId,
                                 int shapeId) = 0;
    virtual int addAudioEffect(int timelineId, int clipId,
                               const QString& audioEffectDefId) = 0;
    virtual void setAudioEffectParam(int timelineId, int clipId,
                                     int effectInstanceId,
                                     const QString& paramId,
                                     double value) = 0;
    virtual void removeAudioEffect(int timelineId, int clipId,
                                   int effectInstanceId) = 0;
    virtual std::vector<AudioEffectDef> listAudioEffects() = 0;

    // --- Phase 6: AI, Proxy, Multi-Cam ---
    virtual int startAiSegmentation(int timelineId, int clipId,
                                    const AiSegmentationConfig& config) = 0;
    virtual double getAiSegmentationProgress(int jobId) = 0;
    virtual void cancelAiSegmentation(int jobId) = 0;
    virtual void enableProxyMode(int timelineId,
                                 const ProxyConfig& config) = 0;
    virtual void disableProxyMode(int timelineId) = 0;
    virtual bool isProxyModeActive(int timelineId) = 0;
    virtual int createMultiCamClip(int timelineId, int trackIndex,
                                   const MultiCamConfig& config) = 0;
    virtual void switchMultiCamAngle(int timelineId, int clipId,
                                     int angleIndex, double atTimeSec) = 0;
    virtual void flattenMultiCam(int timelineId, int clipId) = 0;

    // --- Texture Bridge ---
    virtual int createTextureBridge(int width, int height) = 0;
    virtual void destroyTextureBridge() = 0;
    virtual bool renderToTextureBridge(int timelineId) = 0;
    virtual void resizeTextureBridge(int width, int height) = 0;
    virtual std::optional<QByteArray> getTextureBridgePixels() = 0;
};

} // namespace gopost::rendering
