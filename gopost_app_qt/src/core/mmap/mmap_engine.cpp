#include "mmap_engine.h"

namespace gopost::mmap {

static size_t defaultMaxWindow() {
    // 2GB on 64-bit, 256MB on 32-bit
    if constexpr (sizeof(void*) >= 8)
        return 2ULL * 1024 * 1024 * 1024;
    else
        return 256 * 1024 * 1024;
}

MmapEngine::MmapEngine(ILogger& logger, size_t maxWindowSize)
    : logger_(logger)
    , provider_(logger)
    , hints_(logger)
    , slider_(provider_, hints_, logger, maxWindowSize > 0 ? maxWindowSize : defaultMaxWindow())
    , fallback_(logger) {}

MmapEngine::~MmapEngine() = default;

Result<MappedRegion> MmapEngine::mapFile(const std::string& path, uint64_t offset, size_t length) {
    // 1. Try direct mmap via provider
    auto result = provider_.map(path, offset, length);
    if (result.ok()) {
        // Issue sequential advisory hint
        hints_.sequential(result.get().data, result.get().length);
        return result;
    }

    // 2. If "too small" error, propagate it — caller should use buffered read
    if (result.error.find("too small") != std::string::npos)
        return result;

    // 3. If mmap failed for other reasons, try fallback buffered read
    logger_.warn(QStringLiteral("Mmap"),
                 QStringLiteral("mmap failed, falling back to buffered read: %1")
                     .arg(QString::fromStdString(result.error)));

    auto fallbackResult = fallback_.readAsRegion(path, offset, length);
    if (fallbackResult.ok()) {
        std::lock_guard lock(trackMutex_);
        fallbackRegions_.insert(fallbackResult.get().data);
    }
    return fallbackResult;
}

void MmapEngine::unmapFile(MappedRegion& region) {
    if (!region.isValid || !region.data) return;

    {
        std::lock_guard lock(trackMutex_);

        if (fallbackRegions_.count(region.data)) {
            fallbackRegions_.erase(region.data);
            fallback_.release(region.data);
            region.data = nullptr;
            region.length = 0;
            region.isValid = false;
            return;
        }

        if (slidingRegions_.count(region.data)) {
            slidingRegions_.erase(region.data);
            slider_.unmap(region);
            return;
        }
    }

    // Standard mmap unmap
    provider_.unmap(region.data);
    region.data = nullptr;
    region.length = 0;
    region.isValid = false;
}

void MmapEngine::advisorySequential(MappedRegion& region) {
    if (!region.isValid || !region.data) return;

    // Advisory hints only meaningful for real mmapped regions
    {
        std::lock_guard lock(trackMutex_);
        if (fallbackRegions_.count(region.data)) return; // no-op for buffered
    }

    hints_.sequential(region.data, region.length);
}

void MmapEngine::advisoryWillNeed(MappedRegion& region, uint64_t offset, size_t length) {
    if (!region.isValid || !region.data) return;

    {
        std::lock_guard lock(trackMutex_);
        if (fallbackRegions_.count(region.data)) return; // no-op for buffered
    }

    // offset is relative to region start
    if (offset >= region.length) return;
    auto clampedLen = std::min<size_t>(length, region.length - static_cast<size_t>(offset));
    hints_.willNeed(region.data + offset, clampedLen);
}

} // namespace gopost::mmap
