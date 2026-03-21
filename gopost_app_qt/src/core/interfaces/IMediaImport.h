#pragma once

#include "IResult.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gopost::core::interfaces {

struct ImportRequest {
    std::string sourceUri;
    std::string destinationFilename;
    bool validateFormat = true;
};

enum class ImportState {
    PENDING,
    IMPORTING,
    VALIDATING,
    COMPLETE,
    FAILED,
    CANCELLED
};

struct ImportStatus {
    int totalFiles = 0;
    int completedFiles = 0;
    int failedFiles = 0;
    std::string currentFileName;
    uint64_t bytesTransferred = 0;
    uint64_t totalBytes = 0;
    ImportState state = ImportState::PENDING;
};

using ImportHandle = int64_t;

class IMediaImport {
public:
    virtual ~IMediaImport() = default;

    [[nodiscard]] virtual ImportHandle importFiles(const std::vector<ImportRequest>& requests) = 0;
    virtual void cancelImport(ImportHandle handle) = 0;
    [[nodiscard]] virtual ImportStatus getImportStatus(ImportHandle handle) const = 0;
};

} // namespace gopost::core::interfaces
