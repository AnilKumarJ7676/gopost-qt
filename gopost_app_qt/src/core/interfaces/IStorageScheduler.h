#pragma once

#include "IResult.h"
#include "IDeviceCapability.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace gopost::core::interfaces {

enum class ReadPriority { LOW, NORMAL, HIGH, CRITICAL };

using ReadTicket = int64_t;

struct StorageProfile {
    StorageType storageType = StorageType::Unknown;
    int recommendedBlockSize = 4096;
    int maxConcurrentReads = 4;
    bool supportsDirectIO = false;
    double estimatedSequentialMBps = 0.0;
};

struct ReadRequest {
    std::string path;
    int64_t offset = 0;
    int64_t length = -1;
    ReadPriority priority = ReadPriority::NORMAL;
    std::function<void(Result<std::vector<uint8_t>>)> callback;
};

class IStorageScheduler {
public:
    virtual ~IStorageScheduler() = default;

    [[nodiscard]] virtual Result<ReadTicket> scheduleRead(const ReadRequest& request) = 0;
    virtual void cancelRead(ReadTicket ticket) = 0;
    [[nodiscard]] virtual StorageProfile getStorageProfile() const = 0;
};

} // namespace gopost::core::interfaces
