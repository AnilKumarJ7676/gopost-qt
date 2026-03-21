#pragma once

#include "IResult.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gopost::core::interfaces {

struct FilePickerOptions {
    std::vector<std::string> allowedExtensions;
    bool allowMultiple = false;
    std::string title;
};

struct FilePickerResult {
    std::string uri;
    std::string displayName;
    uint64_t sizeBytes = 0;
    std::string mimeType;
};

struct FileMetadata {
    std::string name;
    uint64_t sizeBytes = 0;
    std::string mimeType;
    uint64_t lastModified = 0; // epoch ms
    bool isReadOnly = false;
};

class IPlatformBridge {
public:
    virtual ~IPlatformBridge() = default;

    // File Access
    [[nodiscard]] virtual Result<std::vector<FilePickerResult>> showFilePicker(const FilePickerOptions& options) = 0;
    [[nodiscard]] virtual Result<std::vector<uint8_t>> readFileBytes(const std::string& pathOrUri, int64_t offset = 0, int64_t length = -1) = 0;
    [[nodiscard]] virtual Result<FileMetadata> getFileMetadata(const std::string& pathOrUri) = 0;
    [[nodiscard]] virtual Result<std::string> copyToAppCache(const std::string& sourceUri, const std::string& destFilename) = 0;
    [[nodiscard]] virtual Result<std::string> getAppCacheDirectory() = 0;
    [[nodiscard]] virtual Result<std::string> getAppDocumentsDirectory() = 0;

    // System Info
    [[nodiscard]] virtual Result<std::string> getDeviceModel() = 0;
    [[nodiscard]] virtual Result<std::string> getOsVersionString() = 0;
    [[nodiscard]] virtual Result<uint64_t> getAvailableStorageBytes() = 0;
    [[nodiscard]] virtual Result<std::string> getTempDirectory() = 0;
};

} // namespace gopost::core::interfaces
