#pragma once

#include "core/interfaces/IDiagnostics.h"
#include "core/interfaces/IDeviceCapability.h"
#include "core/interfaces/IMemoryBudget.h"
#include "core/diagnostics/frame_timer.h"
#include "core/diagnostics/scope_profiler.h"
#include "core/diagnostics/overlay_data_provider.h"

#include <QObject>
#include <QQmlApplicationEngine>
#include <memory>

namespace gopost::diagnostics {

using core::interfaces::IDiagnostics;
using core::interfaces::IDeviceCapability;
using core::interfaces::IMemoryBudget;
using core::interfaces::FrameTimingSnapshot;

class DiagnosticEngine : public QObject, public IDiagnostics {
    Q_OBJECT
public:
    explicit DiagnosticEngine(const IDeviceCapability& deviceCap,
                               IMemoryBudget* memBudget = nullptr,
                               QObject* parent = nullptr);

    // IDiagnostics
    void beginFrame() override;
    void endFrame() override;
    [[nodiscard]] FrameTimingSnapshot getTimingSnapshot() const override;
    void beginScope(const QString& name) override;
    void endScope(const QString& name) override;
    void setOverlayVisible(bool visible) override;
    [[nodiscard]] bool isOverlayVisible() const override;
    [[nodiscard]] QVariantMap exportSnapshot() const override;

    // QML registration
    void registerWithQml(QQmlApplicationEngine* engine);

    // Accessor for overlay provider
    IMemoryBudget* memoryBudgets() const { return memBudget_; }

private:
    const IDeviceCapability& deviceCap_;
    IMemoryBudget* memBudget_;

    FrameTimer frameTimer_;
    ScopeProfiler scopeProfiler_;
    std::unique_ptr<OverlayDataProvider> overlay_;
};

} // namespace gopost::diagnostics
