#include "image_editor/data/repositories/filter_repository_impl.h"
#include "image_editor/data/datasources/image_editor_engine.h"

#include "core/error/exceptions.h"

#include <QMap>

namespace gopost::image_editor {

FilterRepositoryImpl::FilterRepositoryImpl(ImageEditorEngine& engine)
    : m_engine(engine) {}

QList<rendering::EffectDef> FilterRepositoryImpl::getAdjustmentEffects() {
    try {
        return m_engine.getEffectsByCategory(rendering::EffectCategory::Adjustment);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to get adjustments: %1").arg(e.what()));
    }
}

QList<FilterCategory> FilterRepositoryImpl::getPresetCategories() {
    try {
        const auto presets = m_engine.getPresetFilters();
        QMap<QString, QList<FilterPreset>> grouped;
        for (const auto& p : presets) {
            grouped[p.category].append(FilterPreset::fromInfo(p));
        }
        QList<FilterCategory> result;
        for (auto it = grouped.constBegin(); it != grouped.constEnd(); ++it) {
            result.append(FilterCategory(it.key(), it.value()));
        }
        return result;
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to get presets: %1").arg(e.what()));
    }
}

int FilterRepositoryImpl::addAdjustment(int canvasId, int layerId,
                                        const QString& effectId) {
    try {
        return m_engine.addEffectToLayer(canvasId, layerId, effectId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to add adjustment: %1").arg(e.what()));
    }
}

void FilterRepositoryImpl::setAdjustmentParam(int canvasId, int layerId,
                                              int instanceId,
                                              const QString& paramId,
                                              double value) {
    try {
        m_engine.setEffectParam(canvasId, layerId, instanceId, paramId, value);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to set adjustment: %1").arg(e.what()));
    }
}

void FilterRepositoryImpl::removeAdjustment(int canvasId, int layerId,
                                            int instanceId) {
    try {
        m_engine.removeEffectFromLayer(canvasId, layerId, instanceId);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to remove adjustment: %1").arg(e.what()));
    }
}

std::optional<rendering::DecodedImage> FilterRepositoryImpl::applyPreset(
    int canvasId, int layerId, int presetIndex, double intensity)
{
    try {
        return m_engine.applyPreset(canvasId, layerId, presetIndex, intensity);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to apply preset: %1").arg(e.what()));
    }
}

} // namespace gopost::image_editor
