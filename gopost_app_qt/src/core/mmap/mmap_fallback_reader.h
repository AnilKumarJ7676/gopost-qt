#pragma once

#include "core/interfaces/IMmap.h"
#include "core/interfaces/ILogger.h"

#include <mutex>
#include <vector>

namespace gopost::mmap {

using core::interfaces::MappedRegion;
using core::interfaces::ILogger;

class MmapFallbackReader {
public:
    explicit MmapFallbackReader(ILogger& logger);
    ~MmapFallbackReader();

    [[nodiscard]] Result<MappedRegion> readAsRegion(const std::string& path, uint64_t offset, size_t length);
    void release(const uint8_t* data);

private:
    ILogger& logger_;
    mutable std::mutex mutex_;

    struct FallbackBuffer {
        std::vector<uint8_t> data;
        uint64_t fileOffset = 0;
    };
    std::vector<FallbackBuffer*> buffers_;
};

} // namespace gopost::mmap
