#pragma once

#include "core/interfaces/IResult.h"
#include "core/interfaces/IStorageScheduler.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

namespace gopost::io {

using core::interfaces::ReadPriority;

struct AsyncReadResult {
    int64_t handle;
    gopost::Result<std::vector<uint8_t>> data;
    double elapsedMs;
};

using AsyncReadCallback = std::function<void(AsyncReadResult)>;

class AsyncReadPool {
public:
    AsyncReadPool(int poolSize, int bufferSize);
    ~AsyncReadPool();

    int64_t submitRead(const std::string& path, int64_t offset, int64_t length,
                       ReadPriority priority, AsyncReadCallback callback);
    void cancel(int64_t handle);
    void drain();

private:
    struct ReadJob {
        int64_t handle;
        std::string path;
        int64_t offset;
        int64_t length;
        ReadPriority priority;
        AsyncReadCallback callback;

        bool operator>(const ReadJob& other) const {
            return static_cast<int>(priority) > static_cast<int>(other.priority);
        }
    };

    void workerLoop();
    static std::vector<uint8_t> doRead(const std::string& path, int64_t offset, int64_t length);

    int bufferSize_;
    std::atomic<int64_t> nextHandle_{1};
    std::atomic<bool> running_{true};

    std::mutex mutex_;
    std::condition_variable cv_;
    std::priority_queue<ReadJob, std::vector<ReadJob>, std::greater<ReadJob>> queue_;
    std::unordered_set<int64_t> cancelled_;
    std::vector<std::thread> workers_;
};

} // namespace gopost::io
