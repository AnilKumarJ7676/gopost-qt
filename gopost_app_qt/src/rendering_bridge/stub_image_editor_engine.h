#pragma once

#include "rendering_bridge/engine_api.h"

#include <vector>

namespace gopost::rendering {

/// Stub implementation of ImageEditorEngine for development and fallback.
/// Keeps layers in memory and composites them for preview on all platforms.
class StubImageEditorEngine final : public ImageEditorEngine {
public:
    StubImageEditorEngine() = default;
    ~StubImageEditorEngine() override = default;

    // --- CanvasManager ---
    int createCanvas(const CanvasConfig& config) override;
    void destroyCanvas(int canvasId) override;
    CanvasSize getCanvasSize(int canvasId) override;
    void resizeCanvas(int canvasId, int width, int height) override;

    // --- LayerManager ---
    int addImageLayer(int canvasId, const QByteArray& rgbaPixels, int width,
                      int height, int index = -1) override;
    int addSolidLayer(int canvasId, double r, double g, double b, double a,
                      int width, int height, int index = -1) override;
    int addGroupLayer(int canvasId, const QString& name,
                      int index = -1) override;
    void removeLayer(int canvasId, int layerId) override;
    void reorderLayer(int canvasId, int layerId, int newIndex) override;
    int duplicateLayer(int canvasId, int layerId) override;
    int getLayerCount(int canvasId) override;
    LayerInfo getLayerInfo(int canvasId, int layerId) override;
    std::vector<int> getLayerIds(int canvasId) override;

    // --- LayerPropertyEditor ---
    void setLayerVisible(int canvasId, int layerId, bool visible) override;
    void setLayerLocked(int canvasId, int layerId, bool locked) override;
    void setLayerOpacity(int canvasId, int layerId, double opacity) override;
    void setLayerBlendMode(int canvasId, int layerId,
                           BlendMode blendMode) override;
    void setLayerName(int canvasId, int layerId, const QString& name) override;
    void setLayerTransform(int canvasId, int layerId, double tx, double ty,
                           double sx, double sy, double rotation) override;

    // --- CanvasRenderer ---
    DecodedImage renderCanvas(int canvasId) override;
    void invalidateCanvas(int canvasId) override;
    void invalidateLayer(int canvasId, int layerId) override;

    // --- GpuTextureOutput ---
    GpuTextureResult renderToGpuTexture(int canvasId) override;
    void setViewport(int canvasId, const ViewportState& viewport) override;
    ViewportState getViewport(int canvasId) override;

    // --- ImageDecoder ---
    ImageInfo probeImage(const QByteArray& data) override;
    DecodedImage decodeImage(const QByteArray& data) override;
    DecodedImage decodeImageResized(const QByteArray& data, int maxWidth,
                                    int maxHeight) override;
    DecodedImage decodeImageFile(const QString& path) override;

    // --- ImageEncoder ---
    QByteArray encodeJpeg(const DecodedImage& image, int quality) override;
    QByteArray encodePng(const DecodedImage& image) override;
    QByteArray encodeWebp(const DecodedImage& image, int quality,
                          bool lossless) override;
    void encodeToFile(const DecodedImage& image, const QString& path,
                      ImageFormat format, int quality) override;

    // --- EffectRegistry ---
    void initEffects() override;
    void shutdownEffects() override;
    std::vector<EffectDef> getAllEffects() override;
    std::optional<EffectDef> findEffect(const QString& effectId) override;
    std::vector<EffectDef> getEffectsByCategory(
        EffectCategory category) override;

    // --- LayerEffectManager ---
    int addEffectToLayer(int canvasId, int layerId,
                         const QString& effectId) override;
    void removeEffectFromLayer(int canvasId, int layerId,
                               int instanceId) override;
    std::vector<EffectInstance> getLayerEffects(int canvasId,
                                                int layerId) override;
    void setEffectEnabled(int canvasId, int layerId, int instanceId,
                          bool enabled) override;
    void setEffectMix(int canvasId, int layerId, int instanceId,
                      double mix) override;
    void setEffectParam(int canvasId, int layerId, int instanceId,
                        const QString& paramId, double value) override;
    double getEffectParam(int canvasId, int layerId, int instanceId,
                          const QString& paramId) override;

    // --- PresetFilterEngine ---
    std::vector<PresetFilterInfo> getPresetFilters() override;
    std::optional<DecodedImage> applyPreset(int canvasId, int layerId,
                                             int presetIndex,
                                             double intensity) override;

    // --- TextManager ---
    void initText() override;
    void shutdownText() override;
    std::vector<QString> getAvailableFonts() override;
    int addTextLayer(int canvasId, const TextConfig& config, int maxWidth,
                     int index) override;
    void updateTextLayer(int canvasId, int layerId, const TextConfig& config,
                         int maxWidth) override;

    // --- ImageExporter ---
    ExportResult exportToFile(int canvasId, const ExportConfig& config,
                              const QString& outputPath,
                              std::function<void(double)> onProgress) override;
    int estimateFileSize(int canvasId, const ExportConfig& config) override;

    // --- MaskManager ---
    void addMask(int canvasId, int layerId, MaskType type) override;
    void removeMask(int canvasId, int layerId) override;
    bool hasMask(int canvasId, int layerId) override;
    void invertMask(int canvasId, int layerId) override;
    void setMaskEnabled(int canvasId, int layerId, bool enabled) override;
    void maskPaint(int canvasId, int layerId, double cx, double cy,
                   double radius, double hardness, MaskBrushMode mode,
                   double opacity) override;
    void maskFill(int canvasId, int layerId, int value) override;

    // --- ProjectManager ---
    void saveProject(int canvasId, const QString& filePath) override;
    int loadProject(const QString& filePath) override;

private:
    struct StubLayer {
        int id = 0;
        LayerType type = LayerType::Image;
        QString name;
        double opacity = 1.0;
        BlendMode blendMode = BlendMode::Normal;
        bool visible = true;
        bool locked = false;
        double tx = 0, ty = 0, sx = 1, sy = 1, rotation = 0;
        int contentWidth = 0;
        int contentHeight = 0;
        QByteArray imagePixels; // RGBA for image layers
        double solidR = 0, solidG = 0, solidB = 0, solidA = 0;

        LayerInfo toLayerInfo() const;
    };

    StubLayer* findLayer(int layerId);

    static void blendSolid(QByteArray& out, int cw, int ch, double r, double g,
                           double b, double a);
    static void blendImage(QByteArray& out, int cw, int ch,
                           const QByteArray& src, int sw, int sh,
                           double opacity);

    int nextCanvasId_ = 1;
    int nextLayerId_ = 1;
    int currentCanvasId_ = 0;
    int canvasWidth_ = 1080;
    int canvasHeight_ = 1080;
    double canvasDpi_ = 72.0;
    ViewportState viewport_;
    std::vector<StubLayer> layers_;
};

} // namespace gopost::rendering
