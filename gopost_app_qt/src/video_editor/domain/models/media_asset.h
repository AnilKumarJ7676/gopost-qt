#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QDateTime>
#include <QFileInfo>
#include <optional>
#include <cstdint>

namespace gopost::video_editor {

// ── Enums ───────────────────────────────────────────────────────────────────

enum class MediaAssetType : int { Video = 0, Image, Audio };
enum class MediaAssetStatus : int { Online = 0, Offline, Probing };

// ── MediaAsset ──────────────────────────────────────────────────────────────

struct MediaAsset {
    QString id;
    QString filePath;
    QString fileName;
    MediaAssetType type{MediaAssetType::Video};
    MediaAssetStatus status{MediaAssetStatus::Online};
    std::optional<QString> binId;

    // Metadata
    int width{0};
    int height{0};
    double durationSeconds{0.0};
    double frameRate{0.0};
    QString codec;
    int64_t fileSizeBytes{0};
    QDateTime importedAt;

    [[nodiscard]] QString resolution() const {
        if (width > 0 && height > 0)
            return QStringLiteral("%1x%2").arg(width).arg(height);
        return {};
    }

    [[nodiscard]] QString formattedDuration() const {
        if (durationSeconds <= 0) return {};
        int total = static_cast<int>(durationSeconds + 0.5);
        int h = total / 3600;
        int m = (total % 3600) / 60;
        int s = total % 60;
        if (h > 0) return QStringLiteral("%1h %2m %3s").arg(h).arg(m).arg(s);
        if (m > 0) return QStringLiteral("%1m %2s").arg(m).arg(s);
        return QStringLiteral("%1s").arg(s);
    }

    [[nodiscard]] QString formattedFileSize() const {
        if (fileSizeBytes <= 0) return {};
        if (fileSizeBytes < 1024) return QStringLiteral("%1B").arg(fileSizeBytes);
        if (fileSizeBytes < 1024 * 1024)
            return QStringLiteral("%1KB").arg(fileSizeBytes / 1024.0, 0, 'f', 1);
        if (fileSizeBytes < 1024LL * 1024 * 1024)
            return QStringLiteral("%1MB").arg(fileSizeBytes / (1024.0 * 1024.0), 0, 'f', 1);
        return QStringLiteral("%1GB").arg(fileSizeBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }

    [[nodiscard]] QString typeLabel() const {
        switch (type) {
            case MediaAssetType::Video: return QStringLiteral("Video");
            case MediaAssetType::Image: return QStringLiteral("Image");
            case MediaAssetType::Audio: return QStringLiteral("Audio");
        }
        return {};
    }

    [[nodiscard]] bool checkOnline() const {
        return QFileInfo::exists(filePath);
    }

    struct CopyWithOpts {
        std::optional<MediaAssetStatus> status;
        std::optional<QString> binId;
        bool clearBin{false};
        std::optional<QString> filePath;
        std::optional<int> width;
        std::optional<int> height;
        std::optional<double> durationSeconds;
        std::optional<double> frameRate;
        std::optional<QString> codec;
        std::optional<int64_t> fileSizeBytes;
    };

    [[nodiscard]] MediaAsset copyWith(const CopyWithOpts& o) const {
        return {
            .id = id,
            .filePath = o.filePath.value_or(filePath),
            .fileName = fileName,
            .type = type,
            .status = o.status.value_or(status),
            .binId = o.clearBin ? std::nullopt : (o.binId.has_value() ? o.binId : binId),
            .width = o.width.value_or(width),
            .height = o.height.value_or(height),
            .durationSeconds = o.durationSeconds.value_or(durationSeconds),
            .frameRate = o.frameRate.value_or(frameRate),
            .codec = o.codec.value_or(codec),
            .fileSizeBytes = o.fileSizeBytes.value_or(fileSizeBytes),
            .importedAt = importedAt,
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        QJsonObject obj;
        obj[QStringLiteral("id")] = id;
        obj[QStringLiteral("filePath")] = filePath;
        obj[QStringLiteral("fileName")] = fileName;
        obj[QStringLiteral("type")] = static_cast<int>(type);
        obj[QStringLiteral("status")] = static_cast<int>(status);
        if (binId.has_value())
            obj[QStringLiteral("binId")] = *binId;
        obj[QStringLiteral("width")] = width;
        obj[QStringLiteral("height")] = height;
        obj[QStringLiteral("durationSeconds")] = durationSeconds;
        obj[QStringLiteral("frameRate")] = frameRate;
        obj[QStringLiteral("codec")] = codec;
        obj[QStringLiteral("fileSizeBytes")] = static_cast<qint64>(fileSizeBytes);
        obj[QStringLiteral("importedAt")] = importedAt.toString(Qt::ISODate);
        return obj;
    }

    [[nodiscard]] static MediaAsset fromMap(const QJsonObject& m) {
        MediaAsset a;
        a.id = m.value(QStringLiteral("id")).toString();
        a.filePath = m.value(QStringLiteral("filePath")).toString();
        a.fileName = m.value(QStringLiteral("fileName")).toString();
        a.type = static_cast<MediaAssetType>(m.value(QStringLiteral("type")).toInt());
        a.status = static_cast<MediaAssetStatus>(m.value(QStringLiteral("status")).toInt(0));
        if (m.contains(QStringLiteral("binId")))
            a.binId = m.value(QStringLiteral("binId")).toString();
        a.width = m.value(QStringLiteral("width")).toInt(0);
        a.height = m.value(QStringLiteral("height")).toInt(0);
        a.durationSeconds = m.value(QStringLiteral("durationSeconds")).toDouble(0);
        a.frameRate = m.value(QStringLiteral("frameRate")).toDouble(0);
        a.codec = m.value(QStringLiteral("codec")).toString();
        a.fileSizeBytes = m.value(QStringLiteral("fileSizeBytes")).toInteger(0);
        a.importedAt = QDateTime::fromString(
            m.value(QStringLiteral("importedAt")).toString(), Qt::ISODate);
        if (!a.importedAt.isValid()) a.importedAt = QDateTime::currentDateTime();
        return a;
    }

    bool operator==(const MediaAsset& other) const { return id == other.id; }
};

// ── MediaBin ────────────────────────────────────────────────────────────────

struct MediaBin {
    QString id;
    QString name;
    std::optional<QString> parentId;

    struct CopyWithOpts {
        std::optional<QString> name;
        std::optional<QString> parentId;
        bool clearParent{false};
    };

    [[nodiscard]] MediaBin copyWith(const CopyWithOpts& o) const {
        return {
            .id = id,
            .name = o.name.value_or(name),
            .parentId = o.clearParent ? std::nullopt : (o.parentId.has_value() ? o.parentId : parentId),
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        QJsonObject obj;
        obj[QStringLiteral("id")] = id;
        obj[QStringLiteral("name")] = name;
        if (parentId.has_value())
            obj[QStringLiteral("parentId")] = *parentId;
        return obj;
    }

    [[nodiscard]] static MediaBin fromMap(const QJsonObject& m) {
        MediaBin b;
        b.id = m.value(QStringLiteral("id")).toString();
        b.name = m.value(QStringLiteral("name")).toString();
        if (m.contains(QStringLiteral("parentId")))
            b.parentId = m.value(QStringLiteral("parentId")).toString();
        return b;
    }

    bool operator==(const MediaBin& other) const { return id == other.id; }
};

// ── MediaPoolData ───────────────────────────────────────────────────────────

struct MediaPoolData {
    QList<MediaAsset> assets;
    QList<MediaBin> bins;

    [[nodiscard]] QJsonObject toMap() const {
        QJsonArray assetsArr;
        for (const auto& a : assets) assetsArr.append(a.toMap());
        QJsonArray binsArr;
        for (const auto& b : bins) binsArr.append(b.toMap());
        return {
            {QStringLiteral("assets"), assetsArr},
            {QStringLiteral("bins"), binsArr},
        };
    }

    [[nodiscard]] static MediaPoolData fromMap(const QJsonObject& m) {
        MediaPoolData result;
        const auto assetsArr = m.value(QStringLiteral("assets")).toArray();
        for (const auto& v : assetsArr)
            result.assets.append(MediaAsset::fromMap(v.toObject()));
        const auto binsArr = m.value(QStringLiteral("bins")).toArray();
        for (const auto& v : binsArr)
            result.bins.append(MediaBin::fromMap(v.toObject()));
        return result;
    }
};

} // namespace gopost::video_editor
