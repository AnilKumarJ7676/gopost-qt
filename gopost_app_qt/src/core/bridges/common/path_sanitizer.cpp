#include "path_sanitizer.h"
#include <algorithm>

namespace gopost::bridges {

bool PathSanitizer::isValidUtf8(const std::string& s) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0;
    while (i < s.size()) {
        if (bytes[i] <= 0x7F) {
            ++i;
        } else if ((bytes[i] & 0xE0) == 0xC0) {
            if (i + 1 >= s.size() || (bytes[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((bytes[i] & 0xF0) == 0xE0) {
            if (i + 2 >= s.size() || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if ((bytes[i] & 0xF8) == 0xF0) {
            if (i + 3 >= s.size() || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

Result<std::string> PathSanitizer::sanitize(const std::string& path) {
    if (path.empty())
        return Result<std::string>::failure("Empty path");

    if (!isValidUtf8(path))
        return Result<std::string>::failure("Invalid UTF-8 in path");

    // Reject null bytes
    if (path.find('\0') != std::string::npos)
        return Result<std::string>::failure("Path contains null bytes");

    std::string result = path;

    // Strip leading/trailing whitespace
    while (!result.empty() && (result.front() == ' ' || result.front() == '\t'))
        result.erase(result.begin());
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t'))
        result.pop_back();

    if (result.empty())
        return Result<std::string>::failure("Path is whitespace-only");

    // Normalize: on Windows keep backslashes, on others convert to forward slashes
#ifndef _WIN32
    std::replace(result.begin(), result.end(), '\\', '/');
#endif

    return Result<std::string>::success(std::move(result));
}

} // namespace gopost::bridges
