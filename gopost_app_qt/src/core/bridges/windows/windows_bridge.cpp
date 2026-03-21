#include "windows_bridge.h"
#include "core/bridges/common/path_sanitizer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <commdlg.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace gopost::bridges::windows {

using namespace core::interfaces;

// ============================================================================
// Component 1: UTF-8 ↔ UTF-16 helpers
// ============================================================================

static std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (len <= 0) return {};
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), result.data(), len);
    return result;
}

static std::string wideToUtf8(const std::wstring& s) {
    if (s.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), result.data(), len, nullptr, nullptr);
    return result;
}

static uint64_t fileTimeToEpochMs(FILETIME ft) {
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    // FILETIME is 100-ns intervals since 1601-01-01. Epoch is 1970-01-01.
    constexpr uint64_t epochDiff = 116444736000000000ULL;
    if (ull.QuadPart < epochDiff) return 0;
    return (ull.QuadPart - epochDiff) / 10000ULL;
}

static std::string mimeFromExtension(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return "application/octet-stream";
    auto ext = path.substr(dot);
    std::string lower;
    lower.resize(ext.size());
    std::transform(ext.begin(), ext.end(), lower.begin(), [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

    if (lower == ".mp4" || lower == ".m4v") return "video/mp4";
    if (lower == ".mov") return "video/quicktime";
    if (lower == ".avi") return "video/x-msvideo";
    if (lower == ".mkv") return "video/x-matroska";
    if (lower == ".webm") return "video/webm";
    if (lower == ".wmv") return "video/x-ms-wmv";
    if (lower == ".flv") return "video/x-flv";
    if (lower == ".3gp") return "video/3gpp";
    if (lower == ".jpg" || lower == ".jpeg") return "image/jpeg";
    if (lower == ".png") return "image/png";
    if (lower == ".gif") return "image/gif";
    if (lower == ".bmp") return "image/bmp";
    if (lower == ".webp") return "image/webp";
    if (lower == ".tiff" || lower == ".tif") return "image/tiff";
    if (lower == ".heic" || lower == ".heif") return "image/heif";
    if (lower == ".mp3") return "audio/mpeg";
    if (lower == ".wav") return "audio/wav";
    if (lower == ".aac") return "audio/aac";
    if (lower == ".m4a") return "audio/mp4";
    if (lower == ".flac") return "audio/flac";
    if (lower == ".ogg") return "audio/ogg";
    if (lower == ".wma") return "audio/x-ms-wma";
    if (lower == ".opus") return "audio/opus";
    if (lower == ".aiff") return "audio/aiff";
    if (lower == ".cpp" || lower == ".h" || lower == ".txt") return "text/plain";
    return "application/octet-stream";
}

// ============================================================================
// Component 2: ComFileDialogAdapter — IFileOpenDialog (COM)
// ============================================================================

struct ComInit {
    HRESULT hr;
    ComInit() { hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); }
    ~ComInit() { if (SUCCEEDED(hr)) CoUninitialize(); }
    bool ok() const { return SUCCEEDED(hr); }
};

Result<std::vector<FilePickerResult>> WindowsBridge::showFilePicker(const FilePickerOptions& options) {
    ComInit com;
    if (!com.ok())
        return Result<std::vector<FilePickerResult>>::failure("COM initialization failed");

    IFileOpenDialog* pFileOpen = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                                  IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    if (FAILED(hr))
        return Result<std::vector<FilePickerResult>>::failure("Failed to create file dialog");

    // Title
    if (!options.title.empty()) {
        auto wTitle = utf8ToWide(options.title);
        pFileOpen->SetTitle(wTitle.c_str());
    }

    // Multi-select
    DWORD dwFlags = 0;
    pFileOpen->GetOptions(&dwFlags);
    if (options.allowMultiple)
        dwFlags |= FOS_ALLOWMULTISELECT;
    dwFlags |= FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST;
    pFileOpen->SetOptions(dwFlags);

    // File type filters
    if (!options.allowedExtensions.empty()) {
        std::wstring filterSpec;
        for (size_t i = 0; i < options.allowedExtensions.size(); ++i) {
            if (i > 0) filterSpec += L";";
            auto ext = options.allowedExtensions[i];
            if (!ext.empty() && ext[0] == '.') ext = "*" + ext;
            else if (!ext.empty() && ext[0] != '*') ext = "*." + ext;
            filterSpec += utf8ToWide(ext);
        }
        COMDLG_FILTERSPEC rgSpec[] = {
            { L"Supported Files", filterSpec.c_str() },
            { L"All Files", L"*.*" }
        };
        pFileOpen->SetFileTypes(2, rgSpec);
    }

    hr = pFileOpen->Show(nullptr);
    if (FAILED(hr)) {
        pFileOpen->Release();
        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return Result<std::vector<FilePickerResult>>::success({});
        return Result<std::vector<FilePickerResult>>::failure("File dialog cancelled or failed");
    }

    std::vector<FilePickerResult> results;
    IShellItemArray* pItems = nullptr;
    hr = pFileOpen->GetResults(&pItems);
    if (SUCCEEDED(hr) && pItems) {
        DWORD count = 0;
        pItems->GetCount(&count);
        for (DWORD i = 0; i < count; ++i) {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pItems->GetItemAt(i, &pItem))) {
                PWSTR pszPath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    auto path = wideToUtf8(pszPath);
                    CoTaskMemFree(pszPath);

                    PWSTR pszName = nullptr;
                    std::string displayName;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName))) {
                        displayName = wideToUtf8(pszName);
                        CoTaskMemFree(pszName);
                    }

                    // Get file size
                    WIN32_FILE_ATTRIBUTE_DATA fad{};
                    auto wpath = utf8ToWide(path);
                    uint64_t sz = 0;
                    if (GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad)) {
                        ULARGE_INTEGER ull;
                        ull.LowPart = fad.nFileSizeLow;
                        ull.HighPart = fad.nFileSizeHigh;
                        sz = ull.QuadPart;
                    }

                    results.push_back({path, displayName, sz, mimeFromExtension(path)});
                }
                pItem->Release();
            }
        }
        pItems->Release();
    }

    pFileOpen->Release();
    return Result<std::vector<FilePickerResult>>::success(std::move(results));
}

// ============================================================================
// Component 3: WindowsFileReader — CreateFileW + ReadFile
// ============================================================================

Result<std::vector<uint8_t>> WindowsBridge::readFileBytes(const std::string& pathOrUri, int64_t offset, int64_t length) {
    auto sanitized = PathSanitizer::sanitize(pathOrUri);
    if (!sanitized.ok())
        return Result<std::vector<uint8_t>>::failure(sanitized.error);

    auto wpath = utf8ToWide(sanitized.get());
    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        return Result<std::vector<uint8_t>>::failure("CreateFileW failed, error " + std::to_string(err), static_cast<int>(err));
    }

    // Get file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return Result<std::vector<uint8_t>>::failure("GetFileSizeEx failed");
    }

    int64_t actualOffset = (offset >= 0) ? offset : 0;
    int64_t actualLength = length;
    if (actualLength < 0)
        actualLength = fileSize.QuadPart - actualOffset;
    if (actualOffset + actualLength > fileSize.QuadPart)
        actualLength = fileSize.QuadPart - actualOffset;
    if (actualLength <= 0) {
        CloseHandle(hFile);
        return Result<std::vector<uint8_t>>::success({});
    }

    // Seek
    LARGE_INTEGER seekPos;
    seekPos.QuadPart = actualOffset;
    if (!SetFilePointerEx(hFile, seekPos, nullptr, FILE_BEGIN)) {
        CloseHandle(hFile);
        return Result<std::vector<uint8_t>>::failure("SetFilePointerEx failed");
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(actualLength));
    DWORD bytesRead = 0;
    size_t totalRead = 0;

    while (totalRead < buffer.size()) {
        DWORD toRead = static_cast<DWORD>(std::min<size_t>(buffer.size() - totalRead, 1 << 30));
        if (!ReadFile(hFile, buffer.data() + totalRead, toRead, &bytesRead, nullptr) || bytesRead == 0) {
            break;
        }
        totalRead += bytesRead;
    }

    CloseHandle(hFile);
    buffer.resize(totalRead);
    return Result<std::vector<uint8_t>>::success(std::move(buffer));
}

// ============================================================================
// Component 4: WindowsMetadataReader — GetFileAttributesExW
// ============================================================================

Result<FileMetadata> WindowsBridge::getFileMetadata(const std::string& pathOrUri) {
    auto sanitized = PathSanitizer::sanitize(pathOrUri);
    if (!sanitized.ok())
        return Result<FileMetadata>::failure(sanitized.error);

    auto wpath = utf8ToWide(sanitized.get());
    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fad))
        return Result<FileMetadata>::failure("GetFileAttributesExW failed, error " + std::to_string(GetLastError()));

    FileMetadata meta;

    // Extract filename from path
    auto lastSlash = sanitized.get().find_last_of("/\\");
    meta.name = (lastSlash != std::string::npos) ? sanitized.get().substr(lastSlash + 1) : sanitized.get();

    ULARGE_INTEGER ull;
    ull.LowPart = fad.nFileSizeLow;
    ull.HighPart = fad.nFileSizeHigh;
    meta.sizeBytes = ull.QuadPart;
    meta.mimeType = mimeFromExtension(sanitized.get());
    meta.lastModified = fileTimeToEpochMs(fad.ftLastWriteTime);
    meta.isReadOnly = (fad.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;

    return Result<FileMetadata>::success(std::move(meta));
}

// ============================================================================
// Component 5: WindowsCacheCopier — CopyFileExW
// ============================================================================

Result<std::string> WindowsBridge::copyToAppCache(const std::string& sourceUri, const std::string& destFilename) {
    auto cacheDir = getAppCacheDirectory();
    if (!cacheDir.ok())
        return Result<std::string>::failure("Cannot resolve cache dir: " + cacheDir.error);

    auto sanitizedSrc = PathSanitizer::sanitize(sourceUri);
    if (!sanitizedSrc.ok())
        return Result<std::string>::failure(sanitizedSrc.error);

    // Ensure cache directory exists
    auto wcacheDir = utf8ToWide(cacheDir.get());
    CreateDirectoryW(wcacheDir.c_str(), nullptr);

    std::string destPath = cacheDir.get() + "\\" + destFilename;
    auto wsrc = utf8ToWide(sanitizedSrc.get());
    auto wdst = utf8ToWide(destPath);

    if (!CopyFileExW(wsrc.c_str(), wdst.c_str(), nullptr, nullptr, nullptr, 0)) {
        DWORD err = GetLastError();
        return Result<std::string>::failure("CopyFileExW failed, error " + std::to_string(err), static_cast<int>(err));
    }

    return Result<std::string>::success(std::move(destPath));
}

// ============================================================================
// Component 6: WindowsDirectoryResolver — SHGetKnownFolderPath
// ============================================================================

static Result<std::string> getKnownFolder(const KNOWNFOLDERID& folderId, const std::string& subdir) {
    PWSTR pszPath = nullptr;
    HRESULT hr = SHGetKnownFolderPath(folderId, 0, nullptr, &pszPath);
    if (FAILED(hr))
        return Result<std::string>::failure("SHGetKnownFolderPath failed");

    auto base = wideToUtf8(pszPath);
    CoTaskMemFree(pszPath);

    if (!subdir.empty())
        base += "\\" + subdir;

    // Ensure it exists
    auto wbase = utf8ToWide(base);
    CreateDirectoryW(wbase.c_str(), nullptr);

    return Result<std::string>::success(std::move(base));
}

Result<std::string> WindowsBridge::getAppCacheDirectory() {
    return getKnownFolder(FOLDERID_LocalAppData, "GoPost\\cache");
}

Result<std::string> WindowsBridge::getAppDocumentsDirectory() {
    return getKnownFolder(FOLDERID_Documents, "GoPost");
}

Result<std::string> WindowsBridge::getTempDirectory() {
    wchar_t buf[MAX_PATH + 1];
    DWORD len = GetTempPathW(MAX_PATH + 1, buf);
    if (len == 0)
        return Result<std::string>::failure("GetTempPathW failed");

    auto base = wideToUtf8(std::wstring(buf, len));
    // Remove trailing backslash for consistency
    while (!base.empty() && (base.back() == '\\' || base.back() == '/'))
        base.pop_back();
    base += "\\GoPost";

    auto wbase = utf8ToWide(base);
    CreateDirectoryW(wbase.c_str(), nullptr);

    return Result<std::string>::success(std::move(base));
}

// ============================================================================
// Component 7: WindowsStorageInfo — GetDiskFreeSpaceExW
// ============================================================================

Result<uint64_t> WindowsBridge::getAvailableStorageBytes() {
    // Query the drive where the app's cache lives
    auto cacheDir = getAppCacheDirectory();
    std::wstring queryPath;
    if (cacheDir.ok()) {
        queryPath = utf8ToWide(cacheDir.get());
    } else {
        queryPath = L"C:\\";
    }

    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (!GetDiskFreeSpaceExW(queryPath.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
        return Result<uint64_t>::failure("GetDiskFreeSpaceExW failed, error " + std::to_string(GetLastError()));

    return Result<uint64_t>::success(freeBytesAvailable.QuadPart);
}

// ============================================================================
// Component 8: WindowsDeviceInfo — registry + GetComputerNameW
// ============================================================================

Result<std::string> WindowsBridge::getDeviceModel() {
    wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(buf, &size))
        return Result<std::string>::failure("GetComputerNameW failed");
    return Result<std::string>::success(wideToUtf8(std::wstring(buf, size)));
}

Result<std::string> WindowsBridge::getOsVersionString() {
    // Read from registry — RtlGetVersion is more reliable but requires ntdll linkage
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                       0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return Result<std::string>::failure("Cannot read OS version from registry");
    }

    auto readReg = [&](const wchar_t* name) -> std::string {
        wchar_t buf[256]{};
        DWORD size = sizeof(buf);
        DWORD type = 0;
        if (RegQueryValueExW(hKey, name, nullptr, &type, reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS
            && type == REG_SZ) {
            return wideToUtf8(buf);
        }
        return {};
    };

    auto readRegDword = [&](const wchar_t* name) -> int {
        DWORD val = 0, size = sizeof(val), type = 0;
        if (RegQueryValueExW(hKey, name, nullptr, &type, reinterpret_cast<LPBYTE>(&val), &size) == ERROR_SUCCESS
            && type == REG_DWORD) {
            return static_cast<int>(val);
        }
        return -1;
    };

    auto productName = readReg(L"ProductName");
    auto displayVersion = readReg(L"DisplayVersion");
    int majorVersion = readRegDword(L"CurrentMajorVersionNumber");
    int minorVersion = readRegDword(L"CurrentMinorVersionNumber");
    auto buildNumber = readReg(L"CurrentBuildNumber");

    RegCloseKey(hKey);

    std::string result;
    if (!productName.empty()) {
        result = productName;
        if (!displayVersion.empty())
            result += " " + displayVersion;
        if (majorVersion >= 0 && !buildNumber.empty())
            result += " (Build " + std::to_string(majorVersion) + "." + std::to_string(std::max(0, minorVersion)) + "." + buildNumber + ")";
    } else {
        result = "Windows (unknown version)";
    }

    return Result<std::string>::success(std::move(result));
}

} // namespace gopost::bridges::windows
