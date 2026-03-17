#pragma once

#include <QString>
#include <QList>
#include <optional>

#include "video_editor/domain/models/media_asset.h"
#include "video_editor/domain/models/video_project.h"
#include "video_editor/domain/repositories/video_project_repository.h"

namespace gopost::video_editor {

/// Local file-system data source for video projects.
/// Projects are stored as JSON in the app's documents directory.
class VideoProjectLocalDatasource {
public:
    VideoProjectLocalDatasource() = default;

    QString save(const QString& name, const VideoProject& project);
    void update(const QString& id, const VideoProject& project);
    std::optional<VideoProject> load(const QString& id);
    QList<SavedProjectMeta> list();
    void deleteProject(const QString& id);

    // Media Pool persistence
    void saveMediaPool(const QString& projectId, const MediaPoolData& poolData);
    std::optional<MediaPoolData> loadMediaPool(const QString& projectId);

private:
    static constexpr const char* kProjectsDir = "video_projects";
    static constexpr const char* kMetaFile = "meta.json";
    static constexpr const char* kProjectFile = "project.json";
    static constexpr const char* kMediaPoolFile = "media_pool.json";

    [[nodiscard]] QString projectsRoot() const;
    [[nodiscard]] QString projectDir(const QString& id) const;
};

} // namespace gopost::video_editor
