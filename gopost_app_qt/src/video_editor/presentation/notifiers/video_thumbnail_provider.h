#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QImage>
#include <QMutex>
#include <QCache>
#include <QString>
#include <QRunnable>
#include <QThreadPool>
#include <QMap>
#include <QList>
#include <QSet>
#include <QTimer>
#include <QDir>
#include <QObject>
#include <optional>

namespace gopost::video_editor {

/// Shared cache for extracted thumbnails (thread-safe).
/// Two-tier: in-memory QCache + on-disk JPEG cache in temp directory.
class ThumbnailCache {
public:
    ThumbnailCache(int maxEntries = 200);

    bool contains(const QString& key) const;
    QImage get(const QString& key) const;
    void insert(const QString& key, const QImage& img);
    void clear();

    int memoryHits() const { return memoryHits_; }
    int diskHits() const { return diskHits_; }
    int misses() const { return misses_; }

    /// Remove oldest disk cache files when total size exceeds limit.
    void purgeDiskCache(qint64 maxBytes = 200 * 1024 * 1024);

private:
    QDir ensureDiskDir() const;
    QString diskPath(const QString& key) const;
    QImage loadFromDisk(const QString& key) const;
    void saveToDisk(const QString& key, const QImage& img) const;

    mutable QMutex mutex_;
    mutable QCache<QString, QImage> cache_;
    mutable std::optional<QDir> diskDir_;
    mutable int memoryHits_{0};
    mutable int diskHits_{0};
    mutable int misses_{0};
};

// ---------------------------------------------------------------------------
// BatchThumbnailResponse — waits for result from a batch worker
// ---------------------------------------------------------------------------
class BatchThumbnailResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    BatchThumbnailResponse();

    /// Called by the batch worker when the frame is ready.
    void deliver(const QImage& img);

    QQuickTextureFactory* textureFactory() const override;

private:
    QImage result_;
};

// ---------------------------------------------------------------------------
// BatchCoordinator — QObject helper that coordinates batch dispatch.
//
// Collects thumbnail requests for a short window (50ms), then dispatches
// a BatchFileWorker per unique file. Workers process all frames for one
// file sequentially in sorted timestamp order (forward seeks are fast).
// ---------------------------------------------------------------------------
class BatchCoordinator : public QObject {
    Q_OBJECT
public:
    struct PendingRequest {
        double timeSec;
        int height;
        BatchThumbnailResponse* response;
    };

    explicit BatchCoordinator(ThumbnailCache* cache, QThreadPool* pool, QObject* parent = nullptr);

    /// Add a request. Starts the collection timer if not already running.
    void enqueue(const QString& filePath, const PendingRequest& req);

    /// Called by worker threads when a file batch is complete.
    Q_INVOKABLE void onFileComplete(const QString& filePath);

public slots:
    void dispatchBatches();

private:
    ThumbnailCache* cache_;
    QThreadPool* pool_;
    QTimer collectTimer_;

    QMutex mutex_;
    QMap<QString, QList<PendingRequest>> pending_;
    QSet<QString> activeFiles_;  // files currently being processed
};

// ---------------------------------------------------------------------------
// BatchFileWorker — processes all queued thumbnail requests for ONE file
//
// Extracts frames sequentially sorted by timestamp (forward-only seeking).
// Uses ffmpeg with pipe output (image2pipe) to avoid temp file I/O.
// ---------------------------------------------------------------------------
class BatchFileWorker : public QRunnable {
public:
    struct Request {
        double timeSec;
        int height;
        BatchThumbnailResponse* response;
    };

    BatchFileWorker(const QString& filePath, QList<Request> requests,
                    ThumbnailCache* cache, BatchCoordinator* coordinator);
    void run() override;

private:
    QString filePath_;
    QList<Request> requests_;
    ThumbnailCache* cache_;
    BatchCoordinator* coordinator_;

    QImage extractFramePipe(const QString& filePath, double timeSec, int width, int height);
    QImage makePlaceholder(int width, int height, double timeSec);
};

// ---------------------------------------------------------------------------
// VideoThumbnailProvider — async image provider with batch coordination
//
// URL format: image://videothumbnail/<base64_path>@<timeSeconds>[@<height>]
// ---------------------------------------------------------------------------
class VideoThumbnailProvider : public QQuickAsyncImageProvider {
public:
    VideoThumbnailProvider();
    ~VideoThumbnailProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                               const QSize& requestedSize) override;

private:
    ThumbnailCache cache_;
    QThreadPool pool_;
    BatchCoordinator* coordinator_;  // owned, lives on main thread
};

} // namespace gopost::video_editor
