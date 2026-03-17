#include "video_editor/data/services/ffmpeg_runner_kit.h"

namespace gopost::video_editor {

// On desktop platforms (Windows/Linux), ffmpeg_kit is not available.
// This provides a stub implementation. On mobile/macOS, integrate with
// the actual ffmpeg_kit C API here.

FfmpegResult FfmpegKitRunner::execute(const QString& command) {
    Q_UNUSED(command)
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID) || defined(Q_OS_MACOS)
    // TODO: Integrate with ffmpeg_kit C library.
    // FFmpegKit::execute(command.toStdString().c_str());
    return {.success = false, .output = QStringLiteral("ffmpeg_kit not yet integrated"), .returnCode = -1};
#else
    return {.success = false, .output = QStringLiteral("ffmpeg_kit not available on this platform"), .returnCode = -1};
#endif
}

void FfmpegKitRunner::cancel() {
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID) || defined(Q_OS_MACOS)
    // TODO: FFmpegKit::cancel();
#endif
}

void FfmpegKitRunner::onStatistics(std::function<void(int timeMs)> callback) {
    statsCallback_ = std::move(callback);
}

} // namespace gopost::video_editor
