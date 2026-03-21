#pragma once

#include "core/interfaces/IResult.h"
#include <string>

namespace gopost::io {

enum class MediaFileType { Video, Audio, Image, Unknown };

struct ValidationResult {
    bool valid = false;
    MediaFileType type = MediaFileType::Unknown;
    std::string mimeType;
    std::string reason;
};

class FileValidator {
public:
    [[nodiscard]] static ValidationResult validate(const std::string& path);
    [[nodiscard]] static MediaFileType classifyExtension(const std::string& ext);
    [[nodiscard]] static MediaFileType classifyMagicBytes(const uint8_t* data, size_t length);

private:
    static std::string toLower(const std::string& s);
    static std::string getExtension(const std::string& path);
};

} // namespace gopost::io
