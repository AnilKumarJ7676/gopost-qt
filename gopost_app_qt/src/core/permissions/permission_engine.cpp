#include "permission_engine.h"
#include "permission_policy.h"
#include "permission_rationale_provider.h"

namespace gopost::permissions {

PermissionEngine::PermissionEngine(IPlatformBridge& bridge, ILogger& logger, IEventBus& eventBus)
    : logger_(logger)
    , eventBus_(eventBus)
    , adapter_(bridge)
    , denialHandler_(logger, eventBus) {}

PermissionStatus PermissionEngine::checkPermission(PermissionType type) {
    // 1. Check policy — if no runtime request needed, return GRANTED
    auto policy = PermissionPolicy::lookup(type);
    if (!policy.requiresRuntimeRequest) {
        cache_.set(type, PermissionStatus::GRANTED);
        return PermissionStatus::GRANTED;
    }

    // 2. Check cache — return cached value if available
    if (cache_.has(type)) {
        return cache_.get(type);
    }

    // 3. Delegate to platform adapter for real check
    auto status = adapter_.check(type);
    cache_.set(type, status);
    return status;
}

Result<PermissionStatus> PermissionEngine::requestPermission(PermissionType type) {
    // 1. Check cache first — permanently denied overrides everything
    if (cache_.has(type) && cache_.get(type) == PermissionStatus::PERMANENTLY_DENIED) {
        denialHandler_.handle(type, PermissionStatus::PERMANENTLY_DENIED);
        return Result<PermissionStatus>::success(PermissionStatus::PERMANENTLY_DENIED);
    }

    // 2. Check cache — if already GRANTED, return immediately
    if (cache_.has(type) && cache_.get(type) == PermissionStatus::GRANTED) {
        QVariantMap data;
        data[QStringLiteral("permissionType")] = static_cast<int>(type);
        data[QStringLiteral("status")] = static_cast<int>(PermissionStatus::GRANTED);
        eventBus_.publish(QStringLiteral("PermissionRequested"), data);

        return Result<PermissionStatus>::success(PermissionStatus::GRANTED);
    }

    // 3. Check policy — if no runtime request needed, return GRANTED immediately
    auto policy = PermissionPolicy::lookup(type);
    if (!policy.requiresRuntimeRequest) {
        cache_.set(type, PermissionStatus::GRANTED);

        QVariantMap data;
        data[QStringLiteral("permissionType")] = static_cast<int>(type);
        data[QStringLiteral("status")] = static_cast<int>(PermissionStatus::GRANTED);
        eventBus_.publish(QStringLiteral("PermissionRequested"), data);

        return Result<PermissionStatus>::success(PermissionStatus::GRANTED);
    }

    // 4. Log the rationale
    auto rationale = PermissionRationaleProvider::get(type);
    logger_.info(QStringLiteral("Permissions"),
                 QStringLiteral("Requesting permission: %1 — %2")
                     .arg(QString::fromStdString(rationale.title),
                          QString::fromStdString(rationale.explanation)));

    // 5. Delegate to platform adapter
    auto result = adapter_.request(type);
    if (!result.ok())
        return result;

    auto status = result.get();

    // 6. Update cache
    cache_.set(type, status);

    // 7. Publish event
    QVariantMap data;
    data[QStringLiteral("permissionType")] = static_cast<int>(type);
    data[QStringLiteral("status")] = static_cast<int>(status);
    eventBus_.publish(QStringLiteral("PermissionRequested"), data);

    // 8. If denied → denial handler
    if (status == PermissionStatus::DENIED || status == PermissionStatus::PERMANENTLY_DENIED) {
        denialHandler_.handle(type, status);
    }

    return Result<PermissionStatus>::success(status);
}

Result<void> PermissionEngine::openAppSettings() {
    return adapter_.openSettings();
}

void PermissionEngine::forceCacheStatus(PermissionType type, PermissionStatus status) {
    cache_.set(type, status);
}

} // namespace gopost::permissions
