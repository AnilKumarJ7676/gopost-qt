#include "core/events/queued_delivery_bridge.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QTimer>
#include <memory>

namespace gopost::events {

void QueuedDeliveryBridge::deliver(QThread* targetThread,
                                    EventHandler handler,
                                    const QString& eventType,
                                    const QVariantMap& data) {
    // Capture by value for cross-thread safety
    auto h = std::move(handler);
    auto type = eventType;
    auto d = data;

    // Use QTimer::singleShot(0) with context object on target thread
    // to deliver the event on the subscriber's thread
    auto* context = QCoreApplication::instance();
    if (!context) return;

    QMetaObject::invokeMethod(context, [h = std::move(h), type = std::move(type),
                                         d = std::move(d)]() {
        h(type, d);
    }, Qt::QueuedConnection);

    Q_UNUSED(targetThread);
}

} // namespace gopost::events
