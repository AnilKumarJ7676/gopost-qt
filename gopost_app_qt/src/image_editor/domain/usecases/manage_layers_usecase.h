#pragma once

#include "image_editor/domain/entities/layer_entity.h"
#include "image_editor/domain/repositories/canvas_repository.h"
#include "rendering_bridge/engine_types.h"

#include <QByteArray>
#include <QList>
#include <QString>

namespace gopost::image_editor {

/// SRP: Adds an image layer from raw RGBA pixels.
class AddImageLayerUseCase {
public:
    explicit AddImageLayerUseCase(LayerRepository& repository);

    LayerEntity operator()(int canvasId, const QByteArray& pixels,
                           int width, int height, int index = -1);

private:
    LayerRepository& m_repository;
};

/// SRP: Adds a solid-color fill layer.
class AddSolidLayerUseCase {
public:
    explicit AddSolidLayerUseCase(LayerRepository& repository);

    LayerEntity operator()(int canvasId, double r, double g, double b, double a,
                           int width, int height, int index = -1);

private:
    LayerRepository& m_repository;
};

/// SRP: Removes a layer by ID.
class RemoveLayerUseCase {
public:
    explicit RemoveLayerUseCase(LayerRepository& repository);

    void operator()(int canvasId, int layerId);

private:
    LayerRepository& m_repository;
};

/// SRP: Reorders a layer to a new position.
class ReorderLayerUseCase {
public:
    explicit ReorderLayerUseCase(LayerRepository& repository);

    void operator()(int canvasId, int layerId, int newIndex);

private:
    LayerRepository& m_repository;
};

/// SRP: Duplicates an existing layer.
class DuplicateLayerUseCase {
public:
    explicit DuplicateLayerUseCase(LayerRepository& repository);

    LayerEntity operator()(int canvasId, int layerId);

private:
    LayerRepository& m_repository;
};

/// SRP: Updates a single layer property (opacity, visibility, blend mode, etc.).
class UpdateLayerPropertyUseCase {
public:
    explicit UpdateLayerPropertyUseCase(LayerPropertyRepository& repository);

    void setOpacity(int canvasId, int layerId, double opacity);
    void setVisible(int canvasId, int layerId, bool visible);
    void setLocked(int canvasId, int layerId, bool locked);
    void setBlendMode(int canvasId, int layerId, rendering::BlendMode mode);
    void setName(int canvasId, int layerId, const QString& name);

private:
    LayerPropertyRepository& m_repository;
};

/// SRP: Fetches all layers for a canvas.
class GetAllLayersUseCase {
public:
    explicit GetAllLayersUseCase(LayerRepository& repository);

    QList<LayerEntity> operator()(int canvasId);

private:
    LayerRepository& m_repository;
};

} // namespace gopost::image_editor
