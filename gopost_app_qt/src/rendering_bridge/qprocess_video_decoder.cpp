#include "rendering_bridge/qprocess_video_decoder.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>

namespace gopost::rendering {

// ============================================================================
// Helper — locate ffmpeg / ffprobe on PATH or known locations
// ============================================================================

static QString findExecutable(const QString& name) {
    // 1. Check common known location on Windows
#ifdef Q_OS_WIN
    static const QStringList knownPaths = {
        QStringLiteral("C:/ffmpeg/bin"),
        QStringLiteral("C:/Program Files/ffmpeg/bin"),
    };
    for (const auto& dir : knownPaths) {
        QString candidate = dir + "/" + name + ".exe";
        if (QFileInfo::exists(candidate)) return candidate;
    }
#endif

    // 2. Try QStandardPaths (searches PATH)
    QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) return found;

    // 3. Fallback — just return name, let QProcess resolve it
    return name;
}

// ============================================================================
// Destruction
// ============================================================================

QProcessVideoDecoder::~QProcessVideoDecoder() {
    close();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool QProcessVideoDecoder::open(const QString& path) {
    if (open_) close();

    if (!QFileInfo::exists(path)) {
        qWarning() << "[QProcessDecoder] file not found:" << path;
        return false;
    }

    path_ = path;

    if (!probe(path)) {
        qWarning() << "[QProcessDecoder] probe failed for:" << path;
        return false;
    }
    qDebug() << "[QProcessDecoder] probed:" << width_ << "x" << height_
             << "fps:" << fps_ << "dur:" << duration_;

    // Compute decode dimensions — cap at 960px wide for preview performance
    constexpr int kMaxPreviewWidth = 960;
    if (width_ > kMaxPreviewWidth) {
        double scale = static_cast<double>(kMaxPreviewWidth) / width_;
        decodeWidth_  = kMaxPreviewWidth;
        decodeHeight_ = static_cast<int>(height_ * scale);
        // Ensure even dimensions (required by many ffmpeg pixel formats)
        decodeWidth_  &= ~1;
        decodeHeight_ &= ~1;
    } else {
        decodeWidth_  = width_  & ~1;
        decodeHeight_ = height_ & ~1;
    }

    if (decodeWidth_ <= 0 || decodeHeight_ <= 0) return false;

    // NOTE: Don't start the ffmpeg process here — open() runs on the main thread,
    // but the QProcess will be used from the decode thread. We defer process creation
    // to the first decodeNextFrame() or seekTo() call, which run on the decode thread.
    open_ = true;
    eof_  = false;
    currentTime_ = 0.0;
    frameIndex_  = 0;
    return true;
}

void QProcessVideoDecoder::close() {
    killProcess();
    open_ = false;
    eof_  = false;
    width_ = height_ = 0;
    decodeWidth_ = decodeHeight_ = 0;
    fps_ = 30.0;
    duration_ = 0.0;
    currentTime_ = 0.0;
    frameIndex_ = 0;
    path_.clear();
}

// ============================================================================
// Probe — extract metadata via ffprobe
// ============================================================================

bool QProcessVideoDecoder::probe(const QString& path) {
    QString ffprobe = findExecutable(QStringLiteral("ffprobe"));

    QProcess proc;
    proc.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    proc.start(ffprobe, {
        QStringLiteral("-v"), QStringLiteral("quiet"),
        QStringLiteral("-print_format"), QStringLiteral("json"),
        QStringLiteral("-show_format"),
        QStringLiteral("-show_streams"),
        QStringLiteral("-select_streams"), QStringLiteral("v:0"),
        path
    });

    // Read incrementally to avoid QRingBuffer overflow
    QByteArray output;
    while (proc.state() != QProcess::NotRunning || proc.bytesAvailable() > 0) {
        if (proc.bytesAvailable() > 0) {
            output.append(proc.read(proc.bytesAvailable()));
        } else if (!proc.waitForReadyRead(10000)) {
            break;
        }
    }

    if (proc.exitCode() != 0) return false;
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError) return false;

    QJsonObject root = doc.object();

    // Parse video stream
    QJsonArray streams = root["streams"].toArray();
    if (streams.isEmpty()) return false;

    QJsonObject vstream = streams[0].toObject();
    width_  = vstream["width"].toInt(0);
    height_ = vstream["height"].toInt(0);

    if (width_ <= 0 || height_ <= 0) return false;

    // Frame rate — try r_frame_rate first (e.g. "30000/1001"), then avg_frame_rate
    QString rateStr = vstream["r_frame_rate"].toString();
    if (rateStr.isEmpty()) rateStr = vstream["avg_frame_rate"].toString();
    if (!rateStr.isEmpty()) {
        int slash = rateStr.indexOf('/');
        if (slash > 0) {
            double num = rateStr.left(slash).toDouble();
            double den = rateStr.mid(slash + 1).toDouble();
            if (den > 0) fps_ = num / den;
        } else {
            double val = rateStr.toDouble();
            if (val > 0) fps_ = val;
        }
    }
    if (fps_ <= 0) fps_ = 30.0;

    // Duration — prefer stream duration, fall back to format duration
    double dur = vstream["duration"].toString().toDouble();
    if (dur <= 0) {
        QJsonObject fmt = root["format"].toObject();
        dur = fmt["duration"].toString().toDouble();
    }
    if (dur <= 0) dur = 0.0;
    duration_ = dur;

    return true;
}

// ============================================================================
// Start / restart ffmpeg decode process
// ============================================================================

bool QProcessVideoDecoder::startDecodeProcess(double startTime) {
    killProcess();

    QString ffmpeg = findExecutable(QStringLiteral("ffmpeg"));

    QStringList args;

    // Seek before input for fast seeking
    if (startTime > 0.01) {
        args << QStringLiteral("-ss")
             << QString::number(startTime, 'f', 4);
    }

    args << QStringLiteral("-i") << path_
         << QStringLiteral("-f") << QStringLiteral("rawvideo")
         << QStringLiteral("-pix_fmt") << QStringLiteral("rgba")
         << QStringLiteral("-s")
         << QString("%1x%2").arg(decodeWidth_).arg(decodeHeight_)
         << QStringLiteral("-v") << QStringLiteral("quiet")
         << QStringLiteral("-nostdin")
         << QStringLiteral("pipe:1");

    ffmpeg_ = std::make_unique<QProcess>();
    // ForwardedErrorChannel sends stderr directly to parent's stderr,
    // completely bypassing Qt's internal QRingBuffer that can overflow.
    ffmpeg_->setProcessChannelMode(QProcess::ForwardedErrorChannel);

    qDebug() << "[QProcessDecoder] starting ffmpeg:" << ffmpeg << args.join(' ');
    ffmpeg_->start(ffmpeg, args);

    if (!ffmpeg_->waitForStarted(5000)) {
        qWarning() << "[QProcessDecoder] ffmpeg failed to start! error:" << ffmpeg_->errorString();
        ffmpeg_.reset();
        return false;
    }

    qDebug() << "[QProcessDecoder] ffmpeg started, pid:" << ffmpeg_->processId()
             << "decodeSize:" << decodeWidth_ << "x" << decodeHeight_;
    eof_ = false;
    return true;
}

// ============================================================================
// Seek
// ============================================================================

bool QProcessVideoDecoder::seekTo(double timestampSeconds) {
    if (!open_) return false;

    timestampSeconds = std::clamp(timestampSeconds, 0.0, duration_);

    // If seeking forward by a short distance and the ffmpeg process is
    // still running, decode and discard frames instead of killing/restarting
    // the process.  On Windows, process creation costs 10-50ms.
    double delta = timestampSeconds - currentTime_;
    if (delta > 0 && delta < 2.0 && ffmpeg_ &&
        ffmpeg_->state() == QProcess::Running) {
        int framesToSkip = static_cast<int>(delta * fps_);
        for (int i = 0; i < framesToSkip; ++i) {
            auto f = decodeNextFrame();
            if (!f.has_value()) break;
        }
        return true;
    }

    if (!startDecodeProcess(timestampSeconds)) return false;

    currentTime_ = timestampSeconds;
    frameIndex_  = static_cast<int64_t>(timestampSeconds * fps_);
    eof_ = false;
    return true;
}

// ============================================================================
// Decode next frame
// ============================================================================

std::optional<RenderDecodedFrame> QProcessVideoDecoder::decodeNextFrame() {
    if (!open_ || eof_) return std::nullopt;

    // Lazy-start: create the ffmpeg process on the calling thread (decode thread)
    if (!ffmpeg_) {
        qDebug() << "[QProcessDecoder] lazy-starting ffmpeg from decode thread";
        if (!startDecodeProcess(currentTime_)) {
            qWarning() << "[QProcessDecoder] lazy-start failed!";
            eof_ = true;
            return std::nullopt;
        }
    }

    const size_t frameBytes = static_cast<size_t>(decodeWidth_) * decodeHeight_ * 4;
    if (frameBytes == 0) return std::nullopt;

    // Read exactly one frame worth of RGBA data from stdout pipe.
    // stderr is forwarded (ForwardedErrorChannel) so no QRingBuffer risk.
    QByteArray data;
    data.reserve(static_cast<int>(frameBytes));

    while (static_cast<size_t>(data.size()) < frameBytes) {
        if (ffmpeg_->state() == QProcess::NotRunning && ffmpeg_->bytesAvailable() == 0) {
            // Process exited and no more data — EOF
            eof_ = true;
            return std::nullopt;
        }

        qint64 available = ffmpeg_->bytesAvailable();
        if (available > 0) {
            qint64 need = static_cast<qint64>(frameBytes) - data.size();
            QByteArray chunk = ffmpeg_->read(std::min(available, need));
            if (chunk.isEmpty()) {
                eof_ = true;
                return std::nullopt;
            }
            data.append(chunk);
        } else {
            // Wait for more data (up to 2 seconds)
            if (!ffmpeg_->waitForReadyRead(2000)) {
                // Timeout or process died
                if (ffmpeg_->state() == QProcess::NotRunning) {
                    eof_ = true;
                }
                if (static_cast<size_t>(data.size()) < frameBytes) {
                    return std::nullopt;
                }
            }
        }
    }

    // Build the frame
    RenderDecodedFrame frame;
    frame.width  = static_cast<uint32_t>(decodeWidth_);
    frame.height = static_cast<uint32_t>(decodeHeight_);
    frame.timestamp_seconds = currentTime_;
    frame.pts = frameIndex_;
    frame.pixels.assign(
        reinterpret_cast<const uint8_t*>(data.constData()),
        reinterpret_cast<const uint8_t*>(data.constData()) + frameBytes);

    // Advance time tracking
    currentTime_ += 1.0 / fps_;
    frameIndex_++;

    return frame;
}

// ============================================================================
// Kill process
// ============================================================================

void QProcessVideoDecoder::killProcess() {
    if (!ffmpeg_) return;

    if (ffmpeg_->state() != QProcess::NotRunning) {
        ffmpeg_->closeWriteChannel();
        ffmpeg_->kill();
        ffmpeg_->waitForFinished(1000);
    }
    ffmpeg_.reset();
}

} // namespace gopost::rendering
