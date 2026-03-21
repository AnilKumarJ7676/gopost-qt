#pragma once

#include "core/interfaces/ILogger.h"
#include <cstddef>
#include <cstdint>

namespace gopost::mmap {

using core::interfaces::ILogger;

class AdvisoryHintManager {
public:
    explicit AdvisoryHintManager(ILogger& logger);

    void sequential(const uint8_t* addr, size_t length);
    void willNeed(const uint8_t* addr, size_t length);
    void dontNeed(const uint8_t* addr, size_t length);

private:
    ILogger& logger_;
};

} // namespace gopost::mmap
