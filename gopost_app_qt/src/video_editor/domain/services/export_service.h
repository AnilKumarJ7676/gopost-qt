#pragma once

#include <QString>
#include <functional>

#include "video_editor/domain/models/export_config.h"
#include "video_editor/domain/models/video_project.h"

namespace gopost::video_editor {

/// DIP: High-level modules depend on this abstraction, not on FFmpegExportService.
class ExportService {
public:
    virtual ~ExportService() = default;

    virtual void exportProject(
        const VideoProject& project,
        const ExportPreset& preset,
        const QString& outputPath,
        std::function<void(double percent)> onProgress
    ) = 0;

    virtual void cancel() = 0;
};

} // namespace gopost::video_editor
