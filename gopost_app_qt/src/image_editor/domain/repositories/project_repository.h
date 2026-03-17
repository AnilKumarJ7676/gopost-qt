#pragma once

#include <QString>

namespace gopost::image_editor {

class ProjectRepository {
public:
    virtual ~ProjectRepository() = default;

    virtual void saveProject(int canvasId, const QString& filePath) = 0;
    virtual int loadProject(const QString& filePath) = 0;
    virtual QString getAutoSavePath() = 0;
};

} // namespace gopost::image_editor
