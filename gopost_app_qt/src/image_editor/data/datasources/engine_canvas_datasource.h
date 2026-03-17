#pragma once

#include "rendering_bridge/engine_types.h"

#include <QByteArray>
#include <QList>
#include <QString>

namespace gopost::image_editor {

/// SRP: Adapter between the domain layer and the native engine FFI.
/// DIP: Depends on abstract interface, not the FFI implementation.
class EngineCanvasDataSource {
public:
    virtual ~EngineCanvasDataSource() = default;

    virtual int createCanvas(const rendering::CanvasConfig& config) = 0;
    virtual void destroyCanvas(int canvasId) = 0;
    virtual rendering::CanvasSize getCanvasSize(int canvasId) = 0;
    virtual void resizeCanvas(int canvasId, int width, int height) = 0;

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

    virtual rendering::DecodedImage renderCanvas(int canvasId) = 0;
    virtual void invalidateCanvas(int canvasId) = 0;

    virtual rendering::DecodedImage decodeImageFile(const QString& path) = 0;
    virtual rendering::DecodedImage decodeImageBytes(const QByteArray& data) = 0;
    virtual void encodeToFile(const rendering::DecodedImage& image,
                              const QString& path,
                              rendering::ImageFormat format,
                              int quality = 85) = 0;
};

// Forward declaration for the engine interface consumed by the impl.
// The concrete ImageEditorEngine type lives in the rendering_bridge module.
class ImageEditorEngine;

/// LSP: Substitutable implementation backed by ImageEditorEngine.
class EngineCanvasDataSourceImpl : public EngineCanvasDataSource {
public:
    explicit EngineCanvasDataSourceImpl(ImageEditorEngine& engine);

    int createCanvas(const rendering::CanvasConfig& config) override;
    void destroyCanvas(int canvasId) override;
    rendering::CanvasSize getCanvasSize(int canvasId) override;
    void resizeCanvas(int canvasId, int width, int height) override;

    int addImageLayer(int canvasId, const QByteArray& pixels,
                      int width, int height, int index = -1) override;
    int addSolidLayer(int canvasId,
                      double r, double g, double b, double a,
                      int width, int height, int index = -1) override;
    int addGroupLayer(int canvasId, const QString& name, int index = -1) override;
    void removeLayer(int canvasId, int layerId) override;
    void reorderLayer(int canvasId, int layerId, int newIndex) override;
    int duplicateLayer(int canvasId, int layerId) override;

    int getLayerCount(int canvasId) override;
    rendering::LayerInfo getLayerInfo(int canvasId, int layerId) override;
    QList<int> getLayerIds(int canvasId) override;

    void setLayerVisible(int canvasId, int layerId, bool visible) override;
    void setLayerLocked(int canvasId, int layerId, bool locked) override;
    void setLayerOpacity(int canvasId, int layerId, double opacity) override;
    void setLayerBlendMode(int canvasId, int layerId,
                           rendering::BlendMode blendMode) override;
    void setLayerName(int canvasId, int layerId, const QString& name) override;
    void setLayerTransform(int canvasId, int layerId,
                           double tx = 0.0, double ty = 0.0,
                           double sx = 1.0, double sy = 1.0,
                           double rotation = 0.0) override;

    rendering::DecodedImage renderCanvas(int canvasId) override;
    void invalidateCanvas(int canvasId) override;

    rendering::DecodedImage decodeImageFile(const QString& path) override;
    rendering::DecodedImage decodeImageBytes(const QByteArray& data) override;
    void encodeToFile(const rendering::DecodedImage& image,
                      const QString& path,
                      rendering::ImageFormat format,
                      int quality = 85) override;

private:
    ImageEditorEngine& m_engine;
};

} // namespace gopost::image_editor
