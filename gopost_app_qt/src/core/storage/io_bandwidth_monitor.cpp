#include "io_bandwidth_monitor.h"
#include <algorithm>

namespace gopost::storage {

IOBandwidthMonitor::IOBandwidthMonitor(IEventBus& eventBus, double expectedMBps)
    : expectedMBps_(expectedMBps), eventBus_(eventBus) {}

void IOBandwidthMonitor::pruneOldRecords() {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::milliseconds(static_cast<int>(kWindowSeconds * 1000));
    while (!records_.empty() && records_.front().timestamp < cutoff)
        records_.pop_front();
}

void IOBandwidthMonitor::recordCompletion(int64_t bytesRead, double elapsedMs, const std::string& filePath) {
    double mbps = (elapsedMs > 0.01) ? (static_cast<double>(bytesRead) / (1024.0 * 1024.0)) / (elapsedMs / 1000.0) : 0.0;

    std::lock_guard lock(mutex_);

    records_.push_back({std::chrono::steady_clock::now(), bytesRead, elapsedMs});
    pruneOldRecords();

    if (mbps > peakMBps_) peakMBps_ = mbps;

    checkBottleneck(mbps, filePath);
}

void IOBandwidthMonitor::checkBottleneck(double currentMBps, const std::string& filePath) {
    auto now = std::chrono::steady_clock::now();

    if (expectedMBps_ > 0 && currentMBps < expectedMBps_ * kBottleneckThreshold) {
        if (!inBottleneck_) {
            inBottleneck_ = true;
            bottleneckStart_ = now;
        } else {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - bottleneckStart_).count();
            if (elapsed >= kBottleneckDurationMs) {
                QVariantMap data;
                data[QStringLiteral("currentMBps")] = currentMBps;
                data[QStringLiteral("expectedMBps")] = expectedMBps_;
                data[QStringLiteral("affectedFile")] = QString::fromStdString(filePath);
                eventBus_.publish(QStringLiteral("IOBottleneck"), data);
                bottleneckStart_ = now; // reset to avoid flooding
            }
        }
    } else {
        inBottleneck_ = false;
    }
}

BandwidthSnapshot IOBandwidthMonitor::snapshot() const {
    std::lock_guard lock(mutex_);

    BandwidthSnapshot s;
    s.peakMBps = peakMBps_;

    if (records_.empty()) return s;

    // Current = most recent read's throughput
    auto& last = records_.back();
    if (last.elapsedMs > 0.01)
        s.currentMBps = (static_cast<double>(last.bytes) / (1024.0 * 1024.0)) / (last.elapsedMs / 1000.0);

    // Average over window
    double totalBytes = 0;
    double totalMs = 0;
    for (auto& r : records_) {
        totalBytes += r.bytes;
        totalMs += r.elapsedMs;
    }
    if (totalMs > 0.01)
        s.avgMBps = (totalBytes / (1024.0 * 1024.0)) / (totalMs / 1000.0);

    return s;
}

} // namespace gopost::storage
