#include "video_editor/data/services/ffmpeg_runner_cli.h"

#include <QRegularExpression>

namespace gopost::video_editor {

FfmpegResult FfmpegCliRunner::execute(const QString& command) {
    const auto args = parseArgs(command);

    auto process = std::make_unique<QProcess>();
    activeProcess_ = process.get();
    // ForwardedChannels sends both stdout and stderr directly to the parent,
    // completely bypassing Qt's internal QRingBuffer. No pipe buffering at all.
    process->setProcessChannelMode(QProcess::ForwardedChannels);

    process->start(QStringLiteral("ffmpeg"), args);

    if (!process->waitForStarted(10000)) {
        activeProcess_ = nullptr;
        return {
            .success = false,
            .output = QStringLiteral("FFmpeg not found. Install FFmpeg and add it to your system PATH.\n"
                                      "Download: https://ffmpeg.org/download.html\n"
                                      "Windows: winget install FFmpeg"),
            .returnCode = -1,
        };
    }

    // With ForwardedChannels, no pipe data to read — just wait for completion.
    // Progress reporting is not available in this mode (no stderr capture).
    // For long encodes, the caller should use -progress pipe:2 or a temp file.
    const bool finished = process->waitForFinished(6 * 3600 * 1000);
    activeProcess_ = nullptr;

    if (!finished) {
        process->kill();
        return {.success = false, .output = QStringLiteral("FFmpeg timed out"), .returnCode = -1};
    }

    const int exitCode = process->exitCode();
    return {
        .success = exitCode == 0,
        .output = {},
        .returnCode = exitCode,
    };
}

void FfmpegCliRunner::cancel() {
    if (activeProcess_) {
        activeProcess_->terminate();
        activeProcess_ = nullptr;
    }
}

void FfmpegCliRunner::onStatistics(std::function<void(int timeMs)> callback) {
    statsCallback_ = std::move(callback);
}

void FfmpegCliRunner::parseProgress(const QString& data) {
    if (!statsCallback_) return;

    static const QRegularExpression re(QStringLiteral(R"(time=(\d+):(\d+):(\d+)\.(\d+))"));
    const auto match = re.match(data);
    if (match.hasMatch()) {
        const int hours = match.captured(1).toInt();
        const int minutes = match.captured(2).toInt();
        const int seconds = match.captured(3).toInt();
        const auto centisStr = match.captured(4).leftJustified(2, QLatin1Char('0')).left(2);
        const int centis = centisStr.toInt();
        const int totalMs = ((hours * 3600 + minutes * 60 + seconds) * 1000) + (centis * 10);
        statsCallback_(totalMs);
    }
}

QStringList FfmpegCliRunner::parseArgs(const QString& command) {
    QStringList args;
    QString current;
    bool inDoubleQuote = false;
    bool inSingleQuote = false;

    for (int i = 0; i < command.size(); ++i) {
        const QChar c = command[i];
        if (c == QLatin1Char('"') && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
        } else if (c == QLatin1Char('\'') && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
        } else if (c == QLatin1Char(' ') && !inDoubleQuote && !inSingleQuote) {
            if (!current.isEmpty()) {
                args.append(current);
                current.clear();
            }
        } else {
            current.append(c);
        }
    }
    if (!current.isEmpty()) args.append(current);
    return args;
}

} // namespace gopost::video_editor
