#pragma once

#include "core/interfaces/IPlatformBridge.h"

#ifdef __APPLE__
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#endif

namespace gopost::bridges::macos {

/// MacosBridge — IPlatformBridge for macOS (AppKit + Foundation).
///
/// Sub-components (to implement):
///   1. NSOpenPanelAdapter     — NSOpenPanel for file picker
///   2. MacosFileReader        — POSIX read with NSFileManager permission checks
///   3. MacosMetadataReader    — NSFileManager attributesOfItemAtPath + UTType for MIME
///   4. MacosCacheCopier       — NSFileManager copyItemAtPath to ~/Library/Caches/com.gopost.editor/
///   5. MacosDirectoryResolver — NSSearchPathForDirectoriesInDomains for standard paths
///   6. MacosStorageInfo       — NSFileManager attributesOfFileSystemForPath
///   7. MacosDeviceInfo        — sysctlbyname("hw.model") + NSProcessInfo operatingSystemVersionString
///
/// All ObjC calls must be wrapped in @autoreleasepool {}.
/// NSString ↔ std::string conversion at the bridge boundary.
class MacosBridge : public core::interfaces::IPlatformBridge {
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

} // namespace gopost::bridges::macos
