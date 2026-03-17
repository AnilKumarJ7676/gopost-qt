#pragma once

#include "rendering_bridge/engine_types.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <optional>
#include <vector>

namespace gopost::image_editor {

/// Abstract interface for the image-editor engine.
/// The concrete implementation lives in the rendering_bridge module
/// (either the native or stub engine).  Code in the image_editor module
/// depends only on this interface (DIP).
class ImageEditorEngine {
public:
    virtual ~ImageEditorEngine() = default;

    // Canvas
    virtual int createCanvas(const rendering::CanvasConfig& config) = 0;
    virtual void destroyCanvas(int canvasId) = 0;
    virtual rendering::CanvasSize getCanvasSize(int canvasId) = 0;
    virtual void resizeCanvas(int canvasId, int width, int height) = 0;

    // Layers
    virtual int addImageLayer(int canvasId, const QByteArray& pixels,
                              int width, int height, int index = -1) = 0;
    virtual int addSolidLayer(int canvasId,
                              double r, double g, double b, double a,
                              int width, int height, int index = -1) = 0;
    virtual int addGroupLayer(int canvasId, const QString& name, int index = -1) = 0;
    virtual void removeLayer(int canvasId, int layerId) = 0;
    virtual void reorderLayer(int canvasId, int layerId, int newIndex) = 0;
    virtual int duplicateLayer(int canvasId, int layerId) = 0;

    virtual int getLayerCount(int canvasId) = 0;
    virtual rendering::LayerInfo getLayerInfo(int canvasId, int layerId) = 0;
    virtual QList<int> getLayerIds(int canvasId) = 0;

    // Layer properties
    virtual void setLayerVisible(int canvasId, int layerId, bool visible) = 0;
    virtual void setLayerLocked(int canvasId, int layerId, bool locked) = 0;
    virtual void setLayerOpacity(int canvasId, int layerId, double opacity) = 0;
    virtual void setLayerBlendMode(int canvasId, int layerId,
                                   rendering::BlendMode blendMode) = 0;
    virtual void setLayerName(int canvasId, int layerId, const QString& name) = 0;
    virtual void setLayerTransform(int canvasId, int layerId,
                                   double tx = 0.0, double ty = 0.0,
                                   double sx = 1.0, double sy = 1.0,
                                   double rotation = 0.0) = 0;

    // Render
    virtual rendering::DecodedImage renderCanvas(int canvasId) = 0;
    virtual void invalidateCanvas(int canvasId) = 0;

    // Image decode/encode
    virtual rendering::DecodedImage decodeImageFile(const QString& path) = 0;
    virtual rendering::DecodedImage decodeImageBytes(const QByteArray& data) = 0;
    virtual void encodeToFile(const rendering::DecodedImage& image,
                              const QString& path,
                              rendering::ImageFormat format,
                              int quality = 85) = 0;

    // Effects (used by filter_repository_impl and text_repository_impl)
    virtual QList<rendering::EffectDef> getEffectsByCategory(rendering::EffectCategory cat) = 0;
    virtual QList<rendering::PresetFilterInfo> getPresetFilters() = 0;
    virtual int addEffectToLayer(int canvasId, int layerId, const QString& effectId) = 0;
    virtual void setEffectParam(int canvasId, int layerId, int instanceId,
                                const QString& paramId, double value) = 0;
    virtual void removeEffectFromLayer(int canvasId, int layerId, int instanceId) = 0;
    virtual std::optional<rendering::DecodedImage> applyPreset(
        int canvasId, int layerId, int presetIndex, double intensity) = 0;

    // Text / Fonts
    virtual QList<QString> getAvailableFonts() = 0;
    virtual int addTextLayer(int canvasId, const rendering::TextConfig& config,
                             int maxWidth, int index = -1) = 0;
    virtual void updateTextLayer(int canvasId, int layerId,
                                 const rendering::TextConfig& config, int maxWidth) = 0;
};

} // namespace gopost::image_editor
