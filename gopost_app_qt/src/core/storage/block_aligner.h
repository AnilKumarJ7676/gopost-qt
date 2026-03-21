#pragma once

#include <cstdint>

namespace gopost::storage {

struct AlignedRange {
    int64_t alignedOffset = 0;
    int64_t alignedLength = 0;
    int64_t trimStart = 0;      // bytes to skip at start of aligned buffer
    int64_t trimLength = 0;     // actual requested bytes within aligned buffer
};

class BlockAligner {
public:
    explicit BlockAligner(int blockSize) : blockSize_(blockSize > 0 ? blockSize : 4096) {}

    [[nodiscard]] AlignedRange align(int64_t offset, int64_t length) const {
        AlignedRange r;
        r.alignedOffset = (offset / blockSize_) * blockSize_;
        int64_t end = offset + length;
        int64_t alignedEnd = ((end + blockSize_ - 1) / blockSize_) * blockSize_;
        r.alignedLength = alignedEnd - r.alignedOffset;
        r.trimStart = offset - r.alignedOffset;
        r.trimLength = length;
        return r;
    }

    [[nodiscard]] int blockSize() const { return blockSize_; }

private:
    int blockSize_;
};

} // namespace gopost::storage
