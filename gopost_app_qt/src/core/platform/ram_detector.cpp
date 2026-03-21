#include "core/platform/ram_detector.h"
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#include <unistd.h>
#include <fstream>
#include <string>
#include <cstdio>
#endif

#ifdef Q_OS_WASM
#include <emscripten.h>
#endif

namespace gopost::platform {

int64_t RamDetector::detect() {
    int64_t totalRam = 0;

#ifdef Q_OS_WIN
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus))
        totalRam = static_cast<int64_t>(memStatus.ullTotalPhys);

#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    int64_t memSize = 0;
    size_t len = sizeof(memSize);
    if (sysctlbyname("hw.memsize", &memSize, &len, nullptr, 0) == 0)
        totalRam = memSize;

#elif defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                long long kb = 0;
                std::sscanf(line.c_str(), "MemTotal: %lld kB", &kb);
                if (kb > 0) totalRam = kb * 1024;
                break;
            }
        }
    }
    if (totalRam <= 0) {
        long pages    = sysconf(_SC_PHYS_PAGES);
        long pageSize = sysconf(_SC_PAGE_SIZE);
        if (pages > 0 && pageSize > 0)
            totalRam = static_cast<int64_t>(pages) * static_cast<int64_t>(pageSize);
    }

#elif defined(Q_OS_WASM)
    double deviceMemGb = emscripten_run_script_double("navigator.deviceMemory || 2");
    totalRam = static_cast<int64_t>(deviceMemGb * 1024.0 * 1024.0 * 1024.0);
#endif

    if (totalRam <= 0)
        totalRam = int64_t(2) * 1024 * 1024 * 1024; // 2 GB fallback
    return totalRam;
}

} // namespace gopost::platform
