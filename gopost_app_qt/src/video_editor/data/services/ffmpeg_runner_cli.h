#pragma once

#include "video_editor/data/services/ffmpeg_runner.h"

#include <QProcess>
#include <QRegularExpression>
#include <functional>
#include <memory>

namespace gopost::video_editor {

/// FFmpeg runner that spawns a system `ffmpeg` process.
/// Works on Windows and Linux where ffmpeg_kit is not supported.
/// Requires FFmpeg to be installed and available on the system PATH.
class FfmpegCliRunner : public FfmpegRunner {
public:
    FfmpegCliRunner() = default;

    FfmpegResult execute(const QString& command) override;
    void cancel() override;
    void onStatistics(std::function<void(int timeMs)> callback) override;

private:
    QProcess* activeProcess_{nullptr};
    std::function<void(int timeMs)> statsCallback_;

    /// Parse FFmpeg stderr progress lines like `time=00:01:23.45`.
    void parseProgress(const QString& data);

    /// Split a command string into arguments, respecting quoted strings.
    static QStringList parseArgs(const QString& command);
};

} // namespace gopost::video_editor
