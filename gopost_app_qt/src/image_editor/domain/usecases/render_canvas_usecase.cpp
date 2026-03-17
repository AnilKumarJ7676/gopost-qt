#include "image_editor/domain/usecases/render_canvas_usecase.h"

namespace gopost::image_editor {

// =========================================================================
// RenderCanvasUseCase
// =========================================================================

RenderCanvasUseCase::RenderCanvasUseCase(RenderRepository& repository)
    : m_repository(repository) {}

rendering::DecodedImage RenderCanvasUseCase::operator()(int canvasId) {
    return m_repository.renderCanvas(canvasId);
}

// =========================================================================
// InvalidateCanvasUseCase
// =========================================================================

InvalidateCanvasUseCase::InvalidateCanvasUseCase(RenderRepository& repository)
    : m_repository(repository) {}

void InvalidateCanvasUseCase::operator()(int canvasId) {
    m_repository.invalidateCanvas(canvasId);
}

} // namespace gopost::image_editor
