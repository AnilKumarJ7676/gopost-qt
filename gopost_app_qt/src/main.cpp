#include <QGuiApplication>
#include "app.h"
#include "core/logging/app_logger.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Gopost"));
    app.setOrganizationDomain(QStringLiteral("gopost.app"));
    app.setApplicationName(QStringLiteral("Gopost"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    // Initialize logging
    gopost::core::AppLogger::init();
    gopost::core::AppLogger::info(QStringLiteral("Starting Gopost application"));

    // Create and run the app
    gopost::GopostApp gopostApp(&app);
    return gopostApp.run();
}
