#include "core/diagnostics/overlay_data_provider.h"
#include "core/diagnostics/diagnostic_engine.h"

namespace gopost::diagnostics {

OverlayDataProvider::OverlayDataProvider(DiagnosticEngine* engine, QObject* parent)
    : QObject(parent), engine_(engine)
{
    timer_.setInterval(500);
    connect(&timer_, &QTimer::timeout, this, &OverlayDataProvider::refresh);
    timer_.start();
}

void OverlayDataProvider::setVisible(bool v) {
    if (visible_ != v) {
        visible_ = v;
        emit visibleChanged();
    }
}

void OverlayDataProvider::refresh() {
    if (!engine_) return;

    auto snap = engine_->getTimingSnapshot();
    fps_ = snap.fps;
    frameTime_ = snap.avgFrameMs;
    droppedFrames_ = snap.droppedFrames;

    // Format memory usage from IMemoryBudget
    auto budgets = engine_->memoryBudgets();
    if (budgets) {
        double usedGb = 0;
        double totalGb = 0;
        auto b = budgets->computeBudgets();
        totalGb = static_cast<double>(b.globalBudgetBytes) / (1024.0 * 1024.0 * 1024.0);

        // Sum usage across all pools
        for (const auto& name : {QStringLiteral("frameCache"), QStringLiteral("thumbnailCache"),
                                  QStringLiteral("audioCache"), QStringLiteral("effectCache"),
                                  QStringLiteral("importBuffer"), QStringLiteral("reserve")}) {
            usedGb += static_cast<double>(budgets->currentUsage(name)) / (1024.0 * 1024.0 * 1024.0);
        }
        memoryUsed_ = QStringLiteral("%1 / %2 GB")
            .arg(usedGb, 0, 'f', 1)
            .arg(totalGb, 0, 'f', 1);
    } else {
        memoryUsed_ = QStringLiteral("N/A");
    }

    emit updated();
}

} // namespace gopost::diagnostics
