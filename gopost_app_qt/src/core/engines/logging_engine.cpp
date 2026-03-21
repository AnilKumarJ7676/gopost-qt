#include "logging_engine.h"
#include "platform_capability_engine.h"
#include "memory_management_engine.h"

#include <QDateTime>
#include <QDir>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

namespace gopost::core::engines {

LoggingEngine* LoggingEngine::s_instance = nullptr;

LoggingEngine::LoggingEngine(PlatformCapabilityEngine* platform,
                             MemoryManagementEngine* memory,
                             QObject* parent)
    : QObject(parent)
    , m_platform(platform)
    , m_memory(memory) {}

LoggingEngine::~LoggingEngine() {
    shutdown();
}

void LoggingEngine::initialize() {
    if (m_initialized) return;

    s_instance = this;

    // Set up log directory
    m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
               + QStringLiteral("/logs");
    QDir().mkpath(m_logDir);

    // Open log file
    QString logPath = m_logDir + QStringLiteral("/gopost_app.log");
    m_logFile.setFileName(logPath);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

    // Install Qt message handler
    qInstallMessageHandler(qtMessageHandler);

    m_initialized = true;

    info(QStringLiteral("Logging"), QStringLiteral("Logging engine initialized, log dir: %1").arg(m_logDir));
}

void LoggingEngine::shutdown() {
    if (!m_initialized) return;

    qInstallMessageHandler(nullptr); // Restore default handler
    m_logFile.close();

    if (s_instance == this) {
        s_instance = nullptr;
    }

    m_initialized = false;
}

bool LoggingEngine::isInitialized() const {
    return m_initialized;
}

LoggingEngine* LoggingEngine::instance() {
    return s_instance;
}

void LoggingEngine::log(Level level, const QString& module, const QString& message,
                        const QVariantMap& context) {
    // Check global level
    if (level < m_globalLevel) return;

    // Check per-module level
    {
        QMutexLocker lock(&m_mutex);
        auto it = m_moduleLevels.constFind(module);
        if (it != m_moduleLevels.constEnd() && level < it.value()) {
            return;
        }
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Format message
    QString timestamp = QDateTime::fromMSecsSinceEpoch(now).toString(
        QStringLiteral("hh:mm:ss.zzz"));
    QString formatted = QStringLiteral("[%1] [%2] [%3] %4")
        .arg(timestamp, levelToString(level), module, message);

    // Write to stderr
    QTextStream err(stderr);
    err << formatted << "\n";
    err.flush();

    // Write to file
    writeToFile(formatted);

    // Add to ring buffer
    {
        QMutexLocker lock(&m_mutex);
        if (m_ringBuffer.size() >= m_ringBufferMax) {
            m_ringBuffer.removeFirst();
        }
        m_ringBuffer.append({now, static_cast<int>(level), module, message, context});
    }

    // Emit signal for live listeners
    emit logEntry(module, static_cast<int>(level), message);
}

void LoggingEngine::trace(const QString& module, const QString& message) {
    log(Level::Trace, module, message);
}

void LoggingEngine::debug(const QString& module, const QString& message) {
    log(Level::Debug, module, message);
}

void LoggingEngine::info(const QString& module, const QString& message) {
    log(Level::Info, module, message);
}

void LoggingEngine::warning(const QString& module, const QString& message) {
    log(Level::Warning, module, message);
}

void LoggingEngine::error(const QString& module, const QString& message,
                          const QVariantMap& context) {
    log(Level::Error, module, message, context);
}

void LoggingEngine::fatal(const QString& module, const QString& message,
                          const QVariantMap& context) {
    log(Level::Fatal, module, message, context);

    // Trigger crash report callback
    if (onCrashReport) {
        onCrashReport(message, QString(), context);
    }
}

void LoggingEngine::setModuleLevel(const QString& module, Level level) {
    QMutexLocker lock(&m_mutex);
    m_moduleLevels[module] = level;
}

LoggingEngine::Level LoggingEngine::moduleLevel(const QString& module) const {
    QMutexLocker lock(&m_mutex);
    auto it = m_moduleLevels.constFind(module);
    return it != m_moduleLevels.constEnd() ? it.value() : m_globalLevel;
}

void LoggingEngine::setGlobalLevel(Level level) {
    m_globalLevel = level;
}

LoggingEngine::Level LoggingEngine::globalLevel() const {
    return m_globalLevel;
}

QVariantList LoggingEngine::recentLogs(int count) const {
    QMutexLocker lock(&m_mutex);
    QVariantList result;
    int start = qMax(0, m_ringBuffer.size() - count);
    for (int i = start; i < m_ringBuffer.size(); ++i) {
        const auto& entry = m_ringBuffer[i];
        QVariantMap map;
        map[QStringLiteral("timestamp")] = entry.timestamp;
        map[QStringLiteral("level")] = entry.level;
        map[QStringLiteral("module")] = entry.module;
        map[QStringLiteral("message")] = entry.message;
        if (!entry.context.isEmpty()) {
            map[QStringLiteral("context")] = entry.context;
        }
        result.append(map);
    }
    return result;
}

void LoggingEngine::setMaxLogFileSizeBytes(qint64 bytes) {
    m_maxFileSize = bytes;
}

void LoggingEngine::setMaxLogFiles(int count) {
    m_maxFiles = count;
}

void LoggingEngine::writeToFile(const QString& formatted) {
    if (!m_logFile.isOpen()) return;

    rotateLogFileIfNeeded();

    QTextStream out(&m_logFile);
    out << formatted << "\n";
    out.flush();
}

void LoggingEngine::rotateLogFileIfNeeded() {
    if (m_logFile.size() < m_maxFileSize) return;

    m_logFile.close();

    // Rotate: .log.4 → delete, .log.3 → .log.4, ... .log → .log.1
    QString basePath = m_logDir + QStringLiteral("/gopost_app");
    QFile::remove(basePath + QStringLiteral(".log.%1").arg(m_maxFiles));
    for (int i = m_maxFiles - 1; i >= 1; --i) {
        QString src = basePath + QStringLiteral(".log.%1").arg(i);
        QString dst = basePath + QStringLiteral(".log.%1").arg(i + 1);
        QFile::rename(src, dst);
    }
    QFile::rename(basePath + QStringLiteral(".log"),
                  basePath + QStringLiteral(".log.1"));

    m_logFile.setFileName(basePath + QStringLiteral(".log"));
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

QString LoggingEngine::levelToString(Level level) {
    switch (level) {
    case Level::Trace:   return QStringLiteral("trace");
    case Level::Debug:   return QStringLiteral("debug");
    case Level::Info:    return QStringLiteral("info");
    case Level::Warning: return QStringLiteral("warn");
    case Level::Error:   return QStringLiteral("error");
    case Level::Fatal:   return QStringLiteral("fatal");
    }
    return QStringLiteral("unknown");
}

void LoggingEngine::qtMessageHandler(QtMsgType type, const QMessageLogContext& /*ctx*/,
                                     const QString& msg) {
    if (!s_instance) return;

    Level level;
    switch (type) {
    case QtDebugMsg:    level = Level::Debug; break;
    case QtInfoMsg:     level = Level::Info; break;
    case QtWarningMsg:  level = Level::Warning; break;
    case QtCriticalMsg: level = Level::Error; break;
    case QtFatalMsg:    level = Level::Fatal; break;
    }

    s_instance->log(level, QStringLiteral("Qt"), msg);
}

} // namespace gopost::core::engines
