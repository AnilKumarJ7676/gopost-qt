#pragma once

#include "core/interfaces/IDeviceCapability.h"
#include <QString>

namespace gopost::platform {

using core::interfaces::StorageType;

class StorageDetector {
public:
    static StorageType detect(const QString& path = {});
};

} // namespace gopost::platform
