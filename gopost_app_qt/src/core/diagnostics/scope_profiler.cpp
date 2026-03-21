#include "core/diagnostics/scope_profiler.h"
#include "core/diagnostics/high_res_timer.h"

#include <algorithm>

namespace gopost::diagnostics {

thread_local std::vector<std::pair<std::string, uint64_t>> ScopeProfiler::scopeStack_;

void ScopeProfiler::beginScope(const QString& name) {
    scopeStack_.emplace_back(name.toStdString(), HighResTimer::now());
}

void ScopeProfiler::endScope(const QString& name) {
    uint64_t endTime = HighResTimer::now();
    std::string key = name.toStdString();

    // Find matching scope on stack (search from back)
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        if (it->first == key) {
            double ms = HighResTimer::elapsedMs(it->second, endTime);

            std::lock_guard<std::mutex> lock(mutex_);
            auto& s = stats_[key];
            s.callCount++;
            s.totalMs += ms;
            if (ms > s.maxMs) s.maxMs = ms;

            // Erase from stack
            scopeStack_.erase(std::next(it).base());
            return;
        }
    }
}

std::map<std::string, ScopeStats> ScopeProfiler::allStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

std::vector<std::pair<QString, ScopeStats>> ScopeProfiler::topScopes(int n) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<QString, ScopeStats>> sorted;
    sorted.reserve(stats_.size());
    for (const auto& [k, v] : stats_)
        sorted.emplace_back(QString::fromStdString(k), v);

    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second.totalMs > b.second.totalMs; });

    if (static_cast<int>(sorted.size()) > n)
        sorted.resize(n);
    return sorted;
}

} // namespace gopost::diagnostics
