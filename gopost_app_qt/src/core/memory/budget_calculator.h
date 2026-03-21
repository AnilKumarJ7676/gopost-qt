#pragma once

#include "core/interfaces/IMemoryBudget.h"
#include "core/memory/tier_budget_policy.h"

namespace gopost::memory {

using core::interfaces::MemoryBudgets;

class BudgetCalculator {
public:
    static MemoryBudgets calculate(int64_t totalRamBytes,
                                    const BudgetFractions& fractions);
};

} // namespace gopost::memory
