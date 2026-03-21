#pragma once

#include <fstream>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace gopost::io {

class FileHandlePool;

class FileHandleLoan {
public:
    FileHandleLoan() = default;
    FileHandleLoan(std::ifstream* stream, FileHandlePool* pool, const std::string& path);
    ~FileHandleLoan();
    FileHandleLoan(FileHandleLoan&& other) noexcept;
    FileHandleLoan& operator=(FileHandleLoan&& other) noexcept;
    FileHandleLoan(const FileHandleLoan&) = delete;
    FileHandleLoan& operator=(const FileHandleLoan&) = delete;

    std::ifstream* get() const { return stream_; }
    bool valid() const { return stream_ != nullptr && stream_->is_open(); }

private:
    std::ifstream* stream_ = nullptr;
    FileHandlePool* pool_ = nullptr;
    std::string path_;
};

class FileHandlePool {
public:
    explicit FileHandlePool(int maxEntries = 32);
    ~FileHandlePool();

    [[nodiscard]] FileHandleLoan checkout(const std::string& path);
    void returnHandle(const std::string& path);
    int64_t accessCount(const std::string& path) const;

private:
    struct Entry {
        std::unique_ptr<std::ifstream> stream;
        int64_t accessCount = 0;
    };

    void evictLru();

    int maxEntries_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Entry> pool_;
    std::list<std::string> lruOrder_; // front = most recent
};

} // namespace gopost::io
