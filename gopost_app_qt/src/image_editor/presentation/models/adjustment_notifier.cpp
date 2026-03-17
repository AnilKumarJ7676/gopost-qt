#include "image_editor/presentation/models/adjustment_notifier.h"
#include "rendering_bridge/engine_api.h"

#include <QDebug>
#include <QVariantMap>

namespace gopost::image_editor {

AdjustmentNotifier::AdjustmentNotifier(rendering::ImageEditorEngine* engine,
                                       QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

QVariantList AdjustmentNotifier::adjustmentsList() const
{
    QVariantList result;
    for (const auto& adj : m_adjustments) {
        QVariantMap map;
        map[QStringLiteral("effectId")] = adj.effectId;
        map[QStringLiteral("displayName")] = adj.displayName;
        map[QStringLiteral("value")] = adj.value;
        map[QStringLiteral("defaultValue")] = adj.defaultValue;
        map[QStringLiteral("minValue")] = adj.minValue;
        map[QStringLiteral("maxValue")] = adj.maxValue;
        map[QStringLiteral("isDefault")] = adj.isDefault();
        result.append(map);
    }
    return result;
}

bool AdjustmentNotifier::hasChanges() const
{
    for (const auto& a : m_adjustments) {
        if (!a.isDefault()) return true;
    }
    return false;
}

void AdjustmentNotifier::loadAdjustments()
{
    if (!m_adjustments.isEmpty() || !m_engine) return;

    m_isLoading = true;
    emit stateChanged();

    try {
        auto defs = m_engine->getEffectsByCategory(
            rendering::EffectCategory::Adjustment);

        m_adjustments.clear();
        for (const auto& d : defs) {
            AdjustmentValue adj;
            adj.effectId = d.id;
            adj.displayName = d.displayName;
            if (!d.params.empty()) {
                adj.value = d.params[0].defaultValue;
                adj.defaultValue = d.params[0].defaultValue;
                adj.minValue = d.params[0].minValue;
                adj.maxValue = d.params[0].maxValue;
            }
            m_adjustments.append(adj);
        }

        m_isLoading = false;
        emit stateChanged();
    } catch (const std::exception& e) {
        m_isLoading = false;
        m_error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void AdjustmentNotifier::setAdjustment(int canvasId, int layerId,
                                         const QString& effectId, double value)
{
    int idx = -1;
    for (int i = 0; i < m_adjustments.size(); ++i) {
        if (m_adjustments[i].effectId == effectId) {
            idx = i;
            break;
        }
    }
    if (idx < 0 || !m_engine) return;

    m_adjustments[idx].value = value;
    emit stateChanged();

    try {
        int instanceId = -1;
        if (m_instanceIds.contains(effectId)) {
            instanceId = m_instanceIds[effectId];
        } else {
            instanceId = m_engine->addEffectToLayer(canvasId, layerId, effectId);
            m_instanceIds[effectId] = instanceId;
        }

        // Extract param id from effectId (e.g., "adjust.brightness" -> "brightness")
        QString paramId = effectId;
        int dotPos = effectId.lastIndexOf(QLatin1Char('.'));
        if (dotPos >= 0) {
            paramId = effectId.mid(dotPos + 1);
        }

        m_engine->setEffectParam(canvasId, layerId, instanceId, paramId, value);
    } catch (const std::exception& e) {
        qWarning() << "AdjustmentNotifier::setAdjustment failed:" << e.what();
    }
}

void AdjustmentNotifier::resetAdjustment(int canvasId, int layerId,
                                           const QString& effectId)
{
    int idx = -1;
    for (int i = 0; i < m_adjustments.size(); ++i) {
        if (m_adjustments[i].effectId == effectId) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return;

    m_adjustments[idx].value = m_adjustments[idx].defaultValue;
    emit stateChanged();

    if (m_instanceIds.contains(effectId) && m_engine) {
        try {
            m_engine->removeEffectFromLayer(canvasId, layerId,
                                             m_instanceIds[effectId]);
            m_instanceIds.remove(effectId);
        } catch (const std::exception& e) {
            qWarning() << "AdjustmentNotifier::resetAdjustment failed:"
                        << e.what();
        }
    }
}

void AdjustmentNotifier::resetAll(int canvasId, int layerId)
{
    for (auto it = m_instanceIds.cbegin(); it != m_instanceIds.cend() && m_engine; ++it) {
        try {
            m_engine->removeEffectFromLayer(canvasId, layerId, it.value());
        } catch (...) {
            // Continue removing remaining instances
        }
    }

    for (auto& adj : m_adjustments) {
        adj.value = adj.defaultValue;
    }
    m_instanceIds.clear();
    emit stateChanged();
}

} // namespace gopost::image_editor
