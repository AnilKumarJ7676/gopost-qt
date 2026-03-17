#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <vector>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// ProjectSummary — lightweight project metadata for listing
// ---------------------------------------------------------------------------
struct ProjectSummary {
    QString   id;
    QString   name;
    int       trackCount = 0;
    int       clipCount  = 0;
    QDateTime lastModified;
    QString   thumbnailPath;
};

// ---------------------------------------------------------------------------
// ProjectListState
// ---------------------------------------------------------------------------
struct ProjectListState {
    std::vector<ProjectSummary> projects;
    bool    isLoading = false;
    QString error;
};

// ---------------------------------------------------------------------------
// ProjectListNotifier — CRUD for video editor projects
//
// Converted 1:1 from project_list_notifier.dart.
// ---------------------------------------------------------------------------
class ProjectListNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(int     projectCount READ projectCount NOTIFY stateChanged)
    Q_PROPERTY(bool    isLoading    READ isLoading    NOTIFY stateChanged)
    Q_PROPERTY(QString error        READ error        NOTIFY stateChanged)

public:
    explicit ProjectListNotifier(QObject* parent = nullptr);
    ~ProjectListNotifier() override;

    int     projectCount() const { return static_cast<int>(state_.projects.size()); }
    bool    isLoading()    const { return state_.isLoading; }
    QString error()        const { return state_.error; }

    const ProjectListState& state() const { return state_; }

    Q_INVOKABLE void loadProjects();
    Q_INVOKABLE void saveProject(const QString& projectJson, const QString& name);
    Q_INVOKABLE void updateProject(const QString& projectId, const QString& projectJson);
    Q_INVOKABLE QString loadProject(const QString& projectId);
    Q_INVOKABLE void deleteProject(const QString& projectId);
    Q_INVOKABLE void autoSave(const QString& projectId, const QString& projectJson);

signals:
    void stateChanged();

private:
    ProjectListState state_;
    QString projectDir() const;
};

} // namespace gopost::video_editor
