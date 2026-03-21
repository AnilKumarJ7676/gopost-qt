#pragma once

#include "core/interfaces/IPermission.h"
#include <shared_mutex>
#include <unordered_map>

namespace gopost::permissions {

using core::interfaces::PermissionType;
using core::interfaces::PermissionStatus;

class PermissionStateCache {
public:
    [[nodiscard]] PermissionStatus get(PermissionType type) const;
    void set(PermissionType type, PermissionStatus status);
    [[nodiscard]] bool has(PermissionType type) const;
    void clear();

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<int, PermissionStatus> cache_;
};

} // namespace gopost::permissions
