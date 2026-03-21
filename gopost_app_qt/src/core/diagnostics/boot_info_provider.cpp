#include "core/diagnostics/boot_info_provider.h"

#include <QQmlContext>

namespace gopost::diagnostics {

BootInfoProvider::BootInfoProvider(QObject* parent)
    : QObject(parent) {}

void BootInfoProvider::registerWithQml(QQmlApplicationEngine* engine) {
    if (engine)
        engine->rootContext()->setContextProperty(
            QStringLiteral("bootInfo"), this);
}

} // namespace gopost::diagnostics
