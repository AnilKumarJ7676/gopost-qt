#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantList>

#include "rendering_bridge/engine_types.h"

namespace gopost::rendering {
class ImageEditorEngine;
}

namespace gopost::image_editor {

class CanvasNotifier;

// -------------------------------------------------------------------------
// FilterCategory — group of preset filters
// -------------------------------------------------------------------------

struct FilterPreset {
    int index = 0;
    QString name;
    QString category;
};

struct FilterCategory {
    QString name;
    QList<FilterPreset> filters;
};

// -------------------------------------------------------------------------
// FilterNotifier — manages preset filter browsing and application
// -------------------------------------------------------------------------

class FilterNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList categories READ categoriesList NOTIFY stateChanged)
    Q_PROPERTY(int activePresetIndex READ activePresetIndex NOTIFY stateChanged)
    Q_PROPERTY(double intensity READ intensity NOTIFY stateChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)

public:
    explicit FilterNotifier(rendering::ImageEditorEngine* engine,
                            CanvasNotifier* canvasNotifier,
                            QObject* parent = nullptr);

    QVariantList categoriesList() const;
    int activePresetIndex() const { return m_activePresetIndex; }
    double intensity() const { return m_intensity; }
    bool isLoading() const { return m_isLoading; }
    QString error() const { return m_error; }

    Q_INVOKABLE void loadPresets();
    Q_INVOKABLE void selectPreset(int canvasId, int layerId, int presetIndex);
    Q_INVOKABLE void setIntensity(int canvasId, int layerId, double intensity);
    Q_INVOKABLE void clearFilter();

signals:
    void stateChanged();

private:
    void clearActive();

    rendering::ImageEditorEngine* m_engine = nullptr;
    CanvasNotifier* m_canvasNotifier = nullptr;

    QList<FilterCategory> m_categories;
    int m_activePresetIndex = -1;
    double m_intensity = 100.0;
    bool m_isLoading = false;
    QString m_error;
};

} // namespace gopost::image_editor
