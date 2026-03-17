#include "image_editor/domain/usecases/manage_layers_usecase.h"

namespace gopost::image_editor {

// =========================================================================
// AddImageLayerUseCase
// =========================================================================

AddImageLayerUseCase::AddImageLayerUseCase(LayerRepository& repository)
    : m_repository(repository) {}

LayerEntity AddImageLayerUseCase::operator()(int canvasId,
                                             const QByteArray& pixels,
                                             int width, int height,
                                             int index) {
    return m_repository.addImageLayer(canvasId, pixels, width, height, index);
}

// =========================================================================
// AddSolidLayerUseCase
// =========================================================================

AddSolidLayerUseCase::AddSolidLayerUseCase(LayerRepository& repository)
    : m_repository(repository) {}

LayerEntity AddSolidLayerUseCase::operator()(int canvasId,
                                             double r, double g, double b, double a,
                                             int width, int height,
                                             int index) {
    return m_repository.addSolidColorLayer(canvasId, r, g, b, a, width, height, index);
}

// =========================================================================
// RemoveLayerUseCase
// =========================================================================

RemoveLayerUseCase::RemoveLayerUseCase(LayerRepository& repository)
    : m_repository(repository) {}

void RemoveLayerUseCase::operator()(int canvasId, int layerId) {
    m_repository.removeLayer(canvasId, layerId);
}

// =========================================================================
// ReorderLayerUseCase
// =========================================================================

ReorderLayerUseCase::ReorderLayerUseCase(LayerRepository& repository)
    : m_repository(repository) {}

void ReorderLayerUseCase::operator()(int canvasId, int layerId, int newIndex) {
    m_repository.reorderLayer(canvasId, layerId, newIndex);
}

// =========================================================================
// DuplicateLayerUseCase
// =========================================================================

DuplicateLayerUseCase::DuplicateLayerUseCase(LayerRepository& repository)
    : m_repository(repository) {}

LayerEntity DuplicateLayerUseCase::operator()(int canvasId, int layerId) {
    return m_repository.duplicateLayer(canvasId, layerId);
}

// =========================================================================
// UpdateLayerPropertyUseCase
// =========================================================================

UpdateLayerPropertyUseCase::UpdateLayerPropertyUseCase(LayerPropertyRepository& repository)
    : m_repository(repository) {}

void UpdateLayerPropertyUseCase::setOpacity(int canvasId, int layerId, double opacity) {
    m_repository.setOpacity(canvasId, layerId, opacity);
}

void UpdateLayerPropertyUseCase::setVisible(int canvasId, int layerId, bool visible) {
    m_repository.setVisible(canvasId, layerId, visible);
}

void UpdateLayerPropertyUseCase::setLocked(int canvasId, int layerId, bool locked) {
    m_repository.setLocked(canvasId, layerId, locked);
}

void UpdateLayerPropertyUseCase::setBlendMode(int canvasId, int layerId,
                                              rendering::BlendMode mode) {
    m_repository.setBlendMode(canvasId, layerId, mode);
}

void UpdateLayerPropertyUseCase::setName(int canvasId, int layerId, const QString& name) {
    m_repository.setName(canvasId, layerId, name);
}

// =========================================================================
// GetAllLayersUseCase
// =========================================================================

GetAllLayersUseCase::GetAllLayersUseCase(LayerRepository& repository)
    : m_repository(repository) {}

QList<LayerEntity> GetAllLayersUseCase::operator()(int canvasId) {
    return m_repository.getAllLayers(canvasId);
}

} // namespace gopost::image_editor
