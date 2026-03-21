#pragma once

#include "core/interfaces/IPermission.h"
#include <string>

namespace gopost::permissions {

using core::interfaces::PermissionType;

enum class Platform { Android, iOS, MacOS, Windows, Linux };

struct PermissionPolicyEntry {
    bool requiresRuntimeRequest = false;
    bool canOpenSettings = true;
    std::string rationale;
};

class PermissionPolicy {
public:
    static Platform currentPlatform() {
#if defined(Q_OS_ANDROID) || defined(__ANDROID__)
        return Platform::Android;
#elif defined(Q_OS_IOS)
        return Platform::iOS;
#elif defined(Q_OS_MACOS) || (defined(__APPLE__) && !defined(Q_OS_IOS))
        return Platform::MacOS;
#elif defined(Q_OS_WIN) || defined(_WIN32)
        return Platform::Windows;
#else
        return Platform::Linux;
#endif
    }

    static PermissionPolicyEntry lookup(Platform platform, PermissionType type) {
        switch (platform) {
        case Platform::Android:
            switch (type) {
            case PermissionType::CAMERA:       return {true,  true, "GoPost needs camera access to record video."};
            case PermissionType::MICROPHONE:   return {true,  true, "GoPost needs microphone access to record audio."};
            case PermissionType::PHOTO_LIBRARY:return {true,  true, "GoPost needs access to your photo library to import media."};
            case PermissionType::FILE_ACCESS:  return {true,  true, "GoPost needs file access to import your media files via SAF."};
            case PermissionType::STORAGE_WRITE:return {true,  true, "GoPost needs storage write access to export your projects."};
            }
            break;
        case Platform::iOS:
            switch (type) {
            case PermissionType::CAMERA:       return {true,  true, "GoPost needs camera access to record video."};
            case PermissionType::MICROPHONE:   return {true,  true, "GoPost needs microphone access to record audio."};
            case PermissionType::PHOTO_LIBRARY:return {true,  true, "GoPost needs access to import your photos and videos."};
            case PermissionType::FILE_ACCESS:  return {false, true, "File access is handled by the document picker on iOS."};
            case PermissionType::STORAGE_WRITE:return {false, true, "Storage write is managed by the sandbox on iOS."};
            }
            break;
        case Platform::MacOS:
            switch (type) {
            case PermissionType::CAMERA:       return {true,  true, "GoPost needs camera access to record video."};
            case PermissionType::MICROPHONE:   return {true,  true, "GoPost needs microphone access to record audio."};
            case PermissionType::PHOTO_LIBRARY:return {false, true, "Photo library access is available on macOS."};
            case PermissionType::FILE_ACCESS:  return {false, true, "File access is available on macOS."};
            case PermissionType::STORAGE_WRITE:return {false, true, "Storage write is available on macOS."};
            }
            break;
        case Platform::Windows:
        case Platform::Linux:
            switch (type) {
            case PermissionType::CAMERA:       return {false, true, "Camera access is available on desktop."};
            case PermissionType::MICROPHONE:   return {false, true, "Microphone access is available on desktop."};
            case PermissionType::PHOTO_LIBRARY:return {false, true, "Photo library access is available on desktop."};
            case PermissionType::FILE_ACCESS:  return {false, true, "File access is available on desktop."};
            case PermissionType::STORAGE_WRITE:return {false, true, "Storage write is available on desktop."};
            }
            break;
        }
        return {false, true, ""};
    }

    static PermissionPolicyEntry lookup(PermissionType type) {
        return lookup(currentPlatform(), type);
    }
};

} // namespace gopost::permissions
