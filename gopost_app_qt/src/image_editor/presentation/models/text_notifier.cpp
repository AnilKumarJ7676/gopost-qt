#include "image_editor/presentation/models/text_notifier.h"
#include "rendering_bridge/engine_api.h"

#include <QDebug>

namespace gopost::image_editor {

TextNotifier::TextNotifier(rendering::ImageEditorEngine* engine,
                           QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

void TextNotifier::loadFonts()
{
    if (!m_availableFonts.isEmpty() || !m_engine) return;

    try {
        auto fonts = m_engine->getAvailableFonts();
        m_availableFonts.clear();
        for (const auto& f : fonts) {
            m_availableFonts.append(f);
        }
        emit stateChanged();
    } catch (const std::exception& e) {
        qWarning() << "TextNotifier::loadFonts failed:" << e.what();
    }
}

void TextNotifier::updateConfig(const QString& text, double fontSize,
                                 int style, int alignment,
                                 double colorR, double colorG, double colorB,
                                 bool hasShadow)
{
    m_config.text = text;
    m_config.fontSize = fontSize;
    m_config.style = static_cast<rendering::FontStyle>(style);
    m_config.alignment = static_cast<rendering::TextAlignment>(alignment);
    m_config.colorR = colorR;
    m_config.colorG = colorG;
    m_config.colorB = colorB;
    m_config.hasShadow = hasShadow;
    emit configChanged();
}

int TextNotifier::addTextLayer(int canvasId, int maxWidth)
{
    if (m_config.text.trimmed().isEmpty() || !m_engine) return -1;

    m_isLoading = true;
    emit stateChanged();

    try {
        int layerId = m_engine->addTextLayer(canvasId, m_config, maxWidth);
        m_activeLayerId = layerId;
        m_isEditing = false;
        m_isLoading = false;
        emit stateChanged();
        return layerId;
    } catch (const std::exception& e) {
        m_isLoading = false;
        m_error = QString::fromStdString(e.what());
        emit stateChanged();
        return -1;
    }
}

void TextNotifier::updateActiveTextLayer(int canvasId, int maxWidth)
{
    if (m_activeLayerId < 0 || !m_engine) return;

    try {
        m_engine->updateTextLayer(canvasId, m_activeLayerId, m_config, maxWidth);
    } catch (const std::exception& e) {
        m_error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void TextNotifier::startEditing(int layerId)
{
    m_isEditing = true;
    if (layerId >= 0) {
        m_activeLayerId = layerId;
    }
    if (layerId < 0) {
        // Reset config for new text
        m_config = rendering::TextConfig{};
    }
    emit stateChanged();
}

void TextNotifier::stopEditing()
{
    m_isEditing = false;
    emit stateChanged();
}

} // namespace gopost::image_editor
