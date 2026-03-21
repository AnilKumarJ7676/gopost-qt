#pragma once

#include <QString>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace gopost::diagnostics {

struct ScopeStats {
    int callCount = 0;
    double totalMs = 0.0;
    double maxMs = 0.0;
    double avgMs() const { return callCount > 0 ? totalMs / callCount : 0.0; }
};

class ScopeProfiler {
public:
    void beginScope(const QString& name);
    void endScope(const QString& name);

    [[nodiscard]] std::map<std::string, ScopeStats> allStats() const;
    [[nodiscard]] std::vector<std::pair<QString, ScopeStats>> topScopes(int n) const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, ScopeStats> stats_;

    // Thread-local scope start timestamps
    static thread_local std::vector<std::pair<std::string, uint64_t>> scopeStack_;
};

} // namespace gopost::diagnostics
