#pragma once

#include "video_editor/data/services/ffmpeg_runner.h"

#include <functional>

namespace gopost::video_editor {

/// FFmpeg runner backed by the ffmpeg_kit library.
/// Works on iOS, Android, and macOS.
/// On Windows/Linux builds, this class is a stub that always fails.
class FfmpegKitRunner : public FfmpegRunner {
public:
    FfmpegKitRunner() = default;

    FfmpegResult execute(const QString& command) override;
    void cancel() override;
    void onStatistics(std::function<void(int timeMs)> callback) override;

private:
    std::function<void(int timeMs)> statsCallback_;
};

} // namespace gopost::video_editor
