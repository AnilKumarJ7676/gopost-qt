#pragma once

#include "core/interfaces/IResult.h"
#include <string>

namespace gopost::bridges {

class PathSanitizer {
public:
    [[nodiscard]] static Result<std::string> sanitize(const std::string& path);
    [[nodiscard]] static bool isValidUtf8(const std::string& s);
};

} // namespace gopost::bridges
