#pragma once

#include "core/interfaces/IResult.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace gopost::io {

class BufferedReader {
public:
    explicit BufferedReader(int blockSize = 1024 * 1024);

    [[nodiscard]] gopost::Result<std::vector<uint8_t>> read(const std::string& path, int64_t offset, int64_t length);

    int64_t hitCount() const { return hits_; }
    int64_t missCount() const { return misses_; }
    void resetStats() { hits_ = 0; misses_ = 0; }

private:
    void fillBuffer(const std::string& path, int64_t alignedOffset);

    int blockSize_;
    std::mutex mutex_;

    std::string cachedPath_;
    int64_t cachedOffset_ = -1;
    std::vector<uint8_t> buffer_;

    int64_t hits_ = 0;
    int64_t misses_ = 0;
};

} // namespace gopost::io
