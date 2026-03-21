#pragma once

#include "core/interfaces/ILogger.h"
#include <QString>

namespace gopost::logging {

using core::interfaces::LogLevel;

class ConsoleLogSink {
public:
    ConsoleLogSink();
    void write(LogLevel level, const QString& formatted);

private:
    bool colorsEnabled_;
};

} // namespace gopost::logging
