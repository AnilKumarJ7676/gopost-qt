#include "image_editor/presentation/models/filter_notifier.h"
#include "image_editor/presentation/models/canvas_notifier.h"
#include "rendering_bridge/engine_api.h"

#include <QDebug>
#include <QVariantMap>

namespace gopost::image_editor {

FilterNotifier::FilterNotifier(rendering::ImageEditorEngine* engine,
                               CanvasNotifier* canvasNotifier,
                               QObject* parent)
    : QObject(parent)
    , m_engine(engine)
    , m_canvasNotifier(canvasNotifier)
{
}

QVariantList FilterNotifier::categoriesList() const
{
    QVariantList result;
    for (const auto& cat : m_categories) {
        QVariantMap catMap;
        catMap[QStringLiteral("name")] = cat.name;

        QVariantList filterList;
        for (const auto& f : cat.filters) {
            QVariantMap fMap;
            fMap[QStringLiteral("index")] = f.index;
            fMap[QStringLiteral("name")] = f.name;
            fMap[QStringLiteral("category")] = f.category;
            filterList.append(fMap);
        }
        catMap[QStringLiteral("filters")] = filterList;
        result.append(catMap);
    }
    return result;
}

void FilterNotifier::loadPresets()
{
    if (!m_categories.isEmpty() || !m_engine) return;

    m_isLoading = true;
    m_error.clear();
    emit stateChanged();

    try {
        auto presets = m_engine->getPresetFilters();

        // Group by category
        QMap<QString, FilterCategory> catMap;
        for (const auto& p : presets) {
            FilterPreset preset;
            preset.index = p.index;
            preset.name = p.name;
            preset.category = p.category;

            if (!catMap.contains(p.category)) {
                FilterCategory cat;
                cat.name = p.category;
                catMap[p.category] = cat;
            }
            catMap[p.category].filters.append(preset);
        }

        m_categories.clear();
        for (auto it = catMap.cbegin(); it != catMap.cend(); ++it) {
            m_categories.append(it.value());
        }

        m_isLoading = false;
        emit stateChanged();
    } catch (const std::exception& e) {
        m_isLoading = false;
        m_error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void FilterNotifier::selectPreset(int canvasId, int layerId, int presetIndex)
{
    if (!m_engine) return;

    if (m_activePresetIndex == presetIndex) {
        clearActive();
        return;
    }

    m_activePresetIndex = presetIndex;
    m_intensity = 100.0;
    emit stateChanged();

    try {
        auto result = m_engine->applyPreset(canvasId, layerId,
                                             presetIndex, m_intensity);
        if (result.has_value() && m_canvasNotifier) {
            m_canvasNotifier->replaceWithImage(result.value());
        }
    } catch (const std::exception& e) {
        clearActive();
        m_error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void FilterNotifier::setIntensity(int canvasId, int layerId, double intensity)
{
    if (m_activePresetIndex < 0 || !m_engine) return;

    double previousIntensity = m_intensity;
    m_intensity = intensity;
    emit stateChanged();

    try {
        auto result = m_engine->applyPreset(canvasId, layerId,
                                             m_activePresetIndex, intensity);
        if (result.has_value() && m_canvasNotifier) {
            m_canvasNotifier->replaceWithImage(result.value());
        }
    } catch (const std::exception& e) {
        qWarning() << "FilterNotifier::setIntensity failed:" << e.what();
        m_intensity = previousIntensity;
        m_error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void FilterNotifier::clearFilter()
{
    clearActive();
}

void FilterNotifier::clearActive()
{
    m_activePresetIndex = -1;
    m_intensity = 100.0;
    m_error.clear();
    emit stateChanged();
}

} // namespace gopost::image_editor
