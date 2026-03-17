#pragma once

#include <QObject>
#include <QStringList>
#include <QString>

#include "rendering_bridge/engine_types.h"

namespace gopost::rendering {
class ImageEditorEngine;
}

namespace gopost::image_editor {

// -------------------------------------------------------------------------
// TextNotifier — manages text tool state and delegates to engine
// -------------------------------------------------------------------------

class TextNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(QStringList availableFonts READ availableFonts NOTIFY stateChanged)
    Q_PROPERTY(int activeLayerId READ activeLayerId NOTIFY stateChanged)
    Q_PROPERTY(bool isEditing READ isEditing NOTIFY stateChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)

    // TextConfig properties exposed individually for QML binding
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY configChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY configChanged)
    Q_PROPERTY(int fontStyle READ fontStyle WRITE setFontStyle NOTIFY configChanged)
    Q_PROPERTY(double fontSize READ fontSize WRITE setFontSize NOTIFY configChanged)
    Q_PROPERTY(double colorR READ colorR WRITE setColorR NOTIFY configChanged)
    Q_PROPERTY(double colorG READ colorG WRITE setColorG NOTIFY configChanged)
    Q_PROPERTY(double colorB READ colorB WRITE setColorB NOTIFY configChanged)
    Q_PROPERTY(int alignment READ alignment WRITE setAlignment NOTIFY configChanged)
    Q_PROPERTY(bool hasShadow READ hasShadow WRITE setHasShadow NOTIFY configChanged)

public:
    explicit TextNotifier(rendering::ImageEditorEngine* engine,
                          QObject* parent = nullptr);

    QStringList availableFonts() const { return m_availableFonts; }
    int activeLayerId() const { return m_activeLayerId; }
    bool isEditing() const { return m_isEditing; }
    bool isLoading() const { return m_isLoading; }
    QString error() const { return m_error; }

    // Config getters
    QString text() const { return m_config.text; }
    QString fontFamily() const { return m_config.fontFamily; }
    int fontStyle() const { return static_cast<int>(m_config.style); }
    double fontSize() const { return m_config.fontSize; }
    double colorR() const { return m_config.colorR; }
    double colorG() const { return m_config.colorG; }
    double colorB() const { return m_config.colorB; }
    int alignment() const { return static_cast<int>(m_config.alignment); }
    bool hasShadow() const { return m_config.hasShadow; }

    // Config setters
    void setText(const QString& v) { m_config.text = v; emit configChanged(); }
    void setFontFamily(const QString& v) { m_config.fontFamily = v; emit configChanged(); }
    void setFontStyle(int v) { m_config.style = static_cast<rendering::FontStyle>(v); emit configChanged(); }
    void setFontSize(double v) { m_config.fontSize = v; emit configChanged(); }
    void setColorR(double v) { m_config.colorR = v; emit configChanged(); }
    void setColorG(double v) { m_config.colorG = v; emit configChanged(); }
    void setColorB(double v) { m_config.colorB = v; emit configChanged(); }
    void setAlignment(int v) { m_config.alignment = static_cast<rendering::TextAlignment>(v); emit configChanged(); }
    void setHasShadow(bool v) { m_config.hasShadow = v; emit configChanged(); }

    const rendering::TextConfig& config() const { return m_config; }

    Q_INVOKABLE void loadFonts();
    Q_INVOKABLE void updateConfig(const QString& text, double fontSize,
                                   int style, int alignment,
                                   double colorR, double colorG, double colorB,
                                   bool hasShadow);
    Q_INVOKABLE int addTextLayer(int canvasId, int maxWidth);
    Q_INVOKABLE void updateActiveTextLayer(int canvasId, int maxWidth);
    Q_INVOKABLE void startEditing(int layerId = -1);
    Q_INVOKABLE void stopEditing();

signals:
    void stateChanged();
    void configChanged();

private:
    rendering::ImageEditorEngine* m_engine = nullptr;

    QStringList m_availableFonts;
    rendering::TextConfig m_config;
    int m_activeLayerId = -1;
    bool m_isEditing = false;
    bool m_isLoading = false;
    QString m_error;
};

} // namespace gopost::image_editor
