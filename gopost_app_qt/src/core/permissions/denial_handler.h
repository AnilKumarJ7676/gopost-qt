#pragma once

#include "core/interfaces/IPermission.h"
#include "core/interfaces/IEventBus.h"
#include "core/interfaces/ILogger.h"
#include "permission_rationale_provider.h"

namespace gopost::permissions {

using core::interfaces::PermissionType;
using core::interfaces::PermissionStatus;
using core::interfaces::IEventBus;
using core::interfaces::ILogger;

struct DenialGuidance {
    bool shouldShowSettings = false;
    bool canRetry = false;
    PermissionRationale rationale;
};

class DenialHandler {
public:
    DenialHandler(ILogger& logger, IEventBus& eventBus);

    [[nodiscard]] DenialGuidance handle(PermissionType type, PermissionStatus status);

private:
    ILogger& logger_;
    IEventBus& eventBus_;
};

} // namespace gopost::permissions
