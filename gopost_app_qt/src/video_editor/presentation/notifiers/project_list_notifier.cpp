#include "video_editor/presentation/notifiers/project_list_notifier.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

namespace gopost::video_editor {

ProjectListNotifier::ProjectListNotifier(QObject* parent) : QObject(parent) {}
ProjectListNotifier::~ProjectListNotifier() = default;

QString ProjectListNotifier::projectDir() const {
    const auto base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + QStringLiteral("/video_projects");
}

void ProjectListNotifier::loadProjects() {
    state_.isLoading = true;
    state_.error.clear();
    emit stateChanged();

    QDir dir(projectDir());
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
        state_.isLoading = false;
        emit stateChanged();
        return;
    }

    state_.projects.clear();
    const auto files = dir.entryInfoList({QStringLiteral("*.json")}, QDir::Files,
                                          QDir::Time | QDir::Reversed);
    for (const auto& fi : files) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) continue;
        auto doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isNull()) continue;
        auto obj = doc.object();

        ProjectSummary ps;
        ps.id           = fi.baseName();
        ps.name         = obj.value(QStringLiteral("name")).toString(fi.baseName());
        ps.trackCount   = obj.value(QStringLiteral("trackCount")).toInt();
        ps.clipCount    = obj.value(QStringLiteral("clipCount")).toInt();
        ps.lastModified = fi.lastModified();
        state_.projects.push_back(std::move(ps));
    }

    state_.isLoading = false;
    emit stateChanged();
}

void ProjectListNotifier::saveProject(const QString& projectJson, const QString& name) {
    QDir dir(projectDir());
    if (!dir.exists()) dir.mkpath(QStringLiteral("."));

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString path = dir.filePath(id + QStringLiteral(".json"));

    // Wrap with metadata
    auto doc = QJsonDocument::fromJson(projectJson.toUtf8());
    QJsonObject wrapper;
    wrapper[QStringLiteral("name")]    = name;
    wrapper[QStringLiteral("project")] = doc.object();

    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(wrapper).toJson(QJsonDocument::Compact));
    }

    loadProjects(); // refresh list
}

void ProjectListNotifier::updateProject(const QString& projectId, const QString& projectJson) {
    const QString path = QDir(projectDir()).filePath(projectId + QStringLiteral(".json"));
    QFile f(path);
    if (!f.exists()) return;

    // Read existing to preserve name
    f.open(QIODevice::ReadOnly);
    auto existing = QJsonDocument::fromJson(f.readAll()).object();
    f.close();

    auto newDoc = QJsonDocument::fromJson(projectJson.toUtf8());
    existing[QStringLiteral("project")] = newDoc.object();

    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(QJsonDocument(existing).toJson(QJsonDocument::Compact));
    f.close();

    loadProjects();
}

QString ProjectListNotifier::loadProject(const QString& projectId) {
    const QString path = QDir(projectDir()).filePath(projectId + QStringLiteral(".json"));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    auto doc = QJsonDocument::fromJson(f.readAll());
    auto project = doc.object().value(QStringLiteral("project")).toObject();
    return QJsonDocument(project).toJson(QJsonDocument::Compact);
}

void ProjectListNotifier::deleteProject(const QString& projectId) {
    const QString path = QDir(projectDir()).filePath(projectId + QStringLiteral(".json"));
    QFile::remove(path);
    loadProjects();
}

void ProjectListNotifier::autoSave(const QString& projectId, const QString& projectJson) {
    if (projectId.isEmpty()) return;
    updateProject(projectId, projectJson);
}

} // namespace gopost::video_editor
