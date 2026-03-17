#pragma once

#include <QObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariantList>

#include "rendering_bridge/engine_types.h"

namespace gopost::rendering {
class ImageEditorEngine;
}

namespace gopost::image_editor {

// -------------------------------------------------------------------------
// AdjustmentValue — single adjustment slider value
// -------------------------------------------------------------------------

struct AdjustmentValue {
    QString effectId;
    QString displayName;
    double value = 0.0;
    double defaultValue = 0.0;
    double minValue = -100.0;
    double maxValue = 100.0;

    bool isDefault() const {
        return qFuzzyCompare(value, defaultValue);
    }
};

// -------------------------------------------------------------------------
// AdjustmentNotifier — manages adjustment slider values and engine sync
// -------------------------------------------------------------------------

class AdjustmentNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList adjustments READ adjustmentsList NOTIFY stateChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY stateChanged)

public:
    explicit AdjustmentNotifier(rendering::ImageEditorEngine* engine,
                                QObject* parent = nullptr);

    QVariantList adjustmentsList() const;
    bool isLoading() const { return m_isLoading; }
    QString error() const { return m_error; }
    bool hasChanges() const;

    const QList<AdjustmentValue>& adjustments() const { return m_adjustments; }

    Q_INVOKABLE void loadAdjustments();
    Q_INVOKABLE void setAdjustment(int canvasId, int layerId,
                                    const QString& effectId, double value);
    Q_INVOKABLE void resetAdjustment(int canvasId, int layerId,
                                      const QString& effectId);
    Q_INVOKABLE void resetAll(int canvasId, int layerId);

signals:
    void stateChanged();

private:
    rendering::ImageEditorEngine* m_engine = nullptr;
    QList<AdjustmentValue> m_adjustments;
    QMap<QString, int> m_instanceIds;
    bool m_isLoading = false;
    QString m_error;
};

} // namespace gopost::image_editor
