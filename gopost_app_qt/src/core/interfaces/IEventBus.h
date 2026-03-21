#pragma once

#include <QString>
#include <QVariantMap>
#include <functional>
#include <typeindex>

namespace gopost::core::interfaces {

enum class DeliveryMode { Sync, Async, Queued };

struct Event {
    QString type;
    QVariantMap data;
    DeliveryMode mode = DeliveryMode::Sync;
    virtual ~Event() = default;
};

template <typename T>
struct TypedEvent : Event {
    T payload;
};

using SubscriptionId = int;
using EventHandler = std::function<void(const QString& eventType,
                                        const QVariantMap& data)>;

class IEventBus {
public:
    virtual ~IEventBus() = default;

    virtual SubscriptionId subscribe(const QString& eventType,
                                     EventHandler handler) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
    virtual void publish(const QString& eventType,
                         const QVariantMap& data = {},
                         DeliveryMode mode = DeliveryMode::Sync) = 0;

    // Type-safe helpers
    template <typename T>
    SubscriptionId on(std::function<void(const T&)> handler) {
        return subscribe(QString::fromStdString(typeid(T).name()),
                         [h = std::move(handler)](const QString&, const QVariantMap&) {
                             h(T{});
                         });
    }

    template <typename T>
    void emit_(const T& event, DeliveryMode mode = DeliveryMode::Sync) {
        publish(QString::fromStdString(typeid(T).name()), {}, mode);
    }
};

} // namespace gopost::core::interfaces
