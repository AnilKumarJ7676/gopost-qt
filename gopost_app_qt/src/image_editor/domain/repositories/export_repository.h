#pragma once

#include "rendering_bridge/engine_types.h"

#include <QString>

#include <functional>

namespace gopost::image_editor {

class ExportRepository {
public:
    virtual ~ExportRepository() = default;

    virtual rendering::ExportResult exportImage(
        int canvasId,
        const rendering::ExportConfig& config,
        const QString& outputPath,
        std::function<void(double progress)> onProgress = nullptr) = 0;

    virtual int estimateSize(int canvasId, const rendering::ExportConfig& config) = 0;
};

} // namespace gopost::image_editor
