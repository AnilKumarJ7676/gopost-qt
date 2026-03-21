#include "core/memory/memory_pool_registry.h"

namespace gopost::memory {

void MemoryPoolRegistry::initialize(const MemoryBudgets& b) {
    pools_.clear();
    pools_["frameCache"]     = std::make_unique<MemoryPool>(static_cast<uint64_t>(b.frameCacheBytes));
    pools_["thumbnailCache"] = std::make_unique<MemoryPool>(static_cast<uint64_t>(b.thumbnailCacheBytes));
    pools_["audioCache"]     = std::make_unique<MemoryPool>(static_cast<uint64_t>(b.audioCacheBytes));
    pools_["effectCache"]    = std::make_unique<MemoryPool>(static_cast<uint64_t>(b.effectCacheBytes));
    pools_["importBuffer"]   = std::make_unique<MemoryPool>(static_cast<uint64_t>(b.importBufferBytes));
    pools_["reserve"]        = std::make_unique<MemoryPool>(static_cast<uint64_t>(b.reserveBytes));
}

MemoryPool* MemoryPoolRegistry::pool(const std::string& name) {
    auto it = pools_.find(name);
    return it != pools_.end() ? it->second.get() : nullptr;
}

const MemoryPool* MemoryPoolRegistry::pool(const std::string& name) const {
    auto it = pools_.find(name);
    return it != pools_.end() ? it->second.get() : nullptr;
}

std::vector<std::string> MemoryPoolRegistry::poolNames() const {
    std::vector<std::string> names;
    names.reserve(pools_.size());
    for (const auto& [k, _] : pools_)
        names.push_back(k);
    return names;
}

} // namespace gopost::memory
