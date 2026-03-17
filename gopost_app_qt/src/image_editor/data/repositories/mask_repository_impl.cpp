#include "image_editor/data/repositories/mask_repository_impl.h"

#include "core/error/exceptions.h"

namespace gopost::image_editor {

/// Minimal abstract interface that the engine mask manager must satisfy.
/// The concrete type lives in the rendering_bridge module.
class LayerMaskManager {
public:
    virtual ~LayerMaskManager() = default;
    virtual void addMask(int canvasId, int layerId, rendering::MaskType type) = 0;
    virtual void removeMask(int canvasId, int layerId) = 0;
    virtual bool hasMask(int canvasId, int layerId) = 0;
    virtual void invertMask(int canvasId, int layerId) = 0;
    virtual void setMaskEnabled(int canvasId, int layerId, bool enabled) = 0;
    virtual void maskPaint(int canvasId, int layerId,
                           double cx, double cy,
                           double radius, double hardness,
                           rendering::MaskBrushMode mode, double opacity) = 0;
    virtual void maskFill(int canvasId, int layerId, int value) = 0;
};

MaskRepositoryImpl::MaskRepositoryImpl(LayerMaskManager& maskManager)
    : m_maskManager(maskManager) {}

void MaskRepositoryImpl::addMask(int canvasId, int layerId, rendering::MaskType type) {
    try {
        m_maskManager.addMask(canvasId, layerId, type);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Add mask failed: %1").arg(e.what()));
    }
}

void MaskRepositoryImpl::removeMask(int canvasId, int layerId) {
    try {
        m_maskManager.removeMask(canvasId, layerId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Remove mask failed: %1").arg(e.what()));
    }
}

bool MaskRepositoryImpl::hasMask(int canvasId, int layerId) {
    try {
        return m_maskManager.hasMask(canvasId, layerId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Query mask failed: %1").arg(e.what()));
    }
}

void MaskRepositoryImpl::invertMask(int canvasId, int layerId) {
    try {
        m_maskManager.invertMask(canvasId, layerId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Invert mask failed: %1").arg(e.what()));
    }
}

void MaskRepositoryImpl::setMaskEnabled(int canvasId, int layerId, bool enabled) {
    try {
        m_maskManager.setMaskEnabled(canvasId, layerId, enabled);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Set mask enabled failed: %1").arg(e.what()));
    }
}

void MaskRepositoryImpl::paintMask(int canvasId, int layerId,
                                   double cx, double cy,
                                   double radius, double hardness,
                                   rendering::MaskBrushMode mode, double opacity) {
    try {
        m_maskManager.maskPaint(canvasId, layerId, cx, cy,
                                radius, hardness, mode, opacity);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Paint mask failed: %1").arg(e.what()));
    }
}

void MaskRepositoryImpl::fillMask(int canvasId, int layerId, int value) {
    try {
        m_maskManager.maskFill(canvasId, layerId, value);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Fill mask failed: %1").arg(e.what()));
    }
}

} // namespace gopost::image_editor
