#pragma once

#include "core/interfaces/IDiagnostics.h"
#include "core/diagnostics/scope_profiler.h"

#include <QVariantMap>
#include <vector>

namespace gopost::diagnostics {

struct PerformanceSnapshot {
    core::interfaces::FrameTimingSnapshot frameTiming;
    std::vector<std::pair<QString, ScopeStats>> hotScopes;
    QVariantMap memoryBreakdown;
};

} // namespace gopost::diagnostics
