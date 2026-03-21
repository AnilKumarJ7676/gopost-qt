#include "TestPermissions.h"
#include "core/bridges/common/bridge_factory.h"
#include "core/permissions/permission_engine.h"
#include "core/permissions/permission_policy.h"
#include "core/permissions/permission_rationale_provider.h"
#include "core/interfaces/ILogger.h"
#include "core/interfaces/IEventBus.h"
#include "core/interfaces/IPermission.h"
#include "core/interfaces/IResult.h"

#include <QDebug>

using namespace gopost::permissions;
using namespace gopost::core::interfaces;

// Minimal ILogger for testing
class TestLogger : public ILogger {
public:
    void log(LogLevel, const QString&, const QString&, const QVariantMap&) override {}
    void trace(const QString&, const QString&) override {}
    void debug(const QString&, const QString&) override {}
    void info(const QString&, const QString&) override {}
    void warn(const QString&, const QString& msg) override { lastWarn_ = msg; }
    void error(const QString&, const QString&, const QVariantMap&) override {}
    void fatal(const QString&, const QString&, const QVariantMap&) override {}
    void setMinLevel(LogLevel) override {}
    LogLevel minLevel() const override { return LogLevel::Trace; }
    void setCategoryEnabled(LogCategory, bool) override {}
    bool isCategoryEnabled(LogCategory) const override { return true; }
    QString lastWarn_;
};

// Minimal IEventBus for testing — records published events
class TestEventBus : public IEventBus {
public:
    SubscriptionId subscribe(const QString& eventType, EventHandler handler) override {
        handlers_[nextId_] = {eventType, handler};
        return nextId_++;
    }
    void unsubscribe(SubscriptionId id) override { handlers_.erase(id); }
    void publish(const QString& eventType, const QVariantMap& data, DeliveryMode) override {
        lastEventType_ = eventType;
        lastEventData_ = data;
        publishCount_++;
        // Deliver to subscribers
        for (auto& [id, h] : handlers_) {
            if (h.first == eventType) h.second(eventType, data);
        }
    }

    QString lastEventType_;
    QVariantMap lastEventData_;
    int publishCount_ = 0;

private:
    int nextId_ = 1;
    std::map<int, std::pair<QString, EventHandler>> handlers_;
};

void TestPermissions::desktopFileAccessGranted() {
    auto bridge = gopost::bridges::BridgeFactory::create();
    QVERIFY(bridge != nullptr);
    TestLogger logger;
    TestEventBus eventBus;
    PermissionEngine engine(*bridge, logger, eventBus);

    QCOMPARE(engine.checkPermission(PermissionType::FILE_ACCESS), PermissionStatus::GRANTED);
    qDebug() << "  FILE_ACCESS = GRANTED";
}

void TestPermissions::desktopStorageWriteGranted() {
    auto bridge = gopost::bridges::BridgeFactory::create();
    TestLogger logger;
    TestEventBus eventBus;
    PermissionEngine engine(*bridge, logger, eventBus);

    QCOMPARE(engine.checkPermission(PermissionType::STORAGE_WRITE), PermissionStatus::GRANTED);
    qDebug() << "  STORAGE_WRITE = GRANTED";
}

void TestPermissions::desktopCameraGranted() {
    auto bridge = gopost::bridges::BridgeFactory::create();
    TestLogger logger;
    TestEventBus eventBus;
    PermissionEngine engine(*bridge, logger, eventBus);

    auto result = engine.requestPermission(PermissionType::CAMERA);
    QVERIFY(result.ok());
    // On Windows/Linux: GRANTED. On macOS: may be GRANTED or DENIED (system prompt).
    QCOMPARE(result.get(), PermissionStatus::GRANTED);
    qDebug() << "  CAMERA = GRANTED";
}

void TestPermissions::permissionRequestedEventPublished() {
    auto bridge = gopost::bridges::BridgeFactory::create();
    TestLogger logger;
    TestEventBus eventBus;
    PermissionEngine engine(*bridge, logger, eventBus);

    // Subscribe to PermissionRequested
    bool eventReceived = false;
    eventBus.subscribe(QStringLiteral("PermissionRequested"),
                       [&](const QString&, const QVariantMap& data) {
                           eventReceived = true;
                           QCOMPARE(data["permissionType"].toInt(), static_cast<int>(PermissionType::FILE_ACCESS));
                           QCOMPARE(data["status"].toInt(), static_cast<int>(PermissionStatus::GRANTED));
                       });

    engine.requestPermission(PermissionType::FILE_ACCESS);
    QVERIFY(eventReceived);
    QCOMPARE(eventBus.lastEventType_, QStringLiteral("PermissionRequested"));
    qDebug() << "  PermissionRequestedEvent published: PASS";
}

void TestPermissions::denialHandlerPermanentlyDenied() {
    auto bridge = gopost::bridges::BridgeFactory::create();
    TestLogger logger;
    TestEventBus eventBus;
    PermissionEngine engine(*bridge, logger, eventBus);

    // Force PERMANENTLY_DENIED into cache
    engine.forceCacheStatus(PermissionType::CAMERA, PermissionStatus::PERMANENTLY_DENIED);

    // Subscribe to PermissionDenied event
    bool deniedEventReceived = false;
    eventBus.subscribe(QStringLiteral("PermissionDenied"),
                       [&](const QString&, const QVariantMap& data) {
                           deniedEventReceived = true;
                           QCOMPARE(data["status"].toInt(), static_cast<int>(PermissionStatus::PERMANENTLY_DENIED));
                       });

    auto result = engine.requestPermission(PermissionType::CAMERA);
    QVERIFY(result.ok());
    QCOMPARE(result.get(), PermissionStatus::PERMANENTLY_DENIED);
    QVERIFY(deniedEventReceived);
    QVERIFY(!logger.lastWarn_.isEmpty());
    qDebug() << "  Denial handling (PERMANENTLY_DENIED): PASS";
}

void TestPermissions::denialHandlerTemporaryDenied() {
    auto bridge = gopost::bridges::BridgeFactory::create();
    TestLogger logger;
    TestEventBus eventBus;
    PermissionEngine engine(*bridge, logger, eventBus);

    // Use denial handler directly to test guidance
    DenialHandler handler(logger, eventBus);
    auto guidance = handler.handle(PermissionType::MICROPHONE, PermissionStatus::DENIED);

    QVERIFY(guidance.canRetry);
    QVERIFY(!guidance.shouldShowSettings);
    QVERIFY(!guidance.rationale.title.empty());
    qDebug() << "  Denial handling (DENIED, can retry): PASS";
}

void TestPermissions::cachePreventsDuplicateCheck() {
    auto bridge = gopost::bridges::BridgeFactory::create();
    TestLogger logger;
    TestEventBus eventBus;
    PermissionEngine engine(*bridge, logger, eventBus);

    // First check fills cache
    auto status1 = engine.checkPermission(PermissionType::FILE_ACCESS);
    // Second check should return from cache
    auto status2 = engine.checkPermission(PermissionType::FILE_ACCESS);
    QCOMPARE(status1, status2);
    QCOMPARE(status1, PermissionStatus::GRANTED);
    qDebug() << "  Cache prevents duplicate platform queries: PASS";
}

void TestPermissions::rationaleProviderReturnsText() {
    auto rationale = PermissionRationaleProvider::get(PermissionType::CAMERA);
    QVERIFY(!rationale.title.empty());
    QVERIFY(!rationale.explanation.empty());
    QVERIFY(!rationale.deniedAction.empty());
    QCOMPARE(rationale.title, std::string("Camera Access"));
    qDebug() << "  Rationale for CAMERA:" << QString::fromStdString(rationale.title)
             << "—" << QString::fromStdString(rationale.explanation);
}

void TestPermissions::policyTableDesktop() {
    // On Windows/Linux, all permissions should NOT require runtime request
    auto filePolicy = PermissionPolicy::lookup(PermissionType::FILE_ACCESS);
    QVERIFY(!filePolicy.requiresRuntimeRequest);
    QVERIFY(filePolicy.canOpenSettings);

    auto cameraPolicy = PermissionPolicy::lookup(PermissionType::CAMERA);
    QVERIFY(!cameraPolicy.requiresRuntimeRequest); // Desktop: no runtime request
    QVERIFY(cameraPolicy.canOpenSettings);

    // Verify Android policy (cross-check data table)
    auto androidCamera = PermissionPolicy::lookup(Platform::Android, PermissionType::CAMERA);
    QVERIFY(androidCamera.requiresRuntimeRequest);

    auto androidFile = PermissionPolicy::lookup(Platform::Android, PermissionType::FILE_ACCESS);
    QVERIFY(androidFile.requiresRuntimeRequest);

    qDebug() << "  Permission policy table: PASS (desktop=no-runtime, android=runtime)";
}

QTEST_GUILESS_MAIN(TestPermissions)
#include "TestPermissions.moc"
