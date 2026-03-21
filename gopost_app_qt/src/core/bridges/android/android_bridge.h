#pragma once

#include "core/interfaces/IPlatformBridge.h"

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace gopost::bridges::android {

/// AndroidBridge — IPlatformBridge for Android (JNI).
///
/// Sub-components (to implement):
///   1. JniFilePickerAdapter   — Intent.ACTION_OPEN_DOCUMENT via JNI
///   2. AndroidFileReader      — ContentResolver.openInputStream() via JNI for content:// URIs
///   3. AndroidMetadataReader  — ContentResolver.query() for DocumentsContract columns
///   4. AndroidCacheCopier     — Copy content:// stream to Context.getCacheDir()
///   5. AndroidDirectoryResolver — Context.getCacheDir(), getExternalFilesDir(), etc.
///   6. AndroidStorageInfo     — StatFs on the data directory
///   7. AndroidDeviceInfo      — Build.MODEL, Build.VERSION.RELEASE via JNI
///
/// JNI lifecycle:
///   - JNI_OnLoad() caches the JavaVM pointer
///   - Each method attaches the current thread, calls Java, detaches
///   - Java companion: com.gopost.bridge.NativeBridge
class AndroidBridge : public core::interfaces::IPlatformBridge {
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

#ifdef __ANDROID__
/// Called by the Android runtime when the native library is loaded.
/// Caches the JavaVM* for later JNI calls.
jint JNI_OnLoad(JavaVM* vm, void* reserved);
#endif

} // namespace gopost::bridges::android
