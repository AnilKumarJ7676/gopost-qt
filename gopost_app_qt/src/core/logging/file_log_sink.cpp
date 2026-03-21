#include "core/logging/file_log_sink.h"

#include <QDir>
#include <QFileInfo>
#include <chrono>

namespace gopost::logging {

static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

FileLogSink::FileLogSink(const QString& filePath,
                           int64_t maxFileSizeBytes,
                           int maxRotatedFiles)
    : filePath_(filePath)
    , maxFileSize_(maxFileSizeBytes)
    , maxRotated_(maxRotatedFiles)
    , lastFlushMs_(nowMs())
{
    QDir().mkpath(QFileInfo(filePath_).absolutePath());
    openFile();
}

FileLogSink::~FileLogSink() {
    flush();
}

void FileLogSink::openFile() {
    file_.setFileName(filePath_);
    file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void FileLogSink::write(const QString& formatted, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!file_.isOpen()) return;

    rotateIfNeeded();

    file_.write(formatted.toUtf8());
    file_.write("\n", 1);
    ++unflushedCount_;

    // Flush policy: immediately for WARN+, batch for lower levels
    if (level >= LogLevel::Warning) {
        file_.flush();
        unflushedCount_ = 0;
        lastFlushMs_ = nowMs();
    } else if (unflushedCount_ >= 100 || (nowMs() - lastFlushMs_) >= 1000) {
        file_.flush();
        unflushedCount_ = 0;
        lastFlushMs_ = nowMs();
    }
}

void FileLogSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.isOpen()) {
        file_.flush();
        unflushedCount_ = 0;
    }
}

int64_t FileLogSink::currentFileSize() const {
    return QFileInfo(filePath_).size();
}

void FileLogSink::rotateIfNeeded() {
    if (file_.size() >= maxFileSize_)
        rotate();
}

void FileLogSink::rotate() {
    file_.close();

    // Shift rotated files: .4 → .5 (delete), .3 → .4, .2 → .3, .1 → .2
    for (int i = maxRotated_; i >= 1; --i) {
        QString src = QStringLiteral("%1.%2").arg(filePath_).arg(i);
        QString dst = QStringLiteral("%1.%2").arg(filePath_).arg(i + 1);
        if (i == maxRotated_)
            QFile::remove(src);
        else
            QFile::rename(src, dst);
    }

    // Current → .1
    QFile::rename(filePath_, QStringLiteral("%1.1").arg(filePath_));

    // Open fresh file
    openFile();
}

} // namespace gopost::logging
