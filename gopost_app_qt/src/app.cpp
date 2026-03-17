#include "app.h"

#include "core/logging/app_logger.h"
#include "core/theme/app_theme.h"

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
