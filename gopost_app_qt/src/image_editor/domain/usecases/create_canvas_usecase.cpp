#include "image_editor/domain/usecases/create_canvas_usecase.h"

namespace gopost::image_editor {

// =========================================================================
// CreateCanvasUseCase
// =========================================================================

CreateCanvasUseCase::CreateCanvasUseCase(CanvasRepository& repository)
    : m_repository(repository) {}

CanvasEntity CreateCanvasUseCase::operator()(const rendering::CanvasConfig& config) {
    return m_repository.createCanvas(config);
}

// =========================================================================
// CreateCanvasFromImageUseCase
// =========================================================================

CreateCanvasFromImageUseCase::CreateCanvasFromImageUseCase(
    CanvasRepository& canvasRepo,
    ImageImportRepository& importRepo,
    LayerRepository& layerRepo)
    : m_canvasRepo(canvasRepo)
    , m_importRepo(importRepo)
    , m_layerRepo(layerRepo) {}

CreateCanvasFromImageResult CreateCanvasFromImageUseCase::operator()(
    const QString& imagePath)
{
    auto decoded = m_importRepo.decodeImageFile(imagePath);

    rendering::CanvasConfig config;
    config.width = decoded.width;
    config.height = decoded.height;

    auto canvas = m_canvasRepo.createCanvas(config);

    auto layer = m_layerRepo.addImageLayer(
        canvas.canvasId, decoded.pixels, decoded.width, decoded.height);

    return {canvas, layer.id};
}

} // namespace gopost::image_editor
