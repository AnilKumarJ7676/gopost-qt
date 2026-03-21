#include "ios_bridge.h"

namespace gopost::bridges::ios {

using namespace core::interfaces;

// TODO: Present UIDocumentPickerViewController with UTType filters.
//       Must be called on main thread.
//       Receive results via UIDocumentPickerDelegate callback.
Result<std::vector<FilePickerResult>> IosBridge::showFilePicker(const FilePickerOptions& /*options*/) {
    @autoreleasepool {
        return Result<std::vector<FilePickerResult>>::failure("Not implemented: showFilePicker on iOS");
    }
}

// TODO: Use NSFileManager + NSData contentsOfFile for sandbox-accessible files.
//       For document picker URIs, start security-scoped access first:
//       [url startAccessingSecurityScopedResource] ... [url stopAccessingSecurityScopedResource]
Result<std::vector<uint8_t>> IosBridge::readFileBytes(const std::string& /*pathOrUri*/, int64_t /*offset*/, int64_t /*length*/) {
    @autoreleasepool {
        return Result<std::vector<uint8_t>>::failure("Not implemented: readFileBytes on iOS");
    }
}

// TODO: Use [[NSFileManager defaultManager] attributesOfItemAtPath:] for size and dates.
//       Use UTType for MIME type from file extension.
Result<FileMetadata> IosBridge::getFileMetadata(const std::string& /*pathOrUri*/) {
    @autoreleasepool {
        return Result<FileMetadata>::failure("Not implemented: getFileMetadata on iOS");
    }
}

// TODO: Copy to app sandbox Caches directory via NSFileManager copyItemAtURL:toURL:error:.
//       Must handle security-scoped access for external files.
Result<std::string> IosBridge::copyToAppCache(const std::string& /*sourceUri*/, const std::string& /*destFilename*/) {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: copyToAppCache on iOS");
    }
}

// TODO: Use NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).
//       Returns app sandbox caches path.
Result<std::string> IosBridge::getAppCacheDirectory() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getAppCacheDirectory on iOS");
    }
}

// TODO: Use NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).
//       This is the app's sandboxed Documents directory.
Result<std::string> IosBridge::getAppDocumentsDirectory() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getAppDocumentsDirectory on iOS");
    }
}

// TODO: Return [[UIDevice currentDevice] model] (e.g. "iPhone", "iPad").
//       For specific model, use sysctlbyname("hw.machine").
Result<std::string> IosBridge::getDeviceModel() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getDeviceModel on iOS");
    }
}

// TODO: Return "iOS " + [[UIDevice currentDevice] systemVersion] (e.g. "iOS 17.5").
Result<std::string> IosBridge::getOsVersionString() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getOsVersionString on iOS");
    }
}

// TODO: Use [[NSFileManager defaultManager] attributesOfFileSystemForPath:error:]
//       with NSFileSystemFreeSize key on the Documents directory.
Result<uint64_t> IosBridge::getAvailableStorageBytes() {
    @autoreleasepool {
        return Result<uint64_t>::failure("Not implemented: getAvailableStorageBytes on iOS");
    }
}

// TODO: Use NSTemporaryDirectory(). iOS manages this automatically.
Result<std::string> IosBridge::getTempDirectory() {
    @autoreleasepool {
        return Result<std::string>::failure("Not implemented: getTempDirectory on iOS");
    }
}

} // namespace gopost::bridges::ios
