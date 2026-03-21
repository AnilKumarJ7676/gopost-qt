#pragma once

#include <QString>
#include <QVariantMap>

namespace gopost::core::interfaces {

enum class LogLevel { Trace = 0, Debug, Info, Warning, Error, Fatal };

enum class LogCategory {
    General,
    Platform,
    Memory,
    Rendering,
    Timeline,
    Audio,
    Network,
    Config
};

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level, const QString& module,
                     const QString& message,
                     const QVariantMap& context = {}) = 0;

    virtual void trace(const QString& module, const QString& message) = 0;
    virtual void debug(const QString& module, const QString& message) = 0;
    virtual void info(const QString& module, const QString& message) = 0;
    virtual void warn(const QString& module, const QString& message) = 0;
    virtual void error(const QString& module, const QString& message,
                       const QVariantMap& context = {}) = 0;
    virtual void fatal(const QString& module, const QString& message,
                       const QVariantMap& context = {}) = 0;

    virtual void setMinLevel(LogLevel level) = 0;
    [[nodiscard]] virtual LogLevel minLevel() const = 0;

    virtual void setCategoryEnabled(LogCategory category, bool enabled) = 0;
    [[nodiscard]] virtual bool isCategoryEnabled(LogCategory category) const = 0;
};

} // namespace gopost::core::interfaces
