#include "core/diagnostics/diagnostic_engine.h"
#include "core/diagnostics/performance_snapshot.h"

#include <QQmlContext>

namespace gopost::diagnostics {

DiagnosticEngine::DiagnosticEngine(const IDeviceCapability& deviceCap,
                                     IMemoryBudget* memBudget,
                                     QObject* parent)
    : QObject(parent)
    , deviceCap_(deviceCap)
    , memBudget_(memBudget)
    , overlay_(std::make_unique<OverlayDataProvider>(this, this))
{
}

void DiagnosticEngine::beginFrame() {
    frameTimer_.beginFrame();
}

void DiagnosticEngine::endFrame() {
    frameTimer_.endFrame();
}

FrameTimingSnapshot DiagnosticEngine::getTimingSnapshot() const {
    return frameTimer_.snapshot();
}

void DiagnosticEngine::beginScope(const QString& name) {
    scopeProfiler_.beginScope(name);
}

void DiagnosticEngine::endScope(const QString& name) {
    scopeProfiler_.endScope(name);
}

void DiagnosticEngine::setOverlayVisible(bool visible) {
    if (overlay_) overlay_->setVisible(visible);
}

bool DiagnosticEngine::isOverlayVisible() const {
    return overlay_ ? overlay_->visible() : false;
}

QVariantMap DiagnosticEngine::exportSnapshot() const {
    auto snap = frameTimer_.snapshot();
    QVariantMap m;
    m[QStringLiteral("fps")]           = snap.fps;
    m[QStringLiteral("avgFrameMs")]    = snap.avgFrameMs;
    m[QStringLiteral("p95FrameMs")]    = snap.p95FrameMs;
    m[QStringLiteral("totalFrames")]   = snap.totalFrames;
    m[QStringLiteral("droppedFrames")] = snap.droppedFrames;

    // Top 10 scopes
    QVariantList scopes;
    for (const auto& [name, stats] : scopeProfiler_.topScopes(10)) {
        QVariantMap s;
        s[QStringLiteral("name")]      = name;
        s[QStringLiteral("calls")]     = stats.callCount;
        s[QStringLiteral("totalMs")]   = stats.totalMs;
        s[QStringLiteral("maxMs")]     = stats.maxMs;
        s[QStringLiteral("avgMs")]     = stats.avgMs();
        scopes.append(s);
    }
    m[QStringLiteral("hotScopes")] = scopes;

    // Memory
    if (memBudget_)
        m[QStringLiteral("memory")] = memBudget_->breakdown();

    // Device info
    m[QStringLiteral("device")] = deviceCap_.diagnosticSummary();

    return m;
}

void DiagnosticEngine::registerWithQml(QQmlApplicationEngine* engine) {
    if (!engine) return;
    engine->rootContext()->setContextProperty(
        QStringLiteral("diagnosticOverlay"), overlay_.get());
}

} // namespace gopost::diagnostics
