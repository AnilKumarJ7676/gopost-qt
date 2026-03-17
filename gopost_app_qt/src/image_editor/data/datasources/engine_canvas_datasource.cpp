#include "image_editor/data/datasources/engine_canvas_datasource.h"
#include "image_editor/data/datasources/image_editor_engine.h"

namespace gopost::image_editor {

EngineCanvasDataSourceImpl::EngineCanvasDataSourceImpl(ImageEditorEngine& engine)
    : m_engine(engine) {}

int EngineCanvasDataSourceImpl::createCanvas(const rendering::CanvasConfig& config) {
    return m_engine.createCanvas(config);
}

void EngineCanvasDataSourceImpl::destroyCanvas(int canvasId) {
    m_engine.destroyCanvas(canvasId);
}

rendering::CanvasSize EngineCanvasDataSourceImpl::getCanvasSize(int canvasId) {
    return m_engine.getCanvasSize(canvasId);
}

void EngineCanvasDataSourceImpl::resizeCanvas(int canvasId, int width, int height) {
    m_engine.resizeCanvas(canvasId, width, height);
}

int EngineCanvasDataSourceImpl::addImageLayer(int canvasId, const QByteArray& pixels,
                                              int width, int height, int index) {
    return m_engine.addImageLayer(canvasId, pixels, width, height, index);
}

int EngineCanvasDataSourceImpl::addSolidLayer(int canvasId,
                                              double r, double g, double b, double a,
                                              int width, int height, int index) {
    return m_engine.addSolidLayer(canvasId, r, g, b, a, width, height, index);
}

int EngineCanvasDataSourceImpl::addGroupLayer(int canvasId, const QString& name,
                                              int index) {
    return m_engine.addGroupLayer(canvasId, name, index);
}

void EngineCanvasDataSourceImpl::removeLayer(int canvasId, int layerId) {
    m_engine.removeLayer(canvasId, layerId);
}

void EngineCanvasDataSourceImpl::reorderLayer(int canvasId, int layerId, int newIndex) {
    m_engine.reorderLayer(canvasId, layerId, newIndex);
}

int EngineCanvasDataSourceImpl::duplicateLayer(int canvasId, int layerId) {
    return m_engine.duplicateLayer(canvasId, layerId);
}

int EngineCanvasDataSourceImpl::getLayerCount(int canvasId) {
    return m_engine.getLayerCount(canvasId);
}

rendering::LayerInfo EngineCanvasDataSourceImpl::getLayerInfo(int canvasId, int layerId) {
    return m_engine.getLayerInfo(canvasId, layerId);
}

QList<int> EngineCanvasDataSourceImpl::getLayerIds(int canvasId) {
    return m_engine.getLayerIds(canvasId);
}

void EngineCanvasDataSourceImpl::setLayerVisible(int canvasId, int layerId, bool visible) {
    m_engine.setLayerVisible(canvasId, layerId, visible);
}

void EngineCanvasDataSourceImpl::setLayerLocked(int canvasId, int layerId, bool locked) {
    m_engine.setLayerLocked(canvasId, layerId, locked);
}

void EngineCanvasDataSourceImpl::setLayerOpacity(int canvasId, int layerId, double opacity) {
    m_engine.setLayerOpacity(canvasId, layerId, opacity);
}

void EngineCanvasDataSourceImpl::setLayerBlendMode(int canvasId, int layerId,
                                                   rendering::BlendMode blendMode) {
    m_engine.setLayerBlendMode(canvasId, layerId, blendMode);
}

void EngineCanvasDataSourceImpl::setLayerName(int canvasId, int layerId, const QString& name) {
    m_engine.setLayerName(canvasId, layerId, name);
}

void EngineCanvasDataSourceImpl::setLayerTransform(int canvasId, int layerId,
                                                   double tx, double ty,
                                                   double sx, double sy,
                                                   double rotation) {
    m_engine.setLayerTransform(canvasId, layerId, tx, ty, sx, sy, rotation);
}

rendering::DecodedImage EngineCanvasDataSourceImpl::renderCanvas(int canvasId) {
    return m_engine.renderCanvas(canvasId);
}

void EngineCanvasDataSourceImpl::invalidateCanvas(int canvasId) {
    m_engine.invalidateCanvas(canvasId);
}

rendering::DecodedImage EngineCanvasDataSourceImpl::decodeImageFile(const QString& path) {
    return m_engine.decodeImageFile(path);
}

rendering::DecodedImage EngineCanvasDataSourceImpl::decodeImageBytes(const QByteArray& data) {
    return m_engine.decodeImageBytes(data);
}

void EngineCanvasDataSourceImpl::encodeToFile(const rendering::DecodedImage& image,
                                              const QString& path,
                                              rendering::ImageFormat format,
                                              int quality) {
    m_engine.encodeToFile(image, path, format, quality);
}

} // namespace gopost::image_editor
