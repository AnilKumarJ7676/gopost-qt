#pragma once

#include "core/events/subscription_registry.h"

#include <QString>
#include <QVariantMap>
#include <vector>

namespace gopost::events {

class ImmediateDispatcher {
public:
    static void dispatch(const std::vector<Subscription>& subs,
                          const QString& eventType,
                          const QVariantMap& data) {
        for (const auto& sub : subs) {
            sub.handler(eventType, data);
        }
    }
};

} // namespace gopost::events
