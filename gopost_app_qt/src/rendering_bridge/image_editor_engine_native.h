#pragma once

#include "rendering_bridge/engine_api.h"

#include <cstdint>
#include <map>

// Forward-declare the C engine API types.
extern "C" {
struct GopostEngine;
struct GopostCanvas;
struct GopostFrame;
struct GopostEngineConfig;
struct GopostCanvasConfig;
struct GopostLayerInfo;
struct GopostExportConfig;
struct GopostImageInfo;
struct GopostEffectDef;
struct GopostEffectInstance;
struct GopostJpegOpts;
struct GopostPngOpts;
struct GopostWebpOpts;

int gopost_engine_create(GopostEngine** out, const GopostEngineConfig* config);
int gopost_engine_destroy(GopostEngine* engine);
const char* gopost_error_string(int err);

int gopost_canvas_create(GopostEngine* engine, const GopostCanvasConfig* config,
                         GopostCanvas** out);
int gopost_canvas_destroy(GopostCanvas* canvas);
int gopost_canvas_get_size(GopostCanvas* canvas, int* w, int* h, float* dpi);
int gopost_canvas_resize(GopostCanvas* canvas, int w, int h);
int gopost_canvas_add_image_layer(GopostCanvas* canvas, const uint8_t* rgba,
                                  int w, int h, int index, int* outId);
int gopost_canvas_add_solid_layer(GopostCanvas* canvas, double r, double g,
                                  double b, double a, int w, int h, int index,
                                  int* outId);
int gopost_canvas_add_group_layer(GopostCanvas* canvas, const char* name,
                                  int index, int* outId);
int gopost_canvas_remove_layer(GopostCanvas* canvas, int layerId);
int gopost_canvas_reorder_layer(GopostCanvas* canvas, int layerId,
                                int newIndex);
int gopost_canvas_duplicate_layer(GopostCanvas* canvas, int layerId,
                                  int* outId);
int gopost_canvas_get_layer_count(GopostCanvas* canvas, int* out);
int gopost_canvas_get_layer_info(GopostCanvas* canvas, int layerId,
                                 GopostLayerInfo* out);
int gopost_canvas_get_layer_ids(GopostCanvas* canvas, int* outIds,
                                int maxCount);
int gopost_layer_set_visible(GopostCanvas* canvas, int layerId, int visible);
int gopost_layer_set_locked(GopostCanvas* canvas, int layerId, int locked);
int gopost_layer_set_opacity(GopostCanvas* canvas, int layerId, double opacity);
int gopost_layer_set_blend_mode(GopostCanvas* canvas, int layerId, int mode);
int gopost_layer_set_name(GopostCanvas* canvas, int layerId, const char* name);
int gopost_layer_set_transform(GopostCanvas* canvas, int layerId, double tx,
                               double ty, double sx, double sy,
                               double rotation);
int gopost_canvas_render(GopostCanvas* canvas, GopostFrame** outFrame);
int gopost_canvas_invalidate(GopostCanvas* canvas);
int gopost_canvas_invalidate_layer(GopostCanvas* canvas, int layerId);
GopostEngine* gopost_canvas_get_engine(GopostCanvas* canvas);
int gopost_frame_release(GopostEngine* engine, GopostFrame* frame);
int gopost_render_frame_free(GopostFrame* frame);
int gopost_export_to_file(GopostCanvas* canvas,
                          const GopostExportConfig* config, const char* path,
                          void* progressCb, void* userData);
int gopost_export_estimate_size(GopostCanvas* canvas,
                                const GopostExportConfig* config,
                                uint64_t* outSize);
int gopost_image_probe(const uint8_t* data, int len, GopostImageInfo* out);
int gopost_image_encode_jpeg(GopostFrame* frame, const GopostJpegOpts* opts,
                             uint8_t** outData, uint64_t* outSize);
int gopost_image_encode_png(GopostFrame* frame, const GopostPngOpts* opts,
                            uint8_t** outData, uint64_t* outSize);
int gopost_image_encode_webp(GopostFrame* frame, const GopostWebpOpts* opts,
                             uint8_t** outData, uint64_t* outSize);
int gopost_image_encode_free(uint8_t* data);
int gopost_image_encode_to_file(GopostFrame* frame, const char* path,
                                int format, int quality);
int gopost_effects_init(GopostEngine* engine);
int gopost_effects_shutdown(GopostEngine* engine);
int gopost_effects_get_count(GopostEngine* engine, int* outCount);
int gopost_effects_get_def(GopostEngine* engine, int index,
                           GopostEffectDef* out);
int gopost_effects_find(GopostEngine* engine, const char* id,
                        GopostEffectDef* out);
int gopost_effects_list_category(GopostEngine* engine, int category,
                                 GopostEffectDef* outDefs, int maxDefs,
                                 int* outCount);
int gopost_layer_add_effect(GopostCanvas* canvas, int layerId, const char* id,
                            int* outInstanceId);
int gopost_layer_remove_effect(GopostCanvas* canvas, int layerId,
                               int instanceId);
int gopost_layer_get_effect_count(GopostCanvas* canvas, int layerId,
                                  int* out);
int gopost_layer_get_effect(GopostCanvas* canvas, int layerId, int index,
                            GopostEffectInstance* out);
int gopost_effect_set_enabled(GopostCanvas* canvas, int layerId,
                              int instanceId, int enabled);
int gopost_effect_set_mix(GopostCanvas* canvas, int layerId, int instanceId,
                          double mix);
int gopost_effect_set_param(GopostCanvas* canvas, int layerId, int instanceId,
                            const char* paramId, double value);
int gopost_effect_get_param(GopostCanvas* canvas, int layerId, int instanceId,
                            const char* paramId, float* outValue);
int gopost_preset_get_count(GopostEngine* engine, int* outCount);
int gopost_preset_get_info(GopostEngine* engine, int index, char* outName,
                           int nameLen, char* outCat, int catLen);
int gopost_preset_apply(GopostEngine* engine, GopostFrame* frame,
                        int presetIndex, double intensity);
} // extern "C"

namespace gopost::rendering {

/// Native C-engine-backed implementation of ImageEditorEngine.
/// Directly calls the gopost_engine C API (linked at compile time).
class GopostImageEditorEngineNative final : public ImageEditorEngine {
public:
    GopostImageEditorEngineNative();
    ~GopostImageEditorEngineNative() override;

    /// Initialize the engine. Call once before using canvas/export.
    void initialize();
    void dispose();

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

    /// Access the raw engine pointer for sharing with the video engine.
    [[nodiscard]] GopostEngine* enginePointer() const { return enginePtr_; }

private:
    void ensureInitialized();
    GopostCanvas* getCanvas(int canvasId);
    void checkErr(int err);

    static int toColorSpace(CanvasColorSpace cs);
    static LayerType fromLayerType(int t);
    static int toBlendMode(BlendMode m);
    static BlendMode fromBlendMode(int m);
    static int toExportFormat(ExportFormat f);
    static int toExportResolution(ExportResolution r);

    GopostEngine* enginePtr_ = nullptr;
    bool initialized_ = false;
    int nextCanvasId_ = 1;
    std::map<int, GopostCanvas*> canvases_;
};

} // namespace gopost::rendering
