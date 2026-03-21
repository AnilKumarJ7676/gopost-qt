#pragma once

#include "core_engine.h"
#include "core/platform/platform_defaults.h"

#include <QObject>
#include <QSize>
#include <QString>

namespace gopost::core::engines {

class PlatformCapabilityEngine : public QObject, public CoreEngine {
    Q_OBJECT
public:
    explicit PlatformCapabilityEngine(QObject* parent = nullptr);
    ~PlatformCapabilityEngine() override;

    // CoreEngine
    void initialize() override;
    void shutdown() override;
    [[nodiscard]] bool isInitialized() const override;
    [[nodiscard]] QString engineName() const override { return QStringLiteral("Platform"); }

    // Device tier
    enum class DeviceTier { Low, Medium, High, Ultra };
    Q_ENUM(DeviceTier)

    // Device form factor
    enum class DeviceType { Desktop, Laptop, Tablet, Phone, Web, Unknown };
    Q_ENUM(DeviceType)

    [[nodiscard]] DeviceTier deviceTier() const;
    [[nodiscard]] DeviceType deviceType() const;

    // Hardware queries
    [[nodiscard]] int cpuCoreCount() const;
    [[nodiscard]] qint64 totalRamBytes() const;
    [[nodiscard]] qint64 availableRamBytes() const;

    // Environment info
    [[nodiscard]] QString gpuRenderer() const;
    [[nodiscard]] qreal screenDpi() const;
    [[nodiscard]] qreal devicePixelRatio() const;
    [[nodiscard]] QSize screenSize() const;
    [[nodiscard]] QSize physicalScreenSize() const;
    [[nodiscard]] QString osVersionString() const;
    [[nodiscard]] QString platformName() const;

    // Feature flags
    [[nodiscard]] bool isMobile() const;
    [[nodiscard]] bool supportsHardwareDecode() const;
    [[nodiscard]] bool supportsGpuCompute() const;
    [[nodiscard]] bool supportsMultiWindow() const;
    [[nodiscard]] bool hasTouchScreen() const;

    // Capacity-based tuning
    [[nodiscard]] int recommendedDecoderCount() const;
    [[nodiscard]] int recommendedFramePoolMb() const;

    // Returns platform::Defaults refined with runtime hardware data
    [[nodiscard]] platform::Defaults adaptiveDefaults() const;

    // Update GPU info after scene graph is ready (call from QML onSceneGraphInitialized)
    Q_INVOKABLE void updateGpuInfo(const QString& renderer);

    // Diagnostic summary
    Q_INVOKABLE QVariantMap diagnosticSummary() const;

signals:
    void memoryPressureDetected();

private:
    void probeHardware();
    void probeCpu();
    void probeRam();
    void probeGpu();
    void probeScreen();
    void probeDeviceType();
    DeviceTier computeTier() const;

    bool m_initialized = false;
    int m_cpuCores = 1;
    qint64 m_totalRam = 0;
    QString m_gpuRenderer;
    qreal m_screenDpi = 96.0;
    qreal m_devicePixelRatio = 1.0;
    QSize m_screenSize;
    QSize m_physicalScreenSize;
    QString m_osVersion;
    QString m_platformName;
    DeviceType m_deviceType = DeviceType::Unknown;
    bool m_hasTouchScreen = false;
};

} // namespace gopost::core::engines
