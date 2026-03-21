#pragma once

#include "core/interfaces/IEventBus.h"
#include <string>

namespace gopost::events {

using core::interfaces::Event;

struct EngineReadyEvent : Event {
    std::string engineName;
    explicit EngineReadyEvent(std::string name = {}) : engineName(std::move(name)) {
        type = QStringLiteral("EngineReady");
    }
};

struct MemoryWarningEvent : Event {
    std::string poolName;
    double usagePercent = 0.0;
    MemoryWarningEvent() { type = QStringLiteral("MemoryWarning"); }
};

struct ConfigChangedEvent : Event {
    std::string key;
    std::string newValue;
    ConfigChangedEvent() { type = QStringLiteral("ConfigChanged"); }
};

struct BootCompleteEvent : Event {
    double bootTimeMs = 0.0;
    BootCompleteEvent() { type = QStringLiteral("BootComplete"); }
};

struct PermissionRequestedEvent : Event {
    int permissionType = 0;
    int status = 0;
    PermissionRequestedEvent() { type = QStringLiteral("PermissionRequested"); }
};

struct PermissionDeniedEvent : Event {
    int permissionType = 0;
    int status = 0;
    std::string rationale;
    PermissionDeniedEvent() { type = QStringLiteral("PermissionDenied"); }
};

struct IOBottleneckEvent : Event {
    double currentMBps = 0.0;
    double expectedMBps = 0.0;
    std::string affectedFile;
    IOBottleneckEvent() { type = QStringLiteral("IOBottleneck"); }
};

} // namespace gopost::events
