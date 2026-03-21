#pragma once

#include "core/interfaces/ILogger.h"
#include <QString>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace gopost::logging {

using core::interfaces::LogLevel;
using core::interfaces::LogCategory;

struct LogEntry {
    LogLevel level      = LogLevel::Info;
    LogCategory category = LogCategory::General;
    QString message;
    QString module;
    uint64_t timestampMs = 0;
};

class AsyncLogQueue {
public:
    using SinkFunc = std::function<void(const LogEntry&)>;

    explicit AsyncLogQueue(int capacity = 4096);
    ~AsyncLogQueue();

    void setSink(SinkFunc sink);
    bool enqueue(LogEntry entry);
    void flush();

private:
    void workerLoop();

    SinkFunc sink_;
    int capacity_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<LogEntry> buffer_;
    bool stopFlag_ = false;

    std::thread worker_;
};

} // namespace gopost::logging
