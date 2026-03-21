#include "core/logging/log_formatter.h"

#include <QDateTime>
#include <QFileInfo>
#include <chrono>

namespace gopost::logging {

const char* LogFormatter::levelStr(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:   return "TRACE";
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO ";
    case LogLevel::Warning: return "WARN ";
    case LogLevel::Error:   return "ERROR";
    case LogLevel::Fatal:   return "FATAL";
    }
    return "?????";
}

const char* LogFormatter::categoryStr(LogCategory cat) {
    switch (cat) {
    case LogCategory::General:   return "GENERAL  ";
    case LogCategory::Platform:  return "PLATFORM ";
    case LogCategory::Memory:    return "MEMORY   ";
    case LogCategory::Rendering: return "RENDERING";
    case LogCategory::Timeline:  return "TIMELINE ";
    case LogCategory::Audio:     return "AUDIO    ";
    case LogCategory::Network:   return "NETWORK  ";
    case LogCategory::Config:    return "CONFIG   ";
    }
    return "UNKNOWN  ";
}

QString LogFormatter::format(LogLevel level, LogCategory category,
                              const QString& message,
                              const char* file, int line,
                              uint64_t timestampMs) {
    // Timestamp
    QDateTime ts;
    if (timestampMs > 0) {
        ts = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestampMs));
    } else {
        ts = QDateTime::currentDateTime();
    }
    QString tsStr = ts.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));

    // File:line suffix
    QString fileSuffix;
    if (file && file[0] != '\0') {
        QString fileName = QFileInfo(QString::fromUtf8(file)).fileName();
        fileSuffix = QStringLiteral(" %1:%2").arg(fileName).arg(line);
    }

    return QStringLiteral("[%1] [%2] [%3]%4 — %5")
        .arg(tsStr,
             QString::fromLatin1(levelStr(level)),
             QString::fromLatin1(categoryStr(category)),
             fileSuffix,
             message);
}

} // namespace gopost::logging
