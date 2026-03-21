#pragma once

#include "core/interfaces/IMmap.h"
#include "core/interfaces/ILogger.h"
#include "mmap_provider.h"
#include "advisory_hint_manager.h"
#include "sliding_window_mapper.h"
#include "mmap_fallback_reader.h"

#include <memory>
#include <mutex>
#include <unordered_set>

namespace gopost::mmap {

using core::interfaces::IMmap;
using core::interfaces::MappedRegion;
using core::interfaces::ILogger;

class MmapEngine : public IMmap {
public:
    MmapEngine(ILogger& logger, size_t maxWindowSize = 0);
    ~MmapEngine() override;

    Result<MappedRegion> mapFile(const std::string& path, uint64_t offset, size_t length) override;
    void unmapFile(MappedRegion& region) override;
    void advisorySequential(MappedRegion& region) override;
    void advisoryWillNeed(MappedRegion& region, uint64_t offset, size_t length) override;

    // Test access
    SlidingWindowMapper& slidingWindowMapper() { return slider_; }

private:
    ILogger& logger_;
    MmapProvider provider_;
    AdvisoryHintManager hints_;
    SlidingWindowMapper slider_;
    MmapFallbackReader fallback_;

    // Track which regions came from fallback vs mmap
    std::mutex trackMutex_;
    std::unordered_set<const uint8_t*> fallbackRegions_;
    std::unordered_set<const uint8_t*> slidingRegions_;
};

} // namespace gopost::mmap
