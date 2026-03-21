#include "platform_permission_adapter.h"
#include "permission_policy.h"

namespace gopost::permissions {

PlatformPermissionAdapter::PlatformPermissionAdapter(IPlatformBridge& bridge)
    : bridge_(bridge) {}

PermissionStatus PlatformPermissionAdapter::check(PermissionType type) {
    auto policy = PermissionPolicy::lookup(type);

    // Desktop (Windows/Linux): all permissions implicitly granted
    // macOS: camera/mic might need TCC prompt, but check returns granted if never denied
    if (!policy.requiresRuntimeRequest)
        return PermissionStatus::GRANTED;

    // Mobile platforms would query via bridge JNI/ObjC calls here
    // For stubs, return NOT_DETERMINED (mobile) since runtime check isn't implemented yet
    return PermissionStatus::NOT_DETERMINED;
}

Result<PermissionStatus> PlatformPermissionAdapter::request(PermissionType type) {
    auto policy = PermissionPolicy::lookup(type);

    // Desktop: no runtime request needed → immediately granted
    if (!policy.requiresRuntimeRequest)
        return Result<PermissionStatus>::success(PermissionStatus::GRANTED);

    // Mobile platforms would trigger the system permission dialog via bridge
    // Android: ActivityCompat.requestPermissions via JNI
    // iOS: PHPhotoLibrary.requestAuthorization / AVCaptureDevice.requestAccess via ObjC
    return Result<PermissionStatus>::success(PermissionStatus::NOT_DETERMINED);
}

Result<void> PlatformPermissionAdapter::openSettings() {
    // Windows: open Settings app to App permissions page
#if defined(Q_OS_WIN) || defined(_WIN32)
    // ShellExecuteW to ms-settings:privacy — lightweight, no extra dependencies
    return Result<void>::failure("Not implemented: openSettings on this platform");
#elif defined(Q_OS_MACOS) || defined(__APPLE__)
    // NSWorkspace open URL: x-apple.systempreferences:com.apple.preference.security?Privacy
    return Result<void>::failure("Not implemented: openSettings on this platform");
#elif defined(Q_OS_ANDROID) || defined(__ANDROID__)
    // Intent ACTION_APPLICATION_DETAILS_SETTINGS with package URI
    return Result<void>::failure("Not implemented: openSettings on this platform");
#elif defined(Q_OS_IOS)
    // UIApplication.open(URL("App-Prefs:root=Privacy"))
    return Result<void>::failure("Not implemented: openSettings on this platform");
#else
    // Linux: xdg-open system settings or gnome-control-center
    return Result<void>::failure("Not implemented: openSettings on this platform");
#endif
}

} // namespace gopost::permissions
