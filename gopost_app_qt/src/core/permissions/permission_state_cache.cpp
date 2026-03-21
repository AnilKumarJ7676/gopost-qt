#include "permission_state_cache.h"

namespace gopost::permissions {

PermissionStatus PermissionStateCache::get(PermissionType type) const {
    std::shared_lock lock(mutex_);
    auto it = cache_.find(static_cast<int>(type));
    return (it != cache_.end()) ? it->second : PermissionStatus::NOT_DETERMINED;
}

void PermissionStateCache::set(PermissionType type, PermissionStatus status) {
    std::unique_lock lock(mutex_);
    cache_[static_cast<int>(type)] = status;
}

bool PermissionStateCache::has(PermissionType type) const {
    std::shared_lock lock(mutex_);
    return cache_.count(static_cast<int>(type)) > 0;
}

void PermissionStateCache::clear() {
    std::unique_lock lock(mutex_);
    cache_.clear();
}

} // namespace gopost::permissions
