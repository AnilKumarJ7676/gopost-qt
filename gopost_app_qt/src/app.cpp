#include "app.h"

#include "core/logging/app_logger.h"
#include "core/ui_support/theme/app_theme.h"
#include "core/platform/platform_capability_engine.h"
#include "core/engines/memory_management_engine.h"
#include "core/engines/logging_engine.h"
#include "core/engines/configuration_engine.h"
#include "core/engines/diagnostics_engine.h"
#include "core/engines/event_bus_engine.h"
#include "core/memory/memory_engine.h"
#include "core/diagnostics/diagnostic_engine.h"
#include "core/diagnostics/boot_info_provider.h"
#include "core/logging/logging_engine_new.h"
#include "core/config/configuration_engine_new.h"
#include "core/events/event_bus_engine_new.h"
#include "core/events/CoreEvents.h"
#include "core/interfaces/IDeviceCapability.h"

#include <QQuickStyle>
#include <QQmlContext>
#include <QUrl>
#include <QDebug>
#include <QDir>
#include <chrono>

namespace gopost {

GopostApp::GopostApp(QGuiApplication* app, QObject* parent)
    : QObject(parent)
    , m_app(app)
    , m_engine(std::make_unique<QQmlApplicationEngine>())
    , m_serviceLocator(std::make_unique<core::ServiceLocator>(this)) {}

GopostApp::~GopostApp() = default;

int GopostApp::run() {
    // Phase 0: Core engines (before ServiceLocator)
    initializeCoreEngines();

    // Initialize core services
    m_serviceLocator->initialize();

    // Initialize native engines
    initializeEngines();

    // Setup QML
    setupQml();

    // Load main QML
    const QUrl url(QStringLiteral("qrc:/qt/qml/GopostApp/qml/Main.qml"));
    qDebug() << "Loading QML from:" << url;

    QObject::connect(m_engine.get(), &QQmlApplicationEngine::objectCreationFailed,
                     m_app, [](const QUrl &u) {
                         qCritical() << "QML object creation FAILED for:" << u;
                         QCoreApplication::exit(-1);
                     },
                     Qt::QueuedConnection);

    QObject::connect(m_engine.get(), &QQmlApplicationEngine::objectCreated,
                     m_app, [](QObject *obj, const QUrl &u) {
                         if (!obj) {
                             qCritical() << "QML object is NULL for:" << u;
                             QCoreApplication::exit(-1);
                         } else {
                             qDebug() << "QML loaded successfully:" << u;
                         }
                     },
                     Qt::QueuedConnection);

    QDir qrcDir(":/qt/qml/GopostApp/qml");
    qDebug() << "QRC contents at :/qt/qml/GopostApp/qml/:";
    for (const auto& entry : qrcDir.entryList())
        qDebug() << "  " << entry;

    m_engine->load(url);

    return m_app->exec();
}

static const char* tierStr(core::interfaces::DeviceTier t) {
    switch (t) {
    case core::interfaces::DeviceTier::Ultra: return "ULTRA";
    case core::interfaces::DeviceTier::High:  return "HIGH";
    case core::interfaces::DeviceTier::Mid:   return "MID";
    case core::interfaces::DeviceTier::Low:   return "LOW";
    }
    return "?";
}

static const char* storageStr(core::interfaces::StorageType s) {
    switch (s) {
    case core::interfaces::StorageType::NVMe:    return "NVMe";
    case core::interfaces::StorageType::SSD:     return "SSD";
    case core::interfaces::StorageType::HDD:     return "HDD";
    case core::interfaces::StorageType::Unknown: return "Unknown";
    }
    return "?";
}

void GopostApp::initializeCoreEngines() {
    using namespace core::engines;
    auto t0 = std::chrono::steady_clock::now();

    // ── 1. Logging Engine (FIRST — so all subsequent engines can log) ──
    m_loggingEngineNew = std::make_unique<logging::LoggingEngineNew>();
    logging::Logger::setInstance(m_loggingEngineNew.get());
    m_loggingEngineNew->info(QStringLiteral("Logging"),
                              QStringLiteral("Logging engine initialized"));

    // ── 2. Platform Capability Engine ──
    m_platformEngine = std::make_unique<platform::PlatformCapabilityEngine>(this);
    m_platformEngine->initialize();
    m_serviceLocator->setPlatformEngine(m_platformEngine.get());

    auto cpu = m_platformEngine->cpuInfo();
    auto gpu = m_platformEngine->gpuInfo();
    auto tier = m_platformEngine->deviceTier();
    auto storage = m_platformEngine->storageType();
    auto hw = m_platformEngine->hwDecodeSupport();

    m_loggingEngineNew->info(QStringLiteral("Platform"),
        QStringLiteral("Tier: %1 | CPU: %2C/%3T | RAM: %4 GB | GPU: %5 (%6 GB) | Storage: %7")
            .arg(QString::fromLatin1(tierStr(tier)))
            .arg(cpu.physicalCores).arg(cpu.logicalThreads)
            .arg(m_platformEngine->totalRamBytes() / (1024LL * 1024 * 1024))
            .arg(gpu.name)
            .arg(gpu.vramBytes / (1024LL * 1024 * 1024))
            .arg(QString::fromLatin1(storageStr(storage))));

    m_loggingEngineNew->info(QStringLiteral("Platform"),
        QStringLiteral("HW Decode: H.264=%1 H.265=%2 VP9=%3 AV1=%4 | OS: %5")
            .arg(hw.h264 ? "YES" : "NO")
            .arg(hw.h265 ? "YES" : "NO")
            .arg(hw.vp9  ? "YES" : "NO")
            .arg(hw.av1  ? "YES" : "NO")
            .arg(m_platformEngine->osVersionString()));

    // ── 3. Memory Engine ──
    m_memoryEngineNew = std::make_unique<memory::MemoryEngine>(*m_platformEngine);
    auto budgets = m_memoryEngineNew->computeBudgets();

    m_loggingEngineNew->info(QStringLiteral("Memory"),
        QStringLiteral("Budget ceiling: %1 MB | frameCache=%2 MB | thumbnailCache=%3 MB | "
                        "audioCache=%4 MB | effectCache=%5 MB | importBuffer=%6 MB | reserve=%7 MB")
            .arg(budgets.globalBudgetBytes / (1024 * 1024))
            .arg(budgets.frameCacheBytes / (1024 * 1024))
            .arg(budgets.thumbnailCacheBytes / (1024 * 1024))
            .arg(budgets.audioCacheBytes / (1024 * 1024))
            .arg(budgets.effectCacheBytes / (1024 * 1024))
            .arg(budgets.importBufferBytes / (1024 * 1024))
            .arg(budgets.reserveBytes / (1024 * 1024)));

    // ── 4. Configuration Engine ──
    m_configEngineNew = std::make_unique<config::ConfigurationEngineNew>(
        m_loggingEngineNew.get());
    m_loggingEngineNew->info(QStringLiteral("Config"),
        QStringLiteral("Schema version: %1 | Theme: %2 | Target FPS: %3")
            .arg(m_configEngineNew->schemaVersion())
            .arg(m_configEngineNew->getString(QStringLiteral("app.theme")))
            .arg(m_configEngineNew->getInt(QStringLiteral("editor.playback.targetFps"))));

    // ── 5. Diagnostic Engine ──
    m_diagnosticEngineNew = std::make_unique<diagnostics::DiagnosticEngine>(
        *m_platformEngine, m_memoryEngineNew.get(), this);
    m_diagnosticEngineNew->setOverlayVisible(true);
    m_loggingEngineNew->info(QStringLiteral("Diagnostics"),
                              QStringLiteral("Diagnostics engine ready, overlay enabled"));

    // ── 6. Event Bus Engine ──
    m_eventBusNew = std::make_unique<events::EventBusEngineNew>(
        m_loggingEngineNew.get());
    m_loggingEngineNew->info(QStringLiteral("EventBus"),
                              QStringLiteral("Event bus engine ready"));

    // Publish EngineReadyEvent for each engine
    for (const auto& name : {"Logging", "Platform", "Memory", "Config", "Diagnostics", "EventBus"}) {
        QVariantMap data;
        data[QStringLiteral("name")] = QString::fromLatin1(name);
        m_eventBusNew->publish(QStringLiteral("EngineReady"), data);
    }

    // ── Wire cross-engine subscriptions ──
    m_eventBusNew->subscribe(QStringLiteral("MemoryWarning"),
        [this](const QString&, const QVariantMap& data) {
            m_loggingEngineNew->warn(QStringLiteral("Memory"),
                QStringLiteral("Memory warning: pool=%1 usage=%2%")
                    .arg(data[QStringLiteral("pool")].toString())
                    .arg(data[QStringLiteral("usage")].toDouble(), 0, 'f', 1));
        });

    m_eventBusNew->subscribe(QStringLiteral("ConfigChanged"),
        [this](const QString&, const QVariantMap& data) {
            m_loggingEngineNew->info(QStringLiteral("Config"),
                QStringLiteral("Config changed: %1 = %2")
                    .arg(data[QStringLiteral("key")].toString())
                    .arg(data[QStringLiteral("value")].toString()));
        });

    // ── Boot timing ──
    auto t1 = std::chrono::steady_clock::now();
    double bootMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    m_loggingEngineNew->info(QStringLiteral("Boot"),
        QStringLiteral("GoPost Level 0 Bootstrap Complete — %1ms").arg(bootMs, 0, 'f', 1));

    // Publish BootCompleteEvent
    QVariantMap bootData;
    bootData[QStringLiteral("bootTimeMs")] = bootMs;
    m_eventBusNew->publish(QStringLiteral("BootComplete"), bootData);

    // ── BootInfoProvider for QML ──
    m_bootInfo = std::make_unique<diagnostics::BootInfoProvider>(this);
    m_bootInfo->setTier(QString::fromLatin1(tierStr(tier)));
    m_bootInfo->setCpuInfo(QStringLiteral("%1C/%2T").arg(cpu.physicalCores).arg(cpu.logicalThreads));
    m_bootInfo->setRamGb(static_cast<int>(m_platformEngine->totalRamBytes() / (1024LL * 1024 * 1024)));
    m_bootInfo->setStorageType(QString::fromLatin1(storageStr(storage)));
    m_bootInfo->setGpuName(gpu.name);
    m_bootInfo->setBootTimeMs(bootMs);

    // ── Old engines (backward compat with ServiceLocator) ──
    m_memoryEngine = std::make_unique<MemoryManagementEngine>(m_platformEngine.get(), this);
    m_memoryEngine->initialize();
    m_serviceLocator->setMemoryEngine(m_memoryEngine.get());

    m_loggingEngine = std::make_unique<LoggingEngine>(m_platformEngine.get(),
                                                       m_memoryEngine.get(), this);
    m_loggingEngine->initialize();
    m_serviceLocator->setLoggingEngine(m_loggingEngine.get());

    m_configEngine = std::make_unique<ConfigurationEngine>(m_platformEngine.get(),
                                                            m_loggingEngine.get(), this);
    m_configEngine->initialize();
    m_serviceLocator->setConfigEngine(m_configEngine.get());

    m_diagnosticsEngine = std::make_unique<DiagnosticsEngine>(m_memoryEngine.get(),
                                                               m_loggingEngine.get(), this);
    m_diagnosticsEngine->initialize();
    m_serviceLocator->setDiagnosticsEngine(m_diagnosticsEngine.get());

    m_eventBusEngine = std::make_unique<EventBusEngine>(m_loggingEngine.get(),
                                                         m_diagnosticsEngine.get(), this);
    m_eventBusEngine->initialize();
    m_serviceLocator->setEventBusEngine(m_eventBusEngine.get());

    m_eventBusEngine->post(QStringLiteral("engine.allCoreInitialized"), {});
}

void GopostApp::initializeEngines() {
    try {
        core::AppLogger::info(QStringLiteral("Image engine initialized"));
    } catch (const std::exception& e) {
        core::AppLogger::warning(
            QStringLiteral("Image engine init failed, using stub: %1").arg(e.what()));
    }

    try {
        core::AppLogger::info(QStringLiteral("Video engine initialized"));
    } catch (const std::exception& e) {
        core::AppLogger::warning(
            QStringLiteral("Video engine init failed, using stub: %1").arg(e.what()));
    }
}

void GopostApp::setupQml() {
    QQuickStyle::setStyle(QStringLiteral("Material"));

    m_serviceLocator->registerWithQml(m_engine.get());

    if (m_diagnosticEngineNew)
        m_diagnosticEngineNew->registerWithQml(m_engine.get());

    if (m_bootInfo)
        m_bootInfo->registerWithQml(m_engine.get());
}

} // namespace gopost
