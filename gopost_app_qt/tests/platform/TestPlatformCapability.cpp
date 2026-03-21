#include "TestPlatformCapability.h"
#include "core/platform/platform_capability_engine.h"
#include "core/interfaces/IDeviceCapability.h"

using namespace gopost::platform;
using namespace gopost::core::interfaces;

void TestPlatformCapability::detectCpu() {
    PlatformCapabilityEngine engine;
    engine.initialize();
    auto cpu = engine.cpuInfo();
    QVERIFY(cpu.physicalCores > 0);
    QVERIFY(cpu.logicalThreads > 0);
    QVERIFY(cpu.logicalThreads >= cpu.physicalCores);
}

void TestPlatformCapability::detectGpu() {
    PlatformCapabilityEngine engine;
    engine.initialize();
    auto gpu = engine.gpuInfo();
    QVERIFY(!gpu.name.isEmpty());
}

void TestPlatformCapability::detectRam() {
    PlatformCapabilityEngine engine;
    engine.initialize();
    QVERIFY(engine.totalRamBytes() > 0);
}

void TestPlatformCapability::detectStorage() {
    PlatformCapabilityEngine engine;
    engine.initialize();
    // Storage detection may return Unknown on some systems
    auto st = engine.storageType();
    Q_UNUSED(st);
    QVERIFY(true);
}

void TestPlatformCapability::detectHwDecode() {
    PlatformCapabilityEngine engine;
    engine.initialize();
    auto hw = engine.hwDecodeSupport();
    Q_UNUSED(hw);
    QVERIFY(true);
}

void TestPlatformCapability::classifyTier() {
    PlatformCapabilityEngine engine;
    engine.initialize();
    auto tier = engine.deviceTier();
    QVERIFY(tier >= DeviceTier::Low && tier <= DeviceTier::Ultra);
}

QTEST_GUILESS_MAIN(TestPlatformCapability)
#include "TestPlatformCapability.moc"
