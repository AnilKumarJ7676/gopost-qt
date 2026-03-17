#pragma once

#include <QString>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <QDir>
#include <QMap>
#include <QLibrary>
#include <memory>
#include <optional>

#include "video_editor/domain/services/thumbnail_service.h"

// Forward-declare opaque C types from decoder_pool_api.h
struct GopostEngine;
struct GopostDecoderPool;
struct GopostThumbnailGenerator;
struct GopostThumbnailRequest;
struct GopostThumbnailResult;

namespace gopost::video_editor {

/// Thumbnail service backed by the native DecoderPool + ThumbnailGenerator.
///
/// Uses the engine's DecoderPool to process videos sequentially — one video at a
/// time — preventing decoder resource exhaustion. Falls back gracefully when the
/// native library is unavailable.
class NativeThumbnailService : public ThumbnailServiceInterface {
public:
    NativeThumbnailService(void* libHandle, void* enginePtr, int maxDecoders = 2);
    ~NativeThumbnailService();

    bool isInitialized() const { return initialized_; }

    void initialize();
    void dispose();

    void setMaxDecoders(int max);
    void flushIdleDecoders();

    // ThumbnailServiceInterface
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
    // --- Native library handle & engine ---
    void* libHandle_{nullptr};
    void* enginePtr_{nullptr};
    int maxDecoders_{2};

    GopostDecoderPool*        poolPtr_{nullptr};
    GopostThumbnailGenerator* genPtr_{nullptr};
    bool initialized_{false};

    // --- Function pointers resolved from native library ---
    using FnPoolCreate    = int(*)(GopostEngine*, int32_t, GopostDecoderPool**);
    using FnPoolDestroy   = void(*)(GopostDecoderPool*);
    using FnPoolSetMax    = int(*)(GopostDecoderPool*, int32_t);
    using FnPoolFlushIdle = int(*)(GopostDecoderPool*);
    using FnGenCreate     = int(*)(GopostDecoderPool*, GopostEngine*, GopostThumbnailGenerator**);
    using FnGenDestroy    = void(*)(GopostThumbnailGenerator*);
    using FnThumbSubmit   = int32_t(*)(GopostThumbnailGenerator*, const GopostThumbnailRequest*);
    using FnThumbCancel   = int(*)(GopostThumbnailGenerator*, int32_t);
    using FnThumbCancelAll= int(*)(GopostThumbnailGenerator*);
    using FnThumbStatus   = int(*)(const GopostThumbnailGenerator*, int32_t);
    using FnThumbResultCnt= int32_t(*)(const GopostThumbnailGenerator*, int32_t);
    using FnThumbGetResult= int(*)(const GopostThumbnailGenerator*, int32_t, int32_t, GopostThumbnailResult*);

    FnPoolCreate     fn_pool_create_{nullptr};
    FnPoolDestroy    fn_pool_destroy_{nullptr};
    FnPoolSetMax     fn_pool_set_max_{nullptr};
    FnPoolFlushIdle  fn_pool_flush_idle_{nullptr};
    FnGenCreate      fn_gen_create_{nullptr};
    FnGenDestroy     fn_gen_destroy_{nullptr};
    FnThumbSubmit    fn_thumb_submit_{nullptr};
    FnThumbCancel    fn_thumb_cancel_{nullptr};
    FnThumbCancelAll fn_thumb_cancel_all_{nullptr};
    FnThumbStatus    fn_thumb_status_{nullptr};
    FnThumbResultCnt fn_thumb_result_count_{nullptr};
    FnThumbGetResult fn_thumb_get_result_{nullptr};

    bool resolveSymbols();

    // --- LRU memory cache + disk cache ---
    static constexpr int kThumbWidth = 160;
    static constexpr int kThumbHeight = 90;
    static constexpr int kMaxCacheEntries = 200;

    QMap<QString, QList<QByteArray>> cache_;
    QMap<int, QString> jobToKey_;
    std::optional<QDir> thumbDir_;

    [[nodiscard]] QString cacheKey(const QString& path, int count) const;
    void promoteAndEvict(const QString& key);
    [[nodiscard]] QDir ensureThumbDir();
    void saveToDisk(const QString& sourcePath, int count, const QList<QByteArray>& thumbs);
    QList<QByteArray> loadFromDisk(const QString& sourcePath, int count);
    bool pollJobUntilDone(int32_t jobId, int timeoutMs = 30000);
};

} // namespace gopost::video_editor
