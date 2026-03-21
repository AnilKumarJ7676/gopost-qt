#pragma once

#include "core/interfaces/IPermission.h"
#include "core/interfaces/IPlatformBridge.h"
#include "core/interfaces/ILogger.h"
#include "core/interfaces/IEventBus.h"
#include "permission_state_cache.h"
#include "platform_permission_adapter.h"
#include "denial_handler.h"

#include <memory>

namespace gopost::permissions {

using core::interfaces::IPermission;
using core::interfaces::IPlatformBridge;
using core::interfaces::ILogger;
using core::interfaces::IEventBus;
using core::interfaces::PermissionType;
using core::interfaces::PermissionStatus;

class PermissionEngine : public IPermission {
public:
    PermissionEngine(IPlatformBridge& bridge, ILogger& logger, IEventBus& eventBus);

    PermissionStatus checkPermission(PermissionType type) override;
    Result<PermissionStatus> requestPermission(PermissionType type) override;
    Result<void> openAppSettings() override;

    // Test support: force a status into the cache (for denial-handling tests)
    void forceCacheStatus(PermissionType type, PermissionStatus status);

private:
    ILogger& logger_;
    IEventBus& eventBus_;
    PermissionStateCache cache_;
    PlatformPermissionAdapter adapter_;
    DenialHandler denialHandler_;
};

} // namespace gopost::permissions
