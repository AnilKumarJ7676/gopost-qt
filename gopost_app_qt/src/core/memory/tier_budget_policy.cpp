#include "core/memory/tier_budget_policy.h"

namespace gopost::memory {

BudgetFractions TierBudgetPolicy::forTier(DeviceTier tier) {
    switch (tier) {
    case DeviceTier::Low:
        return {.frameCache = 0.25, .thumbnailCache = 0.15, .audioCache = 0.10,
                .effectCache = 0.10, .importBuffer = 0.10, .reserve = 0.30,
                .maxRamFraction = 0.40};
    case DeviceTier::Mid:
        return {.frameCache = 0.30, .thumbnailCache = 0.15, .audioCache = 0.10,
                .effectCache = 0.15, .importBuffer = 0.10, .reserve = 0.20,
                .maxRamFraction = 0.50};
    case DeviceTier::High:
        return {.frameCache = 0.35, .thumbnailCache = 0.12, .audioCache = 0.08,
                .effectCache = 0.20, .importBuffer = 0.10, .reserve = 0.15,
                .maxRamFraction = 0.60};
    case DeviceTier::Ultra:
        return {.frameCache = 0.35, .thumbnailCache = 0.10, .audioCache = 0.07,
                .effectCache = 0.25, .importBuffer = 0.10, .reserve = 0.13,
                .maxRamFraction = 0.70};
    }
    return {};
}

} // namespace gopost::memory
