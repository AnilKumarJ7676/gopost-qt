#include "image_editor/data/repositories/export_repository_impl.h"

#include "core/error/exceptions.h"

namespace gopost::image_editor {

/// Minimal abstract interface that the engine exporter must satisfy.
/// The concrete type lives in the rendering_bridge module.
class ImageExporter {
public:
    virtual ~ImageExporter() = default;
    virtual rendering::ExportResult exportToFile(
        int canvasId, const rendering::ExportConfig& config,
        const QString& outputPath,
        std::function<void(double)> onProgress = nullptr) = 0;
    virtual int estimateFileSize(int canvasId, const rendering::ExportConfig& config) = 0;
};

ExportRepositoryImpl::ExportRepositoryImpl(ImageExporter& exporter)
    : m_exporter(exporter) {}

rendering::ExportResult ExportRepositoryImpl::exportImage(
    int canvasId,
    const rendering::ExportConfig& config,
    const QString& outputPath,
    std::function<void(double progress)> onProgress)
{
    try {
        return m_exporter.exportToFile(canvasId, config, outputPath,
                                       std::move(onProgress));
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Export failed: %1").arg(e.what()));
    }
}

int ExportRepositoryImpl::estimateSize(int canvasId,
                                       const rendering::ExportConfig& config) {
    try {
        return m_exporter.estimateFileSize(canvasId, config);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Estimate failed: %1").arg(e.what()));
    }
}

} // namespace gopost::image_editor
