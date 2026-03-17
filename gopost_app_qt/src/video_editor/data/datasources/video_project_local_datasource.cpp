#include "video_editor/data/datasources/video_project_local_datasource.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDateTime>
#include <algorithm>

namespace gopost::video_editor {

QString VideoProjectLocalDatasource::projectsRoot() const {
    const auto docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const auto root = QStringLiteral("%1/%2").arg(docs, QLatin1String(kProjectsDir));
    QDir().mkpath(root);
    return root;
}

QString VideoProjectLocalDatasource::projectDir(const QString& id) const {
    const auto dir = QStringLiteral("%1/%2").arg(projectsRoot(), id);
    QDir().mkpath(dir);
    return dir;
}

QString VideoProjectLocalDatasource::save(const QString& name, const VideoProject& project) {
    const auto id = QString::number(QDateTime::currentMSecsSinceEpoch(), 36);
    const auto dir = projectDir(id);
    const auto now = QDateTime::currentDateTime();

    SavedProjectMeta meta{
        .id = id,
        .name = name,
        .createdAt = now,
        .updatedAt = now,
        .durationSeconds = project.duration(),
        .trackCount = static_cast<int>(project.tracks.size()),
        .clipCount = static_cast<int>(project.allClips().size()),
        .width = project.width,
        .height = project.height,
    };

    {
        QFile f(QStringLiteral("%1/%2").arg(dir, QLatin1String(kMetaFile)));
        if (f.open(QIODevice::WriteOnly))
            f.write(QJsonDocument(meta.toMap()).toJson(QJsonDocument::Compact));
    }
    {
        QFile f(QStringLiteral("%1/%2").arg(dir, QLatin1String(kProjectFile)));
        if (f.open(QIODevice::WriteOnly))
            f.write(QJsonDocument(project.toMap()).toJson(QJsonDocument::Compact));
    }
    return id;
}

void VideoProjectLocalDatasource::update(const QString& id, const VideoProject& project) {
    const auto dir = projectDir(id);
    const auto metaPath = QStringLiteral("%1/%2").arg(dir, QLatin1String(kMetaFile));

    QFile metaFile(metaPath);
    if (!metaFile.exists()) return;

    // Read existing meta
    SavedProjectMeta existingMeta;
    if (metaFile.open(QIODevice::ReadOnly)) {
        const auto doc = QJsonDocument::fromJson(metaFile.readAll());
        existingMeta = SavedProjectMeta::fromMap(doc.object());
        metaFile.close();
    }

    SavedProjectMeta updatedMeta{
        .id = id,
        .name = existingMeta.name,
        .createdAt = existingMeta.createdAt,
        .updatedAt = QDateTime::currentDateTime(),
        .durationSeconds = project.duration(),
        .trackCount = static_cast<int>(project.tracks.size()),
        .clipCount = static_cast<int>(project.allClips().size()),
        .width = project.width,
        .height = project.height,
    };

    if (metaFile.open(QIODevice::WriteOnly))
        metaFile.write(QJsonDocument(updatedMeta.toMap()).toJson(QJsonDocument::Compact));

    QFile projFile(QStringLiteral("%1/%2").arg(dir, QLatin1String(kProjectFile)));
    if (projFile.open(QIODevice::WriteOnly))
        projFile.write(QJsonDocument(project.toMap()).toJson(QJsonDocument::Compact));
}

std::optional<VideoProject> VideoProjectLocalDatasource::load(const QString& id) {
    const auto dir = projectDir(id);
    QFile file(QStringLiteral("%1/%2").arg(dir, QLatin1String(kProjectFile)));
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return std::nullopt;

    const auto doc = QJsonDocument::fromJson(file.readAll());
    return VideoProject::fromMap(doc.object());
}

QList<SavedProjectMeta> VideoProjectLocalDatasource::list() {
    const auto root = projectsRoot();
    QDir rootDir(root);
    if (!rootDir.exists()) return {};

    QList<SavedProjectMeta> results;
    const auto entries = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const auto& entry : entries) {
        QFile metaFile(QStringLiteral("%1/%2/%3").arg(root, entry, QLatin1String(kMetaFile)));
        if (!metaFile.exists() || !metaFile.open(QIODevice::ReadOnly))
            continue;
        try {
            const auto doc = QJsonDocument::fromJson(metaFile.readAll());
            results.append(SavedProjectMeta::fromMap(doc.object()));
        } catch (...) {
            // Skip corrupted metadata
        }
    }

    std::sort(results.begin(), results.end(),
        [](const SavedProjectMeta& a, const SavedProjectMeta& b) {
            return a.updatedAt > b.updatedAt;
        });
    return results;
}

void VideoProjectLocalDatasource::deleteProject(const QString& id) {
    const auto dir = projectDir(id);
    QDir(dir).removeRecursively();
}

void VideoProjectLocalDatasource::saveMediaPool(const QString& projectId, const MediaPoolData& poolData) {
    const auto dir = projectDir(projectId);
    QFile f(QStringLiteral("%1/%2").arg(dir, QLatin1String(kMediaPoolFile)));
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(poolData.toMap()).toJson(QJsonDocument::Compact));
}

std::optional<MediaPoolData> VideoProjectLocalDatasource::loadMediaPool(const QString& projectId) {
    const auto dir = projectDir(projectId);
    QFile file(QStringLiteral("%1/%2").arg(dir, QLatin1String(kMediaPoolFile)));
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return std::nullopt;
    try {
        const auto doc = QJsonDocument::fromJson(file.readAll());
        return MediaPoolData::fromMap(doc.object());
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace gopost::video_editor
