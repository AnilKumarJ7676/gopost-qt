#pragma once

#include "core/interfaces/IResult.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace gopost::io {

class WriteCoalescer {
public:
    WriteCoalescer(size_t maxBufferSize = 4 * 1024 * 1024, int flushIntervalMs = 2000);
    ~WriteCoalescer();

    gopost::Result<void> write(const std::string& path, const std::vector<uint8_t>& data, bool critical = false);
    void flush();

private:
    struct PendingWrite {
        std::string path;
        std::vector<uint8_t> data;
    };

    void flushLoop();
    void doFlush(std::vector<PendingWrite>& writes);

    size_t maxBufferSize_;
    int flushIntervalMs_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<PendingWrite> buffer_;
    size_t currentSize_ = 0;
    std::atomic<bool> running_{true};
    std::thread flushThread_;
};

} // namespace gopost::io
