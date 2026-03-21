#pragma once

#include "core_engine.h"

#include <QObject>
#include <QMutex>
#include <QVariantList>
#include <QVariantMap>
#include <QFile>
#include <functional>

namespace gopost::platform { class PlatformCapabilityEngine; }

namespace gopost::core::engines {

class MemoryManagementEngine;

struct LogEntry {
    qint64 timestamp;
    int level;
    QString module;
    QString message;
    QVariantMap context;
};

class LoggingEngine : public QObject, public CoreEngine {
    Q_OBJECT
public:
    explicit LoggingEngine(platform::PlatformCapabilityEngine* platform,
                           MemoryManagementEngine* memory,
                           QObject* parent = nullptr);
    ~LoggingEngine() override;

    // CoreEngine
    void initialize() override;
    void shutdown() override;
    [[nodiscard]] bool isInitialized() const override;
    [[nodiscard]] QString engineName() const override { return QStringLiteral("Logging"); }

    // Log levels
    enum class Level { Trace = 0, Debug, Info, Warning, Error, Fatal };
    Q_ENUM(Level)

    // Structured logging
    void log(Level level, const QString& module, const QString& message,
             const QVariantMap& context = {});

    // Convenience methods
    void trace(const QString& module, const QString& message);
    void debug(const QString& module, const QString& message);
    void info(const QString& module, const QString& message);
    void warning(const QString& module, const QString& message);
    void error(const QString& module, const QString& message,
               const QVariantMap& context = {});
    void fatal(const QString& module, const QString& message,
               const QVariantMap& context = {});

    // Per-module level control
    void setModuleLevel(const QString& module, Level level);
    [[nodiscard]] Level moduleLevel(const QString& module) const;
    void setGlobalLevel(Level level);
    [[nodiscard]] Level globalLevel() const;

    // Ring buffer for QML debug overlay
    Q_INVOKABLE QVariantList recentLogs(int count = 100) const;

    // File rotation
    void setMaxLogFileSizeBytes(qint64 bytes);
    void setMaxLogFiles(int count);

    // Crash reporting callback (preserved from AppLogger)
    using CrashReportCallback = std::function<void(
        const QString& error, const QString& stackTrace, const QVariantMap& extra)>;
    CrashReportCallback onCrashReport;

    // Singleton access for AppLogger bridge
    static LoggingEngine* instance();

signals:
    void logEntry(const QString& module, int level, const QString& message);

private:
    void writeToFile(const QString& formatted);
    void rotateLogFileIfNeeded();
    static QString levelToString(Level level);
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext& ctx,
                                 const QString& msg);

    platform::PlatformCapabilityEngine* m_platform = nullptr;
    MemoryManagementEngine* m_memory = nullptr;
    bool m_initialized = false;

    Level m_globalLevel = Level::Debug;
    mutable QMutex m_mutex;
    QHash<QString, Level> m_moduleLevels;

    // Ring buffer
    QList<LogEntry> m_ringBuffer;
    int m_ringBufferMax = 1000;

    // File sink
    QString m_logDir;
    QFile m_logFile;
    qint64 m_maxFileSize = 10 * 1024 * 1024; // 10 MB
    int m_maxFiles = 5;

    static LoggingEngine* s_instance;
};

} // namespace gopost::core::engines
