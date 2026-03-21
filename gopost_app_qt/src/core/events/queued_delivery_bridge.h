#pragma once

#include "core/interfaces/IEventBus.h"

#include <QThread>
#include <QString>
#include <QVariantMap>

namespace gopost::events {

using core::interfaces::EventHandler;

class QueuedDeliveryBridge {
public:
    static void deliver(QThread* targetThread,
                         EventHandler handler,
                         const QString& eventType,
                         const QVariantMap& data);
};

} // namespace gopost::events
