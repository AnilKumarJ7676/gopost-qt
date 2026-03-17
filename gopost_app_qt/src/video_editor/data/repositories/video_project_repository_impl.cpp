#include "video_editor/data/repositories/video_project_repository_impl.h"

namespace gopost::video_editor {

VideoProjectRepositoryImpl::VideoProjectRepositoryImpl(
    std::shared_ptr<VideoProjectLocalDatasource> datasource)
    : datasource_(std::move(datasource)) {}

QString VideoProjectRepositoryImpl::saveProject(const QString& name, const VideoProject& project) {
    const auto id = datasource_->save(name, project);
    autoSaveId_ = id;
    return id;
}

void VideoProjectRepositoryImpl::updateProject(const QString& id, const VideoProject& project) {
    datasource_->update(id, project);
}

std::optional<VideoProject> VideoProjectRepositoryImpl::loadProject(const QString& id) {
    return datasource_->load(id);
}

QList<SavedProjectMeta> VideoProjectRepositoryImpl::listProjects() {
    return datasource_->list();
}

void VideoProjectRepositoryImpl::deleteProject(const QString& id) {
    datasource_->deleteProject(id);
    if (autoSaveId_ == id) autoSaveId_ = std::nullopt;
}

std::optional<QString> VideoProjectRepositoryImpl::getAutoSaveId() {
    return autoSaveId_;
}

void VideoProjectRepositoryImpl::autoSave(const QString& id, const VideoProject& project) {
    datasource_->update(id, project);
}

} // namespace gopost::video_editor
