#include "image_editor/data/repositories/text_repository_impl.h"
#include "image_editor/data/datasources/image_editor_engine.h"

#include "core/error/exceptions.h"

namespace gopost::image_editor {

TextRepositoryImpl::TextRepositoryImpl(ImageEditorEngine& engine)
    : m_engine(engine) {}

QList<QString> TextRepositoryImpl::getAvailableFonts() {
    try {
        return m_engine.getAvailableFonts();
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to get fonts: %1").arg(e.what()));
    }
}

int TextRepositoryImpl::addTextLayer(int canvasId, const TextLayerConfig& config,
                                     int maxWidth, int index) {
    try {
        return m_engine.addTextLayer(canvasId, config.toEngineConfig(),
                                     maxWidth, index);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to add text layer: %1").arg(e.what()));
    }
}

void TextRepositoryImpl::updateTextLayer(int canvasId, int layerId,
                                         const TextLayerConfig& config,
                                         int maxWidth) {
    try {
        m_engine.updateTextLayer(canvasId, layerId,
                                 config.toEngineConfig(), maxWidth);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Failed to update text layer: %1").arg(e.what()));
    }
}

} // namespace gopost::image_editor
