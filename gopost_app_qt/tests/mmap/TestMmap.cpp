#include "TestMmap.h"
#include "core/mmap/mmap_engine.h"
#include "core/interfaces/ILogger.h"

#include <QDebug>
#include <QTemporaryDir>

#include <chrono>
#include <cstring>
#include <fstream>

using namespace gopost::mmap;
using namespace gopost::core::interfaces;

class MmapTestLogger : public ILogger {
public:
    void log(LogLevel, const QString&, const QString&, const QVariantMap&) override {}
    void trace(const QString&, const QString&) override {}
    void debug(const QString&, const QString&) override {}
    void info(const QString&, const QString&) override {}
    void warn(const QString&, const QString&) override {}
    void error(const QString&, const QString&, const QVariantMap&) override {}
    void fatal(const QString&, const QString&, const QVariantMap&) override {}
    void setMinLevel(LogLevel) override {}
    LogLevel minLevel() const override { return LogLevel::Trace; }
    void setCategoryEnabled(LogCategory, bool) override {}
    bool isCategoryEnabled(LogCategory) const override { return true; }
};

static std::string createPatternFile(const QString& dir, const QString& name, size_t size) {
    auto path = (dir + "/" + name).toStdString();
    std::ofstream f(path, std::ios::binary);
    constexpr size_t chunk = 64 * 1024;
    std::vector<uint8_t> buf(chunk);
    size_t written = 0;
    while (written < size) {
        size_t toWrite = std::min(chunk, size - written);
        for (size_t i = 0; i < toWrite; ++i)
            buf[i] = static_cast<uint8_t>((written + i) % 256);
        f.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(toWrite));
        written += toWrite;
    }
    f.close();
    return path;
}

void TestMmap::smallFileRejection() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    auto path = createPatternFile(tmpDir.path(), "small.bin", 32 * 1024); // 32KB
    MmapTestLogger logger;
    MmapEngine engine(logger);

    auto result = engine.mapFile(path, 0, 32 * 1024);
    QVERIFY(!result.ok());
    QVERIFY(result.error.find("too small") != std::string::npos);
    qDebug() << "  Small file rejection: PASS";
}

void TestMmap::basicMmap10MB() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    constexpr size_t fileSize = 10 * 1024 * 1024;
    auto path = createPatternFile(tmpDir.path(), "test10mb.bin", fileSize);
    MmapTestLogger logger;
    MmapEngine engine(logger);

    auto result = engine.mapFile(path, 0, fileSize);
    QVERIFY2(result.ok(), result.error.c_str());
    QVERIFY(result.get().data != nullptr);
    QCOMPARE(result.get().length, fileSize);
    QVERIFY(result.get().isValid);

    // Verify byte pattern for first 1000 bytes
    auto& region = result.get();
    for (int i = 0; i < 1000; ++i) {
        uint8_t expected = static_cast<uint8_t>(i % 256);
        QCOMPARE(region.data[i], expected);
    }

    auto r = result.get();
    engine.unmapFile(r);
    qDebug() << "  Basic mmap 10MB: PASS — verified byte pattern";
}

void TestMmap::offsetMapping() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    constexpr size_t fileSize = 10 * 1024 * 1024;
    auto path = createPatternFile(tmpDir.path(), "test_offset.bin", fileSize);
    MmapTestLogger logger;
    MmapEngine engine(logger);

    uint64_t mapOffset = 5 * 1024 * 1024;
    size_t mapLength = 2 * 1024 * 1024;
    auto result = engine.mapFile(path, mapOffset, mapLength);
    QVERIFY2(result.ok(), result.error.c_str());
    QCOMPARE(result.get().length, mapLength);

    // Verify bytes match expected pattern at those offsets
    auto& region = result.get();
    for (int i = 0; i < 1000; ++i) {
        uint8_t expected = static_cast<uint8_t>((mapOffset + i) % 256);
        QCOMPARE(region.data[i], expected);
    }

    auto r = result.get();
    engine.unmapFile(r);
    qDebug() << "  Offset mmap (5MB+2MB): PASS";
}

void TestMmap::advisoryHints() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    constexpr size_t fileSize = 1 * 1024 * 1024;
    auto path = createPatternFile(tmpDir.path(), "advisory.bin", fileSize);
    MmapTestLogger logger;
    MmapEngine engine(logger);

    auto result = engine.mapFile(path, 0, fileSize);
    QVERIFY2(result.ok(), result.error.c_str());

    auto region = result.get();
    // Advisory hints should not crash
    engine.advisorySequential(region);
    engine.advisoryWillNeed(region, 0, fileSize);

    engine.unmapFile(region);
    qDebug() << "  Advisory hints: PASS";
}

void TestMmap::sequentialThroughput() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    constexpr size_t fileSize = 100 * 1024 * 1024; // 100MB
    auto path = createPatternFile(tmpDir.path(), "throughput.bin", fileSize);
    MmapTestLogger logger;
    MmapEngine engine(logger);

    auto result = engine.mapFile(path, 0, fileSize);
    QVERIFY2(result.ok(), result.error.c_str());
    QCOMPARE(result.get().length, fileSize);

    // Touch every page (one byte per 4KB) to force page faults
    auto start = std::chrono::steady_clock::now();
    volatile uint8_t sink = 0;
    auto* data = result.get().data;
    for (size_t off = 0; off < fileSize; off += 4096) {
        sink = data[off];
    }
    auto finish = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(finish - start).count();
    double mbps = (static_cast<double>(fileSize) / (1024.0 * 1024.0)) / (ms / 1000.0);

    QVERIFY(ms > 0);
    (void)sink;

    auto r = result.get();
    engine.unmapFile(r);

    qDebug().nospace() << "  mmap sequential read: " << (fileSize / (1024 * 1024))
                       << "MB in " << QString::number(ms, 'f', 1) << "ms = "
                       << QString::number(mbps, 'f', 1) << " MB/s";
}

void TestMmap::slidingWindow() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    constexpr size_t fileSize = 10 * 1024 * 1024; // 10MB
    auto path = createPatternFile(tmpDir.path(), "sliding.bin", fileSize);
    MmapTestLogger logger;

    // Set maxWindowSize to 2MB to force sliding
    MmapEngine engine(logger, 2 * 1024 * 1024);

    auto result = engine.mapFile(path, 0, fileSize);
    QVERIFY2(result.ok(), result.error.c_str());
    QVERIFY(result.get().data != nullptr);

    // Read bytes at 0, 1MB, 2MB, ..., 9MB — verify all correct
    auto& region = result.get();
    for (int i = 0; i < 10; ++i) {
        size_t off = static_cast<size_t>(i) * 1024 * 1024;
        if (off < region.length) {
            uint8_t expected = static_cast<uint8_t>(off % 256);
            QCOMPARE(region.data[off], expected);
        }
    }

    int slides = engine.slidingWindowMapper().slideCount();
    auto r = result.get();
    engine.unmapFile(r);

    qDebug() << "  Sliding window (2MB window, 10MB file): PASS —" << slides << "slides";
}

void TestMmap::cleanupUnmap() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    constexpr size_t fileSize = 1 * 1024 * 1024;
    auto path = createPatternFile(tmpDir.path(), "cleanup.bin", fileSize);
    MmapTestLogger logger;
    MmapEngine engine(logger);

    auto result = engine.mapFile(path, 0, fileSize);
    QVERIFY(result.ok());

    auto region = result.get();
    QVERIFY(region.isValid);
    QVERIFY(region.data != nullptr);

    engine.unmapFile(region);
    QVERIFY(!region.isValid);
    QVERIFY(region.data == nullptr);
    QCOMPARE(region.length, size_t(0));

    qDebug() << "  Cleanup: PASS";
}

QTEST_GUILESS_MAIN(TestMmap)
#include "TestMmap.moc"
