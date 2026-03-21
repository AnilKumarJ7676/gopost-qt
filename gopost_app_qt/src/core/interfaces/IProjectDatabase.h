#pragma once

#include "IResult.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gopost::core::interfaces {

struct MediaAsset {
    int64_t id = 0;
    std::string filename;
    std::string originalUri;
    std::string cachePath;
    std::string mimeType;
    uint64_t fileSizeBytes = 0;
    int64_t durationMs = 0;      // for video/audio
    int width = 0;               // for video/image
    int height = 0;              // for video/image
    std::string codec;
    double frameRate = 0.0;
    int sampleRate = 0;
    int channels = 0;
    uint64_t importedAt = 0;     // epoch ms
    std::string sha256Hash;
};

class IProjectDatabase {
public:
    virtual ~IProjectDatabase() = default;

    [[nodiscard]] virtual Result<void> open(const std::string& projectPath) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual Result<int64_t> addMediaAsset(const MediaAsset& asset) = 0;
    [[nodiscard]] virtual Result<MediaAsset> getMediaAsset(int64_t id) const = 0;
    [[nodiscard]] virtual Result<std::vector<MediaAsset>> getAllMediaAssets() const = 0;
    [[nodiscard]] virtual Result<void> removeMediaAsset(int64_t id) = 0;
    [[nodiscard]] virtual int getSchemaVersion() const = 0;
};

} // namespace gopost::core::interfaces
