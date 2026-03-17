#include "image_editor/domain/usecases/import_image_usecase.h"

namespace gopost::image_editor {

// =========================================================================
// DecodeImageFileUseCase
// =========================================================================

DecodeImageFileUseCase::DecodeImageFileUseCase(ImageImportRepository& repository)
    : m_repository(repository) {}

rendering::DecodedImage DecodeImageFileUseCase::operator()(const QString& path) {
    return m_repository.decodeImageFile(path);
}

// =========================================================================
// DecodeImageBytesUseCase
// =========================================================================

DecodeImageBytesUseCase::DecodeImageBytesUseCase(ImageImportRepository& repository)
    : m_repository(repository) {}

rendering::DecodedImage DecodeImageBytesUseCase::operator()(const QByteArray& data) {
    return m_repository.decodeImageBytes(data);
}

// =========================================================================
// ExportImageUseCase
// =========================================================================

ExportImageUseCase::ExportImageUseCase(RenderRepository& renderRepo,
                                       ImageImportRepository& importRepo)
    : m_renderRepo(renderRepo)
    , m_importRepo(importRepo) {}

void ExportImageUseCase::operator()(int canvasId, const QString& outputPath,
                                    rendering::ImageFormat format, int quality) {
    auto rendered = m_renderRepo.renderCanvas(canvasId);
    m_importRepo.encodeToFile(rendered, outputPath, format, quality);
}

} // namespace gopost::image_editor
