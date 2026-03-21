#pragma once

#include "hash_calculator.h"
#include "project_database.h"
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

namespace gopost::io {

struct DuplicateCheckResult {
    bool isDuplicate = false;
    int64_t existingAssetId = -1;
    std::string sha256Hash;
};

class DuplicateDetector {
public:
    explicit DuplicateDetector(ProjectDatabase& db);

    [[nodiscard]] gopost::Result<DuplicateCheckResult> check(const std::string& filePath);

private:
    ProjectDatabase& db_;

    // LRU hash cache (256 entries)
    static constexpr int kMaxCache = 256;
    std::mutex cacheMutex_;
    std::list<std::pair<std::string, std::string>> cacheOrder_; // path → hash
    std::unordered_map<std::string, decltype(cacheOrder_)::iterator> cacheMap_;

    std::string getCachedHash(const std::string& path);
    void cacheHash(const std::string& path, const std::string& hash);
};

} // namespace gopost::io
