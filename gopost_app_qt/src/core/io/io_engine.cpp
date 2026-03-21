#include "io_engine.h"
#include <filesystem>
#include <fstream>

namespace gopost::io {

IoEngine::IoEngine(ILogger& logger, int maxConcurrentReads, int blockSize)
    : logger_(logger)
    , pool_(maxConcurrentReads, blockSize)
    , reader_(blockSize)
    , dupDetector_(projectDb_) {}

IoEngine::~IoEngine() = default;

gopost::Result<std::vector<uint8_t>> IoEngine::readFile(const std::string& path, int64_t offset, int64_t length) {
    if (length < 0) {
        // Read entire file
        auto sz = fileSize(path);
        if (!sz.ok()) return gopost::Result<std::vector<uint8_t>>::failure(sz.error);
        length = static_cast<int64_t>(sz.get()) - offset;
    }
    return reader_.read(path, offset, length);
}

AsyncHandle IoEngine::readFileAsync(const std::string& path, int64_t offset, int64_t length,
                                     std::function<void(gopost::Result<std::vector<uint8_t>>)> callback) {
    return pool_.submitRead(path, offset, length, core::interfaces::ReadPriority::NORMAL,
                            [cb = std::move(callback)](AsyncReadResult result) {
                                cb(std::move(result.data));
                            });
}

gopost::Result<void> IoEngine::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    // Direct write for synchronous API — bypass coalescer
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        return gopost::Result<void>::failure("Cannot open file for writing: " + path);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file.good())
        return gopost::Result<void>::failure("Write failed: " + path);
    return gopost::Result<void>::success();
}

AsyncHandle IoEngine::writeFileAsync(const std::string& path, const std::vector<uint8_t>& data,
                                      std::function<void(gopost::Result<void>)> callback) {
    auto result = writer_.write(path, data, false);
    // Since write coalescer is async, return a handle and invoke callback after flush
    static std::atomic<int64_t> writeHandle{1000000};
    auto handle = writeHandle.fetch_add(1);

    // Fire callback on a detached thread after coalescer processes
    std::thread([cb = std::move(callback), result = std::move(result)]() {
        cb(std::move(result));
    }).detach();

    return handle;
}

bool IoEngine::fileExists(const std::string& path) const {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

gopost::Result<uint64_t> IoEngine::fileSize(const std::string& path) const {
    std::error_code ec;
    auto sz = std::filesystem::file_size(path, ec);
    if (ec)
        return gopost::Result<uint64_t>::failure("Cannot get file size: " + ec.message());
    return gopost::Result<uint64_t>::success(sz);
}

gopost::Result<void> IoEngine::deleteFile(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::remove(path, ec))
        return gopost::Result<void>::failure("Delete failed: " + ec.message());
    return gopost::Result<void>::success();
}

gopost::Result<void> IoEngine::createDirectory(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec)
        return gopost::Result<void>::failure("Create directory failed: " + ec.message());
    return gopost::Result<void>::success();
}

void IoEngine::cancelAsync(AsyncHandle handle) {
    pool_.cancel(handle);
}

} // namespace gopost::io
