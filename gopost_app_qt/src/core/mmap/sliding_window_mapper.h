#pragma once

#include "mmap_provider.h"
#include "advisory_hint_manager.h"
#include "core/interfaces/IMmap.h"
#include "core/interfaces/ILogger.h"

#include <string>

namespace gopost::mmap {

using core::interfaces::MappedRegion;
using core::interfaces::ILogger;

class SlidingWindowMapper {
public:
    SlidingWindowMapper(MmapProvider& provider, AdvisoryHintManager& hints,
                        ILogger& logger, size_t maxWindowSize);

    [[nodiscard]] Result<MappedRegion> mapFile(const std::string& path, uint64_t fileSize);
    void unmap(MappedRegion& region);

    [[nodiscard]] int slideCount() const { return slideCount_; }

    // Ensure a byte at the given file offset is accessible; may trigger a window slide
    [[nodiscard]] const uint8_t* accessByte(uint64_t fileOffset);

private:
    void slideWindow(uint64_t targetOffset);

    MmapProvider& provider_;
    AdvisoryHintManager& hints_;
    ILogger& logger_;
    size_t maxWindowSize_;

    // Current state
    std::string filePath_;
    uint64_t fileSize_ = 0;
    MappedRegion currentWindow_{};
    uint64_t windowStart_ = 0;
    size_t windowSize_ = 0;
    int slideCount_ = 0;

    // The "master" region returned to the caller — backed by a heap buffer
    std::vector<uint8_t> masterBuffer_;
    bool usingSlidingWindow_ = false;
};

} // namespace gopost::mmap
