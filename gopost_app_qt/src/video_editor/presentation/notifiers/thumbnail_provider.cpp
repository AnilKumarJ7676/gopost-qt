#include "video_editor/presentation/notifiers/thumbnail_provider.h"
#include "video_editor/data/services/thumbnail_service.h"

#include <QDebug>
#include <QtConcurrent>

namespace gopost::video_editor {

ThumbnailProvider::ThumbnailProvider(QObject* parent) : QObject(parent) {}
ThumbnailProvider::~ThumbnailProvider() = default;

void ThumbnailProvider::promoteAndEvict(const QString& key) {
    auto value = cache_.take(key);
    cache_.insert(key, std::move(value));
    while (cache_.size() > kMaxCacheEntries) {
        cache_.erase(cache_.begin());
    }
}

void ThumbnailProvider::requestThumbnails(const QString& sourcePath,
                                           double duration, int count,
                                           int priority) {
    Q_UNUSED(priority);

    // Check in-memory cache first
    auto it = cache_.find(sourcePath);
    if (it != cache_.end() && static_cast<int>(it->size()) >= count) {
        promoteAndEvict(sourcePath);
        QList<QImage> list;
        for (const auto& img : *it) list.append(img);
        emit thumbnailsReady(sourcePath, list);
        return;
    }

    ClipThumbRequest req;
    req.sourcePath    = sourcePath;
    req.sourceDuration = duration;
    req.count         = count;
    req.priority      = priority;

    // Run extraction on thread pool using real ThumbnailService
    QtConcurrent::run([this, req]() {
        extractThumbnails(req);
    });
}

std::vector<QImage> ThumbnailProvider::cached(const QString& sourcePath) const {
    auto it = cache_.find(sourcePath);
    return (it != cache_.end()) ? *it : std::vector<QImage>{};
}

void ThumbnailProvider::clearCache() {
    cache_.clear();
    ThumbnailService::instance().clearCache();
}

void ThumbnailProvider::extractThumbnails(const ClipThumbRequest& request) {
    auto& service = ThumbnailService::instance();

    // Use the real FFmpeg-based thumbnail service
    QList<QByteArray> rawThumbs = service.extractThumbnails(
        request.sourcePath, request.sourceDuration, request.count);

    std::vector<QImage> thumbnails;

    if (!rawThumbs.isEmpty()) {
        // Convert raw JPEG bytes to QImages
        for (const auto& bytes : rawThumbs) {
            QImage img;
            if (img.loadFromData(bytes, "JPEG")) {
                thumbnails.push_back(std::move(img));
            } else if (img.loadFromData(bytes)) {
                thumbnails.push_back(std::move(img));
            }
        }
    }

    // Fallback: if FFmpeg extraction failed, generate colored placeholders
    if (thumbnails.empty()) {
        qDebug() << "[ThumbnailProvider] FFmpeg extraction returned empty for:"
                 << request.sourcePath << "- using placeholders";
        for (int i = 0; i < request.count; ++i) {
            QImage img(80, 45, QImage::Format_ARGB32);
            img.fill(QColor(30 + i * 10, 30, 60));
            thumbnails.push_back(std::move(img));
        }
    }

    // Cache and notify on main thread
    QMetaObject::invokeMethod(this, [this, path = request.sourcePath,
                                     thumbs = std::move(thumbnails)]() mutable {
        cache_[path] = thumbs;
        promoteAndEvict(path);
        QList<QImage> list;
        for (const auto& img : thumbs) list.append(img);
        emit thumbnailsReady(path, list);
    }, Qt::QueuedConnection);
}

} // namespace gopost::video_editor
