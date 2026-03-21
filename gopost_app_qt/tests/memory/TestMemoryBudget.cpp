#include "TestMemoryBudget.h"

#include "core/platform/platform_capability_engine.h"
#include "core/memory/memory_engine.h"
#include "core/interfaces/IDeviceCapability.h"

#include <cstdio>

using namespace gopost::memory;
using namespace gopost::core::interfaces;

static const char* tierName(DeviceTier t) {
    switch (t) {
    case DeviceTier::Ultra: return "ULTRA";
    case DeviceTier::High:  return "HIGH";
    case DeviceTier::Mid:   return "MID";
    case DeviceTier::Low:   return "LOW";
    }
    return "?";
}

static void printPool(const char* name, int64_t bytes, int64_t ceiling) {
    double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    double pct = ceiling > 0 ? static_cast<double>(bytes) / static_cast<double>(ceiling) * 100.0 : 0.0;
    std::printf("  %-16s %6.2f GB (%4.0f%%)\n", name, gb, pct);
}

void TestMemoryEngine::printBudgets() {
    gopost::platform::PlatformCapabilityEngine platform;
    platform.initialize();

    MemoryEngine engine(platform);
    auto b = engine.computeBudgets();

    double totalGb = static_cast<double>(platform.totalRamBytes()) / (1024.0 * 1024.0 * 1024.0);
    double ceilingGb = static_cast<double>(b.globalBudgetBytes) / (1024.0 * 1024.0 * 1024.0);
    double ceilingPct = static_cast<double>(b.globalBudgetBytes) / static_cast<double>(platform.totalRamBytes()) * 100.0;

    std::printf("\n========================================\n");
    std::printf(" Memory Engine — Budget Report\n");
    std::printf("========================================\n");
    std::printf("Device Tier: %s | Total RAM: %.0f GB | Budget Ceiling: %.1f GB (%.0f%%)\n",
                tierName(platform.deviceTier()), totalGb, ceilingGb, ceilingPct);
    printPool("frameCache:",     b.frameCacheBytes,     b.globalBudgetBytes);
    printPool("thumbnailCache:", b.thumbnailCacheBytes, b.globalBudgetBytes);
    printPool("audioCache:",     b.audioCacheBytes,     b.globalBudgetBytes);
    printPool("effectCache:",    b.effectCacheBytes,    b.globalBudgetBytes);
    printPool("importBuffer:",   b.importBufferBytes,   b.globalBudgetBytes);
    printPool("reserve:",        b.reserveBytes,        b.globalBudgetBytes);
    std::printf("========================================\n\n");
    std::fflush(stdout);

    QVERIFY(b.globalBudgetBytes > 0);
    QVERIFY(b.frameCacheBytes > 0);
    QVERIFY(b.thumbnailCacheBytes > 0);
    QVERIFY(b.audioCacheBytes > 0);
    QVERIFY(b.effectCacheBytes > 0);
    QVERIFY(b.importBufferBytes > 0);
    QVERIFY(b.reserveBytes > 0);
}

void TestMemoryEngine::enforcement() {
    gopost::platform::PlatformCapabilityEngine platform;
    platform.initialize();

    MemoryEngine engine(platform);

    int64_t budget = engine.budgetFor(QStringLiteral("frameCache"));
    QVERIFY(budget > 0);

    // a. Acquire almost the full budget → should succeed
    bool ok1 = engine.tryAcquire(QStringLiteral("frameCache"), budget - 100);
    QVERIFY2(ok1, "tryAcquire(budget - 100) should succeed");

    // b. Try to acquire 200 more → should fail (only 100 left)
    bool ok2 = engine.tryAcquire(QStringLiteral("frameCache"), 200);
    QVERIFY2(!ok2, "tryAcquire(200) should fail when only 100 bytes remain");

    // c. Release 1000 bytes
    engine.release(QStringLiteral("frameCache"), 1000);

    // d. Now acquire 200 → should succeed (1100 bytes available)
    bool ok3 = engine.tryAcquire(QStringLiteral("frameCache"), 200);
    QVERIFY2(ok3, "tryAcquire(200) should succeed after releasing 1000");
}

void TestMemoryEngine::usageTracking() {
    gopost::platform::PlatformCapabilityEngine platform;
    platform.initialize();

    MemoryEngine engine(platform);

    QCOMPARE(engine.currentUsage(QStringLiteral("frameCache")), 0LL);

    engine.tryAcquire(QStringLiteral("frameCache"), 5000);
    QCOMPARE(engine.currentUsage(QStringLiteral("frameCache")), 5000LL);

    engine.tryAcquire(QStringLiteral("frameCache"), 3000);
    QCOMPARE(engine.currentUsage(QStringLiteral("frameCache")), 8000LL);

    engine.release(QStringLiteral("frameCache"), 2000);
    QCOMPARE(engine.currentUsage(QStringLiteral("frameCache")), 6000LL);
}

QTEST_GUILESS_MAIN(TestMemoryEngine)
#include "TestMemoryBudget.moc"
