#include "video_editor/presentation/notifiers/project_list_notifier.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
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

// ===========================================================================
// Template save/load
// ===========================================================================

static QString templateDir() {
    const auto base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + QStringLiteral("/video_templates");
}

void ProjectListNotifier::saveTemplate(const QString& projectJson, const QString& name,
                                        const QString& description, const QString& category,
                                        const QString& tags) {
    if (name.trimmed().isEmpty()) {
        emit templateSaveError(QStringLiteral("Template name is required"));
        return;
    }

    QDir dir(templateDir());
    if (!dir.exists()) dir.mkpath(QStringLiteral("."));

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString path = dir.filePath(id + QStringLiteral(".json"));

    auto doc = QJsonDocument::fromJson(projectJson.toUtf8());
    if (doc.isNull()) {
        emit templateSaveError(QStringLiteral("Invalid project data"));
        return;
    }

    QJsonObject wrapper;
    wrapper[QStringLiteral("id")]          = id;
    wrapper[QStringLiteral("name")]        = name.trimmed();
    wrapper[QStringLiteral("description")] = description.trimmed();
    wrapper[QStringLiteral("category")]    = category;
    wrapper[QStringLiteral("tags")]        = tags;
    wrapper[QStringLiteral("type")]        = QStringLiteral("video");
    wrapper[QStringLiteral("createdAt")]   = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    wrapper[QStringLiteral("project")]     = doc.object();

    // Compute metadata from project
    auto proj = doc.object();
    int clipCount = 0;
    int placeholderCount = 0;
    const auto tracksArr = proj.value(QStringLiteral("tracks")).toArray();
    for (const auto& tv : tracksArr) {
        const auto clips = tv.toObject().value(QStringLiteral("clips")).toArray();
        clipCount += clips.size();
        for (const auto& cv : clips) {
            if (cv.toObject().value(QStringLiteral("isPlaceholder")).toBool())
                ++placeholderCount;
        }
    }
    wrapper[QStringLiteral("trackCount")]       = static_cast<int>(tracksArr.size());
    wrapper[QStringLiteral("clipCount")]         = clipCount;
    wrapper[QStringLiteral("placeholderCount")]  = placeholderCount;
    wrapper[QStringLiteral("width")]  = proj.value(QStringLiteral("width")).toInt(1920);
    wrapper[QStringLiteral("height")] = proj.value(QStringLiteral("height")).toInt(1080);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        emit templateSaveError(QStringLiteral("Could not write template file"));
        return;
    }
    f.write(QJsonDocument(wrapper).toJson(QJsonDocument::Compact));
    f.close();

    qDebug() << "[ProjectList] template saved:" << name << "id:" << id
             << "clips:" << clipCount << "placeholders:" << placeholderCount;
    emit templateSaved();
}

QVariantList ProjectListNotifier::loadTemplates() {
    QVariantList result;
    QDir dir(templateDir());
    if (!dir.exists()) return result;

    const auto files = dir.entryInfoList({QStringLiteral("*.json")}, QDir::Files,
                                          QDir::Time | QDir::Reversed);
    for (const auto& fi : files) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) continue;
        auto doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isNull()) continue;
        auto obj = doc.object();

        QVariantMap entry;
        entry[QStringLiteral("id")]          = fi.baseName();
        entry[QStringLiteral("name")]        = obj.value(QStringLiteral("name")).toString(fi.baseName());
        entry[QStringLiteral("description")] = obj.value(QStringLiteral("description")).toString();
        entry[QStringLiteral("category")]    = obj.value(QStringLiteral("category")).toString();
        entry[QStringLiteral("tags")]        = obj.value(QStringLiteral("tags")).toString();
        entry[QStringLiteral("type")]        = obj.value(QStringLiteral("type")).toString(QStringLiteral("video"));
        entry[QStringLiteral("createdAt")]   = obj.value(QStringLiteral("createdAt")).toString();
        entry[QStringLiteral("trackCount")]  = obj.value(QStringLiteral("trackCount")).toInt();
        entry[QStringLiteral("clipCount")]   = obj.value(QStringLiteral("clipCount")).toInt();
        entry[QStringLiteral("placeholderCount")] = obj.value(QStringLiteral("placeholderCount")).toInt();
        entry[QStringLiteral("width")]       = obj.value(QStringLiteral("width")).toInt(1920);
        entry[QStringLiteral("height")]      = obj.value(QStringLiteral("height")).toInt(1080);
        result.append(entry);
    }
    return result;
}

QString ProjectListNotifier::loadTemplate(const QString& templateId) {
    const QString path = QDir(templateDir()).filePath(templateId + QStringLiteral(".json"));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    auto doc = QJsonDocument::fromJson(f.readAll());
    auto project = doc.object().value(QStringLiteral("project")).toObject();
    return QJsonDocument(project).toJson(QJsonDocument::Compact);
}

void ProjectListNotifier::deleteTemplate(const QString& templateId) {
    const QString path = QDir(templateDir()).filePath(templateId + QStringLiteral(".json"));
    QFile::remove(path);
}

} // namespace gopost::video_editor
