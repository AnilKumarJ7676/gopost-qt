#pragma once

#include "image_editor/domain/repositories/canvas_repository.h"
#include "rendering_bridge/engine_types.h"

namespace gopost::image_editor {

/// SRP: Triggers a full canvas render and returns the composited image.
class RenderCanvasUseCase {
public:
    explicit RenderCanvasUseCase(RenderRepository& repository);

    rendering::DecodedImage operator()(int canvasId);

private:
    RenderRepository& m_repository;
};

/// SRP: Invalidates the canvas to force a re-render.
class InvalidateCanvasUseCase {
public:
    explicit InvalidateCanvasUseCase(RenderRepository& repository);

    void operator()(int canvasId);

private:
    RenderRepository& m_repository;
};

} // namespace gopost::image_editor
