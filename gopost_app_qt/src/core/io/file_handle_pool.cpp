#include "file_handle_pool.h"

namespace gopost::io {

// FileHandleLoan
FileHandleLoan::FileHandleLoan(std::ifstream* stream, FileHandlePool* pool, const std::string& path)
    : stream_(stream), pool_(pool), path_(path) {}

FileHandleLoan::~FileHandleLoan() {
    if (pool_ && !path_.empty()) pool_->returnHandle(path_);
}

FileHandleLoan::FileHandleLoan(FileHandleLoan&& other) noexcept
    : stream_(other.stream_), pool_(other.pool_), path_(std::move(other.path_)) {
    other.stream_ = nullptr;
    other.pool_ = nullptr;
}

FileHandleLoan& FileHandleLoan::operator=(FileHandleLoan&& other) noexcept {
    if (this != &other) {
        if (pool_ && !path_.empty()) pool_->returnHandle(path_);
        stream_ = other.stream_;
        pool_ = other.pool_;
        path_ = std::move(other.path_);
        other.stream_ = nullptr;
        other.pool_ = nullptr;
    }
    return *this;
}

// FileHandlePool
FileHandlePool::FileHandlePool(int maxEntries) : maxEntries_(maxEntries) {}

FileHandlePool::~FileHandlePool() {
    std::lock_guard lock(mutex_);
    pool_.clear();
    lruOrder_.clear();
}

FileHandleLoan FileHandlePool::checkout(const std::string& path) {
    std::lock_guard lock(mutex_);

    auto it = pool_.find(path);
    if (it != pool_.end() && it->second.stream && it->second.stream->is_open()) {
        it->second.accessCount++;
        // Move to front of LRU
        lruOrder_.remove(path);
        lruOrder_.push_front(path);
        return FileHandleLoan(it->second.stream.get(), this, path);
    }

    // Cache miss — open new handle
    if (static_cast<int>(pool_.size()) >= maxEntries_) evictLru();

    auto stream = std::make_unique<std::ifstream>(path, std::ios::binary);
    if (!stream->is_open())
        return FileHandleLoan(nullptr, nullptr, "");

    auto* raw = stream.get();
    pool_[path] = {std::move(stream), 1};
    lruOrder_.push_front(path);
    return FileHandleLoan(raw, this, path);
}

void FileHandlePool::returnHandle(const std::string& /*path*/) {
    // Handle stays in pool — loan just releases the reference
}

void FileHandlePool::evictLru() {
    if (lruOrder_.empty()) return;
    auto& lruPath = lruOrder_.back();
    pool_.erase(lruPath);
    lruOrder_.pop_back();
}

int64_t FileHandlePool::accessCount(const std::string& path) const {
    std::lock_guard lock(mutex_);
    auto it = pool_.find(path);
    return (it != pool_.end()) ? it->second.accessCount : 0;
}

} // namespace gopost::io
