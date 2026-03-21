#pragma once

#include "IResult.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace gopost::core::interfaces {

using AsyncHandle = int64_t;

class IFileIO {
public:
    virtual ~IFileIO() = default;

    [[nodiscard]] virtual Result<std::vector<uint8_t>> readFile(const std::string& path, int64_t offset = 0, int64_t length = -1) = 0;
    [[nodiscard]] virtual AsyncHandle readFileAsync(const std::string& path, int64_t offset, int64_t length, std::function<void(Result<std::vector<uint8_t>>)> callback) = 0;
    [[nodiscard]] virtual Result<void> writeFile(const std::string& path, const std::vector<uint8_t>& data) = 0;
    [[nodiscard]] virtual AsyncHandle writeFileAsync(const std::string& path, const std::vector<uint8_t>& data, std::function<void(Result<void>)> callback) = 0;
    [[nodiscard]] virtual bool fileExists(const std::string& path) const = 0;
    [[nodiscard]] virtual Result<uint64_t> fileSize(const std::string& path) const = 0;
    [[nodiscard]] virtual Result<void> deleteFile(const std::string& path) = 0;
    [[nodiscard]] virtual Result<void> createDirectory(const std::string& path) = 0;
    virtual void cancelAsync(AsyncHandle handle) = 0;
};

} // namespace gopost::core::interfaces
