#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <QMap>
#include <QFuture>
#include <vector>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// ClipThumbRequest
// ---------------------------------------------------------------------------
struct ClipThumbRequest {
    QString sourcePath;
    double  sourceDuration = 0.0;
    int     count          = 8;
    int     priority       = 0; // higher = more urgent
};

// ---------------------------------------------------------------------------
// ThumbnailProvider — async thumbnail extraction with bounded LRU caching
// ---------------------------------------------------------------------------
class ThumbnailProvider : public QObject {
    Q_OBJECT

public:
    explicit ThumbnailProvider(QObject* parent = nullptr);
    ~ThumbnailProvider() override;

    /// Request thumbnails for a clip. Results arrive via thumbnailsReady signal.
    Q_INVOKABLE void requestThumbnails(const QString& sourcePath,
                                        double duration, int count,
                                        int priority = 0);

    /// Synchronous cache lookup. Returns empty vector on cache miss.
    std::vector<QImage> cached(const QString& sourcePath) const;

    /// Clear all cached thumbnails.
    Q_INVOKABLE void clearCache();

signals:
    void thumbnailsReady(const QString& sourcePath,
                         const QList<QImage>& thumbnails);

private:
    static constexpr int kMaxCacheEntries = 100;

    // QMap preserves insertion order for LRU eviction (oldest = begin())
    QMap<QString, std::vector<QImage>> cache_;

    void extractThumbnails(const ClipThumbRequest& request);
    void promoteAndEvict(const QString& key);
};

} // namespace gopost::video_editor
