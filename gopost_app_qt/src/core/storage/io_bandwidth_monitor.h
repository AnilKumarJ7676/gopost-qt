#pragma once

#include "core/interfaces/IEventBus.h"

#include <chrono>
#include <deque>
#include <mutex>
#include <string>

namespace gopost::storage {

using core::interfaces::IEventBus;

struct BandwidthSnapshot {
    double currentMBps = 0.0;
    double avgMBps = 0.0;
    double peakMBps = 0.0;
};

class IOBandwidthMonitor {
public:
    explicit IOBandwidthMonitor(IEventBus& eventBus, double expectedMBps);

    void recordCompletion(int64_t bytesRead, double elapsedMs, const std::string& filePath);
    [[nodiscard]] BandwidthSnapshot snapshot() const;

private:
    static constexpr double kWindowSeconds = 30.0;
    static constexpr double kBottleneckThreshold = 0.5; // 50% of expected
    static constexpr int kBottleneckDurationMs = 3000;

    struct ReadRecord {
        std::chrono::steady_clock::time_point timestamp;
        int64_t bytes;
        double elapsedMs;
    };

    mutable std::mutex mutex_;
    std::deque<ReadRecord> records_;
    double peakMBps_ = 0.0;
    double expectedMBps_;
    IEventBus& eventBus_;
    std::chrono::steady_clock::time_point bottleneckStart_{};
    bool inBottleneck_ = false;

    void pruneOldRecords();
    void checkBottleneck(double currentMBps, const std::string& filePath);
};

} // namespace gopost::storage
