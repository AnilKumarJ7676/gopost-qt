#include "read_scheduler.h"

#include <chrono>
#include <fstream>

namespace gopost::storage {

ReadScheduler::ReadScheduler(const StorageProfile& profile, BlockAligner& aligner,
                             ReadAheadPredictor& predictor, IOBandwidthMonitor& monitor)
    : profile_(profile), aligner_(aligner), predictor_(predictor), monitor_(monitor) {}

ReadScheduler::~ReadScheduler() {
    stop();
}

void ReadScheduler::start() {
    if (running_.exchange(true)) return;
    dispatchThread_ = std::thread(&ReadScheduler::dispatchLoop, this);
}

void ReadScheduler::stop() {
    running_ = false;
    queueCv_.notify_all();
    if (dispatchThread_.joinable())
        dispatchThread_.join();
}

ReadTicket ReadScheduler::enqueue(const ReadRequest& request) {
    auto ticket = nextTicket_.fetch_add(1);

    auto aligned = aligner_.align(request.offset, request.length > 0 ? request.length : 0);

    QueuedRead qr;
    qr.ticket = ticket;
    qr.request = request;
    qr.alignedOffset = aligned.alignedOffset;
    qr.alignedLength = aligned.alignedLength;
    qr.trimStart = aligned.trimStart;
    qr.trimLength = aligned.trimLength;

    {
        std::lock_guard lock(queueMutex_);
        pending_.push(std::move(qr));
    }
    queueCv_.notify_one();

    return ticket;
}

void ReadScheduler::cancel(ReadTicket ticket) {
    std::lock_guard lock(queueMutex_);
    cancelled_[ticket] = true;
}

void ReadScheduler::dispatchLoop() {
    while (running_) {
        QueuedRead read;
        {
            std::unique_lock lock(queueMutex_);
            queueCv_.wait(lock, [&] { return !pending_.empty() || !running_; });

            if (!running_ && pending_.empty()) return;
            if (pending_.empty()) continue;

            // Wait until we have capacity
            while (activeReads_ >= profile_.maxConcurrentReads && running_) {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                lock.lock();
            }
            if (!running_ && pending_.empty()) return;
            if (pending_.empty()) continue;

            read = pending_.top();
            pending_.pop();

            // Skip cancelled reads
            if (cancelled_.count(read.ticket)) {
                cancelled_.erase(read.ticket);
                continue;
            }
        }

        activeReads_++;
        // For single-reader mode, execute inline. For multi-reader, spawn thread.
        if (profile_.maxConcurrentReads <= 1) {
            executeRead(std::move(read));
        } else {
            std::thread([this, r = std::move(read)]() mutable {
                executeRead(std::move(r));
            }).detach();
        }
    }
}

std::vector<uint8_t> ReadScheduler::readFileRange(const std::string& path, int64_t offset, int64_t length) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};

    file.seekg(offset);
    if (!file.good()) return {};

    std::vector<uint8_t> buf(static_cast<size_t>(length));
    file.read(reinterpret_cast<char*>(buf.data()), length);
    auto bytesRead = file.gcount();
    buf.resize(static_cast<size_t>(bytesRead));
    return buf;
}

void ReadScheduler::executeRead(QueuedRead read) {
    auto start = std::chrono::steady_clock::now();

    // Read the aligned range
    auto data = readFileRange(read.request.path, read.alignedOffset, read.alignedLength);

    auto end = std::chrono::steady_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    // Record for bandwidth monitoring
    monitor_.recordCompletion(static_cast<int64_t>(data.size()), elapsedMs, read.request.path);

    // Record for read-ahead prediction
    predictor_.recordRead(read.request.path, read.request.offset,
                          read.request.length > 0 ? read.request.length : static_cast<int64_t>(data.size()));

    activeReads_--;

    // Check if cancelled after I/O completed
    {
        std::lock_guard lock(queueMutex_);
        if (cancelled_.count(read.ticket)) {
            cancelled_.erase(read.ticket);
            return;
        }
    }

    // Trim to requested range and deliver
    if (read.request.callback) {
        if (data.empty()) {
            read.request.callback(Result<std::vector<uint8_t>>::failure("Read returned no data"));
        } else if (read.trimStart > 0 || read.trimLength < static_cast<int64_t>(data.size())) {
            auto trimEnd = std::min<size_t>(static_cast<size_t>(read.trimStart + read.trimLength), data.size());
            auto trimBegin = std::min<size_t>(static_cast<size_t>(read.trimStart), data.size());
            std::vector<uint8_t> trimmed(data.begin() + trimBegin, data.begin() + trimEnd);
            read.request.callback(Result<std::vector<uint8_t>>::success(std::move(trimmed)));
        } else {
            read.request.callback(Result<std::vector<uint8_t>>::success(std::move(data)));
        }
    }
}

} // namespace gopost::storage
