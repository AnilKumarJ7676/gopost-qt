#pragma once

#include <QString>
#include <QVariantMap>
#include <cstdint>

namespace gopost::core::interfaces {

struct MemoryBudgets {
    int64_t globalBudgetBytes    = 0;
    int64_t frameCacheBytes      = 0;
    int64_t thumbnailCacheBytes  = 0;
    int64_t audioCacheBytes      = 0;
    int64_t effectCacheBytes     = 0;
    int64_t importBufferBytes    = 0;
    int64_t reserveBytes         = 0;
};

class IMemoryBudget {
public:
    virtual ~IMemoryBudget() = default;

    [[nodiscard]] virtual MemoryBudgets computeBudgets() const = 0;
    [[nodiscard]] virtual bool tryAcquire(const QString& category, int64_t bytes) = 0;
    virtual void release(const QString& category, int64_t bytes) = 0;
    [[nodiscard]] virtual int64_t currentUsage(const QString& category) const = 0;
    [[nodiscard]] virtual int64_t budgetFor(const QString& category) const = 0;
    [[nodiscard]] virtual QVariantMap breakdown() const = 0;
};

} // namespace gopost::core::interfaces
