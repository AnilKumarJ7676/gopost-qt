#include "TestLogger.h"

#include "core/logging/logging_engine_new.h"
#include "core/interfaces/ILogger.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

using namespace gopost::logging;
using namespace gopost::core::interfaces;

void TestLogger::coloredConsoleOutput() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    LoggingEngineNew engine(tmpDir.path());

    // Log 10 messages at each level for PLATFORM category
    for (int i = 0; i < 10; ++i) {
        engine.trace(QStringLiteral("Platform"), QStringLiteral("Trace message %1").arg(i));
        engine.debug(QStringLiteral("Platform"), QStringLiteral("Debug message %1").arg(i));
        engine.info(QStringLiteral("Platform"), QStringLiteral("Info message %1").arg(i));
        engine.warn(QStringLiteral("Platform"), QStringLiteral("Warning message %1").arg(i));
        engine.error(QStringLiteral("Platform"), QStringLiteral("Error message %1").arg(i));
        engine.fatal(QStringLiteral("Platform"), QStringLiteral("Fatal message %1").arg(i));
    }

    engine.flush();
    // Visual verification: check console for colored output
    QVERIFY(true);
}

void TestLogger::fileLogging() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    {
        LoggingEngineNew engine(tmpDir.path());

        engine.info(QStringLiteral("Platform"), QStringLiteral("Platform engine started"));
        engine.warn(QStringLiteral("Memory"), QStringLiteral("Memory pressure detected"));
        engine.error(QStringLiteral("Network"), QStringLiteral("Connection failed"));

        engine.flush();
    }

    // Verify log file exists and contains messages
    QString logPath = tmpDir.path() + QStringLiteral("/gopost.log");
    QFile logFile(logPath);
    QVERIFY2(logFile.exists(), "Log file should exist");

    QVERIFY(logFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(logFile.readAll());
    logFile.close();

    QVERIFY2(content.contains(QStringLiteral("Platform engine started")),
             "Log should contain info message");
    QVERIFY2(content.contains(QStringLiteral("Memory pressure detected")),
             "Log should contain warning message");
    QVERIFY2(content.contains(QStringLiteral("Connection failed")),
             "Log should contain error message");
    QVERIFY2(content.contains(QStringLiteral("[INFO ]")),
             "Log should contain level tag");

    std::printf("Log file: %s (%lld bytes)\n",
                logPath.toLocal8Bit().constData(), logFile.size());
    std::fflush(stdout);
}

void TestLogger::categoryFiltering() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    {
        LoggingEngineNew engine(tmpDir.path());

        // Disable MEMORY category
        engine.setCategoryEnabled(LogCategory::Memory, false);
        QVERIFY(!engine.isCategoryEnabled(LogCategory::Memory));

        // Log 5 MEMORY messages — should be filtered out
        for (int i = 0; i < 5; ++i)
            engine.info(QStringLiteral("Memory"), QStringLiteral("filtered_msg_%1").arg(i));

        engine.flush();

        // Re-enable MEMORY category
        engine.setCategoryEnabled(LogCategory::Memory, true);

        // Log 5 more MEMORY messages — should appear
        for (int i = 0; i < 5; ++i)
            engine.info(QStringLiteral("Memory"), QStringLiteral("visible_msg_%1").arg(i));

        engine.flush();
    }

    QString logPath = tmpDir.path() + QStringLiteral("/gopost.log");
    QFile logFile(logPath);
    QVERIFY(logFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(logFile.readAll());
    logFile.close();

    QVERIFY2(!content.contains(QStringLiteral("filtered_msg_")),
             "Filtered messages should NOT appear in log");
    QVERIFY2(content.contains(QStringLiteral("visible_msg_")),
             "Re-enabled messages should appear in log");
}

void TestLogger::fileRotation() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    {
        // Use 1 MB max for faster test
        QString logPath = tmpDir.path() + QStringLiteral("/gopost.log");
        auto fileSink = std::make_unique<gopost::logging::FileLogSink>(logPath, 1 * 1024 * 1024, 5);

        // Write >3 MB of data to trigger multiple rotations
        QString bigMsg(512, QLatin1Char('X')); // 512 chars per line
        for (int i = 0; i < 7000; ++i) {
            fileSink->write(bigMsg, LogLevel::Info);
        }
        fileSink->flush();
    }

    // Check rotation results
    QString mainLog = tmpDir.path() + QStringLiteral("/gopost.log");
    QString rot1 = tmpDir.path() + QStringLiteral("/gopost.log.1");

    QFileInfo mainInfo(mainLog);
    QFileInfo rot1Info(rot1);

    QVERIFY2(mainInfo.exists(), "Main log file should exist");
    QVERIFY2(mainInfo.size() < 1.5 * 1024 * 1024,
             qPrintable(QStringLiteral("Main log should be < 1.5 MB, got %1 bytes")
                        .arg(mainInfo.size())));
    QVERIFY2(rot1Info.exists(), "Rotated file gopost.log.1 should exist");

    // Count rotated files
    int rotCount = 0;
    for (int i = 1; i <= 5; ++i) {
        QString p = QStringLiteral("%1/gopost.log.%2").arg(tmpDir.path()).arg(i);
        if (QFile::exists(p)) ++rotCount;
    }

    std::printf("\nRotation: main=%.2f MB, rotated_files=%d\n",
                mainInfo.size() / (1024.0 * 1024.0), rotCount);
    std::fflush(stdout);
}

QTEST_GUILESS_MAIN(TestLogger)
#include "TestLogger.moc"
