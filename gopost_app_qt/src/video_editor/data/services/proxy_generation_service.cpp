#include "video_editor/data/services/proxy_generation_service.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace gopost::video_editor {

ProxyGenerationService::ProxyGenerationService()
    : ffmpeg_(FfmpegRunner::create()) {}

ProxyGenerationService& ProxyGenerationService::instance() {
    static ProxyGenerationService inst;
    return inst;
}

QDir ProxyGenerationService::ensureProxyDir() {
    if (proxyDir_.has_value()) return *proxyDir_;
    const auto support = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    proxyDir_ = QDir(QStringLiteral("%1/gopost_proxies").arg(support));
    if (!proxyDir_->exists()) proxyDir_->mkpath(QStringLiteral("."));
    return *proxyDir_;
}

QString ProxyGenerationService::proxyFileName(const QString& sourcePath) {
    const auto hash = QCryptographicHash::hash(sourcePath.toUtf8(), QCryptographicHash::Md5).toHex();
    return QStringLiteral("proxy_%1.mp4").arg(QString::fromLatin1(hash));
}

bool ProxyGenerationService::hasMoovAtom(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    // Read up to 128 KB -- faststart puts moov near the top.
    const auto bytes = file.read(131072);
    file.close();

    bool foundMoov = false;
    for (int i = 0; i < bytes.size() - 3; ++i) {
        if (bytes[i] == 0x6D && bytes[i + 1] == 0x6F &&
            bytes[i + 2] == 0x6F && bytes[i + 3] == 0x76) {
            foundMoov = true;
            break;
        }
    }
    return foundMoov;
}

std::optional<QString> ProxyGenerationService::existingProxyPath(const QString& sourcePath) {
    const auto dir = ensureProxyDir();
    const auto filePath = QStringLiteral("%1/%2").arg(dir.path(), proxyFileName(sourcePath));
    QFileInfo fi(filePath);

    if (fi.exists() && fi.size() > 0) {
        if (!hasMoovAtom(filePath)) {
            // Corrupt proxy -- delete so it can be regenerated.
            QFile::remove(filePath);
            return std::nullopt;
        }
        return filePath;
    }
    return std::nullopt;
}

std::optional<QString> ProxyGenerationService::generateProxy(
    const QString& sourcePath,
    std::function<void(double progress)> onProgress)
{
    const auto existing = existingProxyPath(sourcePath);
    if (existing.has_value()) return existing;

    const auto dir = ensureProxyDir();
    const auto outPath = QStringLiteral("%1/%2").arg(dir.path(), proxyFileName(sourcePath));

    const auto cmd = QStringLiteral(
        R"(-y -i "%1" -vf "scale=-2:%2" -c:v %3 -preset %4 -crf %5 -c:a aac -b:a %6k -movflags +faststart "%7")")
        .arg(sourcePath)
        .arg(kProxyHeight)
        .arg(QLatin1String(kProxyCodec))
        .arg(QLatin1String(kProxyPreset))
        .arg(kProxyCrf)
        .arg(kProxyAudioBitrate)
        .arg(outPath);

    if (onProgress) {
        ffmpeg_->onStatistics([&](int timeMs) {
            if (timeMs > 0) onProgress(timeMs / 1000.0);
        });
    }

    const auto result = ffmpeg_->execute(cmd);

    if (result.success) {
        QFileInfo outInfo(outPath);
        if (outInfo.exists() && outInfo.size() > 0)
            return outPath;
    }

    return std::nullopt;
}

void ProxyGenerationService::cancelAll() {
    ffmpeg_->cancel();
}

void ProxyGenerationService::clearProxyForSource(const QString& sourcePath) {
    const auto dir = ensureProxyDir();
    QFile::remove(QStringLiteral("%1/%2").arg(dir.path(), proxyFileName(sourcePath)));
}

int64_t ProxyGenerationService::clearAllProxies() {
    const auto dir = ensureProxyDir();
    if (!dir.exists()) return 0;

    int64_t freed = 0;
    QDirIterator it(dir.path(), QDir::Files);
    while (it.hasNext()) {
        it.next();
        freed += it.fileInfo().size();
        QFile::remove(it.filePath());
    }
    return freed;
}

int64_t ProxyGenerationService::getCacheSize() {
    const auto dir = ensureProxyDir();
    if (!dir.exists()) return 0;

    int64_t total = 0;
    QDirIterator it(dir.path(), QDir::Files);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

bool ProxyGenerationService::verifyProxy(const QString& proxyPath) {
    QFileInfo fi(proxyPath);
    if (!fi.exists() || fi.size() == 0) return false;
    return hasMoovAtom(proxyPath);
}

} // namespace gopost::video_editor
