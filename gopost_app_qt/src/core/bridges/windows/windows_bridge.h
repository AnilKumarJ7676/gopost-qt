#pragma once

#include "core/interfaces/IPlatformBridge.h"

namespace gopost::bridges::windows {

class WindowsBridge : public core::interfaces::IPlatformBridge {
public:
    // File Access
    Result<std::vector<core::interfaces::FilePickerResult>> showFilePicker(const core::interfaces::FilePickerOptions& options) override;
    Result<std::vector<uint8_t>> readFileBytes(const std::string& pathOrUri, int64_t offset, int64_t length) override;
    Result<core::interfaces::FileMetadata> getFileMetadata(const std::string& pathOrUri) override;
    Result<std::string> copyToAppCache(const std::string& sourceUri, const std::string& destFilename) override;
    Result<std::string> getAppCacheDirectory() override;
    Result<std::string> getAppDocumentsDirectory() override;

    // System Info
    Result<std::string> getDeviceModel() override;
    Result<std::string> getOsVersionString() override;
    Result<uint64_t> getAvailableStorageBytes() override;
    Result<std::string> getTempDirectory() override;
};

} // namespace gopost::bridges::windows
