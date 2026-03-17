#pragma once

#include "image_editor/domain/repositories/project_repository.h"

namespace gopost::image_editor {

// Forward declaration for the engine project manager.
class ProjectManager;

class ProjectRepositoryImpl : public ProjectRepository {
public:
    explicit ProjectRepositoryImpl(ProjectManager& projectManager);

    void saveProject(int canvasId, const QString& filePath) override;
    int loadProject(const QString& filePath) override;
    QString getAutoSavePath() override;

private:
    ProjectManager& m_projectManager;
};

} // namespace gopost::image_editor
