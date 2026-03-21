#include "core/platform/tier_classifier.h"

namespace gopost::platform {

static constexpr int64_t GB = int64_t(1024) * 1024 * 1024;

DeviceTier TierClassifier::classify(const CpuInfo& cpu,
                                     int64_t ramBytes,
                                     const GpuInfo& gpu,
                                     StorageType storage) {
    const bool fastStorage = (storage == StorageType::SSD ||
                              storage == StorageType::NVMe);

    // ULTRA: ≥16 threads AND ≥32GB RAM AND ≥8GB VRAM AND SSD/NVMe
    if (cpu.logicalThreads >= 16 &&
        ramBytes >= 32 * GB &&
        gpu.vramBytes >= 8 * GB &&
        fastStorage) {
        return DeviceTier::Ultra;
    }

    // HIGH: ≥8 threads AND ≥16GB RAM AND ≥4GB VRAM
    if (cpu.logicalThreads >= 8 &&
        ramBytes >= 16 * GB &&
        gpu.vramBytes >= 4 * GB) {
        return DeviceTier::High;
    }

    // MID: ≥4 threads AND ≥8GB RAM
    if (cpu.logicalThreads >= 4 &&
        ramBytes >= 8 * GB) {
        return DeviceTier::Mid;
    }

    return DeviceTier::Low;
}

} // namespace gopost::platform
