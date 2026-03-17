#pragma once

#include "image_editor/data/datasources/engine_canvas_datasource.h"
#include "image_editor/domain/entities/canvas_entity.h"
#include "image_editor/domain/entities/layer_entity.h"
#include "image_editor/domain/repositories/canvas_repository.h"

namespace gopost::image_editor {

/// DIP: Implements all canvas repository interfaces.
/// SRP: Maps datasource calls to domain entities + error handling.
class CanvasRepositoryImpl
    : public CanvasRepository
    , public LayerRepository
    , public LayerPropertyRepository
    , public RenderRepository
    , public ImageImportRepository {
public:
    explicit CanvasRepositoryImpl(EngineCanvasDataSource& dataSource);

    // -- CanvasRepository --
    CanvasEntity createCanvas(const rendering::CanvasConfig& config) override;
    void destroyCanvas(int canvasId) override;
    void resizeCanvas(int canvasId, int width, int height) override;

    // -- LayerRepository --
    LayerEntity addImageLayer(int canvasId, const QByteArray& rgbaPixels,
                              int width, int height, int index = -1) override;
    LayerEntity addSolidColorLayer(int canvasId,
                                   double r, double g, double b, double a,
                                   int width, int height, int index = -1) override;
    LayerEntity addGroupLayer(int canvasId, const QString& name,
                              int index = -1) override;
    void removeLayer(int canvasId, int layerId) override;
    void reorderLayer(int canvasId, int layerId, int newIndex) override;
    LayerEntity duplicateLayer(int canvasId, int layerId) override;
    QList<LayerEntity> getAllLayers(int canvasId) override;
    LayerEntity getLayerInfo(int canvasId, int layerId) override;

    // -- LayerPropertyRepository --
    void setVisible(int canvasId, int layerId, bool visible) override;
    void setLocked(int canvasId, int layerId, bool locked) override;
    void setOpacity(int canvasId, int layerId, double opacity) override;
    void setBlendMode(int canvasId, int layerId, rendering::BlendMode mode) override;
    void setName(int canvasId, int layerId, const QString& name) override;
    void setTransform(int canvasId, int layerId,
                      double tx = 0.0, double ty = 0.0,
                      double sx = 1.0, double sy = 1.0,
                      double rotation = 0.0) override;

    // -- RenderRepository --
    rendering::DecodedImage renderCanvas(int canvasId) override;
    void invalidateCanvas(int canvasId) override;

    // -- ImageImportRepository --
    rendering::DecodedImage decodeImageFile(const QString& path) override;
    rendering::DecodedImage decodeImageBytes(const QByteArray& data) override;
    void encodeToFile(const rendering::DecodedImage& image,
                      const QString& path,
                      rendering::ImageFormat format,
                      int quality = 85) override;

private:
    EngineCanvasDataSource& m_dataSource;
};

} // namespace gopost::image_editor
