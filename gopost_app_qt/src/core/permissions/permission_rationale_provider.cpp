#include "permission_rationale_provider.h"

namespace gopost::permissions {

PermissionRationale PermissionRationaleProvider::get(PermissionType type) {
    switch (type) {
    case PermissionType::CAMERA:
        return {
            "Camera Access",
            "GoPost needs access to your camera to record video clips for editing.",
            "Enable in Settings"
        };
    case PermissionType::MICROPHONE:
        return {
            "Microphone Access",
            "GoPost needs access to your microphone to record audio for your projects.",
            "Enable in Settings"
        };
    case PermissionType::PHOTO_LIBRARY:
        return {
            "Photo Library Access",
            "GoPost needs access to import your photos and videos for editing.",
            "Enable in Settings"
        };
    case PermissionType::FILE_ACCESS:
        return {
            "File Access",
            "GoPost needs access to read media files from your device storage.",
            "Enable in Settings"
        };
    case PermissionType::STORAGE_WRITE:
        return {
            "Storage Write Access",
            "GoPost needs write access to save exported projects to your device.",
            "Enable in Settings"
        };
    }
    return {"Unknown Permission", "This permission is required for GoPost to function.", "Enable in Settings"};
}

} // namespace gopost::permissions
