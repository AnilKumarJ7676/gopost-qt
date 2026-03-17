#include "video_editor/data/services/native_thumbnail_service.h"

#include <QFile>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDebug>
#include <QThread>
#include <QElapsedTimer>
#include <cmath>
#include <cstring>

// Replicate the C struct layout from decoder_pool_api.h so we can fill it
// without a hard compile-time dependency on the native headers.
// These must match the ABI defined in gopost/decoder_pool_api.h.
struct GopostThumbnailRequest_FFI {
    char source_path[1024];
    double source_duration;
    int32_t count;
    int32_t thumb_width;
    int32_t thumb_height;
    int32_t priority; // GopostDecoderPriority
};

struct GopostThumbnailResult_FFI {
    const uint8_t* jpeg_data;
    int32_t jpeg_size;
    int32_t width;
    int32_t height;
    double timestamp;
};

// Thumbnail job status constants matching GopostThumbnailJobStatus enum
static constexpr int THUMB_STATUS_QUEUED      = 0;
static constexpr int THUMB_STATUS_IN_PROGRESS = 1;
static constexpr int THUMB_STATUS_COMPLETED   = 2;
static constexpr int THUMB_STATUS_FAILED      = 3;
static constexpr int THUMB_STATUS_CANCELLED   = 4;

namespace gopost::video_editor {

NativeThumbnailService::NativeThumbnailService(void* libHandle, void* enginePtr, int maxDecoders)
    : libHandle_(libHandle), enginePtr_(enginePtr), maxDecoders_(maxDecoders) {}

NativeThumbnailService::~NativeThumbnailService() {
    dispose();
}

// ---------------------------------------------------------------------------
// Symbol resolution from native shared library
// ---------------------------------------------------------------------------

bool NativeThumbnailService::resolveSymbols() {
    if (!libHandle_) return false;

    auto* lib = static_cast<QLibrary*>(libHandle_);
    if (!lib->isLoaded()) return false;

    fn_pool_create_      = reinterpret_cast<FnPoolCreate>(lib->resolve("gopost_decoder_pool_create"));
    fn_pool_destroy_     = reinterpret_cast<FnPoolDestroy>(lib->resolve("gopost_decoder_pool_destroy"));
    fn_pool_set_max_     = reinterpret_cast<FnPoolSetMax>(lib->resolve("gopost_decoder_pool_set_max"));
    fn_pool_flush_idle_  = reinterpret_cast<FnPoolFlushIdle>(lib->resolve("gopost_decoder_pool_flush_idle"));
    fn_gen_create_       = reinterpret_cast<FnGenCreate>(lib->resolve("gopost_thumbnail_generator_create"));
    fn_gen_destroy_      = reinterpret_cast<FnGenDestroy>(lib->resolve("gopost_thumbnail_generator_destroy"));
    fn_thumb_submit_     = reinterpret_cast<FnThumbSubmit>(lib->resolve("gopost_thumbnail_submit"));
    fn_thumb_cancel_     = reinterpret_cast<FnThumbCancel>(lib->resolve("gopost_thumbnail_cancel"));
    fn_thumb_cancel_all_ = reinterpret_cast<FnThumbCancelAll>(lib->resolve("gopost_thumbnail_cancel_all"));
    fn_thumb_status_     = reinterpret_cast<FnThumbStatus>(lib->resolve("gopost_thumbnail_job_status"));
    fn_thumb_result_count_ = reinterpret_cast<FnThumbResultCnt>(lib->resolve("gopost_thumbnail_result_count"));
    fn_thumb_get_result_ = reinterpret_cast<FnThumbGetResult>(lib->resolve("gopost_thumbnail_get_result"));

    // All symbols are required for the native path
    bool ok = fn_pool_create_ && fn_pool_destroy_ && fn_pool_set_max_ &&
              fn_pool_flush_idle_ && fn_gen_create_ && fn_gen_destroy_ &&
              fn_thumb_submit_ && fn_thumb_cancel_ && fn_thumb_cancel_all_ &&
              fn_thumb_status_ && fn_thumb_result_count_ && fn_thumb_get_result_;

    if (!ok) {
        qDebug() << "[NativeThumbnails] Failed to resolve one or more symbols from native library";
    }
    return ok;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void NativeThumbnailService::initialize() {
    if (initialized_) return;

    if (!resolveSymbols()) {
        qDebug() << "[NativeThumbnails] Native library not available — will use disk cache only";
        return;
    }

    auto* engine = static_cast<GopostEngine*>(enginePtr_);
    int err = fn_pool_create_(engine, maxDecoders_, &poolPtr_);
    if (err != 0 || !poolPtr_) {
        qDebug() << "[NativeThumbnails] Failed to create decoder pool, error:" << err;
        return;
    }

    err = fn_gen_create_(poolPtr_, engine, &genPtr_);
    if (err != 0 || !genPtr_) {
        qDebug() << "[NativeThumbnails] Failed to create thumbnail generator, error:" << err;
        fn_pool_destroy_(poolPtr_);
        poolPtr_ = nullptr;
        return;
    }

    initialized_ = true;
    qDebug() << "[NativeThumbnails] Initialized — pool:" << maxDecoders_
             << "decoders, thumb size:" << kThumbWidth << "x" << kThumbHeight;
}

void NativeThumbnailService::dispose() {
    if (genPtr_ && fn_gen_destroy_) {
        fn_gen_destroy_(genPtr_);
        genPtr_ = nullptr;
    }
    if (poolPtr_ && fn_pool_destroy_) {
        fn_pool_destroy_(poolPtr_);
        poolPtr_ = nullptr;
    }
    initialized_ = false;
}

void NativeThumbnailService::setMaxDecoders(int max) {
    if (poolPtr_ && fn_pool_set_max_ && max > 0) {
        fn_pool_set_max_(poolPtr_, max);
        qDebug() << "[NativeThumbnails] Max decoders set to" << max;
    }
}

void NativeThumbnailService::flushIdleDecoders() {
    if (poolPtr_ && fn_pool_flush_idle_) {
        fn_pool_flush_idle_(poolPtr_);
    }
}

// ---------------------------------------------------------------------------
// Cache helpers
// ---------------------------------------------------------------------------

QString NativeThumbnailService::cacheKey(const QString& path, int count) const {
    return QStringLiteral("%1::%2").arg(path).arg(count);
}

void NativeThumbnailService::promoteAndEvict(const QString& key) {
    auto value = cache_.take(key);
    cache_.insert(key, value);
    while (cache_.size() > kMaxCacheEntries) {
        cache_.erase(cache_.begin());
    }
}

QDir NativeThumbnailService::ensureThumbDir() {
    if (thumbDir_.has_value()) return *thumbDir_;
    const auto tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    thumbDir_ = QDir(QStringLiteral("%1/gopost_thumbs").arg(tmp));
    if (!thumbDir_->exists()) thumbDir_->mkpath(QStringLiteral("."));
    return *thumbDir_;
}

void NativeThumbnailService::saveToDisk(const QString& sourcePath, int count,
                                         const QList<QByteArray>& thumbs) {
    const auto dir = ensureThumbDir();
    const auto hash = QCryptographicHash::hash(sourcePath.toUtf8(),
                                                QCryptographicHash::Md5).toHex().left(16);
    for (int i = 0; i < thumbs.size(); ++i) {
        const QString filePath = QStringLiteral("%1/t_%2_%3_%4.jpg")
            .arg(dir.path(), QString::fromLatin1(hash))
            .arg(count).arg(i);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(thumbs[i]);
        }
    }
}

QList<QByteArray> NativeThumbnailService::loadFromDisk(const QString& sourcePath, int count) {
    const auto dir = ensureThumbDir();
    const auto hash = QCryptographicHash::hash(sourcePath.toUtf8(),
                                                QCryptographicHash::Md5).toHex().left(16);
    QList<QByteArray> thumbs;
    for (int i = 0; i < count; ++i) {
        const QString filePath = QStringLiteral("%1/t_%2_%3_%4.jpg")
            .arg(dir.path(), QString::fromLatin1(hash))
            .arg(count).arg(i);
        QFile file(filePath);
        if (!file.exists() || file.size() == 0 || !file.open(QIODevice::ReadOnly)) {
            return {}; // Partial disk cache is invalid
        }
        thumbs.append(file.readAll());
    }
    return thumbs;
}

bool NativeThumbnailService::pollJobUntilDone(int32_t jobId, int timeoutMs) {
    if (!fn_thumb_status_) return false;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        int status = fn_thumb_status_(genPtr_, jobId);
        if (status == THUMB_STATUS_COMPLETED) return true;
        if (status == THUMB_STATUS_FAILED || status == THUMB_STATUS_CANCELLED) return false;
        QThread::msleep(10);
    }
    // Timed out — cancel the job
    if (fn_thumb_cancel_) fn_thumb_cancel_(genPtr_, jobId);
    qDebug() << "[NativeThumbnails] Job" << jobId << "timed out after" << timeoutMs << "ms";
    return false;
}

// ---------------------------------------------------------------------------
// ThumbnailServiceInterface
// ---------------------------------------------------------------------------

QList<QByteArray> NativeThumbnailService::getCached(const QString& sourcePath, int count) {
    const auto key = cacheKey(sourcePath, count);

    // Check memory cache
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        promoteAndEvict(key);
        return *it;
    }

    // Check disk cache
    auto diskThumbs = loadFromDisk(sourcePath, count);
    if (!diskThumbs.isEmpty()) {
        cache_[key] = diskThumbs;
        promoteAndEvict(key);
        return diskThumbs;
    }

    return {};
}

QList<QByteArray> NativeThumbnailService::extractThumbnails(
    const QString& sourcePath,
    double sourceDuration,
    int count)
{
    const auto key = cacheKey(sourcePath, count);

    // 1. Memory cache hit
    if (cache_.contains(key)) {
        promoteAndEvict(key);
        return cache_[key];
    }

    // 2. Disk cache hit
    auto diskThumbs = loadFromDisk(sourcePath, count);
    if (!diskThumbs.isEmpty()) {
        cache_[key] = diskThumbs;
        promoteAndEvict(key);
        qDebug() << "[NativeThumbnails] Disk cache hit for" << QFileInfo(sourcePath).fileName()
                 << "count:" << count;
        return diskThumbs;
    }

    // 3. Native extraction
    if (!initialized_ || !fn_thumb_submit_) {
        qDebug() << "[NativeThumbnails] Not initialized — returning empty for"
                 << QFileInfo(sourcePath).fileName();
        return {};
    }

    // Build the C request struct
    GopostThumbnailRequest_FFI req{};
    QByteArray pathUtf8 = sourcePath.toUtf8();
    std::strncpy(req.source_path, pathUtf8.constData(),
                 sizeof(req.source_path) - 1);
    req.source_path[sizeof(req.source_path) - 1] = '\0';
    req.source_duration = sourceDuration;
    req.count = count;
    req.thumb_width = kThumbWidth;
    req.thumb_height = kThumbHeight;
    req.priority = 1; // GOPOST_DECODER_PRIORITY_MEDIUM

    int32_t jobId = fn_thumb_submit_(
        genPtr_,
        reinterpret_cast<const GopostThumbnailRequest*>(&req));
    if (jobId < 0) {
        qDebug() << "[NativeThumbnails] Submit failed for" << sourcePath;
        return {};
    }

    qDebug() << "[NativeThumbnails] Submitted job" << jobId
             << "for" << QFileInfo(sourcePath).fileName()
             << "count:" << count;

    // 4. Poll until done (synchronous — callers run this from worker threads)
    if (!pollJobUntilDone(jobId)) {
        return {};
    }

    // 5. Collect results
    int32_t resultCount = fn_thumb_result_count_(genPtr_, jobId);
    QList<QByteArray> thumbs;
    thumbs.reserve(resultCount);

    for (int32_t i = 0; i < resultCount; ++i) {
        GopostThumbnailResult_FFI result{};
        int err = fn_thumb_get_result_(
            genPtr_, jobId, i,
            reinterpret_cast<GopostThumbnailResult*>(&result));
        if (err == 0 && result.jpeg_data && result.jpeg_size > 0) {
            thumbs.append(QByteArray(reinterpret_cast<const char*>(result.jpeg_data),
                                     result.jpeg_size));
        }
    }

    if (!thumbs.isEmpty()) {
        // 6. Persist to memory + disk cache
        cache_[key] = thumbs;
        promoteAndEvict(key);
        saveToDisk(sourcePath, count, thumbs);

        qDebug() << "[NativeThumbnails] Extracted" << thumbs.size()
                 << "thumbnails for" << QFileInfo(sourcePath).fileName();
    }

    return thumbs;
}

std::optional<QByteArray> NativeThumbnailService::extractSingleThumbnail(
    const QString& sourcePath, double timeSeconds)
{
    // For a single thumbnail, use count=1 with the time as the "duration"
    // so the generator samples at t=0 (which maps to the start of the range)
    const auto results = extractThumbnails(sourcePath, timeSeconds * 2.0, 1);
    if (!results.isEmpty()) return results.first();
    return std::nullopt;
}

void NativeThumbnailService::clearCache() {
    cache_.clear();
    if (genPtr_ && fn_thumb_cancel_all_) {
        fn_thumb_cancel_all_(genPtr_);
    }
    qDebug() << "[NativeThumbnails] Cache cleared";
}

void NativeThumbnailService::purgeDiskCache(qint64 maxBytes) {
    const auto dir = ensureThumbDir();
    auto entries = dir.entryInfoList({QStringLiteral("*.jpg")}, QDir::Files, QDir::Time);

    qint64 totalSize = 0;
    for (const auto& fi : entries) totalSize += fi.size();

    if (totalSize <= maxBytes) return;

    int removed = 0;
    for (int i = entries.size() - 1; i >= 0 && totalSize > maxBytes; --i) {
        totalSize -= entries[i].size();
        QFile::remove(entries[i].absoluteFilePath());
        ++removed;
    }
    qDebug() << "[NativeThumbnails] Purged" << removed << "disk cache files,"
             << "remaining:" << totalSize / 1024 << "KB";
}

} // namespace gopost::video_editor
