#pragma once

#include "image_editor/domain/entities/canvas_entity.h"
#include "image_editor/domain/repositories/canvas_repository.h"
#include "rendering_bridge/engine_types.h"

#include <QString>

namespace gopost::image_editor {

/// SRP: Creates a new blank canvas with the given configuration.
class CreateCanvasUseCase {
public:
    explicit CreateCanvasUseCase(CanvasRepository& repository);

    CanvasEntity operator()(const rendering::CanvasConfig& config);

private:
    CanvasRepository& m_repository;
};

/// SRP: Creates a canvas pre-populated with an imported image layer.
struct CreateCanvasFromImageResult {
    CanvasEntity canvas;
    int layerId = 0;
};

class CreateCanvasFromImageUseCase {
public:
    CreateCanvasFromImageUseCase(CanvasRepository& canvasRepo,
                                 ImageImportRepository& importRepo,
                                 LayerRepository& layerRepo);

    CreateCanvasFromImageResult operator()(const QString& imagePath);

private:
    CanvasRepository& m_canvasRepo;
    ImageImportRepository& m_importRepo;
    LayerRepository& m_layerRepo;
};

} // namespace gopost::image_editor
