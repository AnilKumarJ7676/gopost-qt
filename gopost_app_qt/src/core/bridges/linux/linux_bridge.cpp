#include "linux_bridge.h"

namespace gopost::bridges::linux_ {

using namespace core::interfaces;

// TODO: Use org.freedesktop.portal.FileChooser D-Bus interface.
//       Fallback chain: xdg-desktop-portal → zenity --file-selection → kdialog --getopenfilename.
//       Return Result::failure if NO picker is available.
Result<std::vector<FilePickerResult>> LinuxBridge::showFilePicker(const FilePickerOptions& /*options*/) {
    return Result<std::vector<FilePickerResult>>::failure("Not implemented: showFilePicker on Linux");
}

// TODO: Use POSIX open() with O_RDONLY | O_CLOEXEC, then read() in a loop.
//       Check file existence and permissions before reading.
//       Return Result::failure with errno description on error.
Result<std::vector<uint8_t>> LinuxBridge::readFileBytes(const std::string& /*pathOrUri*/, int64_t /*offset*/, int64_t /*length*/) {
    return Result<std::vector<uint8_t>>::failure("Not implemented: readFileBytes on Linux");
}

// TODO: Use stat() for size and modified time.
//       Determine MIME type from file extension (or libmagic if available).
Result<FileMetadata> LinuxBridge::getFileMetadata(const std::string& /*pathOrUri*/) {
    return Result<FileMetadata>::failure("Not implemented: getFileMetadata on Linux");
}

// TODO: Use sendfile() for zero-copy to XDG_CACHE_HOME/gopost/.
//       Create cache directory if it doesn't exist.
//       Generate unique filename on collision.
Result<std::string> LinuxBridge::copyToAppCache(const std::string& /*sourceUri*/, const std::string& /*destFilename*/) {
    return Result<std::string>::failure("Not implemented: copyToAppCache on Linux");
}

// TODO: Return $XDG_CACHE_HOME/gopost/ (fallback ~/.cache/gopost/).
//       Use QStandardPaths as secondary fallback.
Result<std::string> LinuxBridge::getAppCacheDirectory() {
    return Result<std::string>::failure("Not implemented: getAppCacheDirectory on Linux");
}

// TODO: Return $XDG_DOCUMENTS_DIR/gopost/ (fallback ~/Documents/gopost/).
Result<std::string> LinuxBridge::getAppDocumentsDirectory() {
    return Result<std::string>::failure("Not implemented: getAppDocumentsDirectory on Linux");
}

// TODO: Read hostname via gethostname() or /etc/hostname.
Result<std::string> LinuxBridge::getDeviceModel() {
    return Result<std::string>::failure("Not implemented: getDeviceModel on Linux");
}

// TODO: Parse /etc/os-release for PRETTY_NAME (e.g. "Ubuntu 24.04 LTS").
//       Fallback to uname -r for kernel version.
Result<std::string> LinuxBridge::getOsVersionString() {
    return Result<std::string>::failure("Not implemented: getOsVersionString on Linux");
}

// TODO: Use statvfs() on the cache directory to query available space.
Result<uint64_t> LinuxBridge::getAvailableStorageBytes() {
    return Result<uint64_t>::failure("Not implemented: getAvailableStorageBytes on Linux");
}

// TODO: Return /tmp/gopost-{pid}/.
//       Create the directory if it doesn't exist.
Result<std::string> LinuxBridge::getTempDirectory() {
    return Result<std::string>::failure("Not implemented: getTempDirectory on Linux");
}

} // namespace gopost::bridges::linux_
