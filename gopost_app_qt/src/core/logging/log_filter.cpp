#include "core/logging/log_filter.h"

namespace gopost::logging {

LogFilter::LogFilter()
    : minLevel_(static_cast<int>(LogLevel::Trace))
    , categoryMask_(0xFFFFFFFF) // all categories enabled
{
}

bool LogFilter::check(LogLevel level, LogCategory category) const {
    if (static_cast<int>(level) < minLevel_.load(std::memory_order_relaxed))
        return false;

    uint32_t bit = 1u << static_cast<int>(category);
    return (categoryMask_.load(std::memory_order_relaxed) & bit) != 0;
}

void LogFilter::setMinLevel(LogLevel level) {
    minLevel_.store(static_cast<int>(level), std::memory_order_relaxed);
}

LogLevel LogFilter::minLevel() const {
    return static_cast<LogLevel>(minLevel_.load(std::memory_order_relaxed));
}

void LogFilter::setCategoryEnabled(LogCategory category, bool enabled) {
    uint32_t bit = 1u << static_cast<int>(category);
    if (enabled)
        categoryMask_.fetch_or(bit, std::memory_order_relaxed);
    else
        categoryMask_.fetch_and(~bit, std::memory_order_relaxed);
}

bool LogFilter::isCategoryEnabled(LogCategory category) const {
    uint32_t bit = 1u << static_cast<int>(category);
    return (categoryMask_.load(std::memory_order_relaxed) & bit) != 0;
}

} // namespace gopost::logging
