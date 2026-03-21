#include "async_read_pool.h"
#include <algorithm>
#include <chrono>
#include <fstream>

namespace gopost::io {

AsyncReadPool::AsyncReadPool(int poolSize, int bufferSize)
    : bufferSize_(bufferSize) {
    int count = std::max(1, std::min(poolSize, static_cast<int>(std::thread::hardware_concurrency() / 2)));
    for (int i = 0; i < count; ++i)
        workers_.emplace_back(&AsyncReadPool::workerLoop, this);
}

AsyncReadPool::~AsyncReadPool() {
    drain();
}

void AsyncReadPool::drain() {
    running_ = false;
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
}

int64_t AsyncReadPool::submitRead(const std::string& path, int64_t offset, int64_t length,
                                   ReadPriority priority, AsyncReadCallback callback) {
    auto handle = nextHandle_.fetch_add(1);
    {
        std::lock_guard lock(mutex_);
        queue_.push({handle, path, offset, length, priority, std::move(callback)});
    }
    cv_.notify_one();
    return handle;
}

void AsyncReadPool::cancel(int64_t handle) {
    std::lock_guard lock(mutex_);
    cancelled_.insert(handle);
}

std::vector<uint8_t> AsyncReadPool::doRead(const std::string& path, int64_t offset, int64_t length) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};

    auto fileSize = static_cast<int64_t>(file.tellg());
    if (length < 0) length = fileSize - offset;
    if (offset + length > fileSize) length = fileSize - offset;
    if (length <= 0) return {};

    file.seekg(offset);
    std::vector<uint8_t> buf(static_cast<size_t>(length));
    file.read(reinterpret_cast<char*>(buf.data()), length);
    buf.resize(static_cast<size_t>(file.gcount()));
    return buf;
}

void AsyncReadPool::workerLoop() {
    while (running_) {
        ReadJob job;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [&] { return !queue_.empty() || !running_; });
            if (!running_ && queue_.empty()) return;
            if (queue_.empty()) continue;
            job = std::move(const_cast<ReadJob&>(queue_.top()));
            queue_.pop();

            if (cancelled_.count(job.handle)) {
                cancelled_.erase(job.handle);
                continue;
            }
        }

        auto start = std::chrono::steady_clock::now();
        auto data = doRead(job.path, job.offset, job.length);
        auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        // Check cancellation after read
        {
            std::lock_guard lock(mutex_);
            if (cancelled_.count(job.handle)) {
                cancelled_.erase(job.handle);
                continue;
            }
        }

        if (job.callback) {
            AsyncReadResult result;
            result.handle = job.handle;
            result.elapsedMs = elapsed;
            if (data.empty() && job.length > 0)
                result.data = gopost::Result<std::vector<uint8_t>>::failure("Read failed");
            else
                result.data = gopost::Result<std::vector<uint8_t>>::success(std::move(data));
            job.callback(std::move(result));
        }
    }
}

} // namespace gopost::io
