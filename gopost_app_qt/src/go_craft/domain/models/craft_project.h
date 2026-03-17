#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <optional>

namespace gopost::go_craft {

enum class CraftProjectType {
    Image = 0,
    Video = 1
};

struct CraftProject {
    QString id;
    QString name;
    CraftProjectType type = CraftProjectType::Image;
    QDateTime createdAt;
    QDateTime updatedAt;
    int width = 1080;
    int height = 1080;
    QString thumbnailPath;

    std::optional<double> durationSeconds;
    std::optional<int> trackCount;
    std::optional<int> clipCount;
    QString sourceTemplateName;

    QJsonObject toMap() const {
        QJsonObject m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("name")] = name;
        m[QStringLiteral("type")] = static_cast<int>(type);
        m[QStringLiteral("createdAt")] = createdAt.toString(Qt::ISODate);
        m[QStringLiteral("updatedAt")] = updatedAt.toString(Qt::ISODate);
        m[QStringLiteral("width")] = width;
        m[QStringLiteral("height")] = height;
        if (!thumbnailPath.isEmpty())
            m[QStringLiteral("thumbnailPath")] = thumbnailPath;
        if (durationSeconds.has_value())
            m[QStringLiteral("durationSeconds")] = durationSeconds.value();
        if (trackCount.has_value())
            m[QStringLiteral("trackCount")] = trackCount.value();
        if (clipCount.has_value())
            m[QStringLiteral("clipCount")] = clipCount.value();
        if (!sourceTemplateName.isEmpty())
            m[QStringLiteral("sourceTemplateName")] = sourceTemplateName;
        return m;
    }

    static CraftProject fromMap(const QJsonObject& m) {
        CraftProject p;
        p.id = m[QStringLiteral("id")].toString();
        p.name = m[QStringLiteral("name")].toString();
        p.type = static_cast<CraftProjectType>(m[QStringLiteral("type")].toInt(0));
        p.createdAt = QDateTime::fromString(m[QStringLiteral("createdAt")].toString(), Qt::ISODate);
        p.updatedAt = QDateTime::fromString(m[QStringLiteral("updatedAt")].toString(), Qt::ISODate);
        p.width = m[QStringLiteral("width")].toInt(1080);
        p.height = m[QStringLiteral("height")].toInt(1080);
        p.thumbnailPath = m[QStringLiteral("thumbnailPath")].toString();
        if (m.contains(QStringLiteral("durationSeconds")))
            p.durationSeconds = m[QStringLiteral("durationSeconds")].toDouble();
        if (m.contains(QStringLiteral("trackCount")))
            p.trackCount = m[QStringLiteral("trackCount")].toInt();
        if (m.contains(QStringLiteral("clipCount")))
            p.clipCount = m[QStringLiteral("clipCount")].toInt();
        p.sourceTemplateName = m[QStringLiteral("sourceTemplateName")].toString();
        return p;
    }

    bool operator==(const CraftProject& other) const {
        return id == other.id;
    }
};

} // namespace gopost::go_craft
