#pragma once

#include "core/interfaces/IDeviceCapability.h"

namespace gopost::platform {

using core::interfaces::CpuInfo;

class CpuDetector {
public:
    static CpuInfo detect();
};

} // namespace gopost::platform
