#pragma once

#include <QString>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QDir>
#include <QMutex>
#include <QSemaphore>
#include <optional>
#include <memory>

#include "video_editor/domain/services/thumbnail_service.h"
#include "video_editor/data/services/ffmpeg_runner.h"

namespace gopost::video_editor {

/// ThumbnailService: FFmpeg CLI-based thumbnail extraction with GPU detection,
/// semaphore-gated concurrency, and sequential video processing queue.
class ThumbnailService : public ThumbnailServiceInterface {
public:
    ThumbnailService();

    static ThumbnailService& instance();

    QList<QByteArray> getCached(const QString& sourcePath, int count) override;

    QList<QByteArray> extractThumbnails(
        const QString& sourcePath,
        double sourceDuration,
        int count
    ) override;

    std::optional<QByteArray> extractSingleThumbnail(
        const QString& sourcePath,
        double timeSeconds = 0.5
    ) override;

    void clearCache() override;

    /// Remove oldest disk cache files when total exceeds limit.
    void purgeDiskCache(qint64 maxBytes = 200 * 1024 * 1024);

private:
    static constexpr int kThumbWidth = 120;
    static constexpr int kThumbHeight = 68;
    static constexpr int kMaxCacheEntries = 200;
    static constexpr int kMaxConcurrentCpu = 1;
    static constexpr int kMaxConcurrentGpu = 3;

    std::shared_ptr<FfmpegRunner> ffmpeg_;
    mutable QMutex cacheMutex_;
    QMap<QString, QList<QByteArray>> cache_;
    std::optional<QDir> thumbDir_;

    // GPU hardware acceleration
    enum class HwAccelStatus { Unknown, Probing, Available, Unavailable };
    static HwAccelStatus hwAccelStatus_;
    static QString hwaccelName_;
    static bool gpuProbed_;

    void ensureGpuProbed();
    static void probeGpu();

    [[nodiscard]] QString cacheKey(const QString& path, int count) const;
    void promoteAndEvict(const QString& key);
    [[nodiscard]] QDir ensureThumbDir();

    [[nodiscard]] QString hwaccelArgs() const;

    // Extract all frames for a single video
    [[nodiscard]] QList<QByteArray> extractAllFramesForVideo(
        const QString& sourcePath, double duration, int count,
        const QString& outDir, uint hash);

    // Extract a single frame
    [[nodiscard]] std::optional<QByteArray> extractSingleFrameThrottled(
        const QString& sourcePath, double seekSec, const QString& outPath);

    // Parse args helper
    [[nodiscard]] static QStringList parseArgs(const QString& command);
};

} // namespace gopost::video_editor
