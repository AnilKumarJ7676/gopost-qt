#pragma once

#include "core_engine.h"

#include <QObject>
#include <QHash>
#include <QTimer>
#include <QMutex>

namespace gopost::core::engines {

class PlatformCapabilityEngine;

class MemoryManagementEngine : public QObject, public CoreEngine {
    Q_OBJECT
public:
    explicit MemoryManagementEngine(PlatformCapabilityEngine* platform,
                                     QObject* parent = nullptr);
    ~MemoryManagementEngine() override;

    // CoreEngine
    void initialize() override;
    void shutdown() override;
    [[nodiscard]] bool isInitialized() const override;
    [[nodiscard]] QString engineName() const override { return QStringLiteral("Memory"); }

    // Pressure levels
    enum class PressureLevel { Normal, Warning, Critical };
    Q_ENUM(PressureLevel)

    // Global budget
    void setGlobalBudgetBytes(qint64 bytes);
    [[nodiscard]] qint64 globalBudgetBytes() const;
    [[nodiscard]] qint64 currentUsageBytes() const;
    [[nodiscard]] double usageRatio() const;

    // Category budgets
    void registerCategory(const QString& name, qint64 initialBudgetBytes);
    void setCategoryBudget(const QString& name, qint64 bytes);
    [[nodiscard]] qint64 categoryBudget(const QString& name) const;
    [[nodiscard]] qint64 categoryUsage(const QString& name) const;

    // Tracking
    void reportAllocation(const QString& category, qint64 bytes);
    void reportDeallocation(const QString& category, qint64 bytes);

    // Pressure
    [[nodiscard]] PressureLevel currentPressure() const;

    // Adaptive sizing
    [[nodiscard]] qint64 recommendedCacheSize(const QString& category) const;

    // Snapshot for diagnostics
    [[nodiscard]] QVariantMap breakdown() const;

signals:
    void pressureChanged(PressureLevel level);
    void budgetExceeded(const QString& category, qint64 overageBytes);

private slots:
    void checkPressure();

private:
    struct CategoryInfo {
        qint64 budget = 0;
        qint64 usage = 0;
    };

    PlatformCapabilityEngine* m_platform = nullptr;
    bool m_initialized = false;
    qint64 m_globalBudget = 0;
    PressureLevel m_currentPressure = PressureLevel::Normal;

    mutable QMutex m_mutex;
    QHash<QString, CategoryInfo> m_categories;
    QTimer m_pressureTimer;
};

} // namespace gopost::core::engines
