#pragma once

#include "rendering_bridge/engine_types.h"

#include <optional>

namespace gopost::image_editor {

/// Represents an active canvas in the image editor.
class CanvasEntity {
public:
    int canvasId = 0;
    int width = 0;
    int height = 0;
    double dpi = 72.0;
    rendering::CanvasColorSpace colorSpace = rendering::CanvasColorSpace::Srgb;
    bool transparentBackground = false;

    CanvasEntity() = default;

    CanvasEntity(int canvasId, int width, int height,
                 double dpi = 72.0,
                 rendering::CanvasColorSpace colorSpace = rendering::CanvasColorSpace::Srgb,
                 bool transparentBackground = false)
        : canvasId(canvasId)
        , width(width)
        , height(height)
        , dpi(dpi)
        , colorSpace(colorSpace)
        , transparentBackground(transparentBackground) {}

    [[nodiscard]] double aspectRatio() const {
        return height != 0 ? static_cast<double>(width) / height : 0.0;
    }

    [[nodiscard]] CanvasEntity copyWith(
        std::optional<int> newWidth = std::nullopt,
        std::optional<int> newHeight = std::nullopt,
        std::optional<double> newDpi = std::nullopt) const
    {
        return CanvasEntity(
            canvasId,
            newWidth.value_or(width),
            newHeight.value_or(height),
            newDpi.value_or(dpi),
            colorSpace,
            transparentBackground);
    }

    bool operator==(const CanvasEntity& other) const = default;
};

} // namespace gopost::image_editor
