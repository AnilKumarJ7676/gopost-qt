#pragma once

#include "core/interfaces/IFileIO.h"
#include "core/interfaces/ILogger.h"
#include "async_read_pool.h"
#include "buffered_reader.h"
#include "write_coalescer.h"
#include "file_handle_pool.h"
#include "hash_calculator.h"
#include "project_database.h"
#include "duplicate_detector.h"
#include "file_validator.h"

#include <memory>

namespace gopost::io {

using core::interfaces::AsyncHandle;
using core::interfaces::IFileIO;
using core::interfaces::ILogger;

class IoEngine : public IFileIO {
public:
    IoEngine(ILogger& logger, int maxConcurrentReads = 4, int blockSize = 1024 * 1024);
    ~IoEngine() override;

    gopost::Result<std::vector<uint8_t>> readFile(const std::string& path, int64_t offset, int64_t length) override;
    AsyncHandle readFileAsync(const std::string& path, int64_t offset, int64_t length,
                              std::function<void(gopost::Result<std::vector<uint8_t>>)> callback) override;
    gopost::Result<void> writeFile(const std::string& path, const std::vector<uint8_t>& data) override;
    AsyncHandle writeFileAsync(const std::string& path, const std::vector<uint8_t>& data,
                               std::function<void(gopost::Result<void>)> callback) override;
    bool fileExists(const std::string& path) const override;
    gopost::Result<uint64_t> fileSize(const std::string& path) const override;
    gopost::Result<void> deleteFile(const std::string& path) override;
    gopost::Result<void> createDirectory(const std::string& path) override;
    void cancelAsync(AsyncHandle handle) override;

    // Sub-engine access
    ProjectDatabase& projectDatabase() { return projectDb_; }
    DuplicateDetector& duplicateDetector() { return dupDetector_; }
    BufferedReader& bufferedReader() { return reader_; }
    AsyncReadPool& asyncReadPool() { return pool_; }

private:
    ILogger& logger_;
    AsyncReadPool pool_;
    BufferedReader reader_;
    WriteCoalescer writer_;
    FileHandlePool handlePool_;
    ProjectDatabase projectDb_;
    DuplicateDetector dupDetector_;
};

} // namespace gopost::io
