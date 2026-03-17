#include "image_editor/data/repositories/project_repository_impl.h"

#include "core/error/exceptions.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

namespace gopost::image_editor {

/// Minimal abstract interface that the engine project manager must satisfy.
/// The concrete type lives in the rendering_bridge module.
class ProjectManager {
public:
    virtual ~ProjectManager() = default;
    virtual void saveProject(int canvasId, const QString& filePath) = 0;
    virtual int loadProject(const QString& filePath) = 0;
};

ProjectRepositoryImpl::ProjectRepositoryImpl(ProjectManager& projectManager)
    : m_projectManager(projectManager) {}

void ProjectRepositoryImpl::saveProject(int canvasId, const QString& filePath) {
    try {
        m_projectManager.saveProject(canvasId, filePath);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Save project failed: %1").arg(e.what()));
    }
}

int ProjectRepositoryImpl::loadProject(const QString& filePath) {
    try {
        return m_projectManager.loadProject(filePath);
    } catch (const std::exception& e) {
        throw core::ServerException(
            QStringLiteral("Load project failed: %1").arg(e.what()));
    }
}

QString ProjectRepositoryImpl::getAutoSavePath() {
    const QString docsPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir autoSaveDir(docsPath + QStringLiteral("/GoPost/autosave"));
    if (!autoSaveDir.exists()) {
        autoSaveDir.mkpath(QStringLiteral("."));
    }
    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    return autoSaveDir.filePath(
        QStringLiteral("project_%1.gpimg").arg(timestamp));
}

} // namespace gopost::image_editor
