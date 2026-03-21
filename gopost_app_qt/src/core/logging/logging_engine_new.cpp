#include "core/logging/logging_engine_new.h"
#include "core/logging/log_formatter.h"

#include <QStandardPaths>
#include <QDir>
#include <chrono>

namespace gopost::logging {

ILogger* Logger::s_instance = nullptr;

ILogger* Logger::instance() { return s_instance; }
void Logger::setInstance(ILogger* logger) { s_instance = logger; }

static uint64_t currentTimestampMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

LoggingEngineNew::LoggingEngineNew(const QString& logDir) {
    consoleSink_ = std::make_unique<ConsoleLogSink>();

    QString dir = logDir;
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
              + QStringLiteral("/logs");
    }
    QDir().mkpath(dir);
    QString logPath = dir + QStringLiteral("/gopost.log");
    fileSink_ = std::make_unique<FileLogSink>(logPath);

    queue_ = std::make_unique<AsyncLogQueue>();
    queue_->setSink([this](const LogEntry& entry) { dispatch(entry); });
}

LoggingEngineNew::~LoggingEngineNew() {
    if (Logger::instance() == this)
        Logger::setInstance(nullptr);

    // Queue destructor flushes remaining entries
    queue_.reset();
    if (fileSink_) fileSink_->flush();
}

void LoggingEngineNew::log(LogLevel level, const QString& module,
                            const QString& message, const QVariantMap&) {
    // Map module string to category (best-effort)
    LogCategory cat = LogCategory::General;
    if (module.contains(QStringLiteral("Platform"), Qt::CaseInsensitive))
        cat = LogCategory::Platform;
    else if (module.contains(QStringLiteral("Memory"), Qt::CaseInsensitive))
        cat = LogCategory::Memory;
    else if (module.contains(QStringLiteral("Render"), Qt::CaseInsensitive))
        cat = LogCategory::Rendering;
    else if (module.contains(QStringLiteral("Timeline"), Qt::CaseInsensitive))
        cat = LogCategory::Timeline;
    else if (module.contains(QStringLiteral("Audio"), Qt::CaseInsensitive))
        cat = LogCategory::Audio;
    else if (module.contains(QStringLiteral("Network"), Qt::CaseInsensitive))
        cat = LogCategory::Network;
    else if (module.contains(QStringLiteral("Config"), Qt::CaseInsensitive))
        cat = LogCategory::Config;

    if (!filter_.check(level, cat))
        return;

    LogEntry entry;
    entry.level = level;
    entry.category = cat;
    entry.message = message;
    entry.module = module;
    entry.timestampMs = currentTimestampMs();

    queue_->enqueue(std::move(entry));
}

void LoggingEngineNew::trace(const QString& module, const QString& message) {
    log(LogLevel::Trace, module, message);
}

void LoggingEngineNew::debug(const QString& module, const QString& message) {
    log(LogLevel::Debug, module, message);
}

void LoggingEngineNew::info(const QString& module, const QString& message) {
    log(LogLevel::Info, module, message);
}

void LoggingEngineNew::warn(const QString& module, const QString& message) {
    log(LogLevel::Warning, module, message);
}

void LoggingEngineNew::error(const QString& module, const QString& message,
                              const QVariantMap& context) {
    log(LogLevel::Error, module, message, context);
}

void LoggingEngineNew::fatal(const QString& module, const QString& message,
                              const QVariantMap& context) {
    log(LogLevel::Fatal, module, message, context);
}

void LoggingEngineNew::setMinLevel(LogLevel level) {
    filter_.setMinLevel(level);
}

LogLevel LoggingEngineNew::minLevel() const {
    return filter_.minLevel();
}

void LoggingEngineNew::setCategoryEnabled(LogCategory category, bool enabled) {
    filter_.setCategoryEnabled(category, enabled);
}

bool LoggingEngineNew::isCategoryEnabled(LogCategory category) const {
    return filter_.isCategoryEnabled(category);
}

void LoggingEngineNew::flush() {
    if (queue_) queue_->flush();
    if (fileSink_) fileSink_->flush();
}

void LoggingEngineNew::dispatch(const LogEntry& entry) {
    QString formatted = LogFormatter::format(
        entry.level, entry.category, entry.message);

    if (consoleSink_)
        consoleSink_->write(entry.level, formatted);

    if (fileSink_)
        fileSink_->write(formatted, entry.level);
}

} // namespace gopost::logging
