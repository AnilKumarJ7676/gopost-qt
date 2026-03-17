#include "go_craft/data/image_project_local_datasource.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <algorithm>

namespace gopost::go_craft {

ImageProjectLocalDatasource::ImageProjectLocalDatasource(QObject* parent)
    : QObject(parent) {}

QString ImageProjectLocalDatasource::registryPath() const {
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString dirPath = docs + QStringLiteral("/") + QLatin1String(kRegistryDir);
    QDir dir(dirPath);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));
    return dirPath + QStringLiteral("/") + QLatin1String(kRegistryFile);
}

QList<CraftProject> ImageProjectLocalDatasource::list() const {
    QFile file(registryPath());
    if (!file.exists())
        return {};

    if (!file.open(QIODevice::ReadOnly))
        return {};

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isArray())
        return {};

    QList<CraftProject> projects;
    const auto arr = doc.array();
    projects.reserve(arr.size());

    for (const auto& val : arr) {
        if (val.isObject())
            projects.append(CraftProject::fromMap(val.toObject()));
    }

    std::sort(projects.begin(), projects.end(),
              [](const CraftProject& a, const CraftProject& b) {
                  return a.updatedAt > b.updatedAt;
              });

    return projects;
}

void ImageProjectLocalDatasource::registerProject(const CraftProject& project) {
    auto projects = list();
    const auto it = std::find_if(projects.begin(), projects.end(),
                                  [&](const CraftProject& p) { return p.id == project.id; });

    if (it != projects.end()) {
        *it = project;
    } else {
        projects.prepend(project);
    }

    persist(projects);
}

void ImageProjectLocalDatasource::deleteProject(const QString& id) {
    auto projects = list();
    projects.removeIf([&](const CraftProject& p) { return p.id == id; });
    persist(projects);
}

void ImageProjectLocalDatasource::persist(const QList<CraftProject>& projects) const {
    QJsonArray arr;
    for (const auto& p : projects)
        arr.append(p.toMap());

    QFile file(registryPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        file.close();
    }
}

} // namespace gopost::go_craft
