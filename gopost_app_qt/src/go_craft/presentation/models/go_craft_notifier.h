#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include "go_craft/domain/models/craft_project.h"

namespace gopost::go_craft {

class ImageProjectLocalDatasource;

} // namespace gopost::go_craft

namespace gopost::video_editor {
class VideoProjectRepository;
} // namespace gopost::video_editor

namespace gopost::go_craft {

class GoCraftNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<CraftProject> projects READ projects NOTIFY projectsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit GoCraftNotifier(video_editor::VideoProjectRepository* videoRepo,
                             ImageProjectLocalDatasource* imageDatasource,
                             QObject* parent = nullptr);

    const QList<CraftProject>& projects() const { return m_projects; }
    bool isLoading() const { return m_isLoading; }
    const QString& error() const { return m_error; }

    Q_INVOKABLE void loadProjects();
    Q_INVOKABLE void deleteProject(const CraftProject& project);
    Q_INVOKABLE void refresh();

signals:
    void projectsChanged();
    void isLoadingChanged();
    void errorChanged();

private:
    QList<CraftProject> loadVideoProjects();

    video_editor::VideoProjectRepository* m_videoRepo = nullptr;
    ImageProjectLocalDatasource* m_imageDatasource = nullptr;

    QList<CraftProject> m_projects;
    bool m_isLoading = false;
    QString m_error;
};

} // namespace gopost::go_craft
