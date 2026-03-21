#include "file_validator.h"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace gopost::io {

std::string FileValidator::toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

std::string FileValidator::getExtension(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return {};
    return toLower(path.substr(dot));
}

MediaFileType FileValidator::classifyExtension(const std::string& ext) {
    auto e = toLower(ext);
    if (e == ".mp4" || e == ".mov" || e == ".avi" || e == ".mkv" || e == ".webm" ||
        e == ".m4v" || e == ".flv" || e == ".wmv" || e == ".3gp")
        return MediaFileType::Video;
    if (e == ".jpg" || e == ".jpeg" || e == ".png" || e == ".gif" || e == ".bmp" ||
        e == ".webp" || e == ".heic" || e == ".heif" || e == ".tiff" || e == ".tif")
        return MediaFileType::Image;
    if (e == ".mp3" || e == ".aac" || e == ".wav" || e == ".m4a" || e == ".flac" ||
        e == ".ogg" || e == ".wma" || e == ".opus" || e == ".aiff")
        return MediaFileType::Audio;
    return MediaFileType::Unknown;
}

MediaFileType FileValidator::classifyMagicBytes(const uint8_t* data, size_t length) {
    if (length < 4) return MediaFileType::Unknown;

    // Video formats
    if (length >= 8 && std::memcmp(data + 4, "ftyp", 4) == 0)
        return MediaFileType::Video; // MP4/MOV
    if (length >= 12 && std::memcmp(data, "RIFF", 4) == 0 && std::memcmp(data + 8, "AVI ", 4) == 0)
        return MediaFileType::Video; // AVI
    if (length >= 4 && data[0] == 0x1A && data[1] == 0x45 && data[2] == 0xDF && data[3] == 0xA3)
        return MediaFileType::Video; // MKV/WebM (EBML)

    // Image formats
    if (length >= 4 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        return MediaFileType::Image; // PNG
    if (length >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
        return MediaFileType::Image; // JPEG
    if (length >= 4 && std::memcmp(data, "GIF8", 4) == 0)
        return MediaFileType::Image; // GIF

    // Audio formats
    if (length >= 12 && std::memcmp(data, "RIFF", 4) == 0 && std::memcmp(data + 8, "WAVE", 4) == 0)
        return MediaFileType::Audio; // WAV
    if (length >= 3 && data[0] == 0xFF && (data[1] == 0xFB || data[1] == 0xF3 || data[1] == 0xF2))
        return MediaFileType::Audio; // MP3
    if (length >= 3 && std::memcmp(data, "ID3", 3) == 0)
        return MediaFileType::Audio; // MP3 with ID3 tag
    if (length >= 4 && std::memcmp(data, "fLaC", 4) == 0)
        return MediaFileType::Audio; // FLAC

    return MediaFileType::Unknown;
}

ValidationResult FileValidator::validate(const std::string& path) {
    auto ext = getExtension(path);
    auto extType = classifyExtension(ext);

    // Read first 16 bytes for magic byte check
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return {false, MediaFileType::Unknown, "", "Cannot open file"};

    uint8_t header[16]{};
    file.read(reinterpret_cast<char*>(header), 16);
    auto bytesRead = static_cast<size_t>(file.gcount());

    if (bytesRead < 4)
        return {false, MediaFileType::Unknown, "", "File too small to identify"};

    auto magicType = classifyMagicBytes(header, bytesRead);

    if (magicType == MediaFileType::Unknown && extType == MediaFileType::Unknown)
        return {false, MediaFileType::Unknown, "", "Unrecognized file format"};

    // Use magic bytes type if available, otherwise trust extension
    auto finalType = (magicType != MediaFileType::Unknown) ? magicType : extType;

    std::string mime;
    switch (finalType) {
    case MediaFileType::Video: mime = "video/*"; break;
    case MediaFileType::Audio: mime = "audio/*"; break;
    case MediaFileType::Image: mime = "image/*"; break;
    default: mime = "application/octet-stream"; break;
    }

    return {true, finalType, mime, ""};
}

} // namespace gopost::io
