#pragma once

#include "IResult.h"

namespace gopost::core::interfaces {

enum class PermissionType {
    CAMERA,
    MICROPHONE,
    PHOTO_LIBRARY,
    FILE_ACCESS,
    STORAGE_WRITE
};

enum class PermissionStatus {
    GRANTED,
    DENIED,
    NOT_DETERMINED,
    RESTRICTED,
    PERMANENTLY_DENIED
};

class IPermission {
public:
    virtual ~IPermission() = default;

    [[nodiscard]] virtual PermissionStatus checkPermission(PermissionType type) = 0;
    [[nodiscard]] virtual Result<PermissionStatus> requestPermission(PermissionType type) = 0;
    [[nodiscard]] virtual Result<void> openAppSettings() = 0;
};

} // namespace gopost::core::interfaces
