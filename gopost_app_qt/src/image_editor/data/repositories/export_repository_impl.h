#pragma once

#include "image_editor/domain/repositories/export_repository.h"

namespace gopost::image_editor {

// Forward declaration for the engine exporter interface.
class ImageExporter;

class ExportRepositoryImpl : public ExportRepository {
public:
    explicit ExportRepositoryImpl(ImageExporter& exporter);

    rendering::ExportResult exportImage(
        int canvasId,
        const rendering::ExportConfig& config,
        const QString& outputPath,
        std::function<void(double progress)> onProgress = nullptr) override;

    int estimateSize(int canvasId, const rendering::ExportConfig& config) override;

private:
    ImageExporter& m_exporter;
};

} // namespace gopost::image_editor
