#include "sliding_window_mapper.h"
#include <algorithm>
#include <cstring>

namespace gopost::mmap {

SlidingWindowMapper::SlidingWindowMapper(MmapProvider& provider, AdvisoryHintManager& hints,
                                         ILogger& logger, size_t maxWindowSize)
    : provider_(provider), hints_(hints), logger_(logger)
    , maxWindowSize_(maxWindowSize > 0 ? maxWindowSize : 256 * 1024 * 1024) {}

Result<MappedRegion> SlidingWindowMapper::mapFile(const std::string& path, uint64_t fileSize) {
    filePath_ = path;
    fileSize_ = fileSize;
    slideCount_ = 0;

    if (fileSize <= maxWindowSize_) {
        // File fits in one window — direct mmap
        usingSlidingWindow_ = false;
        return provider_.map(path, 0, static_cast<size_t>(fileSize));
    }

    // File too large for single mapping — use sliding window with heap buffer
    usingSlidingWindow_ = true;
    masterBuffer_.resize(static_cast<size_t>(fileSize));

    // Map first window
    windowStart_ = 0;
    windowSize_ = maxWindowSize_;

    auto result = provider_.map(path, 0, windowSize_);
    if (!result.ok())
        return Result<MappedRegion>::failure("Initial window map failed: " + result.error);

    currentWindow_ = result.get();

    // Copy first window into master buffer
    std::memcpy(masterBuffer_.data(), currentWindow_.data, windowSize_);
    hints_.sequential(currentWindow_.data, windowSize_);

    logger_.debug(QStringLiteral("Mmap"),
                  QStringLiteral("Sliding window: file=%1MB window=%2MB")
                      .arg(fileSize / (1024 * 1024)).arg(windowSize_ / (1024 * 1024)));

    MappedRegion region;
    region.data = masterBuffer_.data();
    region.length = static_cast<size_t>(fileSize);
    region.fileOffset = 0;
    region.isValid = true;
    return Result<MappedRegion>::success(region);
}

void SlidingWindowMapper::slideWindow(uint64_t targetOffset) {
    // Unmap current window
    if (currentWindow_.isValid) {
        provider_.unmap(currentWindow_.data);
        currentWindow_.isValid = false;
    }

    // Calculate new window with 25% overlap
    size_t overlap = maxWindowSize_ / 4;
    uint64_t newStart = (targetOffset > overlap) ? targetOffset - overlap : 0;

    // Align to page boundary
    auto gran = MmapProvider::pageGranularity();
    newStart = MmapProvider::alignDown(newStart, gran);

    size_t newSize = std::min<size_t>(maxWindowSize_, static_cast<size_t>(fileSize_ - newStart));

    auto result = provider_.map(filePath_, newStart, newSize);
    if (!result.ok()) {
        logger_.warn(QStringLiteral("Mmap"),
                     QStringLiteral("Sliding window remap failed at offset %1").arg(newStart));
        return;
    }

    currentWindow_ = result.get();
    windowStart_ = newStart;
    windowSize_ = newSize;
    slideCount_++;

    // Copy into master buffer
    std::memcpy(masterBuffer_.data() + newStart, currentWindow_.data, newSize);

    hints_.sequential(currentWindow_.data, newSize);

    logger_.trace(QStringLiteral("Mmap"),
                  QStringLiteral("Window slide #%1 to offset %2").arg(slideCount_).arg(newStart));
}

const uint8_t* SlidingWindowMapper::accessByte(uint64_t fileOffset) {
    if (!usingSlidingWindow_ || fileOffset >= fileSize_) return nullptr;

    // Check if offset is within current window
    if (fileOffset < windowStart_ || fileOffset >= windowStart_ + windowSize_) {
        slideWindow(fileOffset);
    }
    // Check if at 75% of window — pre-slide
    else if (fileOffset >= windowStart_ + (windowSize_ * 3 / 4) &&
             windowStart_ + windowSize_ < fileSize_) {
        slideWindow(fileOffset);
    }

    return masterBuffer_.data() + fileOffset;
}

void SlidingWindowMapper::unmap(MappedRegion& region) {
    if (usingSlidingWindow_) {
        if (currentWindow_.isValid) {
            provider_.unmap(currentWindow_.data);
            currentWindow_.isValid = false;
        }
        masterBuffer_.clear();
        masterBuffer_.shrink_to_fit();
        usingSlidingWindow_ = false;
    } else {
        provider_.unmap(region.data);
    }
    region.data = nullptr;
    region.length = 0;
    region.isValid = false;
}

} // namespace gopost::mmap
