#pragma once

#include "core/interfaces/IProjectDatabase.h"
#include <mutex>

struct sqlite3;

namespace gopost::io {

using core::interfaces::IProjectDatabase;
using core::interfaces::MediaAsset;

class ProjectDatabase : public IProjectDatabase {
public:
    ProjectDatabase();
    ~ProjectDatabase() override;

    gopost::Result<void> open(const std::string& projectPath) override;
    void close() override;
    gopost::Result<int64_t> addMediaAsset(const MediaAsset& asset) override;
    gopost::Result<MediaAsset> getMediaAsset(int64_t id) const override;
    gopost::Result<std::vector<MediaAsset>> getAllMediaAssets() const override;
    gopost::Result<void> removeMediaAsset(int64_t id) override;
    int getSchemaVersion() const override;

private:
    void createSchema();
    mutable std::mutex mutex_;
    sqlite3* db_ = nullptr;
};

} // namespace gopost::io
