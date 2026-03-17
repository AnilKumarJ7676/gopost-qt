#pragma once

#include "image_editor/domain/entities/filter_entity.h"
#include "rendering_bridge/engine_types.h"

#include <QList>
#include <QString>

#include <optional>

namespace gopost::image_editor {

/// ISP: Queries available effects/filters from the engine.
class EffectQueryRepository {
public:
    virtual ~EffectQueryRepository() = default;

    virtual QList<rendering::EffectDef> getAdjustmentEffects() = 0;
    virtual QList<FilterCategory> getPresetCategories() = 0;
};

/// ISP: Applies adjustments to a layer.
class AdjustmentRepository {
public:
    virtual ~AdjustmentRepository() = default;

    virtual int addAdjustment(int canvasId, int layerId, const QString& effectId) = 0;
    virtual void setAdjustmentParam(int canvasId, int layerId, int instanceId,
                                    const QString& paramId, double value) = 0;
    virtual void removeAdjustment(int canvasId, int layerId, int instanceId) = 0;
};

/// ISP: Applies preset filters.
/// Returns the filtered image when the engine produced one (caller should
/// replace canvas with it).
class PresetFilterRepository {
public:
    virtual ~PresetFilterRepository() = default;

    virtual std::optional<rendering::DecodedImage> applyPreset(
        int canvasId, int layerId, int presetIndex, double intensity) = 0;
};

} // namespace gopost::image_editor
