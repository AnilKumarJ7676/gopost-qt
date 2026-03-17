#pragma once

#include <QString>
#include <QList>
#include <functional>
#include <memory>

#include "video_editor/domain/services/export_service.h"
#include "video_editor/domain/models/export_config.h"
#include "video_editor/domain/models/video_project.h"
#include "video_editor/domain/models/video_effect.h"
#include "video_editor/data/services/ffmpeg_runner.h"

namespace gopost::video_editor {

/// Translates VideoProject edits into FFmpeg filter chains and executes them.
/// Implements ExportService (DIP) so callers depend on the abstraction.
class FFmpegExportService : public ExportService {
public:
    explicit FFmpegExportService(std::shared_ptr<FfmpegRunner> ffmpeg = nullptr);

    void exportProject(
        const VideoProject& project,
        const ExportPreset& preset,
        const QString& outputPath,
        std::function<void(double percent)> onProgress
    ) override;

    void cancel() override;

private:
    std::shared_ptr<FfmpegRunner> ffmpeg_;

    // Single-clip export
    void exportSingleClip(
        const VideoClip& clip, const ExportPreset& preset,
        const QString& outputPath, double totalDuration,
        std::function<void(double)> onProgress);

    // Multi-clip export (per-clip encode -> concat)
    void exportMultiClip(
        const QList<VideoClip>& clips, const ExportPreset& preset,
        const QString& outputPath, double totalDuration,
        std::function<void(double)> onProgress);

    void concatSegments(const QStringList& segmentPaths, const QString& outputPath);

    // FFmpeg command builder
    [[nodiscard]] QString buildClipCommand(
        const VideoClip& clip, const ExportPreset& preset, const QString& outputPath) const;

    // Filter chains
    [[nodiscard]] QString videoFilterChain(const VideoClip& clip, const ExportPreset& preset) const;
    [[nodiscard]] QString eqFilter(const VideoClip& clip) const;
    [[nodiscard]] QString effectFilter(const VideoEffect& effect) const;
    [[nodiscard]] QString audioFilterChain(const VideoClip& clip) const;
    [[nodiscard]] static QString atempoChain(double speed);

    // FFmpeg execution with progress
    void runFFmpeg(const QString& command, double expectedDurationSec,
                   std::function<void(double)> onProgress);

    // Helpers
    [[nodiscard]] static QString videoEncoder(VideoCodec codec);
    [[nodiscard]] static QString audioEncoder(AudioCodec codec);
    [[nodiscard]] static QList<VideoClip> sortedVideoClips(const VideoProject& project);
    [[nodiscard]] static double clipOutputDuration(const VideoClip& clip);
    [[nodiscard]] static double outputDuration(const QList<VideoClip>& clips);
};

} // namespace gopost::video_editor
