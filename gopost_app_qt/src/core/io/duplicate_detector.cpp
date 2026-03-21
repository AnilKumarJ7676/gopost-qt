#include "duplicate_detector.h"

namespace gopost::io {

DuplicateDetector::DuplicateDetector(ProjectDatabase& db) : db_(db) {}

std::string DuplicateDetector::getCachedHash(const std::string& path) {
    std::lock_guard lock(cacheMutex_);
    auto it = cacheMap_.find(path);
    if (it == cacheMap_.end()) return {};
    // Move to front (MRU)
    cacheOrder_.splice(cacheOrder_.begin(), cacheOrder_, it->second);
    return it->second->second;
}

void DuplicateDetector::cacheHash(const std::string& path, const std::string& hash) {
    std::lock_guard lock(cacheMutex_);
    auto it = cacheMap_.find(path);
    if (it != cacheMap_.end()) {
        it->second->second = hash;
        cacheOrder_.splice(cacheOrder_.begin(), cacheOrder_, it->second);
        return;
    }
    if (static_cast<int>(cacheMap_.size()) >= kMaxCache) {
        auto& last = cacheOrder_.back();
        cacheMap_.erase(last.first);
        cacheOrder_.pop_back();
    }
    cacheOrder_.emplace_front(path, hash);
    cacheMap_[path] = cacheOrder_.begin();
}

gopost::Result<DuplicateCheckResult> DuplicateDetector::check(const std::string& filePath) {
    // 1. Get hash (from cache or compute)
    auto hash = getCachedHash(filePath);
    if (hash.empty()) {
        auto hashResult = HashCalculator::hashFile(filePath);
        if (!hashResult.ok())
            return gopost::Result<DuplicateCheckResult>::failure(hashResult.error);
        hash = hashResult.get();
        cacheHash(filePath, hash);
    }

    // 2. Check database for existing asset with same hash
    auto allAssets = db_.getAllMediaAssets();
    if (allAssets.ok()) {
        for (auto& asset : allAssets.get()) {
            if (asset.sha256Hash == hash) {
                return gopost::Result<DuplicateCheckResult>::success({true, asset.id, hash});
            }
        }
    }

    return gopost::Result<DuplicateCheckResult>::success({false, -1, hash});
}

} // namespace gopost::io
