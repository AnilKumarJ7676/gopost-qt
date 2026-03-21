#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace gopost::storage {

struct ReadAheadPrediction {
    int64_t predictedOffset = -1;
    int64_t predictedLength = 0;
    bool isSequential = false;
};

class ReadAheadPredictor {
public:
    void recordRead(const std::string& path, int64_t offset, int64_t length);
    [[nodiscard]] ReadAheadPrediction predict(const std::string& path) const;
    [[nodiscard]] bool isSequential(const std::string& path) const;

private:
    static constexpr int kRingSize = 8;
    static constexpr int kSeqThreshold = 3;
    static constexpr int kMaxEntries = 16;
    static constexpr int64_t kTolerance = 4096; // offset tolerance for sequential detection

    struct FileTracker {
        struct Entry { int64_t offset = 0; int64_t length = 0; };
        std::array<Entry, kRingSize> ring{};
        int count = 0;
        int head = 0; // next write position
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, FileTracker> trackers_;

    int sequentialCount(const FileTracker& t) const;
};

} // namespace gopost::storage
