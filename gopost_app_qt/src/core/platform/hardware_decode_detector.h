#pragma once

#include "core/interfaces/IDeviceCapability.h"

namespace gopost::platform {

using core::interfaces::HwDecodeSupport;

class HardwareDecodeDetector {
public:
    static HwDecodeSupport detect();
};

} // namespace gopost::platform
