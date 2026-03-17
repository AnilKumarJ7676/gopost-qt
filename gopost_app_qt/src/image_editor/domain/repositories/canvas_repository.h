#pragma once

#include "image_editor/domain/entities/canvas_entity.h"
#include "image_editor/domain/entities/layer_entity.h"
#include "rendering_bridge/engine_types.h"

#include <QByteArray>
#include <QList>
#include <QString>

namespace gopost::image_editor {

/// ISP: Canvas lifecycle operations.
class CanvasRepository {
public:
    virtual ~CanvasRepository() = default;

    virtual CanvasEntity createCanvas(const rendering::CanvasConfig& config) = 0;
    virtual void destroyCanvas(int canvasId) = 0;
    virtual void resizeCanvas(int canvasId, int width, int height) = 0;
};

/// ISP: Layer CRUD operations.
class LayerRepository {
public:
    virtual ~LayerRepository() = default;

    virtual LayerEntity addImageLayer(int canvasId,
                                      const QByteArray& rgbaPixels,
                                      int width, int height,
                                      int index = -1) = 0;
    virtual LayerEntity addSolidColorLayer(int canvasId,
                                           double r, double g, double b, double a,
                                           int width, int height,
                                           int index = -1) = 0;
    virtual LayerEntity addGroupLayer(int canvasId, const QString& name,
                                      int index = -1) = 0;
    virtual void removeLayer(int canvasId, int layerId) = 0;
    virtual void reorderLayer(int canvasId, int layerId, int newIndex) = 0;
    virtual LayerEntity duplicateLayer(int canvasId, int layerId) = 0;
    virtual QList<LayerEntity> getAllLayers(int canvasId) = 0;
    virtual LayerEntity getLayerInfo(int canvasId, int layerId) = 0;
};

/// ISP: Layer property mutations.
class LayerPropertyRepository {
public:
    virtual ~LayerPropertyRepository() = default;

    virtual void setVisible(int canvasId, int layerId, bool visible) = 0;
    virtual void setLocked(int canvasId, int layerId, bool locked) = 0;
    virtual void setOpacity(int canvasId, int layerId, double opacity) = 0;
    virtual void setBlendMode(int canvasId, int layerId, rendering::BlendMode mode) = 0;
    virtual void setName(int canvasId, int layerId, const QString& name) = 0;
    virtual void setTransform(int canvasId, int layerId,
                              double tx = 0.0, double ty = 0.0,
                              double sx = 1.0, double sy = 1.0,
                              double rotation = 0.0) = 0;
};

/// ISP: Rendering operations.
class RenderRepository {
public:
    virtual ~RenderRepository() = default;

    virtual rendering::DecodedImage renderCanvas(int canvasId) = 0;
    virtual void invalidateCanvas(int canvasId) = 0;
};

/// ISP: Image import / codec operations.
class ImageImportRepository {
public:
    virtual ~ImageImportRepository() = default;

    virtual rendering::DecodedImage decodeImageFile(const QString& path) = 0;
    virtual rendering::DecodedImage decodeImageBytes(const QByteArray& data) = 0;
    virtual void encodeToFile(const rendering::DecodedImage& image,
                              const QString& path,
                              rendering::ImageFormat format,
                              int quality = 85) = 0;
};

} // namespace gopost::image_editor
