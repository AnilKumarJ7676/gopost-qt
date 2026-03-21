#include "video_editor/data/services/thumbnail_service.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#include <QDebug>
#include <cmath>

namespace gopost::video_editor {

// Static members
ThumbnailService::HwAccelStatus ThumbnailService::hwAccelStatus_ = HwAccelStatus::Unknown;
QString ThumbnailService::hwaccelName_;
bool ThumbnailService::gpuProbed_ = false;

ThumbnailService::ThumbnailService()
    : ffmpeg_(FfmpegRunner::create()) {}

ThumbnailService& ThumbnailService::instance() {
    static ThumbnailService inst;
    return inst;
}

void ThumbnailService::ensureGpuProbed() {
    if (gpuProbed_) return;
    gpuProbed_ = true;
    probeGpu();
}

void ThumbnailService::probeGpu() {
    hwAccelStatus_ = HwAccelStatus::Probing;

    // Use system() to avoid QProcess QRingBuffer overflow entirely.
    QString tmpPath = QDir::tempPath() + QStringLiteral("/gopost_hwaccel_%1.txt")
        .arg(QCoreApplication::applicationPid());
    QString cmd = QStringLiteral("ffmpeg -hwaccels -hide_banner > \"%1\" 2>NUL").arg(tmpPath);
    system(cmd.toLocal8Bit().constData());
    QByteArray rawOutput;
    QFile tmpFile(tmpPath);
    if (tmpFile.exists() && tmpFile.open(QIODevice::ReadOnly)) {
        rawOutput = tmpFile.readAll();
        tmpFile.close();
    }
    QFile::remove(tmpPath);
    if (rawOutput.isEmpty()) {
        hwAccelStatus_ = HwAccelStatus::Unavailable;
        return;
    }

    const auto output = QString::fromUtf8(rawOutput);
    const auto lines = output.split(QLatin1Char('\n'));

    // Priority: NVIDIA CUDA > D3D11VA > DXVA2 > QSV > VAAPI > VideoToolbox
    static const QStringList priority = {
        QStringLiteral("cuda"), QStringLiteral("d3d11va"),
        QStringLiteral("dxva2"), QStringLiteral("qsv"),
        QStringLiteral("vaapi"), QStringLiteral("videotoolbox"),
    };

    QStringList available;
    for (const auto& line : lines) {
        const auto trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith(QLatin1String("Hardware")))
            available.append(trimmed);
    }

    for (const auto& preferred : priority) {
        if (available.contains(preferred)) {
            hwaccelName_ = preferred;
            hwAccelStatus_ = HwAccelStatus::Available;
            qDebug() << "[GPU] Hardware decoder available:" << preferred;
            return;
        }
    }

    if (!available.isEmpty()) {
        hwaccelName_ = QStringLiteral("auto");
        hwAccelStatus_ = HwAccelStatus::Available;
        qDebug() << "[GPU] Using auto hwaccel, available:" << available.join(QStringLiteral(", "));
        return;
    }

    hwAccelStatus_ = HwAccelStatus::Unavailable;
    qDebug() << "[GPU] No hardware acceleration available";
}

QString ThumbnailService::cacheKey(const QString& path, int count) const {
    return QStringLiteral("%1::%2").arg(path).arg(count);
}

void ThumbnailService::promoteAndEvict(const QString& key) {
    auto value = cache_.take(key);
    cache_.insert(key, value);
    while (cache_.size() > kMaxCacheEntries) {
        cache_.erase(cache_.begin());
    }
}

QDir ThumbnailService::ensureThumbDir() {
    if (thumbDir_.has_value()) return *thumbDir_;
    const auto tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    thumbDir_ = QDir(QStringLiteral("%1/gopost_thumbs").arg(tmp));
    if (!thumbDir_->exists()) thumbDir_->mkpath(QStringLiteral("."));
    return *thumbDir_;
}

QString ThumbnailService::hwaccelArgs() const {
    if (hwAccelStatus_ != HwAccelStatus::Available || hwaccelName_.isEmpty())
        return {};
    return QStringLiteral("-hwaccel %1 ").arg(hwaccelName_);
}

QList<QByteArray> ThumbnailService::getCached(const QString& sourcePath, int count) {
    QMutexLocker lock(&cacheMutex_);
    const auto key = cacheKey(sourcePath, count);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        promoteAndEvict(key);
        return *it;
    }
    return {};
}

QList<QByteArray> ThumbnailService::extractThumbnails(
    const QString& sourcePath, double sourceDuration, int count)
{
    ensureGpuProbed();

    const auto key = cacheKey(sourcePath, count);

    {
        QMutexLocker lock(&cacheMutex_);
        if (cache_.contains(key)) return cache_[key];
    }

    const auto dir = ensureThumbDir();
    const uint hash = static_cast<uint>(qHash(sourcePath));

    // Check disk cache (no lock needed — disk I/O is thread-safe per-file)
    QList<QByteArray> thumbs;
    bool allCached = true;
    for (int i = 0; i < count; ++i) {
        QFile file(QStringLiteral("%1/t_%2_%3_%4.jpg")
            .arg(dir.path()).arg(hash).arg(count).arg(i));
        if (file.exists() && file.size() > 0 && file.open(QIODevice::ReadOnly)) {
            thumbs.append(file.readAll());
        } else {
            allCached = false;
            break;
        }
    }

    if (allCached && thumbs.size() == count) {
        QMutexLocker lock(&cacheMutex_);
        cache_[key] = thumbs;
        promoteAndEvict(key);
        return thumbs;
    }

    const double safeDuration = sourceDuration > 0.1 ? sourceDuration : 0.1;
    const auto result = extractAllFramesForVideo(
        sourcePath, safeDuration, count, dir.path(), hash);

    if (!result.isEmpty()) {
        QMutexLocker lock(&cacheMutex_);
        cache_[key] = result;
        promoteAndEvict(key);
    }
    return result;
}

QList<QByteArray> ThumbnailService::extractAllFramesForVideo(
    const QString& sourcePath, double duration, int count,
    const QString& outDir, uint hash)
{
    QList<QByteArray> results;
    for (int i = 0; i < count; ++i) {
        const double seekSec = count == 1 ? 0.0 : (duration * i / count);
        const auto outFile = QStringLiteral("%1/t_%2_%3_%4.jpg")
            .arg(outDir).arg(hash).arg(count).arg(i);
        const auto bytes = extractSingleFrameThrottled(sourcePath, seekSec, outFile);
        if (bytes.has_value())
            results.append(*bytes);
    }
    return results;
}

std::optional<QByteArray> ThumbnailService::extractSingleThumbnail(
    const QString& sourcePath, double timeSeconds)
{
    ensureGpuProbed();

    const auto singleKey = QStringLiteral("%1::single").arg(sourcePath);

    {
        QMutexLocker lock(&cacheMutex_);
        auto cached = cache_.find(singleKey);
        if (cached != cache_.end() && !cached->isEmpty())
            return cached->first();
    }

    const auto dir = ensureThumbDir();
    const uint hash = static_cast<uint>(qHash(sourcePath));
    const auto outFile = QStringLiteral("%1/ts_%2.jpg").arg(dir.path()).arg(hash);
    QFile file(outFile);

    if (file.exists() && file.size() > 0 && file.open(QIODevice::ReadOnly)) {
        const auto bytes = file.readAll();
        QMutexLocker lock(&cacheMutex_);
        cache_[singleKey] = {bytes};
        promoteAndEvict(singleKey);
        return bytes;
    }

    // Use system() instead of FfmpegRunner to avoid QProcess QRingBuffer overflow
    QString sysCmd = QStringLiteral(
        "ffmpeg -nostdin -y -loglevel quiet %1-ss %2 -i \"%3\" "
        "-frames:v 1 -update 1 -s %4x%5 -q:v 6 \"%6\"")
        .arg(hwaccelArgs())
        .arg(timeSeconds, 0, 'f', 3)
        .arg(sourcePath)
        .arg(kThumbWidth)
        .arg(kThumbHeight)
        .arg(outFile);

    int sysRet = system(sysCmd.toLocal8Bit().constData());

    if (sysRet == 0 && file.exists() && file.size() > 0 && file.open(QIODevice::ReadOnly)) {
        const auto bytes = file.readAll();
        QMutexLocker lock(&cacheMutex_);
        cache_[singleKey] = {bytes};
        promoteAndEvict(singleKey);
        return bytes;
    }
    return std::nullopt;
}

std::optional<QByteArray> ThumbnailService::extractSingleFrameThrottled(
    const QString& sourcePath, double seekSec, const QString& outPath)
{
    QFile file(outPath);
    if (file.exists() && file.size() > 0 && file.open(QIODevice::ReadOnly))
        return file.readAll();

    // Use system() instead of QProcess to completely avoid Qt's internal
    // QRingBuffer which overflows on H.265 MKV files.
    QString cmd = QStringLiteral(
        "ffmpeg -nostdin -y -loglevel quiet %1-ss %2 -i \"%3\" "
        "-frames:v 1 -update 1 -s %4x%5 -q:v 6 \"%6\"")
        .arg(hwaccelArgs())
        .arg(seekSec, 0, 'f', 3)
        .arg(sourcePath)
        .arg(kThumbWidth)
        .arg(kThumbHeight)
        .arg(outPath);

    int ret = system(cmd.toLocal8Bit().constData());

    if (ret == 0) {
        QFile outFile(outPath);
        if (outFile.exists() && outFile.size() > 0 && outFile.open(QIODevice::ReadOnly))
            return outFile.readAll();
    }

    // Clean up incomplete file
    if (QFile::exists(outPath)) QFile::remove(outPath);
    return std::nullopt;
}

QStringList ThumbnailService::parseArgs(const QString& command) {
    QStringList args;
    QString buffer;
    bool inQuote = false;
    QChar quoteChar;

    for (int i = 0; i < command.size(); ++i) {
        const QChar c = command[i];
        if (inQuote) {
            if (c == quoteChar) {
                inQuote = false;
            } else {
                buffer.append(c);
            }
        } else if (c == QLatin1Char('"') || c == QLatin1Char('\'')) {
            inQuote = true;
            quoteChar = c;
        } else if (c == QLatin1Char(' ')) {
            if (!buffer.isEmpty()) {
                args.append(buffer);
                buffer.clear();
            }
        } else {
            buffer.append(c);
        }
    }
    if (!buffer.isEmpty()) args.append(buffer);
    return args;
}

void ThumbnailService::clearCache() {
    QMutexLocker lock(&cacheMutex_);
    cache_.clear();
}

void ThumbnailService::purgeDiskCache(qint64 maxBytes) {
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
    qDebug() << "[ThumbnailService] Purged" << removed << "disk cache files,"
             << "remaining:" << totalSize / 1024 << "KB";
}

} // namespace gopost::video_editor
