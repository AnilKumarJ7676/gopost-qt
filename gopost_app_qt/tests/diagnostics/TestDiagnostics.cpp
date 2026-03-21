#include "TestDiagnostics.h"

#include "core/platform/platform_capability_engine.h"
#include "core/memory/memory_engine.h"
#include "core/diagnostics/diagnostic_engine.h"

#include <cstdio>
#include <thread>

using namespace gopost::diagnostics;
using namespace gopost::core::interfaces;

void TestDiagnostics::frameTracking() {
    gopost::platform::PlatformCapabilityEngine platform;
    platform.initialize();

    gopost::memory::MemoryEngine memEngine(platform);
    DiagnosticEngine engine(platform, &memEngine);

    // Simulate 300 frames. On Windows, sleep_for has ~15.6ms resolution,
    // so requesting 1ms actually sleeps ~15-16ms. We just need to verify
    // that the frame timer tracks correctly — the exact FPS depends on
    // Windows timer granularity.
    for (int i = 0; i < 300; ++i) {
        engine.beginFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        engine.endFrame();
    }

    auto snap = engine.getTimingSnapshot();

    std::printf("\n========================================\n");
    std::printf(" Diagnostic Engine — Frame Timing Test\n");
    std::printf("========================================\n");
    std::printf("FPS:            %.1f\n", snap.fps);
    std::printf("Avg Frame:      %.2f ms\n", snap.avgFrameMs);
    std::printf("P95 Frame:      %.2f ms\n", snap.p95FrameMs);
    std::printf("Last Frame:     %.2f ms\n", snap.lastFrameMs);
    std::printf("Total Frames:   %d\n", snap.totalFrames);
    std::printf("Dropped Frames: %d\n", snap.droppedFrames);
    std::printf("========================================\n\n");
    std::fflush(stdout);

    QVERIFY2(snap.totalFrames == 300,
             qPrintable(QStringLiteral("Expected 300 frames, got %1").arg(snap.totalFrames)));
    QVERIFY2(snap.fps > 0.0,
             qPrintable(QStringLiteral("FPS should be positive: %1").arg(snap.fps)));
    QVERIFY2(snap.avgFrameMs > 0.0,
             qPrintable(QStringLiteral("Avg frame time should be positive: %1 ms").arg(snap.avgFrameMs)));
}

void TestDiagnostics::scopedTiming() {
    gopost::platform::PlatformCapabilityEngine platform;
    platform.initialize();

    DiagnosticEngine engine(platform);

    // Profile a scope
    for (int i = 0; i < 100; ++i) {
        engine.beginScope(QStringLiteral("testOp"));
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        engine.endScope(QStringLiteral("testOp"));
    }

    auto snap = engine.exportSnapshot();
    auto scopes = snap[QStringLiteral("hotScopes")].toList();
    QVERIFY(!scopes.isEmpty());

    auto first = scopes[0].toMap();
    QCOMPARE(first[QStringLiteral("name")].toString(), QStringLiteral("testOp"));
    QCOMPARE(first[QStringLiteral("calls")].toInt(), 100);
    QVERIFY(first[QStringLiteral("totalMs")].toDouble() > 0);

    std::printf("Scope 'testOp': %d calls, total %.2f ms, avg %.3f ms\n",
                first["calls"].toInt(),
                first["totalMs"].toDouble(),
                first["avgMs"].toDouble());
    std::fflush(stdout);
}

void TestDiagnostics::overlayToggle() {
    gopost::platform::PlatformCapabilityEngine platform;
    platform.initialize();

    DiagnosticEngine engine(platform);

    QVERIFY(engine.isOverlayVisible());  // default visible

    engine.setOverlayVisible(false);
    QVERIFY(!engine.isOverlayVisible());

    engine.setOverlayVisible(true);
    QVERIFY(engine.isOverlayVisible());
}

QTEST_GUILESS_MAIN(TestDiagnostics)
#include "TestDiagnostics.moc"
