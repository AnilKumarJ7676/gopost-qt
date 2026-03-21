#pragma once

#include "core/interfaces/IDiagnostics.h"
#include <array>
#include <cstdint>
#include <mutex>

namespace gopost::diagnostics {

using core::interfaces::FrameTimingSnapshot;

class FrameTimer {
public:
    static constexpr int kBufferSize = 120;
    static constexpr double kDropThresholdMs = 16.67;

    void beginFrame();
    void endFrame();

    [[nodiscard]] FrameTimingSnapshot snapshot() const;

private:
    void updateStats();

    mutable std::mutex mutex_;
    uint64_t frameStart_ = 0;

    std::array<double, kBufferSize> frameTimes_{};
    int writeIdx_ = 0;
    int count_ = 0;
    int totalFrames_ = 0;
    int droppedFrames_ = 0;

    // Cached stats (updated on each endFrame)
    FrameTimingSnapshot cached_;
};

} // namespace gopost::diagnostics
