#pragma once

#include <QtGlobal>

namespace gopost::platform {

/// Platform-tuned cache and resource limits.
/// Desktop gets generous limits; mobile is conservative to avoid OOM kills;
/// WASM is memory-only with tight caps.
struct Defaults {
    int    thumbnailMemoryCacheEntries;  // QCache / QMap max entries
    qint64 thumbnailDiskCacheBytes;      // max disk cache before purge
    int    maxDecoders;                  // concurrent video decoder pool size
    bool   enableDiskCache;             // false on WASM (no real filesystem)

    static Defaults forCurrentPlatform() {
#if defined(Q_OS_WASM)
        return { 100, 0, 1, false };           // ~10MB memory, no disk
#elif defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
        return { 150, 64 * 1024 * 1024, 1, true };  // 150 entries, 64MB disk
#else
        return { 500, 200 * 1024 * 1024, 4, true };  // 500 entries, 200MB disk, 4 decoder threads
#endif
    }
};

} // namespace gopost::platform
