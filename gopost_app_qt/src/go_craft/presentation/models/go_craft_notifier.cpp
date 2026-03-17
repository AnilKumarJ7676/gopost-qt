#include "go_craft/presentation/models/go_craft_notifier.h"
#include "go_craft/data/image_project_local_datasource.h"

#include <algorithm>

namespace gopost::go_craft {

GoCraftNotifier::GoCraftNotifier(video_editor::VideoProjectRepository* videoRepo,
                                 ImageProjectLocalDatasource* imageDatasource,
                                 QObject* parent)
    : QObject(parent)
    , m_videoRepo(videoRepo)
    , m_imageDatasource(imageDatasource) {
    if (m_imageDatasource) {
        loadProjects();
    }
}

void GoCraftNotifier::loadProjects() {
    if (!m_imageDatasource) {
        return;
    }

    m_isLoading = true;
    m_error.clear();
    emit isLoadingChanged();
    emit errorChanged();

    try {
        auto videoProjects = loadVideoProjects();
        auto imageProjects = m_imageDatasource->list();

        QList<CraftProject> combined;
        combined.reserve(videoProjects.size() + imageProjects.size());
        combined.append(videoProjects);
        combined.append(imageProjects);

        std::sort(combined.begin(), combined.end(),
                  [](const CraftProject& a, const CraftProject& b) {
                      return a.updatedAt > b.updatedAt;
                  });

        m_projects = std::move(combined);
        m_isLoading = false;
        emit projectsChanged();
        emit isLoadingChanged();
    } catch (const std::exception& e) {
        m_isLoading = false;
        m_error = QString::fromStdString(e.what());
        emit isLoadingChanged();
        emit errorChanged();
    }
}

QList<CraftProject> GoCraftNotifier::loadVideoProjects() {
    // VideoProjectRepository integration - returns CraftProject wrappers
    // This will be connected when VideoProjectRepository is available
    return {};
}

void GoCraftNotifier::deleteProject(const CraftProject& project) {
    if (project.type == CraftProjectType::Video) {
        // m_videoRepo->deleteProject(project.id);
    } else if (m_imageDatasource) {
        m_imageDatasource->deleteProject(project.id);
    }
    loadProjects();
}

void GoCraftNotifier::refresh() {
    loadProjects();
}

} // namespace gopost::go_craft
