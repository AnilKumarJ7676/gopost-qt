#pragma once

#include "core/interfaces/IPlatformBridge.h"

#include <memory>

namespace gopost::bridges {

class BridgeFactory {
public:
    [[nodiscard]] static std::unique_ptr<core::interfaces::IPlatformBridge> create();
};

} // namespace gopost::bridges
