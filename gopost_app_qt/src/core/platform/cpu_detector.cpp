#include "core/platform/cpu_detector.h"
#include <QtGlobal>

#include <thread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <vector>
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <sys/sysctl.h>
#elif defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#include <fstream>
#include <set>
#include <string>
#endif

namespace gopost::platform {

CpuInfo CpuDetector::detect() {
    CpuInfo info;

#ifdef Q_OS_WIN
    // Logical thread count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    info.logicalThreads = static_cast<int>(sysInfo.dwNumberOfProcessors);

    // Physical core count via GetLogicalProcessorInformation
    DWORD bufLen = 0;
    GetLogicalProcessorInformation(nullptr, &bufLen);
    if (bufLen > 0) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buf(
            bufLen / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buf.data(), &bufLen)) {
            int cores = 0;
            for (const auto& entry : buf) {
                if (entry.Relationship == RelationProcessorCore)
                    ++cores;
            }
            if (cores > 0)
                info.physicalCores = cores;
        }
    }
    if (info.physicalCores <= 0)
        info.physicalCores = info.logicalThreads;

#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    int physical = 0, logical = 0;
    size_t len = sizeof(int);
    if (sysctlbyname("hw.physicalcpu", &physical, &len, nullptr, 0) == 0)
        info.physicalCores = physical;
    len = sizeof(int);
    if (sysctlbyname("hw.logicalcpu", &logical, &len, nullptr, 0) == 0)
        info.logicalThreads = logical;
    if (info.physicalCores <= 0)
        info.physicalCores = static_cast<int>(std::thread::hardware_concurrency());
    if (info.logicalThreads <= 0)
        info.logicalThreads = info.physicalCores;

#elif defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    // Parse /proc/cpuinfo for unique physical id + core id pairs
    std::set<std::pair<int, int>> physicalCores;
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        int physId = -1, coreId = -1;
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.rfind("physical id", 0) == 0) {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                    physId = std::stoi(line.substr(pos + 1));
            } else if (line.rfind("core id", 0) == 0) {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                    coreId = std::stoi(line.substr(pos + 1));
            } else if (line.empty()) {
                if (physId >= 0 && coreId >= 0)
                    physicalCores.emplace(physId, coreId);
                physId = coreId = -1;
            }
        }
        if (physId >= 0 && coreId >= 0)
            physicalCores.emplace(physId, coreId);
    }
    info.physicalCores = physicalCores.empty()
        ? static_cast<int>(std::thread::hardware_concurrency())
        : static_cast<int>(physicalCores.size());
    info.logicalThreads = static_cast<int>(std::thread::hardware_concurrency());

#else
    // Fallback
    int n = static_cast<int>(std::thread::hardware_concurrency());
    info.physicalCores  = n > 0 ? n : 1;
    info.logicalThreads = n > 0 ? n : 1;
#endif

    if (info.physicalCores  < 1) info.physicalCores  = 1;
    if (info.logicalThreads < 1) info.logicalThreads = 1;
    return info;
}

} // namespace gopost::platform
