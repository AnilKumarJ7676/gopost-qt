#pragma once

#include "core/interfaces/IMemoryBudget.h"
#include "core/interfaces/IDeviceCapability.h"
#include "core/memory/budget_enforcer.h"
#include "core/memory/memory_pool_registry.h"

namespace gopost::memory {

using core::interfaces::IDeviceCapability;
using core::interfaces::IMemoryBudget;
using core::interfaces::MemoryBudgets;

class MemoryEngine : public IMemoryBudget {
public:
    explicit MemoryEngine(const IDeviceCapability& deviceCap);

    // IMemoryBudget
    [[nodiscard]] MemoryBudgets computeBudgets() const override;
    [[nodiscard]] bool tryAcquire(const QString& category, int64_t bytes) override;
    void release(const QString& category, int64_t bytes) override;
    [[nodiscard]] int64_t currentUsage(const QString& category) const override;
    [[nodiscard]] int64_t budgetFor(const QString& category) const override;
    [[nodiscard]] QVariantMap breakdown() const override;

private:
    MemoryBudgets budgets_;
    MemoryPoolRegistry registry_;
    BudgetEnforcer enforcer_;
};

} // namespace gopost::memory
