#include "macos_bridge.h"

namespace gopost::bridges::macos {

using namespace core::interfaces;

// TODO: Create NSOpenPanel, configure with options (allowsMultipleSelection, allowedContentTypes).
//       Run on main thread via dispatch_async(dispatch_get_main_queue(), ...).
//       Convert selected URLs to FilePickerResult vector.
Result<std::vector<FilePickerResult>> MacosBridge::showFilePicker(const FilePickerOptions& /*options*/) {
    @autoreleasepool {
        return Result<std::vector<FilePickerResult>>::failure("Not implemented: showFilePicker on macOS");
    }
}

// TODO: Use POSIX open()/read() with O_RDONLY.
//       Use NSFileManager for permission checks before reading.
Result<std::vector<uint8_t>> MacosBridge::readFileBytes(const std::string& /*pathOrUri*/, int64_t /*offset*/, int64_t /*length*/) {
    @autoreleasepool {
        return Result<std::vector<uint8_t>>::failure("Not implemented: readFileBytes on macOS");
    }
}

// TODO: Use [[NSFileManager defaultManager] attributesOfItemAtPath:] for size and dates.
//       Use UTType (UniformTypeIdentifiers framework) for MIME type.
Result<FileMetadata> MacosBridge::getFileMetadata(const std::string& /*pathOrUri*/) {
    @autoreleasepool {
        return Result<FileMetadata>::failure("Not implemented: getFileMetadata on macOS");
    }
}

// TODO: Use NSFileManager copyItemAtPath:toPath:error: to ~/Library/Caches/com.gopost.editor/.
Result<std::string> MacosBridge::copyToAppCache(const std::string& /*sourceUri*/, const std::string& /*destFilename*/) {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: copyToAppCache on macOS");
    }
}

// TODO: Use NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).
//       Append "/com.gopost.editor".
Result<std::string> MacosBridge::getAppCacheDirectory() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getAppCacheDirectory on macOS");
    }
}

// TODO: Use NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).
//       Append "/GoPost".
Result<std::string> MacosBridge::getAppDocumentsDirectory() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getAppDocumentsDirectory on macOS");
    }
}

// TODO: Use sysctlbyname("hw.model") to get hardware model string (e.g. "MacBookPro18,1").
Result<std::string> MacosBridge::getDeviceModel() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getDeviceModel on macOS");
    }
}

// TODO: Use [[NSProcessInfo processInfo] operatingSystemVersionString].
//       Returns e.g. "Version 14.5 (Build 23F79)".
Result<std::string> MacosBridge::getOsVersionString() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getOsVersionString on macOS");
    }
}

// TODO: Use [[NSFileManager defaultManager] attributesOfFileSystemForPath:error:]
//       with NSFileSystemFreeSize key.
Result<uint64_t> MacosBridge::getAvailableStorageBytes() {
    @autoreleasepool {
        return Result<uint64_t>::failure("Not implemented: getAvailableStorageBytes on macOS");
    }
}

// TODO: Use NSTemporaryDirectory() + "GoPost/".
Result<std::string> MacosBridge::getTempDirectory() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getTempDirectory on macOS");
    }
}

} // namespace gopost::bridges::macos
