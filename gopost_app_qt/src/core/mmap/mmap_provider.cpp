#include "mmap_provider.h"
#include "core/bridges/common/path_sanitizer.h"

#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace gopost::mmap {

static constexpr size_t kMinMmapSize = 64 * 1024; // 64KB

MmapProvider::MmapProvider(ILogger& logger) : logger_(logger) {}

MmapProvider::~MmapProvider() {
    unmapAll();
}

size_t MmapProvider::pageGranularity() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwAllocationGranularity; // typically 64KB on Windows
#else
    return static_cast<size_t>(sysconf(_SC_PAGESIZE)); // typically 4KB
#endif
}

uint64_t MmapProvider::alignDown(uint64_t offset, size_t granularity) {
    return (offset / granularity) * granularity;
}

uint64_t MmapProvider::getFileSize(const std::string& path) {
#ifdef _WIN32
    auto wpath = std::wstring(path.begin(), path.end()); // simplified — real code uses MultiByteToWideChar
    // Use proper UTF-8 conversion
    int len = MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0);
    if (len <= 0) return 0;
    wpath.resize(len);
    MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), wpath.data(), len);

    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    LARGE_INTEGER sz;
    if (!GetFileSizeEx(hFile, &sz)) { CloseHandle(hFile); return 0; }
    CloseHandle(hFile);
    return static_cast<uint64_t>(sz.QuadPart);
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return static_cast<uint64_t>(st.st_size);
#endif
}

Result<MappedRegion> MmapProvider::map(const std::string& path, uint64_t offset, size_t length) {
    auto sanitized = bridges::PathSanitizer::sanitize(path);
    if (!sanitized.ok())
        return Result<MappedRegion>::failure(sanitized.error);

    auto fileSize = getFileSize(sanitized.get());
    if (fileSize == 0)
        return Result<MappedRegion>::failure("File not found or empty: " + sanitized.get());

    // Determine actual length
    if (length == 0) length = static_cast<size_t>(fileSize - offset);
    if (offset + length > fileSize)
        length = static_cast<size_t>(fileSize - offset);

    if (length < kMinMmapSize)
        return Result<MappedRegion>::failure("File too small for mmap, use buffered read");

    // Align offset down to page boundary
    auto gran = pageGranularity();
    auto alignedOffset = alignDown(offset, gran);
    auto extraBytes = static_cast<size_t>(offset - alignedOffset);
    auto alignedLength = length + extraBytes;

#ifdef _WIN32
    int wlen = MultiByteToWideChar(CP_UTF8, 0, sanitized.get().data(), static_cast<int>(sanitized.get().size()), nullptr, 0);
    std::wstring wpath(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, sanitized.get().data(), static_cast<int>(sanitized.get().size()), wpath.data(), wlen);

    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return Result<MappedRegion>::failure("CreateFileW failed, error " + std::to_string(GetLastError()));

    HANDLE hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hMap) {
        DWORD err = GetLastError();
        CloseHandle(hFile);
        return Result<MappedRegion>::failure("CreateFileMappingW failed, error " + std::to_string(err));
    }

    DWORD offsetHigh = static_cast<DWORD>(alignedOffset >> 32);
    DWORD offsetLow = static_cast<DWORD>(alignedOffset & 0xFFFFFFFF);
    void* addr = MapViewOfFile(hMap, FILE_MAP_READ, offsetHigh, offsetLow, alignedLength);
    if (!addr) {
        DWORD err = GetLastError();
        CloseHandle(hMap);
        CloseHandle(hFile);
        return Result<MappedRegion>::failure("MapViewOfFile failed, error " + std::to_string(err));
    }

    InternalMapping im;
    im.baseAddr = static_cast<const uint8_t*>(addr);
    im.mappedLength = alignedLength;
    im.fileOffset = alignedOffset;
    im.isMmapped = true;
    im.hFile = hFile;
    im.hMap = hMap;

    {
        std::lock_guard lock(mutex_);
        mappings_.push_back(im);
    }

    MappedRegion region;
    region.data = im.baseAddr + extraBytes;
    region.length = length;
    region.fileOffset = offset;
    region.isValid = true;

    logger_.trace(QStringLiteral("Mmap"),
                  QStringLiteral("Mapped %1 bytes at offset %2 from %3")
                      .arg(length).arg(offset).arg(QString::fromStdString(sanitized.get())));

    return Result<MappedRegion>::success(region);

#else
    int fd = open(sanitized.get().c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return Result<MappedRegion>::failure("open() failed: " + std::string(strerror(errno)));

    void* addr = ::mmap(nullptr, alignedLength, PROT_READ, MAP_PRIVATE, fd, static_cast<off_t>(alignedOffset));
    if (addr == MAP_FAILED) {
        auto errStr = std::string(strerror(errno));
        close(fd);
        return Result<MappedRegion>::failure("mmap() failed: " + errStr);
    }

    InternalMapping im;
    im.baseAddr = static_cast<const uint8_t*>(addr);
    im.mappedLength = alignedLength;
    im.fileOffset = alignedOffset;
    im.isMmapped = true;
    im.fd = fd;

    {
        std::lock_guard lock(mutex_);
        mappings_.push_back(im);
    }

    MappedRegion region;
    region.data = im.baseAddr + extraBytes;
    region.length = length;
    region.fileOffset = offset;
    region.isValid = true;

    return Result<MappedRegion>::success(region);
#endif
}

void MmapProvider::unmap(const uint8_t* addr) {
    std::lock_guard lock(mutex_);
    for (auto it = mappings_.begin(); it != mappings_.end(); ++it) {
        // Check if addr falls within this mapping's range
        if (addr >= it->baseAddr && addr < it->baseAddr + it->mappedLength) {
#ifdef _WIN32
            UnmapViewOfFile(it->baseAddr);
            if (it->hMap) CloseHandle(it->hMap);
            if (it->hFile != INVALID_HANDLE_VALUE) CloseHandle(it->hFile);
#else
            ::munmap(const_cast<uint8_t*>(it->baseAddr), it->mappedLength);
            if (it->fd >= 0) close(it->fd);
#endif
            mappings_.erase(it);
            return;
        }
    }
}

bool MmapProvider::isTracked(const uint8_t* addr) const {
    std::lock_guard lock(mutex_);
    for (auto& m : mappings_) {
        if (addr >= m.baseAddr && addr < m.baseAddr + m.mappedLength)
            return true;
    }
    return false;
}

const InternalMapping* MmapProvider::findMapping(const uint8_t* addr) const {
    std::lock_guard lock(mutex_);
    for (auto& m : mappings_) {
        if (addr >= m.baseAddr && addr < m.baseAddr + m.mappedLength)
            return &m;
    }
    return nullptr;
}

void MmapProvider::unmapAll() {
    std::lock_guard lock(mutex_);
    for (auto& m : mappings_) {
#ifdef _WIN32
        UnmapViewOfFile(m.baseAddr);
        if (m.hMap) CloseHandle(m.hMap);
        if (m.hFile != INVALID_HANDLE_VALUE) CloseHandle(m.hFile);
#else
        ::munmap(const_cast<uint8_t*>(m.baseAddr), m.mappedLength);
        if (m.fd >= 0) close(m.fd);
#endif
    }
    mappings_.clear();
}

} // namespace gopost::mmap
