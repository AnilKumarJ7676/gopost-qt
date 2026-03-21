#include "core/platform/platform_capability_engine.h"
#include "core/platform/cpu_detector.h"
#include "core/platform/gpu_detector.h"
#include "core/platform/ram_detector.h"
#include "core/platform/storage_detector.h"
#include "core/platform/hardware_decode_detector.h"
#include "core/platform/tier_classifier.h"

#include <QGuiApplication>
#include <QScreen>
#include <QSysInfo>
#include <QInputDevice>
#include <QVariantMap>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <sys/sysctl.h>
#include <sys/types.h>
#include <mach/mach.h>
#if defined(Q_OS_IOS)
#include <os/proc.h>
#endif
#elif defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#include <unistd.h>
#include <fstream>
#include <string>
#endif

#ifdef Q_OS_WASM
#include <emscripten.h>
#endif

namespace gopost::platform {

PlatformCapabilityEngine::PlatformCapabilityEngine(QObject* parent)
    : QObject(parent) {}

PlatformCapabilityEngine::~PlatformCapabilityEngine() {
    shutdown();
}

void PlatformCapabilityEngine::initialize() {
    if (m_initialized) return;

    // Run all detectors
    m_cpu      = CpuDetector::detect();
    m_totalRam = RamDetector::detect();
    m_gpu      = GpuDetector::detect();
    m_storage  = StorageDetector::detect();
    m_hwDecode = HardwareDecodeDetector::detect();

    // Screen and device type probing (Qt-based, kept inline)
    probeScreen();
    probeDeviceType();

    // OS info
    m_osVersion = QSysInfo::prettyProductName();
#if defined(Q_OS_WIN)
    m_platformName = QStringLiteral("Windows");
#elif defined(Q_OS_MACOS)
    m_platformName = QStringLiteral("macOS");
#elif defined(Q_OS_IOS)
    m_platformName = QStringLiteral("iOS");
#elif defined(Q_OS_ANDROID)
    m_platformName = QStringLiteral("Android");
#elif defined(Q_OS_LINUX)
    m_platformName = QStringLiteral("Linux");
#elif defined(Q_OS_WASM)
    m_platformName = QStringLiteral("WebAssembly");
#else
    m_platformName = QStringLiteral("Unknown");
#endif

    // Classify tier
    m_tier = TierClassifier::classify(m_cpu, m_totalRam, m_gpu, m_storage);

    m_initialized = true;
}

void PlatformCapabilityEngine::shutdown() {
    m_initialized = false;
}

bool PlatformCapabilityEngine::isInitialized() const {
    return m_initialized;
}

// ===========================================================================
// IDeviceCapability
// ===========================================================================

CpuInfo PlatformCapabilityEngine::cpuInfo() const { return m_cpu; }
GpuInfo PlatformCapabilityEngine::gpuInfo() const { return m_gpu; }
int64_t PlatformCapabilityEngine::totalRamBytes() const { return m_totalRam; }
StorageType PlatformCapabilityEngine::storageType() const { return m_storage; }
HwDecodeSupport PlatformCapabilityEngine::hwDecodeSupport() const { return m_hwDecode; }
DeviceTier PlatformCapabilityEngine::deviceTier() const { return m_tier; }
DeviceType PlatformCapabilityEngine::deviceType() const { return m_deviceType; }

// ===========================================================================
// Backward-compatible API
// ===========================================================================

int PlatformCapabilityEngine::cpuCoreCount() const {
    return m_cpu.logicalThreads;
}

qint64 PlatformCapabilityEngine::availableRamBytes() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus))
        return static_cast<qint64>(memStatus.ullAvailPhys);

#elif defined(Q_OS_IOS)
    size_t avail = os_proc_available_memory();
    if (avail > 0) return static_cast<qint64>(avail);
    mach_port_t host = mach_host_self();
    vm_statistics64_data_t vmStat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host, HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vmStat),
                          &count) == KERN_SUCCESS) {
        return static_cast<qint64>(vmStat.free_count + vmStat.inactive_count)
               * static_cast<qint64>(vm_page_size);
    }

#elif defined(Q_OS_MACOS)
    mach_port_t host = mach_host_self();
    vm_statistics64_data_t vmStat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host, HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vmStat),
                          &count) == KERN_SUCCESS) {
        return static_cast<qint64>(vmStat.free_count + vmStat.inactive_count)
               * static_cast<qint64>(vm_page_size);
    }

#elif defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.rfind("MemAvailable:", 0) == 0) {
                long long kb = 0;
                std::sscanf(line.c_str(), "MemAvailable: %lld kB", &kb);
                if (kb > 0) return static_cast<qint64>(kb) * 1024;
                break;
            }
        }
    }
    long pages    = sysconf(_SC_AVPHYS_PAGES);
    long pageSize = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && pageSize > 0)
        return static_cast<qint64>(pages) * static_cast<qint64>(pageSize);

#elif defined(Q_OS_WASM)
    return m_totalRam / 2;
#endif
    return m_totalRam / 2;
}

QString PlatformCapabilityEngine::gpuRenderer() const { return m_gpu.name; }
qreal PlatformCapabilityEngine::screenDpi() const { return m_screenDpi; }
qreal PlatformCapabilityEngine::devicePixelRatio() const { return m_devicePixelRatio; }
QSize PlatformCapabilityEngine::screenSize() const { return m_screenSize; }
QSize PlatformCapabilityEngine::physicalScreenSize() const { return m_physicalScreenSize; }
QString PlatformCapabilityEngine::osVersionString() const { return m_osVersion; }
QString PlatformCapabilityEngine::platformName() const { return m_platformName; }

bool PlatformCapabilityEngine::isMobile() const {
    return m_deviceType == DeviceType::Phone ||
           m_deviceType == DeviceType::Tablet;
}

bool PlatformCapabilityEngine::supportsHardwareDecode() const {
    return m_hwDecode.h264 || m_hwDecode.h265 || m_hwDecode.vp9 || m_hwDecode.av1;
}

bool PlatformCapabilityEngine::supportsGpuCompute() const {
#if defined(Q_OS_WASM)
    return false;
#elif defined(Q_OS_IOS) || defined(Q_OS_MACOS)
    return true;
#else
    return m_tier >= DeviceTier::High;
#endif
}

bool PlatformCapabilityEngine::supportsMultiWindow() const {
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID) || defined(Q_OS_WASM)
    return false;
#else
    return true;
#endif
}

bool PlatformCapabilityEngine::hasTouchScreen() const {
    return m_hasTouchScreen;
}

void PlatformCapabilityEngine::updateGpuInfo(const QString& renderer) {
    if (!renderer.isEmpty())
        m_gpu.name = renderer;
}

// ===========================================================================
// Capacity tuning (preserved from old engine)
// ===========================================================================

int PlatformCapabilityEngine::recommendedDecoderCount() const {
    if (isMobile()) {
        return (m_tier >= DeviceTier::High) ? 2 : 1;
    }
    switch (m_tier) {
    case DeviceTier::Ultra:  return qMin(m_cpu.logicalThreads / 2, 8);
    case DeviceTier::High:   return qMin(m_cpu.logicalThreads / 2, 4);
    case DeviceTier::Mid:    return 2;
    case DeviceTier::Low:    return 1;
    }
    return 1;
}

int PlatformCapabilityEngine::recommendedFramePoolMb() const {
    double ramGb = static_cast<double>(m_totalRam) / (1024.0 * 1024.0 * 1024.0);

    if (isMobile()) {
        switch (m_tier) {
        case DeviceTier::Ultra:  return qMin(static_cast<int>(ramGb * 24), 256);
        case DeviceTier::High:   return qMin(static_cast<int>(ramGb * 16), 128);
        case DeviceTier::Mid:    return qMin(static_cast<int>(ramGb * 8), 64);
        case DeviceTier::Low:    return 32;
        }
    }

    switch (m_tier) {
    case DeviceTier::Ultra:  return qMin(static_cast<int>(ramGb * 64), 1024);
    case DeviceTier::High:   return qMin(static_cast<int>(ramGb * 32), 512);
    case DeviceTier::Mid:    return qMin(static_cast<int>(ramGb * 16), 256);
    case DeviceTier::Low:    return 64;
    }
    return 64;
}

Defaults PlatformCapabilityEngine::adaptiveDefaults() const {
    auto defaults = Defaults::forCurrentPlatform();

    if (isMobile()) {
        switch (m_tier) {
        case DeviceTier::Ultra:
            defaults.thumbnailMemoryCacheEntries = 300;
            defaults.thumbnailDiskCacheBytes = 128 * 1024 * 1024;
            defaults.maxDecoders = 2;
            break;
        case DeviceTier::High:
            defaults.thumbnailMemoryCacheEntries = 200;
            defaults.thumbnailDiskCacheBytes = 64 * 1024 * 1024;
            defaults.maxDecoders = 2;
            break;
        case DeviceTier::Mid:
            defaults.thumbnailMemoryCacheEntries = 100;
            defaults.thumbnailDiskCacheBytes = 32 * 1024 * 1024;
            defaults.maxDecoders = 1;
            break;
        case DeviceTier::Low:
            defaults.thumbnailMemoryCacheEntries = 50;
            defaults.thumbnailDiskCacheBytes = 16 * 1024 * 1024;
            defaults.maxDecoders = 1;
            break;
        }
        return defaults;
    }

    switch (m_tier) {
    case DeviceTier::Ultra:
        defaults.thumbnailMemoryCacheEntries = 1000;
        defaults.thumbnailDiskCacheBytes = 500 * 1024 * 1024;
        defaults.maxDecoders = recommendedDecoderCount();
        break;
    case DeviceTier::High:
        defaults.thumbnailMemoryCacheEntries = 500;
        defaults.thumbnailDiskCacheBytes = 200 * 1024 * 1024;
        defaults.maxDecoders = recommendedDecoderCount();
        break;
    case DeviceTier::Mid:
        defaults.thumbnailMemoryCacheEntries = 200;
        defaults.thumbnailDiskCacheBytes = 100 * 1024 * 1024;
        defaults.maxDecoders = 2;
        break;
    case DeviceTier::Low:
        defaults.thumbnailMemoryCacheEntries = 100;
        defaults.thumbnailDiskCacheBytes = 32 * 1024 * 1024;
        defaults.maxDecoders = 1;
        break;
    }
    return defaults;
}

// ===========================================================================
// Diagnostics
// ===========================================================================

QVariantMap PlatformCapabilityEngine::diagnosticSummary() const {
    QVariantMap m;
    m[QStringLiteral("platform")]          = m_platformName;
    m[QStringLiteral("osVersion")]         = m_osVersion;
    m[QStringLiteral("cpuPhysicalCores")]  = m_cpu.physicalCores;
    m[QStringLiteral("cpuLogicalThreads")] = m_cpu.logicalThreads;
    m[QStringLiteral("totalRamMb")]        = m_totalRam / (1024 * 1024);
    m[QStringLiteral("availableRamMb")]    = availableRamBytes() / (1024 * 1024);
    m[QStringLiteral("gpuName")]           = m_gpu.name;
    m[QStringLiteral("gpuVendor")]         = m_gpu.vendor;
    m[QStringLiteral("gpuVramMb")]         = m_gpu.vramBytes / (1024 * 1024);
    m[QStringLiteral("storageType")]       = static_cast<int>(m_storage);
    m[QStringLiteral("hwDecode_h264")]     = m_hwDecode.h264;
    m[QStringLiteral("hwDecode_h265")]     = m_hwDecode.h265;
    m[QStringLiteral("hwDecode_vp9")]      = m_hwDecode.vp9;
    m[QStringLiteral("hwDecode_av1")]      = m_hwDecode.av1;
    m[QStringLiteral("hwDecode_prores")]   = m_hwDecode.prores;
    m[QStringLiteral("screenDpi")]         = m_screenDpi;
    m[QStringLiteral("devicePixelRatio")]  = m_devicePixelRatio;
    m[QStringLiteral("screenSize")]        = QStringLiteral("%1x%2").arg(m_screenSize.width()).arg(m_screenSize.height());
    m[QStringLiteral("deviceTier")]        = static_cast<int>(m_tier);
    m[QStringLiteral("deviceType")]        = static_cast<int>(m_deviceType);
    m[QStringLiteral("isMobile")]          = isMobile();
    m[QStringLiteral("hasTouchScreen")]    = m_hasTouchScreen;
    m[QStringLiteral("recommendedDecoders")] = recommendedDecoderCount();
    m[QStringLiteral("recommendedFramePoolMb")] = recommendedFramePoolMb();
    return m;
}

QVariantMap PlatformCapabilityEngine::diagnosticSummaryQml() const {
    return diagnosticSummary();
}

// ===========================================================================
// Screen / device type probing (from old engine)
// ===========================================================================

void PlatformCapabilityEngine::probeScreen() {
    auto* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    m_screenDpi = screen->logicalDotsPerInch();
    m_devicePixelRatio = screen->devicePixelRatio();
    m_screenSize = screen->size();

    QSizeF physMm = screen->physicalSize();
    m_physicalScreenSize = QSize(
        static_cast<int>(physMm.width()),
        static_cast<int>(physMm.height()));

    m_hasTouchScreen = false;
    const auto devices = QInputDevice::devices();
    for (const auto* dev : devices) {
        if (dev->type() == QInputDevice::DeviceType::TouchScreen) {
            m_hasTouchScreen = true;
            break;
        }
    }
}

void PlatformCapabilityEngine::probeDeviceType() {
#if defined(Q_OS_WASM)
    m_deviceType = DeviceType::Web;
    return;
#elif defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    double diagInches = 0;
    if (m_physicalScreenSize.width() > 0 && m_physicalScreenSize.height() > 0) {
        double wMm = m_physicalScreenSize.width();
        double hMm = m_physicalScreenSize.height();
        diagInches = std::sqrt(wMm * wMm + hMm * hMm) / 25.4;
    }
    m_deviceType = (diagInches >= 7.0) ? DeviceType::Tablet : DeviceType::Phone;
    return;
#elif defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    if (m_hasTouchScreen && m_screenSize.width() <= 1920 &&
        m_physicalScreenSize.width() > 0) {
        double diagInches = std::sqrt(
            std::pow(m_physicalScreenSize.width(), 2) +
            std::pow(m_physicalScreenSize.height(), 2)) / 25.4;
        if (diagInches < 14.0) {
            m_deviceType = DeviceType::Laptop;
            return;
        }
    }
    m_deviceType = DeviceType::Desktop;
    return;
#endif
    m_deviceType = DeviceType::Unknown;
}

} // namespace gopost::platform
