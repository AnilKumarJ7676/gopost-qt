#pragma once

#include "core/interfaces/IMediaImport.h"

namespace gopost::media_import {

using gopost::core::interfaces::IMediaImport;
using gopost::core::interfaces::ImportHandle;
using gopost::core::interfaces::ImportRequest;
using gopost::core::interfaces::ImportStatus;
using gopost::core::interfaces::ImportState;

class MediaImportEngine : public IMediaImport {
public:
    ImportHandle importFiles(const std::vector<ImportRequest>& requests) override;
    void cancelImport(ImportHandle handle) override;
    ImportStatus getImportStatus(ImportHandle handle) const override;
};

} // namespace gopost::media_import
