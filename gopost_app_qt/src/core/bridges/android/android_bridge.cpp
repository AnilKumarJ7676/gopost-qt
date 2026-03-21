#include "android_bridge.h"

namespace gopost::bridges::android {

using namespace core::interfaces;

#ifdef __ANDROID__
static JavaVM* g_jvm = nullptr;

jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}
#endif

// TODO: Launch Intent.ACTION_OPEN_DOCUMENT via JNI.
//       Set MIME type filters from options.allowedExtensions.
//       Set EXTRA_ALLOW_MULTIPLE from options.allowMultiple.
//       Receive result URIs via onActivityResult callback.
Result<std::vector<FilePickerResult>> AndroidBridge::showFilePicker(const FilePickerOptions& /*options*/) {
    return Result<std::vector<FilePickerResult>>::failure("Not implemented: showFilePicker on Android");
}

// TODO: Use ContentResolver.openInputStream() for content:// URIs.
//       For file:// URIs, use POSIX open()/read() directly.
//       Handle offset/length with skip() and bounded reads.
Result<std::vector<uint8_t>> AndroidBridge::readFileBytes(const std::string& /*pathOrUri*/, int64_t /*offset*/, int64_t /*length*/) {
    return Result<std::vector<uint8_t>>::failure("Not implemented: readFileBytes on Android");
}

// TODO: Use ContentResolver.query() with DocumentsContract columns:
//       COLUMN_DISPLAY_NAME, COLUMN_SIZE, COLUMN_MIME_TYPE, COLUMN_LAST_MODIFIED.
Result<FileMetadata> AndroidBridge::getFileMetadata(const std::string& /*pathOrUri*/) {
    return Result<FileMetadata>::failure("Not implemented: getFileMetadata on Android");
}

// TODO: Open content:// input stream via ContentResolver.
//       Write to Context.getCacheDir() / destFilename.
//       Use buffered copy (8KB chunks).
Result<std::string> AndroidBridge::copyToAppCache(const std::string& /*sourceUri*/, const std::string& /*destFilename*/) {
    return Result<std::string>::failure("Not implemented: copyToAppCache on Android");
}

// TODO: Return Context.getCacheDir().getAbsolutePath() + "/gopost" via JNI.
Result<std::string> AndroidBridge::getAppCacheDirectory() {
    return Result<std::string>::failure("Not implemented: getAppCacheDirectory on Android");
}

// TODO: Return Context.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS) via JNI.
Result<std::string> AndroidBridge::getAppDocumentsDirectory() {
    return Result<std::string>::failure("Not implemented: getAppDocumentsDirectory on Android");
}

// TODO: Return Build.MODEL via JNI (e.g. "Pixel 8 Pro").
Result<std::string> AndroidBridge::getDeviceModel() {
    return Result<std::string>::failure("Not implemented: getDeviceModel on Android");
}

// TODO: Return "Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")" via JNI.
Result<std::string> AndroidBridge::getOsVersionString() {
    return Result<std::string>::failure("Not implemented: getOsVersionString on Android");
}

// TODO: Use StatFs on Context.getCacheDir() path.
//       Return statFs.getAvailableBytes().
Result<uint64_t> AndroidBridge::getAvailableStorageBytes() {
    return Result<uint64_t>::failure("Not implemented: getAvailableStorageBytes on Android");
}

// TODO: Return Context.getCacheDir().getAbsolutePath() + "/tmp" via JNI.
Result<std::string> AndroidBridge::getTempDirectory() {
    return Result<std::string>::failure("Not implemented: getTempDirectory on Android");
}

} // namespace gopost::bridges::android
