#include "core/memory/memory_engine.h"
#include "core/memory/tier_budget_policy.h"
#include "core/memory/budget_calculator.h"

namespace gopost::memory {

MemoryEngine::MemoryEngine(const IDeviceCapability& deviceCap)
    : enforcer_(registry_)
{
    auto fractions = TierBudgetPolicy::forTier(deviceCap.deviceTier());
    budgets_ = BudgetCalculator::calculate(deviceCap.totalRamBytes(), fractions);
    registry_.initialize(budgets_);
}

MemoryBudgets MemoryEngine::computeBudgets() const {
    return budgets_;
}

bool MemoryEngine::tryAcquire(const QString& category, int64_t bytes) {
    return enforcer_.tryAcquire(category, bytes);
}

void MemoryEngine::release(const QString& category, int64_t bytes) {
    enforcer_.release(category, bytes);
}

int64_t MemoryEngine::currentUsage(const QString& category) const {
    return enforcer_.currentUsage(category);
}

int64_t MemoryEngine::budgetFor(const QString& category) const {
    return enforcer_.budgetFor(category);
}

QVariantMap MemoryEngine::breakdown() const {
    return enforcer_.breakdown();
}

} // namespace gopost::memory
