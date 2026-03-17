#pragma once

#include "image_editor/domain/entities/filter_entity.h"
#include "image_editor/domain/repositories/filter_repository.h"

namespace gopost::image_editor {

// Forward declaration for the engine interface.
class ImageEditorEngine;

class FilterRepositoryImpl
    : public EffectQueryRepository
    , public AdjustmentRepository
    , public PresetFilterRepository {
public:
    explicit FilterRepositoryImpl(ImageEditorEngine& engine);

    // -- EffectQueryRepository --
    QList<rendering::EffectDef> getAdjustmentEffects() override;
    QList<FilterCategory> getPresetCategories() override;

    // -- AdjustmentRepository --
    int addAdjustment(int canvasId, int layerId, const QString& effectId) override;
    void setAdjustmentParam(int canvasId, int layerId, int instanceId,
                            const QString& paramId, double value) override;
    void removeAdjustment(int canvasId, int layerId, int instanceId) override;

    // -- PresetFilterRepository --
    std::optional<rendering::DecodedImage> applyPreset(
        int canvasId, int layerId, int presetIndex, double intensity) override;

private:
    ImageEditorEngine& m_engine;
};

} // namespace gopost::image_editor
