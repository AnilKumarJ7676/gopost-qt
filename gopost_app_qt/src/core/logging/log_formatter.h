#pragma once

#include "core/interfaces/ILogger.h"
#include <QString>
#include <cstdint>

namespace gopost::logging {

using core::interfaces::LogLevel;
using core::interfaces::LogCategory;

class LogFormatter {
public:
    static QString format(LogLevel level, LogCategory category,
                          const QString& message,
                          const char* file = nullptr, int line = 0,
                          uint64_t timestampMs = 0);

    static const char* levelStr(LogLevel level);
    static const char* categoryStr(LogCategory category);
};

} // namespace gopost::logging
