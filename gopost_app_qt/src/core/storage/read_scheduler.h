#pragma once

#include "core/interfaces/IStorageScheduler.h"
#include "core/interfaces/IResult.h"
#include "block_aligner.h"
#include "read_ahead_predictor.h"
#include "io_bandwidth_monitor.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gopost::storage {

using core::interfaces::ReadRequest;
using core::interfaces::ReadTicket;
using core::interfaces::ReadPriority;
using core::interfaces::StorageProfile;

class ReadScheduler {
public:
    ReadScheduler(const StorageProfile& profile, BlockAligner& aligner,
                  ReadAheadPredictor& predictor, IOBandwidthMonitor& monitor);
    ~ReadScheduler();

    ReadTicket enqueue(const ReadRequest& request);
    void cancel(ReadTicket ticket);
    void start();
    void stop();

private:
    struct QueuedRead {
        ReadTicket ticket;
        ReadRequest request;
        int64_t alignedOffset;
        int64_t alignedLength;
        int64_t trimStart;
        int64_t trimLength;
    };

    struct PriorityCompare {
        bool operator()(const QueuedRead& a, const QueuedRead& b) const {
            // Higher priority = lower enum value = should come first
            // priority_queue is max-heap, so we want higher priority at top
            return static_cast<int>(a.request.priority) > static_cast<int>(b.request.priority);
        }
    };

    void dispatchLoop();
    void executeRead(QueuedRead read);
    std::vector<uint8_t> readFileRange(const std::string& path, int64_t offset, int64_t length);

    StorageProfile profile_;
    BlockAligner& aligner_;
    ReadAheadPredictor& predictor_;
    IOBandwidthMonitor& monitor_;

    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::priority_queue<QueuedRead, std::vector<QueuedRead>, PriorityCompare> pending_;
    std::unordered_map<ReadTicket, bool> cancelled_;

    std::atomic<ReadTicket> nextTicket_{1};
    std::atomic<int> activeReads_{0};
    std::atomic<bool> running_{false};
    std::thread dispatchThread_;
};

} // namespace gopost::storage
