#pragma once

#include "core/interfaces/IDeviceCapability.h"

namespace gopost::memory {

using core::interfaces::DeviceTier;

struct BudgetFractions {
    double frameCache      = 0.0;
    double thumbnailCache  = 0.0;
    double audioCache      = 0.0;
    double effectCache     = 0.0;
    double importBuffer    = 0.0;
    double reserve         = 0.0;
    double maxRamFraction  = 0.0;
};

class TierBudgetPolicy {
public:
    static BudgetFractions forTier(DeviceTier tier);
};

} // namespace gopost::memory
