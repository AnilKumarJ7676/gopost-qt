#pragma once

#include "core/interfaces/IDeviceCapability.h"
#include "core/interfaces/IStorageScheduler.h"

namespace gopost::storage {

using core::interfaces::IDeviceCapability;
using core::interfaces::StorageProfile;
using core::interfaces::StorageType;

class StorageProfiler {
public:
    explicit StorageProfiler(const IDeviceCapability& deviceCap);

    [[nodiscard]] StorageProfile buildProfile() const;
    [[nodiscard]] bool preferSequential() const;

private:
    static StorageProfile profileForType(StorageType type);
    StorageType detectedType_;
};

} // namespace gopost::storage
