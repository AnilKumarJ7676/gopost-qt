#include "denial_handler.h"

namespace gopost::permissions {

DenialHandler::DenialHandler(ILogger& logger, IEventBus& eventBus)
    : logger_(logger), eventBus_(eventBus) {}

DenialGuidance DenialHandler::handle(PermissionType type, PermissionStatus status) {
    auto rationale = PermissionRationaleProvider::get(type);

    int typeInt = static_cast<int>(type);
    int statusInt = static_cast<int>(status);

    logger_.warn(QStringLiteral("Permissions"),
                 QStringLiteral("Permission denied: type=%1 status=%2")
                     .arg(typeInt).arg(statusInt));

    // Publish PermissionDeniedEvent
    QVariantMap data;
    data[QStringLiteral("permissionType")] = typeInt;
    data[QStringLiteral("status")] = statusInt;
    data[QStringLiteral("rationale")] = QString::fromStdString(rationale.explanation);
    eventBus_.publish(QStringLiteral("PermissionDenied"), data);

    DenialGuidance guidance;
    guidance.rationale = rationale;

    if (status == PermissionStatus::PERMANENTLY_DENIED) {
        guidance.shouldShowSettings = true;
        guidance.canRetry = false;
    } else {
        guidance.shouldShowSettings = false;
        guidance.canRetry = true;
    }

    return guidance;
}

} // namespace gopost::permissions
