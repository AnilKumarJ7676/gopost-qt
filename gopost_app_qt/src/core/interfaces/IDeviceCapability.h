#pragma once

#include <QString>
#include <QVariantMap>
#include <cstdint>

namespace gopost::core::interfaces {

struct CpuInfo {
    int physicalCores  = 0;
    int logicalThreads = 0;
};

struct GpuInfo {
    QString name       = QStringLiteral("Unknown");
    QString vendor     = QStringLiteral("unknown");
    int64_t vramBytes  = 0;
    bool supportsVulkan  = false;
    bool supportsDx12    = false;
    bool supportsMetal   = false;
    bool supportsOpenCL  = false;
};

enum class StorageType { Unknown, HDD, SSD, NVMe };

struct HwDecodeSupport {
    bool h264   = false;
    bool h265   = false;
    bool vp9    = false;
    bool av1    = false;
    bool prores = false;
};

enum class DeviceTier { Low, Mid, High, Ultra };

enum class DeviceType { Desktop, Laptop, Tablet, Phone, Web, Unknown };

class IDeviceCapability {
public:
    virtual ~IDeviceCapability() = default;

    [[nodiscard]] virtual CpuInfo        cpuInfo()         const = 0;
    [[nodiscard]] virtual GpuInfo        gpuInfo()         const = 0;
    [[nodiscard]] virtual int64_t        totalRamBytes()   const = 0;
    [[nodiscard]] virtual StorageType    storageType()     const = 0;
    [[nodiscard]] virtual HwDecodeSupport hwDecodeSupport() const = 0;
    [[nodiscard]] virtual DeviceTier     deviceTier()      const = 0;
    [[nodiscard]] virtual DeviceType     deviceType()      const = 0;
    [[nodiscard]] virtual QVariantMap    diagnosticSummary() const = 0;
};

} // namespace gopost::core::interfaces
