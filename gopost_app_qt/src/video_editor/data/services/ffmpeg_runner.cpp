#include "video_editor/data/services/ffmpeg_runner.h"
#include "video_editor/data/services/ffmpeg_runner_cli.h"

#ifdef Q_OS_IOS
#include "video_editor/data/services/ffmpeg_runner_kit.h"
#endif
#ifdef Q_OS_ANDROID
#include "video_editor/data/services/ffmpeg_runner_kit.h"
#endif
#ifdef Q_OS_MACOS
#include "video_editor/data/services/ffmpeg_runner_kit.h"
#endif

#include <QProcess>
#include <QSysInfo>

namespace gopost::video_editor {

std::shared_ptr<FfmpegRunner> FfmpegRunner::create() {
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    return std::make_shared<FfmpegCliRunner>();
#else
    return std::make_shared<FfmpegKitRunner>();
#endif
}

bool isFfmpegAvailable() {
    QProcess proc;
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(QStringLiteral("ffmpeg"), {QStringLiteral("-version")});
    if (!proc.waitForFinished(5000)) return false;
    return proc.exitCode() == 0;
}

} // namespace gopost::video_editor
