#pragma once

#include "core/interfaces/IPlatformBridge.h"

// POSIX headers — only parsed when compiling on Linux
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>

namespace gopost::bridges::linux_ {

/// LinuxBridge — IPlatformBridge for Linux (X11/Wayland).
///
/// Sub-components (to implement):
///   1. XdgFilePickerAdapter   — xdg-desktop-portal D-Bus → zenity → kdialog fallback
///   2. LinuxFileReader        — POSIX open()/read()/close() with O_RDONLY | O_CLOEXEC
///   3. LinuxMetadataReader    — stat() + extension-based MIME
///   4. LinuxCacheCopier       — sendfile() zero-copy to XDG_CACHE_HOME/gopost/
///   5. LinuxDirectoryResolver — XDG base directories with QStandardPaths fallback
///   6. LinuxStorageInfo       — statvfs()
///   7. LinuxDeviceInfo        — /etc/os-release + hostname
class LinuxBridge : public core::interfaces::IPlatformBridge {
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

} // namespace gopost::bridges::linux_
