#pragma once

#include "image_editor/domain/repositories/canvas_repository.h"
#include "rendering_bridge/engine_types.h"

#include <QByteArray>
#include <QString>

namespace gopost::image_editor {

/// SRP: Decodes an image from a file path.
class DecodeImageFileUseCase {
public:
    explicit DecodeImageFileUseCase(ImageImportRepository& repository);

    rendering::DecodedImage operator()(const QString& path);

private:
    ImageImportRepository& m_repository;
};

/// SRP: Decodes an image from in-memory bytes.
class DecodeImageBytesUseCase {
public:
    explicit DecodeImageBytesUseCase(ImageImportRepository& repository);

    rendering::DecodedImage operator()(const QByteArray& data);

private:
    ImageImportRepository& m_repository;
};

/// SRP: Exports a rendered canvas to a file.
class ExportImageUseCase {
public:
    ExportImageUseCase(RenderRepository& renderRepo,
                       ImageImportRepository& importRepo);

    void operator()(int canvasId, const QString& outputPath,
                    rendering::ImageFormat format, int quality = 85);

private:
    RenderRepository& m_renderRepo;
    ImageImportRepository& m_importRepo;
};

} // namespace gopost::image_editor
