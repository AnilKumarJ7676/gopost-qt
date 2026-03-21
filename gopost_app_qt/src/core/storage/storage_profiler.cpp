#include "storage_profiler.h"

namespace gopost::storage {

StorageProfiler::StorageProfiler(const IDeviceCapability& deviceCap)
    : detectedType_(deviceCap.storageType()) {}

StorageProfile StorageProfiler::profileForType(StorageType type) {
    StorageProfile p;
    p.storageType = type;

    switch (type) {
    case StorageType::HDD:
        p.recommendedBlockSize = 1024 * 1024;       // 1 MB
        p.maxConcurrentReads = 1;
        p.supportsDirectIO = true;
        p.estimatedSequentialMBps = 120.0;
        break;
    case StorageType::SSD:
        p.recommendedBlockSize = 512 * 1024;         // 512 KB
        p.maxConcurrentReads = 4;
        p.supportsDirectIO = true;
        p.estimatedSequentialMBps = 500.0;
        break;
    case StorageType::NVMe:
        p.recommendedBlockSize = 1024 * 1024;        // 1 MB
        p.maxConcurrentReads = 8;
        p.supportsDirectIO = true;
        p.estimatedSequentialMBps = 3000.0;
        break;
    case StorageType::Unknown:
    default:
        // Conservative defaults — treat as slow SSD
        p.recommendedBlockSize = 256 * 1024;         // 256 KB
        p.maxConcurrentReads = 2;
        p.supportsDirectIO = false;
        p.estimatedSequentialMBps = 100.0;
        break;
    }
    return p;
}

StorageProfile StorageProfiler::buildProfile() const {
    return profileForType(detectedType_);
}

bool StorageProfiler::preferSequential() const {
    return detectedType_ == StorageType::HDD || detectedType_ == StorageType::Unknown;
}

} // namespace gopost::storage
