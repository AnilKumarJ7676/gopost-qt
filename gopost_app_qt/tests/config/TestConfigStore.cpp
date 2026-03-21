#include "TestConfigStore.h"

#include "core/config/configuration_engine_new.h"
#include "core/config/config_layer.h"
#include "core/logging/logging_engine_new.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <cstdio>

using namespace gopost::config;

void TestConfigStore::defaults() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    ConfigurationEngineNew engine(nullptr, tmpDir.path());

    QCOMPARE(engine.getInt(QStringLiteral("editor.playback.targetFps")), 30);
    QCOMPARE(engine.getInt(QStringLiteral("editor.timeline.trackHeight")), 80);
    QCOMPARE(engine.getBool(QStringLiteral("editor.autosave.enabled")), true);
    QCOMPARE(engine.getString(QStringLiteral("editor.export.defaultCodec")),
             QStringLiteral("h264"));
    QCOMPARE(engine.getString(QStringLiteral("app.theme")),
             QStringLiteral("dark"));
    QCOMPARE(engine.getDouble(QStringLiteral("editor.timeline.defaultZoom")), 1.0);

    std::printf("Defaults: targetFps=%d, codec=%s, theme=%s\n",
                engine.getInt(QStringLiteral("editor.playback.targetFps")),
                engine.getString(QStringLiteral("editor.export.defaultCodec")).toUtf8().constData(),
                engine.getString(QStringLiteral("app.theme")).toUtf8().constData());
    std::fflush(stdout);
}

void TestConfigStore::setAndGet() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    ConfigurationEngineNew engine(nullptr, tmpDir.path());

    // Default
    QCOMPARE(engine.getInt(QStringLiteral("editor.playback.targetFps")), 30);

    // Override
    engine.setInt(QStringLiteral("editor.playback.targetFps"), 60);
    QCOMPARE(engine.getInt(QStringLiteral("editor.playback.targetFps")), 60);

    engine.setString(QStringLiteral("app.theme"), QStringLiteral("light"));
    QCOMPARE(engine.getString(QStringLiteral("app.theme")),
             QStringLiteral("light"));
}

void TestConfigStore::persistence() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Set and save
    {
        ConfigurationEngineNew engine(nullptr, tmpDir.path());
        engine.setInt(QStringLiteral("editor.playback.targetFps"), 60);
        engine.save();
    }

    // Recreate and verify persisted
    {
        ConfigurationEngineNew engine(nullptr, tmpDir.path());
        QCOMPARE(engine.getInt(QStringLiteral("editor.playback.targetFps")), 60);
    }

    std::printf("Persistence: value persisted across engine restart\n");
    std::fflush(stdout);
}

void TestConfigStore::scopeOverride() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    ConfigurationEngineNew engine(nullptr, tmpDir.path());

    // Set user-level value
    engine.setInt(QStringLiteral("editor.playback.targetFps"), 60);
    QCOMPARE(engine.getInt(QStringLiteral("editor.playback.targetFps")), 60);

    // Push project scope and override
    engine.pushScope(QStringLiteral("project_abc"));
    engine.setInt(QStringLiteral("editor.playback.targetFps"), 24);
    QCOMPARE(engine.getInt(QStringLiteral("editor.playback.targetFps")), 24);

    // Pop scope — back to user value
    engine.popScope();
    QCOMPARE(engine.getInt(QStringLiteral("editor.playback.targetFps")), 60);

    std::printf("Scope: project override 24 → pop → user value 60\n");
    std::fflush(stdout);
}

void TestConfigStore::migration() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Write v1 config with old key names
    QString configDir = tmpDir.path() + QStringLiteral("/config");
    QDir().mkpath(configDir);
    QString configPath = configDir + QStringLiteral("/user_settings.json");

    QJsonObject v1;
    v1[QStringLiteral("meta.schemaVersion")] = 1;
    v1[QStringLiteral("editor.fps")] = 30;
    v1[QStringLiteral("editor.codec")] = QStringLiteral("h265");

    QFile f(configPath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QJsonDocument(v1).toJson());
    f.close();

    // Create engine — should auto-migrate v1 → v2
    ConfigurationEngineNew engine(nullptr, tmpDir.path());

    // Verify migrated keys
    QCOMPARE(engine.getInt(QStringLiteral("editor.playback.targetFps")), 30);
    QCOMPARE(engine.getString(QStringLiteral("editor.export.defaultCodec")),
             QStringLiteral("h265"));
    QCOMPARE(engine.schemaVersion(), 2);

    std::printf("Migration: v1→v2 successful. targetFps=%d, codec=%s, version=%d\n",
                engine.getInt(QStringLiteral("editor.playback.targetFps")),
                engine.getString(QStringLiteral("editor.export.defaultCodec")).toUtf8().constData(),
                engine.schemaVersion());
    std::fflush(stdout);
}

QTEST_GUILESS_MAIN(TestConfigStore)
#include "TestConfigStore.moc"
