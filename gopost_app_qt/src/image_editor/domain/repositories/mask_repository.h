#pragma once

#include "rendering_bridge/engine_types.h"

namespace gopost::image_editor {

class MaskRepository {
public:
    virtual ~MaskRepository() = default;

    virtual void addMask(int canvasId, int layerId, rendering::MaskType type) = 0;
    virtual void removeMask(int canvasId, int layerId) = 0;
    virtual bool hasMask(int canvasId, int layerId) = 0;
    virtual void invertMask(int canvasId, int layerId) = 0;
    virtual void setMaskEnabled(int canvasId, int layerId, bool enabled) = 0;
    virtual void paintMask(int canvasId, int layerId,
                           double cx, double cy,
                           double radius, double hardness,
                           rendering::MaskBrushMode mode, double opacity) = 0;
    virtual void fillMask(int canvasId, int layerId, int value) = 0;
};

} // namespace gopost::image_editor
