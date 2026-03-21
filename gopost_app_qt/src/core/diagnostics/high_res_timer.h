#pragma once

#include <chrono>
#include <cstdint>

namespace gopost::diagnostics {

class HighResTimer {
public:
    static uint64_t now() {
        return static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
    }

    static double elapsedMs(uint64_t startNs, uint64_t endNs) {
        return static_cast<double>(endNs - startNs) / 1'000'000.0;
    }
};

} // namespace gopost::diagnostics
