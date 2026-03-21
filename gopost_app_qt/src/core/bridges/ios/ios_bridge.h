#pragma once

#include "core/interfaces/IPlatformBridge.h"

#ifdef __APPLE__
#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>
#endif

namespace gopost::bridges::ios {

/// IosBridge — IPlatformBridge for iOS (UIKit + Foundation).
///
/// Sub-components (to implement):
///   1. UIDocPickerAdapter     — UIDocumentPickerViewController for file picker
///   2. IosFileReader          — NSFileManager + POSIX read for sandbox-accessible files
///   3. IosMetadataReader      — NSFileManager attributesOfItemAtPath + UTType for MIME
///   4. IosCacheCopier         — NSFileManager copyItemAtURL to app sandbox Caches/
///   5. IosDirectoryResolver   — NSSearchPathForDirectoriesInDomains (sandbox-aware)
///   6. IosStorageInfo          — NSFileManager attributesOfFileSystemForPath
///   7. IosDeviceInfo           — UIDevice.currentDevice.model, systemVersion
///
/// Shares some Foundation-based utilities with MacosBridge via bridges/apple/ (future).
/// All ObjC calls must be wrapped in @autoreleasepool {}.
class IosBridge : public core::interfaces::IPlatformBridge {
public:
    Result<std::vector<core::interfaces::FilePickerResult>> showFilePicker(const core::interfaces::FilePickerOptions& options) override;
    Result<std::vector<uint8_t>> readFileBytes(const std::string& pathOrUri, int64_t offset, int64_t length) override;
    Result<core::interfaces::FileMetadata> getFileMetadata(const std::string& pathOrUri) override;
    Result<std::string> copyToAppCache(const std::string& sourceUri, const std::string& destFilename) override;
    Result<std::string> getAppCacheDirectory() override;
    Result<std::string> getAppDocumentsDirectory() override;
    Result<std::string> getDeviceModel() override;
    Result<std::string> getOsVersionString() override;
    Result<uint64_t> getAvailableStorageBytes() override;
    Result<std::string> getTempDirectory() override;
};

} // namespace gopost::bridges::ios
