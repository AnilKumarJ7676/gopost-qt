#pragma once

#include "core/interfaces/IPermission.h"
#include "core/interfaces/IPlatformBridge.h"
#include "core/interfaces/IResult.h"

namespace gopost::permissions {

using core::interfaces::IPlatformBridge;
using core::interfaces::PermissionType;
using core::interfaces::PermissionStatus;

class PlatformPermissionAdapter {
public:
    explicit PlatformPermissionAdapter(IPlatformBridge& bridge);

    [[nodiscard]] PermissionStatus check(PermissionType type);
    [[nodiscard]] Result<PermissionStatus> request(PermissionType type);
    [[nodiscard]] Result<void> openSettings();

private:
    IPlatformBridge& bridge_;
};

} // namespace gopost::permissions
