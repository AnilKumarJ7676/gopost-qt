#pragma once

#include "core/interfaces/IPermission.h"
#include <string>

namespace gopost::permissions {

using core::interfaces::PermissionType;

struct PermissionRationale {
    std::string title;
    std::string explanation;
    std::string deniedAction;
};

class PermissionRationaleProvider {
public:
    [[nodiscard]] static PermissionRationale get(PermissionType type);
};

} // namespace gopost::permissions
