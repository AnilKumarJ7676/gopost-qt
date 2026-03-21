#include "core/memory/budget_calculator.h"

namespace gopost::memory {

MemoryBudgets BudgetCalculator::calculate(int64_t totalRamBytes,
                                           const BudgetFractions& f) {
    auto ceiling = static_cast<int64_t>(
        static_cast<double>(totalRamBytes) * f.maxRamFraction);

    MemoryBudgets b;
    b.globalBudgetBytes   = ceiling;
    b.frameCacheBytes     = static_cast<int64_t>(static_cast<double>(ceiling) * f.frameCache);
    b.thumbnailCacheBytes = static_cast<int64_t>(static_cast<double>(ceiling) * f.thumbnailCache);
    b.audioCacheBytes     = static_cast<int64_t>(static_cast<double>(ceiling) * f.audioCache);
    b.effectCacheBytes    = static_cast<int64_t>(static_cast<double>(ceiling) * f.effectCache);
    b.importBufferBytes   = static_cast<int64_t>(static_cast<double>(ceiling) * f.importBuffer);
    b.reserveBytes        = static_cast<int64_t>(static_cast<double>(ceiling) * f.reserve);
    return b;
}

} // namespace gopost::memory
