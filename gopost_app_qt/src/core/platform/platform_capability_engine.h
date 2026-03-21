#pragma once

#include "core/engines/core_engine.h"
#include "core/interfaces/IDeviceCapability.h"
#include "core/platform/platform_defaults.h"

#include <QObject>
#include <QSize>
#include <QString>

namespace gopost::platform {

using namespace core::interfaces;

class PlatformCapabilityEngine : public QObject,
                                  public core::engines::CoreEngine,
                                  public IDeviceCapability {
    Q_OBJECT
public:
    Q_ENUM(DeviceTier)
    Q_ENUM(DeviceType)

    explicit PlatformCapabilityEngine(QObject* parent = nullptr);
    ~PlatformCapabilityEngine() override;

    // CoreEngine
    void initialize() override;
    void shutdown() override;
    [[nodiscard]] bool isInitialized() const override;
    [[nodiscard]] QString engineName() const override { return QStringLiteral("Platform"); }

    // IDeviceCapability
    [[nodiscard]] CpuInfo        cpuInfo()          const override;
    [[nodiscard]] GpuInfo        gpuInfo()          const override;
    [[nodiscard]] int64_t        totalRamBytes()    const override;
    [[nodiscard]] StorageType    storageType()      const override;
    [[nodiscard]] HwDecodeSupport hwDecodeSupport() const override;
    [[nodiscard]] DeviceTier     deviceTier()       const override;
    [[nodiscard]] DeviceType     deviceType()       const override;
    [[nodiscard]] QVariantMap    diagnosticSummary() const override;

    // Backward-compatible API (from old engine)
    [[nodiscard]] int    cpuCoreCount()       const;
    [[nodiscard]] qint64 availableRamBytes()  const;
    [[nodiscard]] QString gpuRenderer()       const;
    [[nodiscard]] qreal  screenDpi()          const;
    [[nodiscard]] qreal  devicePixelRatio()   const;
    [[nodiscard]] QSize  screenSize()         const;
    [[nodiscard]] QSize  physicalScreenSize() const;
    [[nodiscard]] QString osVersionString()   const;
    [[nodiscard]] QString platformName()      const;

    // Feature flags
    [[nodiscard]] bool isMobile()               const;
    [[nodiscard]] bool supportsHardwareDecode()  const;
    [[nodiscard]] bool supportsGpuCompute()      const;
    [[nodiscard]] bool supportsMultiWindow()     const;
    [[nodiscard]] bool hasTouchScreen()          const;

    // Capacity tuning
    [[nodiscard]] int recommendedDecoderCount()  const;
    [[nodiscard]] int recommendedFramePoolMb()   const;
    [[nodiscard]] Defaults adaptiveDefaults()    const;

    Q_INVOKABLE void updateGpuInfo(const QString& renderer);
    Q_INVOKABLE QVariantMap diagnosticSummaryQml() const;

signals:
    void memoryPressureDetected();

private:
    void probeScreen();
    void probeDeviceType();

    bool m_initialized = false;

    // Detector results
    CpuInfo        m_cpu;
    GpuInfo        m_gpu;
    int64_t        m_totalRam  = 0;
    StorageType    m_storage   = StorageType::Unknown;
    HwDecodeSupport m_hwDecode;
    DeviceTier     m_tier      = DeviceTier::Low;
    DeviceType     m_deviceType = DeviceType::Unknown;

    // Screen / environment
    qreal   m_screenDpi        = 96.0;
    qreal   m_devicePixelRatio = 1.0;
    QSize   m_screenSize;
    QSize   m_physicalScreenSize;
    QString m_osVersion;
    QString m_platformName;
    bool    m_hasTouchScreen = false;
};

} // namespace gopost::platform
