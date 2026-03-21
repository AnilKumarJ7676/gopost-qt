#include "video_editor/presentation/notifiers/video_thumbnail_provider.h"
#include "core/platform/platform_defaults.h"

#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QPainter>
#include <QColor>
#include <QFont>
#include <QQuickTextureFactory>
#include <QCoreApplication>
#include <QThread>
#include <algorithm>

namespace gopost::video_editor {

// ── ThumbnailCache ─────────────────────────────────────────────────────────

ThumbnailCache::ThumbnailCache(int maxEntries) : cache_(maxEntries) {}

bool ThumbnailCache::contains(const QString& key) const {
    QMutexLocker lock(&mutex_);
    return cache_.contains(key);
}

QImage ThumbnailCache::get(const QString& key) const {
    QMutexLocker lock(&mutex_);

    // 1. Memory cache
    if (auto* img = cache_.object(key)) {
        ++memoryHits_;
        return *img;
    }

    // 2. Disk cache (skipped on platforms without persistent filesystem)
    if (platform::Defaults::forCurrentPlatform().enableDiskCache) {
        QImage diskImg = loadFromDisk(key);
        if (!diskImg.isNull()) {
            ++diskHits_;
            cache_.insert(key, new QImage(diskImg));
            return diskImg;
        }
    }

    ++misses_;
    return {};
}

void ThumbnailCache::insert(const QString& key, const QImage& img) {
    QMutexLocker lock(&mutex_);
    cache_.insert(key, new QImage(img));
    if (platform::Defaults::forCurrentPlatform().enableDiskCache)
        saveToDisk(key, img);
}

void ThumbnailCache::clear() {
    QMutexLocker lock(&mutex_);
    cache_.clear();
    memoryHits_ = 0;
    diskHits_ = 0;
    misses_ = 0;
}

QDir ThumbnailCache::ensureDiskDir() const {
    if (diskDir_.has_value()) return *diskDir_;
    const auto tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    diskDir_ = QDir(QStringLiteral("%1/gopost_qml_thumbs").arg(tmp));
    if (!diskDir_->exists()) diskDir_->mkpath(QStringLiteral("."));
    return *diskDir_;
}

QString ThumbnailCache::diskPath(const QString& key) const {
    const auto hash = QCryptographicHash::hash(key.toUtf8(),
                                                QCryptographicHash::Md5).toHex();
    return QStringLiteral("%1/%2.jpg").arg(ensureDiskDir().path(), QString::fromLatin1(hash));
}

QImage ThumbnailCache::loadFromDisk(const QString& key) const {
    const QString path = diskPath(key);
    if (!QFileInfo::exists(path)) return {};
    QImage img;
    if (img.load(path, "JPEG")) return img;
    return {};
}

void ThumbnailCache::saveToDisk(const QString& key, const QImage& img) const {
    if (img.isNull()) return;
    const QString path = diskPath(key);
    img.save(path, "JPEG", 85);
}

void ThumbnailCache::purgeDiskCache(qint64 maxBytes) {
    const QDir dir = ensureDiskDir();
    auto entries = dir.entryInfoList({QStringLiteral("*.jpg")}, QDir::Files, QDir::Time);

    qint64 totalSize = 0;
    for (const auto& fi : entries) totalSize += fi.size();

    if (totalSize <= maxBytes) return;

    // Remove oldest files (sorted newest-first, so iterate from end)
    int removed = 0;
    for (int i = entries.size() - 1; i >= 0 && totalSize > maxBytes; --i) {
        totalSize -= entries[i].size();
        QFile::remove(entries[i].absoluteFilePath());
        ++removed;
    }
    qDebug() << "[ThumbnailCache] Purged" << removed << "disk cache files,"
             << "remaining:" << totalSize / 1024 << "KB";
}

// ── VideoThumbnailProvider ─────────────────────────────────────────────────

VideoThumbnailProvider::VideoThumbnailProvider()
    : QQuickAsyncImageProvider()
    , cache_(platform::Defaults::forCurrentPlatform().thumbnailMemoryCacheEntries)
    , coordinator_(nullptr)
{
    const auto defaults = platform::Defaults::forCurrentPlatform();
    pool_.setMaxThreadCount(defaults.maxDecoders);
    coordinator_ = new BatchCoordinator(&cache_, &pool_);
    qDebug() << "[Thumbnails] provider created: pool=" << defaults.maxDecoders
             << "cache=" << defaults.thumbnailMemoryCacheEntries
             << "disk=" << (defaults.enableDiskCache ? "on" : "off");
}

VideoThumbnailProvider::~VideoThumbnailProvider() {
    pool_.waitForDone(3000);
    delete coordinator_;
}

QQuickImageResponse* VideoThumbnailProvider::requestImageResponse(
    const QString& id, const QSize& /*requestedSize*/)
{
    // Parse id: "base64path@timeSeconds" or "base64path@timeSeconds@height"
    QString pathBase64;
    double timeSec = 0.0;
    int height = 45;

    if (id.contains('@')) {
        auto parts = id.split('@');
        pathBase64 = parts[0];
        if (parts.size() >= 2) timeSec = parts[1].toDouble();
        if (parts.size() >= 3) height = parts[2].toInt();
    } else {
        int lastSlash = id.lastIndexOf('/');
        if (lastSlash >= 0) {
            pathBase64 = id.left(lastSlash);
            timeSec = id.mid(lastSlash + 1).toDouble();
        }
    }
    if (height <= 0) height = 45;

    QString filePath = QString::fromUtf8(QByteArray::fromBase64(pathBase64.toUtf8()));

    // Check cache — if hit, deliver immediately (no batch needed)
    QString cacheKey = filePath + "@" + QString::number(timeSec, 'f', 1);
    QImage cached = cache_.get(cacheKey);
    if (!cached.isNull()) {
        auto* response = new BatchThumbnailResponse();
        response->deliver(cached);
        return response;
    }

    // Queue for batch extraction
    auto* response = new BatchThumbnailResponse();
    BatchCoordinator::PendingRequest req{timeSec, height, response, response->aliveFlag()};
    coordinator_->enqueue(filePath, req);
    return response;
}

// ── BatchThumbnailResponse ────────────────────────────────────────────────

BatchThumbnailResponse::BatchThumbnailResponse()
    : QQuickImageResponse()
{
}

BatchThumbnailResponse::~BatchThumbnailResponse() {
    // Signal worker threads that this response is destroyed
    alive_->store(false, std::memory_order_release);
}

void BatchThumbnailResponse::deliver(const QImage& img) {
    result_ = img;
    emit finished();
}

QQuickTextureFactory* BatchThumbnailResponse::textureFactory() const {
    return QQuickTextureFactory::textureFactoryForImage(result_);
}

// ── BatchCoordinator ──────────────────────────────────────────────────────

BatchCoordinator::BatchCoordinator(ThumbnailCache* cache, QThreadPool* pool, QObject* parent)
    : QObject(parent)
    , cache_(cache)
    , pool_(pool)
{
    collectTimer_.setSingleShot(true);
    collectTimer_.setInterval(200);  // 200ms collection window — gives staggered clips time to enqueue
    connect(&collectTimer_, &QTimer::timeout, this, &BatchCoordinator::dispatchBatches);
}

void BatchCoordinator::enqueue(const QString& filePath, const PendingRequest& req) {
    QMutexLocker lock(&mutex_);
    pending_[filePath].append(req);

    // Start collection timer if not already running (thread-safe invocation)
    if (!collectTimer_.isActive()) {
        QMetaObject::invokeMethod(&collectTimer_, "start", Qt::QueuedConnection);
    }
}

void BatchCoordinator::dispatchBatches() {
    QMutexLocker lock(&mutex_);

    if (pending_.isEmpty()) return;

    // Cap concurrent active files to avoid resource exhaustion when many
    // clips are added at once (each worker spawns ffmpeg processes).
    // The thread pool already limits parallelism, but capping active files
    // prevents queueing 20+ runnables that would each open ffmpeg pipes.
    static constexpr int kMaxActiveFiles = 4;

    int totalRequests = 0;
    int fileCount = 0;

    for (auto it = pending_.begin(); it != pending_.end(); ) {
        const QString& filePath = it.key();

        // Skip files with active workers — they'll be dispatched when the worker finishes
        if (activeFiles_.contains(filePath)) {
            ++it;
            continue;
        }

        // Don't dispatch more than kMaxActiveFiles at once
        if (static_cast<int>(activeFiles_.size()) >= kMaxActiveFiles) break;

        QList<BatchFileWorker::Request> workerRequests;
        for (const auto& req : it.value()) {
            // Skip already-deleted responses (QML destroyed the Image element)
            if (!req.alive || !req.alive->load(std::memory_order_acquire)) continue;
            workerRequests.append({req.timeSec, req.height, req.response, req.alive});
        }

        totalRequests += workerRequests.size();
        fileCount++;
        activeFiles_.insert(filePath);

        auto* worker = new BatchFileWorker(filePath, std::move(workerRequests), cache_, this);
        worker->setAutoDelete(true);
        pool_->start(worker);

        it = pending_.erase(it);
    }

    if (totalRequests > 0) {
        qDebug() << "[Thumbnails] dispatched" << totalRequests
                 << "requests across" << fileCount << "files"
                 << "(active:" << activeFiles_.size()
                 << "pending:" << pending_.size() << "files remaining)";
    }
}

void BatchCoordinator::onFileComplete(const QString& filePath) {
    {
        QMutexLocker lock(&mutex_);
        activeFiles_.remove(filePath);
    }

    // Check if more requests arrived while the worker was running
    dispatchBatches();
}

// ── BatchFileWorker ───────────────────────────────────────────────────────

BatchFileWorker::BatchFileWorker(const QString& filePath, QList<Request> requests,
                                 ThumbnailCache* cache, BatchCoordinator* coordinator)
    : filePath_(filePath)
    , requests_(std::move(requests))
    , cache_(cache)
    , coordinator_(coordinator)
{
}

void BatchFileWorker::run() {
    if (requests_.isEmpty()) {
        QMetaObject::invokeMethod(coordinator_, "onFileComplete",
                                  Qt::QueuedConnection, Q_ARG(QString, filePath_));
        return;
    }

    // Sort by timestamp — forward-only seeking is significantly faster
    std::sort(requests_.begin(), requests_.end(),
              [](const Request& a, const Request& b) {
                  return a.timeSec < b.timeSec;
              });

    const QString fileName = QFileInfo(filePath_).fileName();
    qDebug() << "[Thumbnails] batch start:" << fileName
             << "frames:" << requests_.size()
             << "range:" << requests_.first().timeSec
             << "-" << requests_.last().timeSec << "sec";

    int extracted = 0;
    int cached = 0;

    for (auto& req : requests_) {
        // Thread-safe guard: skip if QML destroyed the response object
        if (!req.alive || !req.alive->load(std::memory_order_acquire)) continue;

        QString cacheKey = filePath_ + "@" + QString::number(req.timeSec, 'f', 1);

        // Check cache (may have been filled by a duplicate request in this batch)
        QImage img = cache_->get(cacheKey);
        if (!img.isNull()) {
            // Deliver on main thread to avoid cross-thread access
            auto alive = req.alive;
            auto* resp = req.response;
            QMetaObject::invokeMethod(coordinator_, [alive, resp, img]() {
                if (alive->load(std::memory_order_acquire))
                    resp->deliver(img);
            }, Qt::QueuedConnection);
            cached++;
            continue;
        }

        int width = static_cast<int>(req.height * 16.0 / 9.0);
        img = extractFramePipe(filePath_, req.timeSec, width, req.height);

        cache_->insert(cacheKey, img);
        // Deliver on main thread to avoid cross-thread access
        auto alive = req.alive;
        auto* resp = req.response;
        QMetaObject::invokeMethod(coordinator_, [alive, resp, img]() {
            if (alive->load(std::memory_order_acquire))
                resp->deliver(img);
        }, Qt::QueuedConnection);
        extracted++;
    }

    qDebug() << "[Thumbnails] batch done:" << fileName
             << "extracted:" << extracted << "cached:" << cached;

    // Notify coordinator that this file is done (may dispatch more pending requests)
    QMetaObject::invokeMethod(coordinator_, "onFileComplete",
                              Qt::QueuedConnection, Q_ARG(QString, filePath_));
}

QImage BatchFileWorker::extractFramePipe(const QString& filePath, double timeSec,
                                          int width, int height) {
    if (!QFileInfo::exists(filePath)) {
        return makePlaceholder(width, height, timeSec);
    }

    // Write thumbnail to a temp file using system() to completely bypass
    // Qt's QProcess and its internal QRingBuffer which overflows on
    // H.265 MKV files with large metadata streams.
    QString tmpPath = QDir::tempPath() + QStringLiteral("/gopost_thumb_%1_%2.jpg")
        .arg(QCoreApplication::applicationPid())
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16);

    // Remove any stale file from a previous run
    QFile::remove(tmpPath);

    // Build command — use system() to avoid QProcess entirely
    QString cmd = QStringLiteral(
        "ffmpeg -y -nostdin -loglevel quiet -ss %1 -i \"%2\" "
        "-vframes 1 -vf \"scale=%3:%4:force_original_aspect_ratio=decrease,"
        "pad=%3:%4:(ow-iw)/2:(oh-ih)/2:color=0D0D1A\" -q:v 2 \"%5\"")
        .arg(QString::number(timeSec, 'f', 2))
        .arg(filePath)
        .arg(width)
        .arg(height)
        .arg(tmpPath);

    int ret = system(cmd.toLocal8Bit().constData());

    QImage frame;
    if (ret == 0 && QFileInfo::exists(tmpPath)) {
        frame.load(tmpPath);
    }

    QFile::remove(tmpPath);

    if (!frame.isNull()) {
        return frame;
    }

    return makePlaceholder(width, height, timeSec);
}

QImage BatchFileWorker::makePlaceholder(int width, int height, double timeSec) {
    QImage placeholder(width, height, QImage::Format_RGBA8888);
    placeholder.fill(QColor(20, 20, 40));

    QPainter painter(&placeholder);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(0, 0, width, height, QColor(30, 30, 55));

    painter.setPen(QColor(120, 120, 160));
    QFont font("monospace", 7);
    painter.setFont(font);
    int mins = static_cast<int>(timeSec) / 60;
    int secs = static_cast<int>(timeSec) % 60;
    painter.drawText(QRect(0, 0, width, height), Qt::AlignCenter,
                     QStringLiteral("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0')));
    painter.end();

    return placeholder;
}

} // namespace gopost::video_editor
