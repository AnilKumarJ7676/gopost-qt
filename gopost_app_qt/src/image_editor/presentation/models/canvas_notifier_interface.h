#pragma once

#include "rendering_bridge/engine_types.h"

#include <QByteArray>

#include <optional>

namespace gopost::image_editor {

/// Abstract interface consumed by EditCommand subclasses.
/// The concrete CanvasNotifier (presentation layer) inherits this.
class CanvasNotifier {
public:
    virtual ~CanvasNotifier() = default;

    /// Returns the new layer id, or std::nullopt on error.
    virtual std::optional<int> addSolidLayer(double r, double g, double b, double a) = 0;

    /// Returns the new layer id, or std::nullopt on error.
    virtual std::optional<int> addImageLayer(const QByteArray& pixels, int width, int height) = 0;

    virtual void removeLayer(int layerId) = 0;
    virtual void reorderLayer(int layerId, int newIndex) = 0;
    virtual void setLayerOpacity(int layerId, double opacity) = 0;
    virtual void setLayerVisible(int layerId, bool visible) = 0;
    virtual void setLayerBlendMode(int layerId, rendering::BlendMode mode) = 0;
};

} // namespace gopost::image_editor
