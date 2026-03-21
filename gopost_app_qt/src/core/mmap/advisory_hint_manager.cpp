#include "advisory_hint_manager.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

namespace gopost::mmap {

AdvisoryHintManager::AdvisoryHintManager(ILogger& logger) : logger_(logger) {}

void AdvisoryHintManager::sequential(const uint8_t* addr, size_t length) {
    if (!addr || length == 0) return;

    logger_.trace(QStringLiteral("Mmap"), QStringLiteral("Advisory SEQUENTIAL on %1 bytes").arg(length));

#ifdef _WIN32
    // PrefetchVirtualMemory (Windows 8+) — pre-fault pages sequentially
    WIN32_MEMORY_RANGE_ENTRY entry;
    entry.VirtualAddress = const_cast<uint8_t*>(addr);
    entry.NumberOfBytes = length;
    // PrefetchVirtualMemory is available on Win8+, use dynamic loading for safety
    using PrefetchFn = BOOL(WINAPI*)(HANDLE, ULONG_PTR, PWIN32_MEMORY_RANGE_ENTRY, ULONG);
    static auto pPrefetch = reinterpret_cast<PrefetchFn>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "PrefetchVirtualMemory"));
    if (pPrefetch) {
        pPrefetch(GetCurrentProcess(), 1, &entry, 0);
    }
#else
    madvise(const_cast<uint8_t*>(addr), length, MADV_SEQUENTIAL);
#endif
}

void AdvisoryHintManager::willNeed(const uint8_t* addr, size_t length) {
    if (!addr || length == 0) return;

    logger_.trace(QStringLiteral("Mmap"), QStringLiteral("Advisory WILLNEED on %1 bytes").arg(length));

#ifdef _WIN32
    WIN32_MEMORY_RANGE_ENTRY entry;
    entry.VirtualAddress = const_cast<uint8_t*>(addr);
    entry.NumberOfBytes = length;
    using PrefetchFn = BOOL(WINAPI*)(HANDLE, ULONG_PTR, PWIN32_MEMORY_RANGE_ENTRY, ULONG);
    static auto pPrefetch = reinterpret_cast<PrefetchFn>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "PrefetchVirtualMemory"));
    if (pPrefetch) {
        pPrefetch(GetCurrentProcess(), 1, &entry, 0);
    }
#else
    madvise(const_cast<uint8_t*>(addr), length, MADV_WILLNEED);
#endif
}

void AdvisoryHintManager::dontNeed(const uint8_t* addr, size_t length) {
    if (!addr || length == 0) return;

    logger_.trace(QStringLiteral("Mmap"), QStringLiteral("Advisory DONTNEED on %1 bytes").arg(length));

#ifdef _WIN32
    // Windows doesn't have a direct DONTNEED equivalent.
    // VirtualUnlock could hint the OS, but it may fail if pages weren't locked.
    VirtualUnlock(const_cast<uint8_t*>(addr), length); // best effort
#else
    madvise(const_cast<uint8_t*>(addr), length, MADV_DONTNEED);
#endif
}

} // namespace gopost::mmap
