#pragma once

#include "core/interfaces/IEventBus.h"

#include <QThread>
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace gopost::events {

using core::interfaces::DeliveryMode;
using core::interfaces::EventHandler;
using core::interfaces::SubscriptionId;

struct Subscription {
    SubscriptionId id;
    EventHandler handler;
    DeliveryMode mode;
    QThread* subscriberThread;
};

using SubscriptionSnapshot = std::shared_ptr<const std::vector<Subscription>>;

class SubscriptionRegistry {
public:
    SubscriptionId subscribe(const std::string& eventType,
                              EventHandler handler,
                              DeliveryMode mode);

    void unsubscribe(SubscriptionId id);

    SubscriptionSnapshot snapshot(const std::string& eventType) const;

private:
    void rebuildSnapshot(const std::string& eventType);

    mutable std::shared_mutex mutex_;
    std::atomic<int> nextId_{1};

    // Master list (modified under write lock)
    std::unordered_map<std::string, std::vector<Subscription>> subs_;

    // Atomic snapshots (read lock-free)
    mutable std::unordered_map<std::string, std::shared_ptr<const std::vector<Subscription>>> snapshots_;
};

} // namespace gopost::events
