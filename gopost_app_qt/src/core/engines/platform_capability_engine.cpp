#include "platform_capability_engine.h"

#include <QGuiApplication>
#include <QScreen>
#include <QSysInfo>
#include <QThread>
#include <QVariantMap>
#include <QInputDevice>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <sys/sysctl.h>
#include <sys/types.h>
#include <mach/mach.h>
#if defined(Q_OS_IOS)
#include <os/proc.h>   // os_proc_available_memory (iOS 13+)
#endif
#elif defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#include <unistd.h>
#include <fstream>
#include <string>
#endif

#ifdef Q_OS_WASM
#include <emscripten.h>
#endif

namespace gopost::core::engines {

PlatformCapabilityEngine::PlatformCapabilityEngine(QObject* parent)
    : QObject(parent) {}

PlatformCapabilityEngine::~PlatformCapabilityEngine() {
    shutdown();
}

void PlatformCapabilityEngine::initialize() {
    if (m_initialized) return;
    probeHardware();
    m_initialized = true;
}

void PlatformCapabilityEngine::shutdown() {
    m_initialized = false;
}

bool PlatformCapabilityEngine::isInitialized() const {
    return m_initialized;
}

// ===========================================================================
// Hardware probing
// ===========================================================================

void PlatformCapabilityEngine::probeHardware() {
    probeCpu();
    probeRam();
    probeGpu();
    probeScreen();
    probeDeviceType();

    // OS version — works on all platforms via Qt
    m_osVersion = QSysInfo::prettyProductName();

    // Platform name
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
}

void PlatformCapabilityEngine::probeCpu() {
    m_cpuCores = QThread::idealThreadCount();
    if (m_cpuCores < 1) m_cpuCores = 1;

#ifdef Q_OS_WASM
    // Emscripten: navigator.hardwareConcurrency
    m_cpuCores = emscripten_run_script_int("navigator.hardwareConcurrency || 1");
    if (m_cpuCores < 1) m_cpuCores = 1;
#endif
}

void PlatformCapabilityEngine::probeRam() {
    m_totalRam = 0;

#ifdef Q_OS_WIN
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        m_totalRam = static_cast<qint64>(memStatus.ullTotalPhys);
    }

#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    int64_t memSize = 0;
    size_t len = sizeof(memSize);
    if (sysctlbyname("hw.memsize", &memSize, &len, nullptr, 0) == 0) {
        m_totalRam = static_cast<qint64>(memSize);
    }

#elif defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    // Prefer /proc/meminfo for accuracy (includes all memory, not just user pages)
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                // Format: "MemTotal:       16384000 kB"
                qint64 kb = 0;
                sscanf(line.c_str(), "MemTotal: %lld kB", &kb);
                if (kb > 0) m_totalRam = kb * 1024;
                break;
            }
        }
    }
    // Fallback to sysconf if /proc/meminfo unavailable
    if (m_totalRam <= 0) {
        long pages = sysconf(_SC_PHYS_PAGES);
        long pageSize = sysconf(_SC_PAGE_SIZE);
        if (pages > 0 && pageSize > 0) {
            m_totalRam = static_cast<qint64>(pages) * static_cast<qint64>(pageSize);
        }
    }

#elif defined(Q_OS_WASM)
    // navigator.deviceMemory returns GB (may be undefined or capped at 8)
    double deviceMemGb = emscripten_run_script_double(
        "navigator.deviceMemory || 2");
    m_totalRam = static_cast<qint64>(deviceMemGb * 1024.0 * 1024.0 * 1024.0);
#endif

    // Safety: if everything failed, assume 2 GB
    if (m_totalRam <= 0) {
        m_totalRam = qint64(2) * 1024 * 1024 * 1024;
    }
}

void PlatformCapabilityEngine::probeGpu() {
    // GPU renderer can't be queried before scene graph init.
    // Set a platform-based default; updateGpuInfo() is called later from QML.
#if defined(Q_OS_WIN)
    m_gpuRenderer = QStringLiteral("D3D11 (pending)");
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    m_gpuRenderer = QStringLiteral("Metal (pending)");
#elif defined(Q_OS_ANDROID)
    m_gpuRenderer = QStringLiteral("OpenGL ES (pending)");
#elif defined(Q_OS_LINUX)
    m_gpuRenderer = QStringLiteral("Vulkan/OpenGL (pending)");
#elif defined(Q_OS_WASM)
    m_gpuRenderer = QStringLiteral("WebGL");
#else
    m_gpuRenderer = QStringLiteral("unknown");
#endif
}

void PlatformCapabilityEngine::probeScreen() {
    auto* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    m_screenDpi = screen->logicalDotsPerInch();
    m_devicePixelRatio = screen->devicePixelRatio();
    m_screenSize = screen->size();

    // Physical size in mm → QSize for form factor detection
    QSizeF physMm = screen->physicalSize();
    m_physicalScreenSize = QSize(
        static_cast<int>(physMm.width()),
        static_cast<int>(physMm.height()));

    // Touch screen detection
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
#elif defined(Q_OS_IOS)
    // iOS: iPad vs iPhone based on screen diagonal
    double diagInches = 0;
    if (m_physicalScreenSize.width() > 0 && m_physicalScreenSize.height() > 0) {
        double wMm = m_physicalScreenSize.width();
        double hMm = m_physicalScreenSize.height();
        diagInches = std::sqrt(wMm * wMm + hMm * hMm) / 25.4;
    }
    m_deviceType = (diagInches >= 7.0) ? DeviceType::Tablet : DeviceType::Phone;
    return;
#elif defined(Q_OS_ANDROID)
    // Android: phone vs tablet based on screen diagonal and DPI
    double diagInches = 0;
    if (m_physicalScreenSize.width() > 0 && m_physicalScreenSize.height() > 0) {
        double wMm = m_physicalScreenSize.width();
        double hMm = m_physicalScreenSize.height();
        diagInches = std::sqrt(wMm * wMm + hMm * hMm) / 25.4;
    }
    m_deviceType = (diagInches >= 7.0) ? DeviceType::Tablet : DeviceType::Phone;
    return;
#elif defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    // Desktop platforms: check for battery (laptop indicator)
    // Simple heuristic: if touch screen + small screen → could be tablet mode
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

// ===========================================================================
// Tier computation — accounts for mobile vs desktop
// ===========================================================================

PlatformCapabilityEngine::DeviceTier PlatformCapabilityEngine::computeTier() const {
    double ramGb = static_cast<double>(m_totalRam) / (1024.0 * 1024.0 * 1024.0);

    // Mobile devices get stricter thresholds — they have less thermal headroom,
    // battery constraints, and OS memory pressure compared to desktops.
    if (isMobile()) {
        // Mobile scoring: RAM (GB) * 1.5 + cores * 0.5
        double score = ramGb * 1.5 + static_cast<double>(m_cpuCores) * 0.5;
        if (score >= 16.0) return DeviceTier::Ultra;   // 8GB+ RAM, 8+ cores (iPad Pro, flagship)
        if (score >= 10.0) return DeviceTier::High;    // 6GB+ RAM, 6+ cores
        if (score >= 5.0)  return DeviceTier::Medium;  // 3GB+ RAM, 4+ cores
        return DeviceTier::Low;                         // budget phone/tablet
    }

    // Desktop/laptop scoring: RAM (GB) * 2 + cores * 1
    double score = ramGb * 2.0 + static_cast<double>(m_cpuCores);
    if (score >= 40.0) return DeviceTier::Ultra;   // 16GB+ RAM, 8+ cores
    if (score >= 20.0) return DeviceTier::High;    // 8GB+ RAM, 4+ cores
    if (score >= 10.0) return DeviceTier::Medium;  // 4GB+ RAM, 2+ cores
    return DeviceTier::Low;
}

// ===========================================================================
// Public queries
// ===========================================================================

PlatformCapabilityEngine::DeviceTier PlatformCapabilityEngine::deviceTier() const {
    return computeTier();
}

PlatformCapabilityEngine::DeviceType PlatformCapabilityEngine::deviceType() const {
    return m_deviceType;
}

int PlatformCapabilityEngine::cpuCoreCount() const {
    return m_cpuCores;
}

qint64 PlatformCapabilityEngine::totalRamBytes() const {
    return m_totalRam;
}

qint64 PlatformCapabilityEngine::availableRamBytes() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        return static_cast<qint64>(memStatus.ullAvailPhys);
    }

#elif defined(Q_OS_IOS)
    // iOS 13+: os_proc_available_memory — the most accurate API
    size_t avail = os_proc_available_memory();
    if (avail > 0) return static_cast<qint64>(avail);
    // Fallback to Mach API
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
    // Parse /proc/meminfo for MemAvailable (Linux 3.14+, Android 4.4+)
    // MemAvailable is the best estimate — includes reclaimable buffers/cached
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.rfind("MemAvailable:", 0) == 0) {
                qint64 kb = 0;
                sscanf(line.c_str(), "MemAvailable: %lld kB", &kb);
                if (kb > 0) return kb * 1024;
                break;
            }
        }
    }
    // Fallback to sysconf (less accurate — doesn't count reclaimable cache)
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long pageSize = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && pageSize > 0) {
        return static_cast<qint64>(pages) * static_cast<qint64>(pageSize);
    }

#elif defined(Q_OS_WASM)
    // WASM: rough estimate — total minus a conservative usage guess
    return m_totalRam / 2;
#endif

    // Final fallback: assume 50% of total
    return m_totalRam / 2;
}

QString PlatformCapabilityEngine::gpuRenderer() const {
    return m_gpuRenderer;
}

qreal PlatformCapabilityEngine::screenDpi() const {
    return m_screenDpi;
}

qreal PlatformCapabilityEngine::devicePixelRatio() const {
    return m_devicePixelRatio;
}

QSize PlatformCapabilityEngine::screenSize() const {
    return m_screenSize;
}

QSize PlatformCapabilityEngine::physicalScreenSize() const {
    return m_physicalScreenSize;
}

QString PlatformCapabilityEngine::osVersionString() const {
    return m_osVersion;
}

QString PlatformCapabilityEngine::platformName() const {
    return m_platformName;
}

bool PlatformCapabilityEngine::isMobile() const {
    return m_deviceType == DeviceType::Phone ||
           m_deviceType == DeviceType::Tablet;
}

bool PlatformCapabilityEngine::supportsHardwareDecode() const {
#if defined(Q_OS_WASM)
    return false;
#elif defined(Q_OS_IOS)
    return true;  // All iOS devices support VideoToolbox
#elif defined(Q_OS_MACOS)
    return true;  // All modern Macs support VideoToolbox
#elif defined(Q_OS_ANDROID)
    return deviceTier() >= DeviceTier::Medium;  // MediaCodec on mid+ devices
#else
    return deviceTier() >= DeviceTier::Medium;
#endif
}

bool PlatformCapabilityEngine::supportsGpuCompute() const {
#if defined(Q_OS_WASM)
    return false;
#elif defined(Q_OS_IOS) || defined(Q_OS_MACOS)
    return true;  // Metal compute is available on all supported Apple devices
#else
    return deviceTier() >= DeviceTier::High;
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
    if (!renderer.isEmpty()) {
        m_gpuRenderer = renderer;
    }
}

// ===========================================================================
// Capacity-based tuning
// ===========================================================================

int PlatformCapabilityEngine::recommendedDecoderCount() const {
    if (isMobile()) {
        // Mobile: conservative to preserve battery and avoid thermal throttling
        switch (deviceTier()) {
        case DeviceTier::Ultra:  return 2;
        case DeviceTier::High:   return 2;
        case DeviceTier::Medium: return 1;
        case DeviceTier::Low:    return 1;
        }
    }

    switch (deviceTier()) {
    case DeviceTier::Ultra:  return qMin(m_cpuCores / 2, 8);
    case DeviceTier::High:   return qMin(m_cpuCores / 2, 4);
    case DeviceTier::Medium: return 2;
    case DeviceTier::Low:    return 1;
    }
    return 1;
}

int PlatformCapabilityEngine::recommendedFramePoolMb() const {
    double ramGb = static_cast<double>(m_totalRam) / (1024.0 * 1024.0 * 1024.0);

    if (isMobile()) {
        // Mobile: much tighter memory budgets (OS kills apps aggressively)
        switch (deviceTier()) {
        case DeviceTier::Ultra:  return qMin(static_cast<int>(ramGb * 24), 256);
        case DeviceTier::High:   return qMin(static_cast<int>(ramGb * 16), 128);
        case DeviceTier::Medium: return qMin(static_cast<int>(ramGb * 8), 64);
        case DeviceTier::Low:    return 32;
        }
    }

    switch (deviceTier()) {
    case DeviceTier::Ultra:  return qMin(static_cast<int>(ramGb * 64), 1024);
    case DeviceTier::High:   return qMin(static_cast<int>(ramGb * 32), 512);
    case DeviceTier::Medium: return qMin(static_cast<int>(ramGb * 16), 256);
    case DeviceTier::Low:    return 64;
    }
    return 64;
}

platform::Defaults PlatformCapabilityEngine::adaptiveDefaults() const {
    auto defaults = platform::Defaults::forCurrentPlatform();

    if (isMobile()) {
        // Mobile: override compile-time defaults with runtime-aware values
        switch (deviceTier()) {
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
        case DeviceTier::Medium:
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

    // Desktop/laptop
    switch (deviceTier()) {
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
    case DeviceTier::Medium:
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
    m[QStringLiteral("platform")] = m_platformName;
    m[QStringLiteral("osVersion")] = m_osVersion;
    m[QStringLiteral("cpuCores")] = m_cpuCores;
    m[QStringLiteral("totalRamMb")] = m_totalRam / (1024 * 1024);
    m[QStringLiteral("availableRamMb")] = availableRamBytes() / (1024 * 1024);
    m[QStringLiteral("gpuRenderer")] = m_gpuRenderer;
    m[QStringLiteral("screenDpi")] = m_screenDpi;
    m[QStringLiteral("devicePixelRatio")] = m_devicePixelRatio;
    m[QStringLiteral("screenSize")] = QStringLiteral("%1x%2").arg(m_screenSize.width()).arg(m_screenSize.height());
    m[QStringLiteral("physicalScreenMm")] = QStringLiteral("%1x%2").arg(m_physicalScreenSize.width()).arg(m_physicalScreenSize.height());
    m[QStringLiteral("deviceTier")] = static_cast<int>(deviceTier());
    m[QStringLiteral("deviceType")] = static_cast<int>(m_deviceType);
    m[QStringLiteral("isMobile")] = isMobile();
    m[QStringLiteral("hasTouchScreen")] = m_hasTouchScreen;
    m[QStringLiteral("supportsHwDecode")] = supportsHardwareDecode();
    m[QStringLiteral("supportsGpuCompute")] = supportsGpuCompute();
    m[QStringLiteral("recommendedDecoders")] = recommendedDecoderCount();
    m[QStringLiteral("recommendedFramePoolMb")] = recommendedFramePoolMb();
    return m;
}

} // namespace gopost::core::engines
