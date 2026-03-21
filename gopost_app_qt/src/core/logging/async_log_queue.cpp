#include "core/logging/async_log_queue.h"

namespace gopost::logging {

AsyncLogQueue::AsyncLogQueue(int capacity)
    : capacity_(capacity)
{
    buffer_.reserve(capacity_);
    worker_ = std::thread(&AsyncLogQueue::workerLoop, this);
}

AsyncLogQueue::~AsyncLogQueue() {
    flush();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopFlag_ = true;
    }
    cv_.notify_one();
    if (worker_.joinable())
        worker_.join();
}

void AsyncLogQueue::setSink(SinkFunc sink) {
    sink_ = std::move(sink);
}

bool AsyncLogQueue::enqueue(LogEntry entry) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // If full, drop TRACE/DEBUG but always keep WARN+
        if (static_cast<int>(buffer_.size()) >= capacity_) {
            if (entry.level < LogLevel::Warning)
                return false; // dropped
        }

        buffer_.push_back(std::move(entry));
    }
    cv_.notify_one();
    return true;
}

void AsyncLogQueue::flush() {
    // Enqueue a flush signal and wait for it to be processed
    std::vector<LogEntry> batch;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        batch.swap(buffer_);
    }
    // Process remaining entries on calling thread
    if (sink_) {
        for (const auto& entry : batch)
            sink_(entry);
    }
}

void AsyncLogQueue::workerLoop() {
    std::vector<LogEntry> batch;

    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !buffer_.empty() || stopFlag_;
            });

            if (stopFlag_ && buffer_.empty())
                break;

            batch.swap(buffer_);
        }

        if (sink_) {
            for (const auto& entry : batch)
                sink_(entry);
        }
        batch.clear();
    }
}

} // namespace gopost::logging
