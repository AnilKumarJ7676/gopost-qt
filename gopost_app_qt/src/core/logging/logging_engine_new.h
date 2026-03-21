#pragma once

#include "core/interfaces/ILogger.h"
#include "core/logging/log_filter.h"
#include "core/logging/async_log_queue.h"
#include "core/logging/console_log_sink.h"
#include "core/logging/file_log_sink.h"

#include <memory>

namespace gopost::logging {

using core::interfaces::ILogger;
using core::interfaces::LogLevel;
using core::interfaces::LogCategory;

class LoggingEngineNew : public ILogger {
public:
    explicit LoggingEngineNew(const QString& logDir = {});
    ~LoggingEngineNew() override;

    // ILogger
    void log(LogLevel level, const QString& module, const QString& message,
             const QVariantMap& context = {}) override;
    void trace(const QString& module, const QString& message) override;
    void debug(const QString& module, const QString& message) override;
    void info(const QString& module, const QString& message) override;
    void warn(const QString& module, const QString& message) override;
    void error(const QString& module, const QString& message,
               const QVariantMap& context = {}) override;
    void fatal(const QString& module, const QString& message,
               const QVariantMap& context = {}) override;
    void setMinLevel(LogLevel level) override;
    LogLevel minLevel() const override;
    void setCategoryEnabled(LogCategory category, bool enabled) override;
    bool isCategoryEnabled(LogCategory category) const override;

    // Direct access for testing
    FileLogSink* fileSink() const { return fileSink_.get(); }
    void flush();

private:
    void dispatch(const LogEntry& entry);

    LogFilter filter_;
    std::unique_ptr<ConsoleLogSink> consoleSink_;
    std::unique_ptr<FileLogSink> fileSink_;
    std::unique_ptr<AsyncLogQueue> queue_;
};

// Static accessor (NOT singleton — must be explicitly set)
class Logger {
public:
    static ILogger* instance();
    static void setInstance(ILogger* logger);
private:
    static ILogger* s_instance;
};

} // namespace gopost::logging
