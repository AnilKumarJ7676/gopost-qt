#pragma once

#include "core/memory/memory_pool_registry.h"

#include <QString>
#include <QVariantMap>
#include <cstdint>

namespace gopost::memory {

class BudgetEnforcer {
public:
    explicit BudgetEnforcer(MemoryPoolRegistry& registry);

    [[nodiscard]] bool tryAcquire(const QString& category, int64_t bytes);
    void release(const QString& category, int64_t bytes);
    [[nodiscard]] int64_t currentUsage(const QString& category) const;
    [[nodiscard]] int64_t budgetFor(const QString& category) const;
    [[nodiscard]] QVariantMap breakdown() const;

private:
    MemoryPoolRegistry& registry_;
};

} // namespace gopost::memory
