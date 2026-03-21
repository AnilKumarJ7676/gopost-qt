#pragma once

#include "core/interfaces/ILogger.h"
#include <atomic>
#include <cstdint>

namespace gopost::logging {

using core::interfaces::LogLevel;
using core::interfaces::LogCategory;

class LogFilter {
public:
    LogFilter();

    [[nodiscard]] bool check(LogLevel level, LogCategory category) const;

    void setMinLevel(LogLevel level);
    [[nodiscard]] LogLevel minLevel() const;

    void setCategoryEnabled(LogCategory category, bool enabled);
    [[nodiscard]] bool isCategoryEnabled(LogCategory category) const;

private:
    std::atomic<int> minLevel_;
    std::atomic<uint32_t> categoryMask_;
};

} // namespace gopost::logging
