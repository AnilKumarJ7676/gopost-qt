#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <memory>

#include "core/di/service_locator.h"

namespace gopost::platform {
class PlatformCapabilityEngine;
} // namespace gopost::platform

namespace gopost::memory { class MemoryEngine; }
namespace gopost::diagnostics { class DiagnosticEngine; class BootInfoProvider; }
namespace gopost::logging { class LoggingEngineNew; }
namespace gopost::config { class ConfigurationEngineNew; }
namespace gopost::events { class EventBusEngineNew; }

namespace gopost::core::engines {
class MemoryManagementEngine;
class LoggingEngine;
class ConfigurationEngine;
class DiagnosticsEngine;
class EventBusEngine;
} // namespace gopost::core::engines

namespace gopost {

class GopostApp : public QObject {
    Q_OBJECT
public:
    explicit GopostApp(QGuiApplication* app, QObject* parent = nullptr);
    ~GopostApp() override;

    int run();

private:
    void initializeCoreEngines();
    void initializeEngines();
    void setupQml();

    QGuiApplication* m_app = nullptr;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    std::unique_ptr<core::ServiceLocator> m_serviceLocator;

    // Core engines (owned)
    std::unique_ptr<platform::PlatformCapabilityEngine> m_platformEngine;
    std::unique_ptr<core::engines::MemoryManagementEngine> m_memoryEngine;
    std::unique_ptr<core::engines::LoggingEngine> m_loggingEngine;
    std::unique_ptr<core::engines::ConfigurationEngine> m_configEngine;
    std::unique_ptr<core::engines::DiagnosticsEngine> m_diagnosticsEngine;
    std::unique_ptr<core::engines::EventBusEngine> m_eventBusEngine;

    // New engines (interface-based)
    std::unique_ptr<memory::MemoryEngine> m_memoryEngineNew;
    std::unique_ptr<diagnostics::DiagnosticEngine> m_diagnosticEngineNew;
    std::unique_ptr<logging::LoggingEngineNew> m_loggingEngineNew;
    std::unique_ptr<config::ConfigurationEngineNew> m_configEngineNew;
    std::unique_ptr<events::EventBusEngineNew> m_eventBusNew;
    std::unique_ptr<diagnostics::BootInfoProvider> m_bootInfo;
};

} // namespace gopost
