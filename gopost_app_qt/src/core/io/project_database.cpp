#include "project_database.h"

// Bundled SQLite — use Qt's built-in copy
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QString>

#include <chrono>

namespace gopost::io {

// Use Qt SQL instead of raw sqlite3 — avoids linking issues, already available
static QString uniqueConnectionName() {
    static std::atomic<int> counter{0};
    return QStringLiteral("gopost_project_%1").arg(counter.fetch_add(1));
}

ProjectDatabase::ProjectDatabase() = default;

ProjectDatabase::~ProjectDatabase() {
    close();
}

gopost::Result<void> ProjectDatabase::open(const std::string& projectPath) {
    std::lock_guard lock(mutex_);

    auto connName = uniqueConnectionName();
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(QString::fromStdString(projectPath));

    if (!db.open())
        return gopost::Result<void>::failure("Cannot open database: " + db.lastError().text().toStdString());

    // Store connection name as the "db_" equivalent
    // We use QSqlDatabase::database(connName) to retrieve it
    // Store connName in a member
    db_ = reinterpret_cast<sqlite3*>(new std::string(connName.toStdString())); // hack: store name

    // Pragmas
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));

    createSchema();
    return gopost::Result<void>::success();
}

void ProjectDatabase::createSchema() {
    auto connName = QString::fromStdString(*reinterpret_cast<std::string*>(db_));
    auto db = QSqlDatabase::database(connName);
    QSqlQuery q(db);

    q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS media_assets ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  filename TEXT NOT NULL,"
        "  original_uri TEXT NOT NULL,"
        "  cache_path TEXT NOT NULL,"
        "  mime_type TEXT,"
        "  file_size_bytes INTEGER NOT NULL,"
        "  duration_ms INTEGER DEFAULT 0,"
        "  width INTEGER DEFAULT 0,"
        "  height INTEGER DEFAULT 0,"
        "  codec TEXT,"
        "  frame_rate REAL DEFAULT 0.0,"
        "  sample_rate INTEGER DEFAULT 0,"
        "  channels INTEGER DEFAULT 0,"
        "  imported_at INTEGER NOT NULL,"
        "  sha256_hash TEXT NOT NULL,"
        "  UNIQUE(sha256_hash)"
        ")"
    ));

    q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS project_meta ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ")"
    ));

    q.exec(QStringLiteral("INSERT OR IGNORE INTO project_meta (key, value) VALUES ('schema_version', '1')"));

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO project_meta (key, value) VALUES ('created_at', ?)"));
    q.addBindValue(QString::number(now));
    q.exec();

    q.exec(QStringLiteral("INSERT OR IGNORE INTO project_meta (key, value) VALUES ('app_version', '0.1.0')"));
}

void ProjectDatabase::close() {
    std::lock_guard lock(mutex_);
    if (db_) {
        auto connName = QString::fromStdString(*reinterpret_cast<std::string*>(db_));
        {
            auto db = QSqlDatabase::database(connName);
            if (db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase(connName);
        delete reinterpret_cast<std::string*>(db_);
        db_ = nullptr;
    }
}

gopost::Result<int64_t> ProjectDatabase::addMediaAsset(const MediaAsset& asset) {
    std::lock_guard lock(mutex_);
    if (!db_) return gopost::Result<int64_t>::failure("Database not open");

    auto connName = QString::fromStdString(*reinterpret_cast<std::string*>(db_));
    auto db = QSqlDatabase::database(connName);
    QSqlQuery q(db);

    q.prepare(QStringLiteral(
        "INSERT INTO media_assets (filename, original_uri, cache_path, mime_type, "
        "file_size_bytes, duration_ms, width, height, codec, frame_rate, "
        "sample_rate, channels, imported_at, sha256_hash) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    ));

    q.addBindValue(QString::fromStdString(asset.filename));
    q.addBindValue(QString::fromStdString(asset.originalUri));
    q.addBindValue(QString::fromStdString(asset.cachePath));
    q.addBindValue(QString::fromStdString(asset.mimeType));
    q.addBindValue(qint64(asset.fileSizeBytes));
    q.addBindValue(qint64(asset.durationMs));
    q.addBindValue(asset.width);
    q.addBindValue(asset.height);
    q.addBindValue(QString::fromStdString(asset.codec));
    q.addBindValue(asset.frameRate);
    q.addBindValue(asset.sampleRate);
    q.addBindValue(asset.channels);
    q.addBindValue(qint64(asset.importedAt));
    q.addBindValue(QString::fromStdString(asset.sha256Hash));

    if (!q.exec())
        return gopost::Result<int64_t>::failure("Insert failed: " + q.lastError().text().toStdString());

    return gopost::Result<int64_t>::success(q.lastInsertId().toLongLong());
}

gopost::Result<MediaAsset> ProjectDatabase::getMediaAsset(int64_t id) const {
    std::lock_guard lock(mutex_);
    if (!db_) return gopost::Result<MediaAsset>::failure("Database not open");

    auto connName = QString::fromStdString(*reinterpret_cast<std::string*>(db_));
    auto db = QSqlDatabase::database(connName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT * FROM media_assets WHERE id = ?"));
    q.addBindValue(qint64(id));

    if (!q.exec() || !q.next())
        return gopost::Result<MediaAsset>::failure("Asset not found: " + std::to_string(id));

    MediaAsset a;
    a.id = q.value(0).toLongLong();
    a.filename = q.value(1).toString().toStdString();
    a.originalUri = q.value(2).toString().toStdString();
    a.cachePath = q.value(3).toString().toStdString();
    a.mimeType = q.value(4).toString().toStdString();
    a.fileSizeBytes = q.value(5).toULongLong();
    a.durationMs = q.value(6).toLongLong();
    a.width = q.value(7).toInt();
    a.height = q.value(8).toInt();
    a.codec = q.value(9).toString().toStdString();
    a.frameRate = q.value(10).toDouble();
    a.sampleRate = q.value(11).toInt();
    a.channels = q.value(12).toInt();
    a.importedAt = q.value(13).toULongLong();
    a.sha256Hash = q.value(14).toString().toStdString();
    return gopost::Result<MediaAsset>::success(std::move(a));
}

gopost::Result<std::vector<MediaAsset>> ProjectDatabase::getAllMediaAssets() const {
    std::lock_guard lock(mutex_);
    if (!db_) return gopost::Result<std::vector<MediaAsset>>::failure("Database not open");

    auto connName = QString::fromStdString(*reinterpret_cast<std::string*>(db_));
    auto db = QSqlDatabase::database(connName);
    QSqlQuery q(db);
    q.exec(QStringLiteral("SELECT * FROM media_assets ORDER BY id"));

    std::vector<MediaAsset> result;
    while (q.next()) {
        MediaAsset a;
        a.id = q.value(0).toLongLong();
        a.filename = q.value(1).toString().toStdString();
        a.originalUri = q.value(2).toString().toStdString();
        a.cachePath = q.value(3).toString().toStdString();
        a.mimeType = q.value(4).toString().toStdString();
        a.fileSizeBytes = q.value(5).toULongLong();
        a.durationMs = q.value(6).toLongLong();
        a.width = q.value(7).toInt();
        a.height = q.value(8).toInt();
        a.codec = q.value(9).toString().toStdString();
        a.frameRate = q.value(10).toDouble();
        a.sampleRate = q.value(11).toInt();
        a.channels = q.value(12).toInt();
        a.importedAt = q.value(13).toULongLong();
        a.sha256Hash = q.value(14).toString().toStdString();
        result.push_back(std::move(a));
    }
    return gopost::Result<std::vector<MediaAsset>>::success(std::move(result));
}

gopost::Result<void> ProjectDatabase::removeMediaAsset(int64_t id) {
    std::lock_guard lock(mutex_);
    if (!db_) return gopost::Result<void>::failure("Database not open");

    auto connName = QString::fromStdString(*reinterpret_cast<std::string*>(db_));
    auto db = QSqlDatabase::database(connName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM media_assets WHERE id = ?"));
    q.addBindValue(qint64(id));

    if (!q.exec())
        return gopost::Result<void>::failure("Delete failed: " + q.lastError().text().toStdString());
    if (q.numRowsAffected() == 0)
        return gopost::Result<void>::failure("Asset not found: " + std::to_string(id));

    return gopost::Result<void>::success();
}

int ProjectDatabase::getSchemaVersion() const {
    std::lock_guard lock(mutex_);
    if (!db_) return 0;

    auto connName = QString::fromStdString(*reinterpret_cast<std::string*>(db_));
    auto db = QSqlDatabase::database(connName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT value FROM project_meta WHERE key = 'schema_version'"));
    if (q.exec() && q.next())
        return q.value(0).toInt();
    return 0;
}

} // namespace gopost::io
