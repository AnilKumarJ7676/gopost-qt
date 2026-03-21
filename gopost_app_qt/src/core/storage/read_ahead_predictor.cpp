#include "read_ahead_predictor.h"
#include <algorithm>

namespace gopost::storage {

void ReadAheadPredictor::recordRead(const std::string& path, int64_t offset, int64_t length) {
    std::lock_guard lock(mutex_);

    // Evict oldest entry if at capacity and this is a new path
    if (trackers_.size() >= kMaxEntries && trackers_.find(path) == trackers_.end()) {
        trackers_.erase(trackers_.begin());
    }

    auto& t = trackers_[path];
    t.ring[t.head] = {offset, length};
    t.head = (t.head + 1) % kRingSize;
    if (t.count < kRingSize) t.count++;
}

int ReadAheadPredictor::sequentialCount(const FileTracker& t) const {
    if (t.count < 2) return 0;

    int seqCount = 0;
    int n = std::min(t.count, kRingSize);

    for (int i = 1; i < n; ++i) {
        int prevIdx = (t.head - n + i - 1 + kRingSize) % kRingSize;
        int currIdx = (t.head - n + i + kRingSize) % kRingSize;

        auto expectedOffset = t.ring[prevIdx].offset + t.ring[prevIdx].length;
        auto actualOffset = t.ring[currIdx].offset;
        auto diff = actualOffset - expectedOffset;
        if (diff >= -kTolerance && diff <= kTolerance) {
            seqCount++;
        } else {
            seqCount = 0;
        }
    }
    return seqCount;
}

bool ReadAheadPredictor::isSequential(const std::string& path) const {
    std::lock_guard lock(mutex_);
    auto it = trackers_.find(path);
    if (it == trackers_.end()) return false;
    return sequentialCount(it->second) >= kSeqThreshold;
}

ReadAheadPrediction ReadAheadPredictor::predict(const std::string& path) const {
    std::lock_guard lock(mutex_);
    auto it = trackers_.find(path);
    if (it == trackers_.end()) return {};

    const auto& t = it->second;
    bool seq = sequentialCount(t) >= kSeqThreshold;
    if (!seq) return {-1, 0, false};

    // Predict next offset from the most recent read
    int lastIdx = (t.head - 1 + kRingSize) % kRingSize;
    auto nextOffset = t.ring[lastIdx].offset + t.ring[lastIdx].length;
    auto nextLength = t.ring[lastIdx].length;  // same size as last read

    return {nextOffset, nextLength, true};
}

} // namespace gopost::storage
