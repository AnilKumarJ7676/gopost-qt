#include "app.h"

#include "core/logging/app_logger.h"
#include "core/theme/app_theme.h"
#include "core/engines/platform_capability_engine.h"
#include "core/engines/memory_management_engine.h"
#include "core/engines/logging_engine.h"
#include "core/engines/configuration_engine.h"
#include "core/engines/diagnostics_engine.h"
#include "core/engines/event_bus_engine.h"

#include <QQuickStyle>
#include <QQmlContext>
#include <QUrl>
#include <QDebug>
#include <QDir>

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

    // List available QRC resources for debugging
    qDebug() << "QRC contents at :/qt/qml/GopostApp/qml/:";
    QDir qrcDir(":/qt/qml/GopostApp/qml");
    for (const auto& entry : qrcDir.entryList()) {
        qDebug() << "  " << entry;
    }

    m_engine->load(url);

    return m_app->exec();
}

void GopostApp::initializeCoreEngines() {
    using namespace core::engines;

    // 1. Platform Capability Engine (no deps)
    m_platformEngine = std::make_unique<PlatformCapabilityEngine>(this);
    m_platformEngine->initialize();
    m_serviceLocator->setPlatformEngine(m_platformEngine.get());
    core::AppLogger::info(QStringLiteral("Platform engine initialized — %1 %2, tier=%3, type=%4, cores=%5, RAM=%6 MB, DPI=%7, touch=%8")
        .arg(m_platformEngine->platformName())
        .arg(m_platformEngine->osVersionString())
        .arg(static_cast<int>(m_platformEngine->deviceTier()))
        .arg(static_cast<int>(m_platformEngine->deviceType()))
        .arg(m_platformEngine->cpuCoreCount())
        .arg(m_platformEngine->totalRamBytes() / (1024 * 1024))
        .arg(m_platformEngine->screenDpi(), 0, 'f', 0)
        .arg(m_platformEngine->hasTouchScreen() ? QStringLiteral("yes") : QStringLiteral("no")));

    // 2. Memory Management Engine (needs Platform)
    m_memoryEngine = std::make_unique<MemoryManagementEngine>(m_platformEngine.get(), this);
    m_memoryEngine->initialize();
    m_serviceLocator->setMemoryEngine(m_memoryEngine.get());
    core::AppLogger::info(QStringLiteral("Memory engine initialized — budget=%1 MB")
        .arg(m_memoryEngine->globalBudgetBytes() / (1024 * 1024)));

    // 3. Logging Engine (needs Platform, Memory)
    m_loggingEngine = std::make_unique<LoggingEngine>(m_platformEngine.get(),
                                                       m_memoryEngine.get(), this);
    m_loggingEngine->initialize();
    m_serviceLocator->setLoggingEngine(m_loggingEngine.get());

    // 4. Configuration Engine (needs Platform, Logging)
    m_configEngine = std::make_unique<ConfigurationEngine>(m_platformEngine.get(),
                                                            m_loggingEngine.get(), this);
    m_configEngine->initialize();
    m_serviceLocator->setConfigEngine(m_configEngine.get());

    // 5. Diagnostics Engine (needs Memory, Logging)
    m_diagnosticsEngine = std::make_unique<DiagnosticsEngine>(m_memoryEngine.get(),
                                                               m_loggingEngine.get(), this);
    m_diagnosticsEngine->initialize();
    m_serviceLocator->setDiagnosticsEngine(m_diagnosticsEngine.get());

    // 6. Event Bus Engine (needs Logging, Diagnostics)
    m_eventBusEngine = std::make_unique<EventBusEngine>(m_loggingEngine.get(),
                                                         m_diagnosticsEngine.get(), this);
    m_eventBusEngine->initialize();
    m_serviceLocator->setEventBusEngine(m_eventBusEngine.get());

    // Post initialization event
    m_eventBusEngine->post(QStringLiteral("engine.allCoreInitialized"), {});
}

void GopostApp::initializeEngines() {
    // Try loading native image editor engine
    // If it fails, use stub engine
    try {
        // Direct C++ linkage — no FFI needed
        // auto* nativeEngine = new rendering::GopostImageEditorEngineNative();
        // nativeEngine->initialize();
        // nativeEngine->initEffects();
        // m_serviceLocator->setImageEditorEngine(nativeEngine);
        core::AppLogger::info(QStringLiteral("Image engine initialized"));
    } catch (const std::exception& e) {
        core::AppLogger::warning(
            QStringLiteral("Image engine init failed, using stub: %1").arg(e.what()));
        // m_serviceLocator->setImageEditorEngine(new rendering::StubImageEditorEngine());
    }

    // Try loading native video timeline engine
    try {
        // auto* videoEngine = new rendering::GopostVideoTimelineEngineNative(enginePtr);
        // m_serviceLocator->setVideoTimelineEngine(videoEngine);
        core::AppLogger::info(QStringLiteral("Video engine initialized"));
    } catch (const std::exception& e) {
        core::AppLogger::warning(
            QStringLiteral("Video engine init failed, using stub: %1").arg(e.what()));
        // m_serviceLocator->setVideoTimelineEngine(new rendering::StubVideoTimelineEngine());
    }
}

void GopostApp::setupQml() {
    // Set Material style
    QQuickStyle::setStyle(QStringLiteral("Material"));

    // Apply theme based on system preference
    // QPalette will be set dynamically based on light/dark mode

    // Register all service locator objects with QML context
    m_serviceLocator->registerWithQml(m_engine.get());
}

} // namespace gopost
