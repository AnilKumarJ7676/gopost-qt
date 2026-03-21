#include "core/events/subscription_registry.h"

#include <algorithm>

namespace gopost::events {

SubscriptionId SubscriptionRegistry::subscribe(const std::string& eventType,
                                                EventHandler handler,
                                                DeliveryMode mode) {
    int id = nextId_.fetch_add(1, std::memory_order_relaxed);

    std::unique_lock lock(mutex_);
    subs_[eventType].push_back({id, std::move(handler), mode, QThread::currentThread()});
    rebuildSnapshot(eventType);
    return id;
}

void SubscriptionRegistry::unsubscribe(SubscriptionId id) {
    std::unique_lock lock(mutex_);

    for (auto& [type, vec] : subs_) {
        auto it = std::remove_if(vec.begin(), vec.end(),
                                  [id](const Subscription& s) { return s.id == id; });
        if (it != vec.end()) {
            vec.erase(it, vec.end());
            rebuildSnapshot(type);
            return;
        }
    }
}

SubscriptionSnapshot SubscriptionRegistry::snapshot(const std::string& eventType) const {
    std::shared_lock lock(mutex_);
    auto it = snapshots_.find(eventType);
    if (it != snapshots_.end())
        return it->second;
    return nullptr;
}

void SubscriptionRegistry::rebuildSnapshot(const std::string& eventType) {
    // Called under write lock
    auto it = subs_.find(eventType);
    if (it == subs_.end() || it->second.empty()) {
        snapshots_.erase(eventType);
        return;
    }
    snapshots_[eventType] = std::make_shared<const std::vector<Subscription>>(it->second);
}

} // namespace gopost::events
