#pragma once

#include "rendering_bridge/engine_types.h"

#include <QString>
#include <QList>

#include <cmath>
#include <optional>

namespace gopost::image_editor {

/// Domain entity for a preset filter.
class FilterPreset {
public:
    int index = 0;
    QString name;
    QString category;
    double intensity = 100.0;

    FilterPreset() = default;

    FilterPreset(int index, const QString& name, const QString& category,
                 double intensity = 100.0)
        : index(index), name(name), category(category), intensity(intensity) {}

    [[nodiscard]] FilterPreset copyWith(std::optional<double> newIntensity = std::nullopt) const {
        return FilterPreset(index, name, category,
                            newIntensity.value_or(intensity));
    }

    [[nodiscard]] static FilterPreset fromInfo(const rendering::PresetFilterInfo& info) {
        return FilterPreset(info.index, info.name, info.category);
    }

    bool operator==(const FilterPreset&) const = default;
};

/// Domain entity for an adjustment value.
class AdjustmentValue {
public:
    QString effectId;
    QString displayName;
    double value = 0.0;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;

    AdjustmentValue() = default;

    AdjustmentValue(const QString& effectId, const QString& displayName,
                    double value, double defaultValue,
                    double minValue, double maxValue)
        : effectId(effectId), displayName(displayName)
        , value(value), defaultValue(defaultValue)
        , minValue(minValue), maxValue(maxValue) {}

    [[nodiscard]] AdjustmentValue copyWith(std::optional<double> newValue = std::nullopt) const {
        return AdjustmentValue(effectId, displayName,
                               newValue.value_or(value),
                               defaultValue, minValue, maxValue);
    }

    [[nodiscard]] bool isDefault() const {
        return std::abs(value - defaultValue) < 0.01;
    }

    bool operator==(const AdjustmentValue&) const = default;
};

/// Grouped preset filters by category.
class FilterCategory {
public:
    QString name;
    QList<FilterPreset> filters;

    FilterCategory() = default;

    FilterCategory(const QString& name, const QList<FilterPreset>& filters)
        : name(name), filters(filters) {}

    bool operator==(const FilterCategory&) const = default;
};

} // namespace gopost::image_editor
