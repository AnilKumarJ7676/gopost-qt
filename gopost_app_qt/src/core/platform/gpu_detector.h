#pragma once

#include "core/interfaces/IDeviceCapability.h"

namespace gopost::platform {

using core::interfaces::GpuInfo;

class GpuDetector {
public:
    static GpuInfo detect();
};

} // namespace gopost::platform
