#pragma once

#include "core/interfaces/ILogger.h"

#include <QFile>
#include <QString>
#include <cstdint>
#include <mutex>

namespace gopost::logging {

using core::interfaces::LogLevel;

class FileLogSink {
public:
    explicit FileLogSink(const QString& filePath,
                          int64_t maxFileSizeBytes = 5 * 1024 * 1024,
                          int maxRotatedFiles = 5);
    ~FileLogSink();

    void write(const QString& formatted, LogLevel level);
    void flush();

    QString filePath() const { return filePath_; }
    int64_t currentFileSize() const;

private:
    void rotateIfNeeded();
    void rotate();
    void openFile();

    QString filePath_;
    int64_t maxFileSize_;
    int maxRotated_;

    std::mutex mutex_;
    QFile file_;
    int unflushedCount_ = 0;
    int64_t lastFlushMs_ = 0;
};

} // namespace gopost::logging
