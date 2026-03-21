#pragma once

#include "core/interfaces/IDeviceCapability.h"

namespace gopost::platform {

using core::interfaces::CpuInfo;
using core::interfaces::GpuInfo;
using core::interfaces::StorageType;
using core::interfaces::DeviceTier;

class TierClassifier {
public:
    static DeviceTier classify(const CpuInfo& cpu,
                               int64_t ramBytes,
                               const GpuInfo& gpu,
                               StorageType storage);
};

} // namespace gopost::platform
