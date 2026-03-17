#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include "go_craft/domain/models/craft_project.h"

namespace gopost::go_craft {

class ImageProjectLocalDatasource : public QObject {
    Q_OBJECT
public:
    explicit ImageProjectLocalDatasource(QObject* parent = nullptr);

    QList<CraftProject> list() const;
    void registerProject(const CraftProject& project);
    void deleteProject(const QString& id);

private:
    QString registryPath() const;
    void persist(const QList<CraftProject>& projects) const;

    static constexpr const char* kRegistryDir = "image_projects";
    static constexpr const char* kRegistryFile = "registry.json";
};

} // namespace gopost::go_craft
