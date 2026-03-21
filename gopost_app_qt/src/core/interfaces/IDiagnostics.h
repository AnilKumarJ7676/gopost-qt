#pragma once

#include <QString>
#include <QVariantMap>
#include <cstdint>

namespace gopost::core::interfaces {

struct FrameTimingSnapshot {
    double fps              = 0.0;
    double lastFrameMs      = 0.0;
    double avgFrameMs       = 0.0;
    double p95FrameMs       = 0.0;
    double memoryUsageMb    = 0.0;
    int    totalFrames      = 0;
    int    droppedFrames    = 0;
};

class IDiagnostics {
public:
    virtual ~IDiagnostics() = default;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    [[nodiscard]] virtual FrameTimingSnapshot getTimingSnapshot() const = 0;

    virtual void beginScope(const QString& name) = 0;
    virtual void endScope(const QString& name) = 0;

    virtual void setOverlayVisible(bool visible) = 0;
    [[nodiscard]] virtual bool isOverlayVisible() const = 0;

    [[nodiscard]] virtual QVariantMap exportSnapshot() const = 0;
};

} // namespace gopost::core::interfaces
