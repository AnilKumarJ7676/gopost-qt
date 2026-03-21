#pragma once

#include "core_engine.h"

#include <QObject>
#include <QHash>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QVariantMap>
#include <functional>

namespace gopost::core::engines {

class LoggingEngine;
class DiagnosticsEngine;

class EventBusEngine : public QObject, public CoreEngine {
    Q_OBJECT
public:
    explicit EventBusEngine(LoggingEngine* logging,
                            DiagnosticsEngine* diagnostics = nullptr,
                            QObject* parent = nullptr);
    ~EventBusEngine() override;

    // CoreEngine
    void initialize() override;
    void shutdown() override;
    [[nodiscard]] bool isInitialized() const override;
    [[nodiscard]] QString engineName() const override { return QStringLiteral("EventBus"); }

    // Priority
    enum class Priority { Low = 0, Normal = 1, High = 2, Critical = 3 };
    Q_ENUM(Priority)

    using SubscriptionId = int;
    using EventHandler = std::function<void(const QString& eventType,
                                            const QVariantMap& data)>;
    using EventFilter = std::function<bool(const QString& eventType,
                                           QVariantMap& data)>;

    // Post events
    void post(const QString& eventType, const QVariantMap& data = {},
              Priority priority = Priority::Normal);
    void postAsync(const QString& eventType, const QVariantMap& data = {},
                   Priority priority = Priority::Normal);

    // Subscribe
    SubscriptionId subscribe(const QString& eventType, EventHandler handler,
                             Priority minPriority = Priority::Low);
    SubscriptionId subscribePattern(const QString& pattern, EventHandler handler);
    void unsubscribe(SubscriptionId id);

    // Filters
    void addFilter(const QString& eventType, EventFilter filter);

    // Diagnostics
    Q_INVOKABLE QVariantMap eventStats() const;

signals:
    void eventPosted(const QString& eventType, const QVariantMap& data);

private:
    struct Subscription {
        SubscriptionId id;
        EventHandler handler;
        Priority minPriority = Priority::Low;
    };

    struct PatternSubscription {
        SubscriptionId id;
        QRegularExpression regex;
        EventHandler handler;
    };

    struct EventStat {
        int postCount = 0;
        int subscriberCount = 0;
    };

    void deliverEvent(const QString& eventType, const QVariantMap& data,
                      Priority priority);

    LoggingEngine* m_logging = nullptr;
    DiagnosticsEngine* m_diagnostics = nullptr;
    bool m_initialized = false;

    mutable QReadWriteLock m_lock;
    int m_nextId = 1;

    QHash<QString, QList<Subscription>> m_subscriptions;
    QList<PatternSubscription> m_patternSubscriptions;
    QHash<QString, QList<EventFilter>> m_filters;

    mutable QHash<QString, EventStat> m_stats;
};

} // namespace gopost::core::engines
