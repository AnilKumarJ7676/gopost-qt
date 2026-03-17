#pragma once

#include <QString>
#include <optional>
#include <memory>

#include "video_editor/domain/repositories/video_project_repository.h"
#include "video_editor/data/datasources/video_project_local_datasource.h"

namespace gopost::video_editor {

/// Concrete implementation of VideoProjectRepository backed by local storage.
/// Follows Single Responsibility: delegates persistence to the datasource.
class VideoProjectRepositoryImpl : public VideoProjectRepository {
public:
    explicit VideoProjectRepositoryImpl(std::shared_ptr<VideoProjectLocalDatasource> datasource);

    QString saveProject(const QString& name, const VideoProject& project) override;
    void updateProject(const QString& id, const VideoProject& project) override;
    std::optional<VideoProject> loadProject(const QString& id) override;
    QList<SavedProjectMeta> listProjects() override;
    void deleteProject(const QString& id) override;
    std::optional<QString> getAutoSaveId() override;
    void autoSave(const QString& id, const VideoProject& project) override;

private:
    std::shared_ptr<VideoProjectLocalDatasource> datasource_;
    std::optional<QString> autoSaveId_;
};

} // namespace gopost::video_editor
