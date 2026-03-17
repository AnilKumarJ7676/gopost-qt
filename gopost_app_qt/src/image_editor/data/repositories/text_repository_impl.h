#pragma once

#include "image_editor/domain/repositories/text_repository.h"

namespace gopost::image_editor {

// Forward declaration for the engine interface.
class ImageEditorEngine;

class TextRepositoryImpl : public TextLayerRepository {
public:
    explicit TextRepositoryImpl(ImageEditorEngine& engine);

    QList<QString> getAvailableFonts() override;
    int addTextLayer(int canvasId, const TextLayerConfig& config,
                     int maxWidth, int index = -1) override;
    void updateTextLayer(int canvasId, int layerId,
                         const TextLayerConfig& config, int maxWidth) override;

private:
    ImageEditorEngine& m_engine;
};

} // namespace gopost::image_editor
