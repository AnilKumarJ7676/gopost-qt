#include "core/memory/budget_enforcer.h"

#include <cstdio>

namespace gopost::memory {

BudgetEnforcer::BudgetEnforcer(MemoryPoolRegistry& registry)
    : registry_(registry) {}

bool BudgetEnforcer::tryAcquire(const QString& category, int64_t bytes) {
    auto* p = registry_.pool(category.toStdString());
    if (!p) return false;

    bool ok = p->tryAcquire(static_cast<uint64_t>(bytes));

    if (ok) {
        double ratio = static_cast<double>(p->currentUsage()) /
                       static_cast<double>(p->budget());
        if (ratio > 0.80) {
            std::fprintf(stderr,
                "[MemoryEngine] WARNING: pool \"%s\" at %.0f%% usage (%llu / %llu bytes)\n",
                category.toUtf8().constData(),
                ratio * 100.0,
                static_cast<unsigned long long>(p->currentUsage()),
                static_cast<unsigned long long>(p->budget()));
        }
    }

    return ok;
}

void BudgetEnforcer::release(const QString& category, int64_t bytes) {
    auto* p = registry_.pool(category.toStdString());
    if (p) p->release(static_cast<uint64_t>(bytes));
}

int64_t BudgetEnforcer::currentUsage(const QString& category) const {
    auto* p = registry_.pool(category.toStdString());
    return p ? static_cast<int64_t>(p->currentUsage()) : 0;
}

int64_t BudgetEnforcer::budgetFor(const QString& category) const {
    auto* p = registry_.pool(category.toStdString());
    return p ? static_cast<int64_t>(p->budget()) : 0;
}

QVariantMap BudgetEnforcer::breakdown() const {
    QVariantMap m;
    for (const auto& name : registry_.poolNames()) {
        auto* p = registry_.pool(name);
        if (!p) continue;
        QVariantMap pool;
        pool[QStringLiteral("budget")]  = static_cast<qint64>(p->budget());
        pool[QStringLiteral("usage")]   = static_cast<qint64>(p->currentUsage());
        pool[QStringLiteral("ratio")]   = p->budget() > 0
            ? static_cast<double>(p->currentUsage()) / static_cast<double>(p->budget())
            : 0.0;
        m[QString::fromStdString(name)] = pool;
    }
    return m;
}

} // namespace gopost::memory
