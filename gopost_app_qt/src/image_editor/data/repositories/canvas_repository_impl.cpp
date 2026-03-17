#include "image_editor/data/repositories/canvas_repository_impl.h"

#include "core/error/exceptions.h"

namespace gopost::image_editor {

CanvasRepositoryImpl::CanvasRepositoryImpl(EngineCanvasDataSource& dataSource)
    : m_dataSource(dataSource) {}

// =========================================================================
// CanvasRepository
// =========================================================================

CanvasEntity CanvasRepositoryImpl::createCanvas(const rendering::CanvasConfig& config) {
    try {
        const int canvasId = m_dataSource.createCanvas(config);
        return CanvasEntity(
            canvasId, config.width, config.height,
            config.dpi, config.colorSpace, config.transparentBackground);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to create canvas: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::destroyCanvas(int canvasId) {
    try {
        m_dataSource.destroyCanvas(canvasId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to destroy canvas: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::resizeCanvas(int canvasId, int width, int height) {
    try {
        m_dataSource.resizeCanvas(canvasId, width, height);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to resize canvas: %1").arg(e.what()));
    }
}

// =========================================================================
// LayerRepository
// =========================================================================

LayerEntity CanvasRepositoryImpl::addImageLayer(int canvasId,
                                                const QByteArray& rgbaPixels,
                                                int width, int height,
                                                int index) {
    try {
        const int layerId = m_dataSource.addImageLayer(
            canvasId, rgbaPixels, width, height, index);
        const auto info = m_dataSource.getLayerInfo(canvasId, layerId);
        return LayerEntity::fromLayerInfo(info);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to add image layer: %1").arg(e.what()));
    }
}

LayerEntity CanvasRepositoryImpl::addSolidColorLayer(int canvasId,
                                                     double r, double g, double b, double a,
                                                     int width, int height,
                                                     int index) {
    try {
        const int layerId = m_dataSource.addSolidLayer(
            canvasId, r, g, b, a, width, height, index);
        const auto info = m_dataSource.getLayerInfo(canvasId, layerId);
        return LayerEntity::fromLayerInfo(info);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to add solid layer: %1").arg(e.what()));
    }
}

LayerEntity CanvasRepositoryImpl::addGroupLayer(int canvasId, const QString& name,
                                                int index) {
    try {
        const int layerId = m_dataSource.addGroupLayer(canvasId, name, index);
        const auto info = m_dataSource.getLayerInfo(canvasId, layerId);
        return LayerEntity::fromLayerInfo(info);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to add group layer: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::removeLayer(int canvasId, int layerId) {
    try {
        m_dataSource.removeLayer(canvasId, layerId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to remove layer: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::reorderLayer(int canvasId, int layerId, int newIndex) {
    try {
        m_dataSource.reorderLayer(canvasId, layerId, newIndex);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to reorder layer: %1").arg(e.what()));
    }
}

LayerEntity CanvasRepositoryImpl::duplicateLayer(int canvasId, int layerId) {
    try {
        const int newId = m_dataSource.duplicateLayer(canvasId, layerId);
        const auto info = m_dataSource.getLayerInfo(canvasId, newId);
        return LayerEntity::fromLayerInfo(info);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to duplicate layer: %1").arg(e.what()));
    }
}

QList<LayerEntity> CanvasRepositoryImpl::getAllLayers(int canvasId) {
    try {
        const auto ids = m_dataSource.getLayerIds(canvasId);
        QList<LayerEntity> layers;
        layers.reserve(ids.size());
        for (const int id : ids) {
            const auto info = m_dataSource.getLayerInfo(canvasId, id);
            layers.append(LayerEntity::fromLayerInfo(info));
        }
        return layers;
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to get layers: %1").arg(e.what()));
    }
}

LayerEntity CanvasRepositoryImpl::getLayerInfo(int canvasId, int layerId) {
    try {
        const auto info = m_dataSource.getLayerInfo(canvasId, layerId);
        return LayerEntity::fromLayerInfo(info);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to get layer info: %1").arg(e.what()));
    }
}

// =========================================================================
// LayerPropertyRepository
// =========================================================================

void CanvasRepositoryImpl::setVisible(int canvasId, int layerId, bool visible) {
    try {
        m_dataSource.setLayerVisible(canvasId, layerId, visible);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to set visibility: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::setLocked(int canvasId, int layerId, bool locked) {
    try {
        m_dataSource.setLayerLocked(canvasId, layerId, locked);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to set locked: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::setOpacity(int canvasId, int layerId, double opacity) {
    try {
        m_dataSource.setLayerOpacity(canvasId, layerId, opacity);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to set opacity: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::setBlendMode(int canvasId, int layerId,
                                        rendering::BlendMode mode) {
    try {
        m_dataSource.setLayerBlendMode(canvasId, layerId, mode);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to set blend mode: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::setName(int canvasId, int layerId, const QString& name) {
    try {
        m_dataSource.setLayerName(canvasId, layerId, name);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to set name: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::setTransform(int canvasId, int layerId,
                                        double tx, double ty,
                                        double sx, double sy,
                                        double rotation) {
    try {
        m_dataSource.setLayerTransform(canvasId, layerId, tx, ty, sx, sy, rotation);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to set transform: %1").arg(e.what()));
    }
}

// =========================================================================
// RenderRepository
// =========================================================================

rendering::DecodedImage CanvasRepositoryImpl::renderCanvas(int canvasId) {
    try {
        return m_dataSource.renderCanvas(canvasId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to render canvas: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::invalidateCanvas(int canvasId) {
    try {
        m_dataSource.invalidateCanvas(canvasId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to invalidate canvas: %1").arg(e.what()));
    }
}

// =========================================================================
// ImageImportRepository
// =========================================================================

rendering::DecodedImage CanvasRepositoryImpl::decodeImageFile(const QString& path) {
    try {
        return m_dataSource.decodeImageFile(path);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to decode image: %1").arg(e.what()));
    }
}

rendering::DecodedImage CanvasRepositoryImpl::decodeImageBytes(const QByteArray& data) {
    try {
        return m_dataSource.decodeImageBytes(data);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to decode image bytes: %1").arg(e.what()));
    }
}

void CanvasRepositoryImpl::encodeToFile(const rendering::DecodedImage& image,
                                        const QString& path,
                                        rendering::ImageFormat format,
                                        int quality) {
    try {
        m_dataSource.encodeToFile(image, path, format, quality);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to encode image: %1").arg(e.what()));
    }
}

} // namespace gopost::image_editor
