#pragma once

#include <cstdint>

namespace gopost::platform {

class RamDetector {
public:
    static int64_t detect();
};

} // namespace gopost::platform
