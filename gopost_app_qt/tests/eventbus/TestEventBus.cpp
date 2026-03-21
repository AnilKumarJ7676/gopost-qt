#include "TestEventBus.h"

#include "core/events/event_bus_engine_new.h"
#include "core/events/CoreEvents.h"
#include "core/interfaces/IEventBus.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <chrono>
#include <cstdio>

using namespace gopost::events;
using namespace gopost::core::interfaces;

void TestEventBus::immediateDelivery() {
    EventBusEngineNew bus;

    QString receivedType;
    QString receivedName;
    QThread* handlerThread = nullptr;

    bus.subscribeWithMode(QStringLiteral("EngineReady"),
        [&](const QString& type, const QVariantMap& data) {
            receivedType = type;
            receivedName = data[QStringLiteral("name")].toString();
            handlerThread = QThread::currentThread();
        }, DeliveryMode::Sync);

    QVariantMap data;
    data[QStringLiteral("name")] = QStringLiteral("Platform");
    bus.publish(QStringLiteral("EngineReady"), data, DeliveryMode::Sync);

    QCOMPARE(receivedType, QStringLiteral("EngineReady"));
    QCOMPARE(receivedName, QStringLiteral("Platform"));
    QCOMPARE(handlerThread, QThread::currentThread());
}

void TestEventBus::queuedDelivery() {
    EventBusEngineNew bus;

    std::atomic<bool> received{false};
    QThread* handlerThread = nullptr;
    QString receivedPool;

    // Subscribe with QUEUED delivery
    bus.subscribeWithMode(QStringLiteral("MemoryWarning"),
        [&](const QString&, const QVariantMap& data) {
            received.store(true);
            handlerThread = QThread::currentThread();
            receivedPool = data[QStringLiteral("pool")].toString();
        }, DeliveryMode::Queued);

    // Publish from main thread
    QVariantMap data;
    data[QStringLiteral("pool")] = QStringLiteral("frameCache");
    data[QStringLiteral("usage")] = 85.0;
    bus.publish(QStringLiteral("MemoryWarning"), data, DeliveryMode::Queued);

    // Process events to allow queued delivery
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();

    QVERIFY2(received.load(), "Handler should have been called via queued delivery");
    QCOMPARE(receivedPool, QStringLiteral("frameCache"));
}

void TestEventBus::unsubscribeTest() {
    EventBusEngineNew bus;

    int callCount = 0;
    auto id = bus.subscribe(QStringLiteral("TestEvent"),
        [&](const QString&, const QVariantMap&) { ++callCount; });

    bus.publish(QStringLiteral("TestEvent"));
    QCOMPARE(callCount, 1);

    bus.publish(QStringLiteral("TestEvent"));
    QCOMPARE(callCount, 2);

    // Unsubscribe
    bus.unsubscribe(id);

    bus.publish(QStringLiteral("TestEvent"));
    QCOMPARE(callCount, 2); // Should NOT increment
}

void TestEventBus::performance() {
    EventBusEngineNew bus;

    int counter = 0;
    bus.subscribeWithMode(QStringLiteral("PerfEvent"),
        [&](const QString&, const QVariantMap&) { ++counter; },
        DeliveryMode::Sync);

    constexpr int N = 1'000'000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < N; ++i)
        bus.publish(QStringLiteral("PerfEvent"), {}, DeliveryMode::Sync);

    auto end = std::chrono::high_resolution_clock::now();

    QCOMPARE(counter, N);

    auto totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double totalMs = static_cast<double>(totalNs) / 1'000'000.0;
    double perPublishNs = static_cast<double>(totalNs) / N;

    std::printf("\n1M IMMEDIATE publishes: %.1fms (%.0fns/publish)\n\n",
                totalMs, perPublishNs);
    std::fflush(stdout);

    QVERIFY2(perPublishNs < 2000,
             qPrintable(QStringLiteral("Too slow: %1ns/publish").arg(perPublishNs)));
}

QTEST_MAIN(TestEventBus)
#include "TestEventBus.moc"
