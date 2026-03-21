#pragma once

#include "IResult.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace gopost::core::interfaces {

struct MappedRegion {
    const uint8_t* data = nullptr;
    size_t length = 0;
    uint64_t fileOffset = 0;
    bool isValid = false;
};

class IMmap {
public:
    virtual ~IMmap() = default;

    [[nodiscard]] virtual Result<MappedRegion> mapFile(const std::string& path, uint64_t offset = 0, size_t length = 0) = 0;
    virtual void unmapFile(MappedRegion& region) = 0;
    virtual void advisorySequential(MappedRegion& region) = 0;
    virtual void advisoryWillNeed(MappedRegion& region, uint64_t offset, size_t length) = 0;
};

} // namespace gopost::core::interfaces
