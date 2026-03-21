#include "core/diagnostics/frame_timer.h"
#include "core/diagnostics/high_res_timer.h"

#include <algorithm>
#include <numeric>

namespace gopost::diagnostics {

void FrameTimer::beginFrame() {
    frameStart_ = HighResTimer::now();
}

void FrameTimer::endFrame() {
    double ms = HighResTimer::elapsedMs(frameStart_, HighResTimer::now());

    std::lock_guard<std::mutex> lock(mutex_);
    frameTimes_[writeIdx_] = ms;
    writeIdx_ = (writeIdx_ + 1) % kBufferSize;
    if (count_ < kBufferSize) ++count_;
    ++totalFrames_;
    if (ms > kDropThresholdMs) ++droppedFrames_;

    updateStats();
}

void FrameTimer::updateStats() {
    if (count_ == 0) return;

    // Compute avg
    double sum = 0;
    for (int i = 0; i < count_; ++i)
        sum += frameTimes_[i];
    double avg = sum / count_;

    // Compute p95
    std::array<double, kBufferSize> sorted;
    std::copy_n(frameTimes_.begin(), count_, sorted.begin());
    std::sort(sorted.begin(), sorted.begin() + count_);
    int p95Idx = static_cast<int>(count_ * 0.95);
    if (p95Idx >= count_) p95Idx = count_ - 1;

    // Last frame time
    int lastIdx = (writeIdx_ - 1 + kBufferSize) % kBufferSize;

    cached_.fps = (avg > 0.0) ? (1000.0 / avg) : 0.0;
    cached_.lastFrameMs = frameTimes_[lastIdx];
    cached_.avgFrameMs = avg;
    cached_.p95FrameMs = sorted[p95Idx];
    cached_.totalFrames = totalFrames_;
    cached_.droppedFrames = droppedFrames_;
}

FrameTimingSnapshot FrameTimer::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cached_;
}

} // namespace gopost::diagnostics
