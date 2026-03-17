#pragma once

#include "image_editor/domain/entities/text_entity.h"

#include <QList>
#include <QString>

namespace gopost::image_editor {

/// ISP: Text layer management.
class TextLayerRepository {
public:
    virtual ~TextLayerRepository() = default;

    virtual QList<QString> getAvailableFonts() = 0;
    virtual int addTextLayer(int canvasId, const TextLayerConfig& config,
                             int maxWidth, int index = -1) = 0;
    virtual void updateTextLayer(int canvasId, int layerId,
                                 const TextLayerConfig& config, int maxWidth) = 0;
};

} // namespace gopost::image_editor
