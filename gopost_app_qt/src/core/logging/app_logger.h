#pragma once

#include <QString>
#include <QDebug>
#include <functional>
#include <QVariantMap>

namespace gopost::core {

using CrashReportCallback = std::function<void(
    const QString& error,
    const QString& stackTrace,
    const QVariantMap& extra)>;

class AppLogger {
public:
    AppLogger() = delete;

    static void init();
    static void debug(const QString& message);
    static void info(const QString& message);
    static void warning(const QString& message);
    static void error(const QString& message,
                      const QString& error = {},
                      const QString& stackTrace = {});
    static void fatal(const QString& error, const QString& stackTrace = {});

    static CrashReportCallback onCrashReport;

private:
    static void reportCrash(const QString& error,
                            const QString& stackTrace,
                            bool fatal = false);
    static bool s_initialized;
};

} // namespace gopost::core
