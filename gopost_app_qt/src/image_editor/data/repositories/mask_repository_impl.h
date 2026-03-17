#pragma once

#include "image_editor/domain/repositories/mask_repository.h"

namespace gopost::image_editor {

// Forward declaration for the engine mask manager.
class LayerMaskManager;

class MaskRepositoryImpl : public MaskRepository {
public:
    explicit MaskRepositoryImpl(LayerMaskManager& maskManager);

    void addMask(int canvasId, int layerId, rendering::MaskType type) override;
    void removeMask(int canvasId, int layerId) override;
    bool hasMask(int canvasId, int layerId) override;
    void invertMask(int canvasId, int layerId) override;
    void setMaskEnabled(int canvasId, int layerId, bool enabled) override;
    void paintMask(int canvasId, int layerId,
                   double cx, double cy,
                   double radius, double hardness,
                   rendering::MaskBrushMode mode, double opacity) override;
    void fillMask(int canvasId, int layerId, int value) override;

private:
    LayerMaskManager& m_maskManager;
};

} // namespace gopost::image_editor
