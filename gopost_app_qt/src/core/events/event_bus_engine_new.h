#pragma once

#include "core/interfaces/IEventBus.h"
#include "core/interfaces/ILogger.h"
#include "core/events/subscription_registry.h"

namespace gopost::events {

using core::interfaces::IEventBus;
using core::interfaces::ILogger;
using core::interfaces::DeliveryMode;
using core::interfaces::EventHandler;
using core::interfaces::SubscriptionId;

class EventBusEngineNew : public IEventBus {
public:
    explicit EventBusEngineNew(ILogger* logger = nullptr);

    // IEventBus
    SubscriptionId subscribe(const QString& eventType,
                              EventHandler handler) override;
    void unsubscribe(SubscriptionId id) override;
    void publish(const QString& eventType,
                 const QVariantMap& data = {},
                 DeliveryMode mode = DeliveryMode::Sync) override;

    // Extended: subscribe with explicit delivery mode
    SubscriptionId subscribeWithMode(const QString& eventType,
                                      EventHandler handler,
                                      DeliveryMode mode);

private:
    ILogger* logger_;
    SubscriptionRegistry registry_;
};

} // namespace gopost::events
