#include "media_import_engine.h"

namespace gopost::media_import {

ImportHandle MediaImportEngine::importFiles(const std::vector<ImportRequest>& /*requests*/) {
    return -1;
}

void MediaImportEngine::cancelImport(ImportHandle /*handle*/) {
    // No-op stub
}

ImportStatus MediaImportEngine::getImportStatus(ImportHandle /*handle*/) const {
    return ImportStatus{};
}

} // namespace gopost::media_import
