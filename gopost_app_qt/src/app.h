#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <memory>

#include "core/di/service_locator.h"

namespace gopost::core::engines {
class PlatformCapabilityEngine;
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
    std::unique_ptr<core::engines::PlatformCapabilityEngine> m_platformEngine;
    std::unique_ptr<core::engines::MemoryManagementEngine> m_memoryEngine;
    std::unique_ptr<core::engines::LoggingEngine> m_loggingEngine;
    std::unique_ptr<core::engines::ConfigurationEngine> m_configEngine;
    std::unique_ptr<core::engines::DiagnosticsEngine> m_diagnosticsEngine;
    std::unique_ptr<core::engines::EventBusEngine> m_eventBusEngine;
};

} // namespace gopost
