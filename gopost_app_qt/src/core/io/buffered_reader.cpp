#include "buffered_reader.h"
#include <algorithm>
#include <fstream>

namespace gopost::io {

BufferedReader::BufferedReader(int blockSize)
    : blockSize_(blockSize > 0 ? blockSize : 1024 * 1024) {}

void BufferedReader::fillBuffer(const std::string& path, int64_t alignedOffset) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) { buffer_.clear(); cachedOffset_ = -1; return; }

    auto fileSize = static_cast<int64_t>(file.tellg());
    auto readLen = std::min<int64_t>(blockSize_, fileSize - alignedOffset);
    if (readLen <= 0) { buffer_.clear(); cachedOffset_ = -1; return; }

    file.seekg(alignedOffset);
    buffer_.resize(static_cast<size_t>(readLen));
    file.read(reinterpret_cast<char*>(buffer_.data()), readLen);
    buffer_.resize(static_cast<size_t>(file.gcount()));
    cachedPath_ = path;
    cachedOffset_ = alignedOffset;
    misses_++;
}

gopost::Result<std::vector<uint8_t>> BufferedReader::read(const std::string& path, int64_t offset, int64_t length) {
    std::lock_guard lock(mutex_);

    if (length <= 0)
        return gopost::Result<std::vector<uint8_t>>::failure("Invalid length");

    // Check if fully within current buffer
    if (cachedPath_ == path && cachedOffset_ >= 0 &&
        offset >= cachedOffset_ &&
        offset + length <= cachedOffset_ + static_cast<int64_t>(buffer_.size())) {
        // Full buffer hit
        hits_++;
        auto start = static_cast<size_t>(offset - cachedOffset_);
        return gopost::Result<std::vector<uint8_t>>::success(
            std::vector<uint8_t>(buffer_.begin() + start, buffer_.begin() + start + length));
    }

    // Buffer miss — fill new aligned block
    auto alignedOffset = (offset / blockSize_) * blockSize_;
    fillBuffer(path, alignedOffset);

    if (buffer_.empty())
        return gopost::Result<std::vector<uint8_t>>::failure("Failed to read file: " + path);

    // Serve from newly filled buffer
    if (offset >= cachedOffset_ &&
        offset + length <= cachedOffset_ + static_cast<int64_t>(buffer_.size())) {
        auto start = static_cast<size_t>(offset - cachedOffset_);
        return gopost::Result<std::vector<uint8_t>>::success(
            std::vector<uint8_t>(buffer_.begin() + start, buffer_.begin() + start + length));
    }

    // Requested range spans beyond one block — direct read
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return gopost::Result<std::vector<uint8_t>>::failure("Cannot open file: " + path);

    file.seekg(offset);
    std::vector<uint8_t> data(static_cast<size_t>(length));
    file.read(reinterpret_cast<char*>(data.data()), length);
    data.resize(static_cast<size_t>(file.gcount()));
    return gopost::Result<std::vector<uint8_t>>::success(std::move(data));
}

} // namespace gopost::io
