#include "core/events/event_bus_engine_new.h"
#include "core/events/immediate_dispatcher.h"
#include "core/events/queued_delivery_bridge.h"

namespace gopost::events {

EventBusEngineNew::EventBusEngineNew(ILogger* logger)
    : logger_(logger) {}

SubscriptionId EventBusEngineNew::subscribe(const QString& eventType,
                                              EventHandler handler) {
    return subscribeWithMode(eventType, std::move(handler), DeliveryMode::Sync);
}

SubscriptionId EventBusEngineNew::subscribeWithMode(const QString& eventType,
                                                      EventHandler handler,
                                                      DeliveryMode mode) {
    auto id = registry_.subscribe(eventType.toStdString(), std::move(handler), mode);

    if (logger_)
        logger_->debug(QStringLiteral("EventBus"),
                        QStringLiteral("Subscribe id=%1 type=%2 mode=%3")
                            .arg(id).arg(eventType).arg(static_cast<int>(mode)));
    return id;
}

void EventBusEngineNew::unsubscribe(SubscriptionId id) {
    registry_.unsubscribe(id);

    if (logger_)
        logger_->debug(QStringLiteral("EventBus"),
                        QStringLiteral("Unsubscribe id=%1").arg(id));
}

void EventBusEngineNew::publish(const QString& eventType,
                                  const QVariantMap& data,
                                  DeliveryMode mode) {
    auto snap = registry_.snapshot(eventType.toStdString());
    if (!snap || snap->empty()) return;

    if (logger_)
        logger_->trace(QStringLiteral("EventBus"),
                        QStringLiteral("Publish type=%1 subs=%2")
                            .arg(eventType).arg(snap->size()));

    for (const auto& sub : *snap) {
        DeliveryMode effectiveMode = (mode == DeliveryMode::Sync) ? sub.mode : mode;

        if (effectiveMode == DeliveryMode::Queued) {
            QueuedDeliveryBridge::deliver(
                sub.subscriberThread, sub.handler, eventType, data);
        } else {
            // Sync / Async → immediate dispatch on publisher's thread
            sub.handler(eventType, data);
        }
    }
}

} // namespace gopost::events
