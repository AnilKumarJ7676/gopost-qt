#include "write_coalescer.h"
#include <fstream>

namespace gopost::io {

WriteCoalescer::WriteCoalescer(size_t maxBufferSize, int flushIntervalMs)
    : maxBufferSize_(maxBufferSize), flushIntervalMs_(flushIntervalMs) {
    flushThread_ = std::thread(&WriteCoalescer::flushLoop, this);
}

WriteCoalescer::~WriteCoalescer() {
    flush();
    running_ = false;
    cv_.notify_all();
    if (flushThread_.joinable()) flushThread_.join();
}

gopost::Result<void> WriteCoalescer::write(const std::string& path, const std::vector<uint8_t>& data, bool critical) {
    {
        std::lock_guard lock(mutex_);
        buffer_.push_back({path, data});
        currentSize_ += data.size();
    }

    if (critical || currentSize_ >= maxBufferSize_) {
        flush();
    }

    return gopost::Result<void>::success();
}

void WriteCoalescer::flush() {
    std::vector<PendingWrite> toFlush;
    {
        std::lock_guard lock(mutex_);
        toFlush.swap(buffer_);
        currentSize_ = 0;
    }
    if (!toFlush.empty()) doFlush(toFlush);
}

void WriteCoalescer::doFlush(std::vector<PendingWrite>& writes) {
    for (auto& w : writes) {
        std::ofstream file(w.path, std::ios::binary | std::ios::app);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(w.data.data()),
                       static_cast<std::streamsize>(w.data.size()));
        }
    }
}

void WriteCoalescer::flushLoop() {
    while (running_) {
        {
            std::unique_lock lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(flushIntervalMs_),
                         [&] { return !running_ || currentSize_ >= maxBufferSize_; });
        }
        if (currentSize_ > 0) flush();
    }
}

} // namespace gopost::io
