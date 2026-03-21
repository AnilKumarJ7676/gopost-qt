#pragma once

#include "core/interfaces/IMmap.h"
#include "core/interfaces/ILogger.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace gopost::mmap {

using core::interfaces::MappedRegion;
using core::interfaces::ILogger;
using gopost::Result;

struct InternalMapping {
    const uint8_t* baseAddr = nullptr;
    size_t mappedLength = 0;
    uint64_t fileOffset = 0;
    bool isMmapped = true;
#ifdef _WIN32
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = nullptr;
#else
    int fd = -1;
#endif
};

class MmapProvider {
public:
    explicit MmapProvider(ILogger& logger);
    ~MmapProvider();

    [[nodiscard]] Result<MappedRegion> map(const std::string& path, uint64_t offset, size_t length);
    void unmap(const uint8_t* addr);
    [[nodiscard]] bool isTracked(const uint8_t* addr) const;
    [[nodiscard]] const InternalMapping* findMapping(const uint8_t* addr) const;

    static size_t pageGranularity();
    static uint64_t alignDown(uint64_t offset, size_t granularity);

    void unmapAll();

private:
    ILogger& logger_;
    mutable std::mutex mutex_;
    std::vector<InternalMapping> mappings_;

    static uint64_t getFileSize(const std::string& path);
};

} // namespace gopost::mmap
