#pragma once

#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QList>
#include <optional>

#include "video_editor/domain/models/video_project.h"

namespace gopost::video_editor {

// ── SavedProjectMeta ────────────────────────────────────────────────────────

struct SavedProjectMeta {
    QString id;
    QString name;
    QDateTime createdAt;
    QDateTime updatedAt;
    double durationSeconds{0.0};
    int trackCount{0};
    int clipCount{0};
    int width{1920};
    int height{1080};

    [[nodiscard]] QJsonObject toMap() const {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("createdAt"), createdAt.toString(Qt::ISODate)},
            {QStringLiteral("updatedAt"), updatedAt.toString(Qt::ISODate)},
            {QStringLiteral("durationSeconds"), durationSeconds},
            {QStringLiteral("trackCount"), trackCount},
            {QStringLiteral("clipCount"), clipCount},
            {QStringLiteral("width"), width},
            {QStringLiteral("height"), height},
        };
    }

    [[nodiscard]] static SavedProjectMeta fromMap(const QJsonObject& m) {
        return {
            .id = m.value(QStringLiteral("id")).toString(),
            .name = m.value(QStringLiteral("name")).toString(),
            .createdAt = QDateTime::fromString(m.value(QStringLiteral("createdAt")).toString(), Qt::ISODate),
            .updatedAt = QDateTime::fromString(m.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate),
            .durationSeconds = m.value(QStringLiteral("durationSeconds")).toDouble(),
            .trackCount = m.value(QStringLiteral("trackCount")).toInt(),
            .clipCount = m.value(QStringLiteral("clipCount")).toInt(),
            .width = m.value(QStringLiteral("width")).toInt(1920),
            .height = m.value(QStringLiteral("height")).toInt(1080),
        };
    }
};

// ── VideoProjectRepository (interface) ──────────────────────────────────────

class VideoProjectRepository {
public:
    virtual ~VideoProjectRepository() = default;

    /// Save a project to local storage. Returns the project ID.
    virtual QString saveProject(const QString& name, const VideoProject& project) = 0;

    /// Update an existing saved project.
    virtual void updateProject(const QString& id, const VideoProject& project) = 0;

    /// Load a saved project by ID.
    virtual std::optional<VideoProject> loadProject(const QString& id) = 0;

    /// List all saved project metadata, ordered by last updated.
    virtual QList<SavedProjectMeta> listProjects() = 0;

    /// Delete a saved project.
    virtual void deleteProject(const QString& id) = 0;

    /// Check if the current project has unsaved changes (auto-save support).
    virtual std::optional<QString> getAutoSaveId() = 0;

    /// Auto-save the current project state.
    virtual void autoSave(const QString& id, const VideoProject& project) = 0;
};

} // namespace gopost::video_editor
