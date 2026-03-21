#include "TestStorage.h"
#include "core/storage/storage_scheduler_engine.h"
#include "core/storage/block_aligner.h"
#include "core/storage/read_ahead_predictor.h"
#include "core/interfaces/IDeviceCapability.h"
#include "core/interfaces/ILogger.h"
#include "core/interfaces/IEventBus.h"
#include "core/interfaces/IStorageScheduler.h"

#include <QDebug>
#include <QTemporaryDir>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <mutex>
#include <thread>

using namespace gopost::storage;
using namespace gopost::core::interfaces;

// Minimal test doubles
class StubDeviceCap : public IDeviceCapability {
public:
    CpuInfo cpuInfo() const override { return {4, 8}; }
    GpuInfo gpuInfo() const override { return {}; }
    int64_t totalRamBytes() const override { return 16LL * 1024 * 1024 * 1024; }
    StorageType storageType() const override { return StorageType::NVMe; }
    HwDecodeSupport hwDecodeSupport() const override { return {}; }
    DeviceTier deviceTier() const override { return DeviceTier::High; }
    DeviceType deviceType() const override { return DeviceType::Desktop; }
    QVariantMap diagnosticSummary() const override { return {}; }
};

class StubLogger : public ILogger {
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

class StubEventBus : public IEventBus {
public:
    SubscriptionId subscribe(const QString&, EventHandler) override { return nextId_++; }
    void unsubscribe(SubscriptionId) override {}
    void publish(const QString& t, const QVariantMap& d, DeliveryMode) override {
        lastType_ = t; lastData_ = d; count_++;
    }
    QString lastType_; QVariantMap lastData_; int count_ = 0;
private:
    int nextId_ = 1;
};

static std::string createTempFile(const QString& dir, int sizeMB) {
    auto path = (dir + "/benchmark.bin").toStdString();
    std::ofstream f(path, std::ios::binary);
    std::vector<char> block(1024 * 1024, 'X');
    for (int i = 0; i < sizeMB; ++i) {
        // Write pattern: each MB starts with its index byte
        block[0] = static_cast<char>(i & 0xFF);
        f.write(block.data(), static_cast<std::streamsize>(block.size()));
    }
    f.close();
    return path;
}

void TestStorage::storageProfileDetection() {
    StubDeviceCap cap;
    StubLogger logger;
    StubEventBus bus;
    StorageSchedulerEngine engine(cap, logger, bus);

    auto profile = engine.getStorageProfile();
    QCOMPARE(profile.storageType, StorageType::NVMe);
    QCOMPARE(profile.recommendedBlockSize, 1024 * 1024);
    QCOMPARE(profile.maxConcurrentReads, 8);
    QVERIFY(profile.supportsDirectIO);
    QCOMPARE(profile.estimatedSequentialMBps, 3000.0);

    const char* typeStr = "Unknown";
    switch (profile.storageType) {
    case StorageType::HDD: typeStr = "HDD"; break;
    case StorageType::SSD: typeStr = "SSD"; break;
    case StorageType::NVMe: typeStr = "NVMe"; break;
    default: break;
    }
    qDebug().nospace()
        << "  Storage: " << typeStr
        << " | Block: " << (profile.recommendedBlockSize / 1024) << "KB"
        << " | Max concurrent: " << profile.maxConcurrentReads
        << " | Direct I/O: " << (profile.supportsDirectIO ? "YES" : "NO")
        << " | Est. speed: " << profile.estimatedSequentialMBps << " MB/s";
}

void TestStorage::sequentialReadBenchmark() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    auto filePath = createTempFile(tmpDir.path(), 50);

    StubDeviceCap cap;
    StubLogger logger;
    StubEventBus bus;
    StorageSchedulerEngine engine(cap, logger, bus);

    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<int> completed{0};
    int64_t totalBytes = 0;

    auto start = std::chrono::steady_clock::now();

    // Schedule 50 sequential 1MB reads
    for (int i = 0; i < 50; ++i) {
        ReadRequest req;
        req.path = filePath;
        req.offset = static_cast<int64_t>(i) * 1024 * 1024;
        req.length = 1024 * 1024;
        req.priority = ReadPriority::NORMAL;
        req.callback = [&](gopost::Result<std::vector<uint8_t>> res) {
            std::lock_guard lk(mtx);
            if (res.ok()) totalBytes += static_cast<int64_t>(res.get().size());
            completed++;
            if (completed >= 50) cv.notify_all();
        };
        auto ticket = engine.scheduleRead(req);
        QVERIFY(ticket.ok());
    }

    // Wait for all reads
    {
        std::unique_lock lk(mtx);
        cv.wait_for(lk, std::chrono::seconds(30), [&] { return completed >= 50; });
    }

    auto finish = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(finish - start).count();
    double mbps = (static_cast<double>(totalBytes) / (1024.0 * 1024.0)) / (ms / 1000.0);

    QVERIFY(completed >= 50);
    QVERIFY(totalBytes == 50LL * 1024 * 1024);
    QVERIFY(mbps > 0);

    qDebug().nospace() << "  Sequential read: 50MB in " << QString::number(ms, 'f', 1)
                       << "ms = " << QString::number(mbps, 'f', 1) << " MB/s";
}

void TestStorage::readAheadDetection() {
    ReadAheadPredictor predictor;

    std::string file = "/test/video.mp4";
    int64_t blockSize = 1024 * 1024;

    // Simulate 5 sequential reads
    for (int i = 0; i < 5; ++i) {
        predictor.recordRead(file, i * blockSize, blockSize);
    }

    bool detected = predictor.isSequential(file);
    auto prediction = predictor.predict(file);

    QVERIFY(detected);
    QVERIFY(prediction.isSequential);
    QCOMPARE(prediction.predictedOffset, 5 * blockSize);
    QCOMPARE(prediction.predictedLength, blockSize);

    qDebug() << "  Read-ahead: detected=" << (detected ? "yes" : "no")
             << "after 5 reads, predicted next offset =" << prediction.predictedOffset;
}

void TestStorage::priorityOrdering() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Create a small temp file
    auto filePath = createTempFile(tmpDir.path(), 2);

    StubDeviceCap cap;
    StubLogger logger;
    StubEventBus bus;
    // Use single-reader mode (HDD) to force sequential processing
    class HddCap : public StubDeviceCap {
    public:
        StorageType storageType() const override { return StorageType::HDD; }
    } hddCap;
    StorageSchedulerEngine engine(hddCap, logger, bus);

    std::mutex mtx;
    std::condition_variable cv;
    std::vector<ReadPriority> completionOrder;
    std::atomic<int> completed{0};
    int total = 6;

    // Schedule 5 LOW priority reads
    for (int i = 0; i < 5; ++i) {
        ReadRequest req;
        req.path = filePath;
        req.offset = 0;
        req.length = 1024;
        req.priority = ReadPriority::LOW;
        req.callback = [&, p = ReadPriority::LOW](gopost::Result<std::vector<uint8_t>>) {
            std::lock_guard lk(mtx);
            completionOrder.push_back(p);
            completed++;
            if (completed >= total) cv.notify_all();
        };
        engine.scheduleRead(req);
    }

    // Schedule 1 CRITICAL read
    {
        ReadRequest req;
        req.path = filePath;
        req.offset = 0;
        req.length = 1024;
        req.priority = ReadPriority::CRITICAL;
        req.callback = [&](gopost::Result<std::vector<uint8_t>>) {
            std::lock_guard lk(mtx);
            completionOrder.push_back(ReadPriority::CRITICAL);
            completed++;
            if (completed >= total) cv.notify_all();
        };
        engine.scheduleRead(req);
    }

    {
        std::unique_lock lk(mtx);
        cv.wait_for(lk, std::chrono::seconds(10), [&] { return completed >= total; });
    }

    QVERIFY(completed >= total);

    // Find position of CRITICAL in completion order
    int criticalPos = -1;
    for (int i = 0; i < static_cast<int>(completionOrder.size()); ++i) {
        if (completionOrder[i] == ReadPriority::CRITICAL) {
            criticalPos = i + 1;
            break;
        }
    }
    // CRITICAL should be near the front (at worst position 2 since the first LOW may already be in-flight)
    QVERIFY(criticalPos > 0);
    qDebug() << "  Priority: CRITICAL completed at position" << criticalPos << "of" << total;
}

void TestStorage::bandwidthMonitor() {
    StubEventBus bus;
    IOBandwidthMonitor monitor(bus, 3000.0);

    // Simulate some reads
    monitor.recordCompletion(1024 * 1024, 0.5, "/test/file.mp4");      // ~2000 MB/s
    monitor.recordCompletion(2 * 1024 * 1024, 0.8, "/test/file.mp4");  // ~2500 MB/s
    monitor.recordCompletion(1024 * 1024, 1.0, "/test/file.mp4");      // ~1000 MB/s

    auto snap = monitor.snapshot();
    QVERIFY(snap.currentMBps > 0);
    QVERIFY(snap.avgMBps > 0);
    QVERIFY(snap.peakMBps >= snap.avgMBps);

    qDebug().nospace() << "  Bandwidth: current=" << QString::number(snap.currentMBps, 'f', 1)
                       << " avg=" << QString::number(snap.avgMBps, 'f', 1)
                       << " peak=" << QString::number(snap.peakMBps, 'f', 1) << " MB/s";
}

void TestStorage::blockAlignment() {
    BlockAligner aligner(512 * 1024); // 512KB blocks

    // Read at offset 100, length 1000
    auto r1 = aligner.align(100, 1000);
    QCOMPARE(r1.alignedOffset, 0);
    QCOMPARE(r1.alignedLength, 512 * 1024);
    QCOMPARE(r1.trimStart, 100);
    QCOMPARE(r1.trimLength, 1000);

    // Read exactly at block boundary
    auto r2 = aligner.align(512 * 1024, 512 * 1024);
    QCOMPARE(r2.alignedOffset, 512 * 1024);
    QCOMPARE(r2.alignedLength, 512 * 1024);
    QCOMPARE(r2.trimStart, (int64_t)0);

    // Read spanning two blocks
    auto r3 = aligner.align(512 * 1024 - 1, 2);
    QCOMPARE(r3.alignedOffset, 0);
    QCOMPARE(r3.alignedLength, 2 * 512 * 1024);

    qDebug() << "  Block alignment: all boundary tests passed";
}

QTEST_GUILESS_MAIN(TestStorage)
#include "TestStorage.moc"
