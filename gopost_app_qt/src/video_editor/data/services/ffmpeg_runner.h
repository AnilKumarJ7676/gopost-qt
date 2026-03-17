#pragma once

#include <QString>
#include <functional>
#include <memory>

namespace gopost::video_editor {

/// Result of an FFmpeg command execution.
struct FfmpegResult {
    bool success{false};
    QString output;
    int returnCode{-1};
};

/// Platform-agnostic interface for running FFmpeg commands.
///
/// - Windows / Linux       -> FfmpegCliRunner (spawns ffmpeg process from PATH)
/// - iOS / Android / macOS -> FfmpegKitRunner (uses ffmpeg_kit library)
class FfmpegRunner {
public:
    virtual ~FfmpegRunner() = default;

    /// Execute an FFmpeg command string (arguments only, no leading `ffmpeg`).
    virtual FfmpegResult execute(const QString& command) = 0;

    /// Cancel any currently running FFmpeg session.
    virtual void cancel() = 0;

    /// Register a callback that receives progress as elapsed milliseconds.
    virtual void onStatistics(std::function<void(int timeMs)> callback) = 0;

    /// Returns the singleton instance appropriate for the current platform.
    static std::shared_ptr<FfmpegRunner> create();
};

/// Check whether FFmpeg is available on the system PATH (Windows/Linux).
bool isFfmpegAvailable();

} // namespace gopost::video_editor
