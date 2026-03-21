#include "event_bus_engine.h"
#include "logging_engine.h"
#include "diagnostics_engine.h"

#include <QMetaObject>
#include <QReadLocker>
#include <QWriteLocker>
#include <algorithm>

namespace gopost::core::engines {

EventBusEngine::EventBusEngine(LoggingEngine* logging,
                               DiagnosticsEngine* diagnostics,
                               QObject* parent)
    : QObject(parent)
    , m_logging(logging)
    , m_diagnostics(diagnostics) {}

EventBusEngine::~EventBusEngine() {
    shutdown();
}

void EventBusEngine::initialize() {
    if (m_initialized) return;
    m_initialized = true;

    if (m_logging) {
        m_logging->info(QStringLiteral("EventBus"),
                        QStringLiteral("Event bus engine initialized"));
    }
}

void EventBusEngine::shutdown() {
    if (!m_initialized) return;

    {
        QWriteLocker lock(&m_lock);
        m_subscriptions.clear();
        m_patternSubscriptions.clear();
        m_filters.clear();
    }

    m_initialized = false;
}

bool EventBusEngine::isInitialized() const {
    return m_initialized;
}

void EventBusEngine::post(const QString& eventType, const QVariantMap& data,
                          Priority priority) {
    deliverEvent(eventType, data, priority);
}

void EventBusEngine::postAsync(const QString& eventType, const QVariantMap& data,
                               Priority priority) {
    QVariantMap dataCopy = data;
    QMetaObject::invokeMethod(this, [this, eventType, dataCopy, priority]() {
        deliverEvent(eventType, dataCopy, priority);
    }, Qt::QueuedConnection);
}

EventBusEngine::SubscriptionId EventBusEngine::subscribe(const QString& eventType,
                                                          EventHandler handler,
                                                          Priority minPriority) {
    QWriteLocker lock(&m_lock);
    int id = m_nextId++;
    m_subscriptions[eventType].append({id, std::move(handler), minPriority});
    m_stats[eventType].subscriberCount++;
    return id;
}

EventBusEngine::SubscriptionId EventBusEngine::subscribePattern(const QString& pattern,
                                                                 EventHandler handler) {
    QWriteLocker lock(&m_lock);
    int id = m_nextId++;

    // Convert wildcard pattern to regex: "memory.*" → "^memory\\..*$"
    QString regexStr = QRegularExpression::escape(pattern)
                           .replace(QStringLiteral("\\*"), QStringLiteral(".*"));
    regexStr = QStringLiteral("^") + regexStr + QStringLiteral("$");

    m_patternSubscriptions.append({id, QRegularExpression(regexStr), std::move(handler)});
    return id;
}

void EventBusEngine::unsubscribe(SubscriptionId id) {
    QWriteLocker lock(&m_lock);

    // Remove from exact subscriptions
    for (auto mapIt = m_subscriptions.begin(); mapIt != m_subscriptions.end(); ++mapIt) {
        auto& list = mapIt.value();
        for (int i = 0; i < list.size(); ++i) {
            if (list[i].id == id) {
                list.removeAt(i);
                if (m_stats.contains(mapIt.key())) {
                    m_stats[mapIt.key()].subscriberCount--;
                }
                return;
            }
        }
    }

    // Remove from pattern subscriptions
    for (int i = 0; i < m_patternSubscriptions.size(); ++i) {
        if (m_patternSubscriptions[i].id == id) {
            m_patternSubscriptions.removeAt(i);
            return;
        }
    }
}

void EventBusEngine::addFilter(const QString& eventType, EventFilter filter) {
    QWriteLocker lock(&m_lock);
    m_filters[eventType].append(std::move(filter));
}

void EventBusEngine::deliverEvent(const QString& eventType, const QVariantMap& data,
                                  Priority priority) {
    QVariantMap mutableData = data;

    // Apply filters
    {
        QReadLocker lock(&m_lock);
        auto filterIt = m_filters.constFind(eventType);
        if (filterIt != m_filters.constEnd()) {
            for (const auto& filter : filterIt.value()) {
                if (!filter(eventType, mutableData)) {
                    return; // Event suppressed
                }
            }
        }
    }

    // Track stats
    m_stats[eventType].postCount++;

    // Collect handlers to call (release lock before calling)
    QList<EventHandler> handlers;
    {
        QReadLocker lock(&m_lock);

        // Exact match subscribers
        auto it = m_subscriptions.constFind(eventType);
        if (it != m_subscriptions.constEnd()) {
            for (const auto& sub : it.value()) {
                if (priority >= sub.minPriority) {
                    handlers.append(sub.handler);
                }
            }
        }

        // Pattern match subscribers
        for (const auto& psub : m_patternSubscriptions) {
            if (psub.regex.match(eventType).hasMatch()) {
                handlers.append(psub.handler);
            }
        }
    }

    // Deliver
    for (const auto& handler : handlers) {
        handler(eventType, mutableData);
    }

    // Emit QML-accessible signal
    emit eventPosted(eventType, mutableData);
}

QVariantMap EventBusEngine::eventStats() const {
    QVariantMap result;
    QReadLocker lock(&m_lock);
    for (auto it = m_stats.constBegin(); it != m_stats.constEnd(); ++it) {
        QVariantMap stat;
        stat[QStringLiteral("postCount")] = it->postCount;
        stat[QStringLiteral("subscriberCount")] = it->subscriberCount;
        result[it.key()] = stat;
    }
    return result;
}

} // namespace gopost::core::engines
