#include "core/logging/app_logger.h"
#include "core/engines/logging_engine.h"

#include <QDateTime>
#include <QFile>
#include <QLoggingCategory>
#include <QMutex>
#include <QTextStream>

#include <cstdio>

namespace gopost::core {

bool AppLogger::s_initialized = false;
CrashReportCallback AppLogger::onCrashReport = nullptr;

static QFile* s_logFile = nullptr;
static QMutex s_logMutex;

static void fileMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    Q_UNUSED(ctx);
    const char* typeStr = "unknown";
    switch (type) {
    case QtDebugMsg:    typeStr = "debug"; break;
    case QtInfoMsg:     typeStr = "info"; break;
    case QtWarningMsg:  typeStr = "warning"; break;
    case QtCriticalMsg: typeStr = "critical"; break;
    case QtFatalMsg:    typeStr = "fatal"; break;
    }
    QString line = QStringLiteral("[%1] [%2] %3\n")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz")))
        .arg(QString::fromLatin1(typeStr))
        .arg(msg);

    // Write to log file
    QMutexLocker lock(&s_logMutex);
    if (s_logFile && s_logFile->isOpen()) {
        s_logFile->write(line.toUtf8());
        s_logFile->flush();
    }
    // Also write to stderr
    fprintf(stderr, "%s", line.toUtf8().constData());
    fflush(stderr);
}

void AppLogger::init() {
    if (s_initialized) return;
    s_initialized = true;

    // Open log file
    s_logFile = new QFile(QStringLiteral("C:/tmp/gopost_app.log"));
    s_logFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);

    // Install message handler
    qInstallMessageHandler(fileMessageHandler);

    qSetMessagePattern(QStringLiteral("[%{time hh:mm:ss.zzz}] [%{type}] %{message}"));
}

void AppLogger::debug(const QString& message) {
    if (auto* engine = engines::LoggingEngine::instance()) {
        engine->debug(QStringLiteral("App"), message);
        return;
    }
    qDebug().noquote() << message;
}

void AppLogger::info(const QString& message) {
    if (auto* engine = engines::LoggingEngine::instance()) {
        engine->info(QStringLiteral("App"), message);
        return;
    }
    qInfo().noquote() << message;
}

void AppLogger::warning(const QString& message) {
    if (auto* engine = engines::LoggingEngine::instance()) {
        engine->warning(QStringLiteral("App"), message);
        return;
    }
    qWarning().noquote() << message;
}

void AppLogger::error(const QString& message,
                      const QString& error,
                      const QString& stackTrace) {
    if (auto* engine = engines::LoggingEngine::instance()) {
        QVariantMap ctx;
        if (!error.isEmpty()) ctx[QStringLiteral("error")] = error;
        if (!stackTrace.isEmpty()) ctx[QStringLiteral("stackTrace")] = stackTrace;
        engine->error(QStringLiteral("App"), message, ctx);
    } else {
        qCritical().noquote() << message;
        if (!error.isEmpty())
            qCritical().noquote() << QStringLiteral("  Error: ") << error;
    }
    reportCrash(error.isEmpty() ? message : error, stackTrace);
}

void AppLogger::fatal(const QString& error, const QString& stackTrace) {
    if (auto* engine = engines::LoggingEngine::instance()) {
        QVariantMap ctx;
        if (!stackTrace.isEmpty()) ctx[QStringLiteral("stackTrace")] = stackTrace;
        engine->fatal(QStringLiteral("App"), error, ctx);
    } else {
        qCritical().noquote() << QStringLiteral("FATAL: ") << error;
    }
    reportCrash(error, stackTrace, true);
}

void AppLogger::reportCrash(const QString& error,
                             const QString& stackTrace,
                             bool fatal) {
    if (!onCrashReport) return;
    try {
        QVariantMap extra;
        extra[QStringLiteral("fatal")] = fatal;
        onCrashReport(error, stackTrace, extra);
    } catch (...) {
#ifdef QT_DEBUG
        throw;
#endif
    }
}

} // namespace gopost::core
