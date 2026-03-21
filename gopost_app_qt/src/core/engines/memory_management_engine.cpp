#include "memory_management_engine.h"
#include "core/platform/platform_capability_engine.h"
#include "core/interfaces/IDeviceCapability.h"

#include <QMutexLocker>
#include <QVariantMap>

namespace gopost::core::engines {

MemoryManagementEngine::MemoryManagementEngine(platform::PlatformCapabilityEngine* platform,
                                                 QObject* parent)
    : QObject(parent)
    , m_platform(platform) {}

MemoryManagementEngine::~MemoryManagementEngine() {
    shutdown();
}

void MemoryManagementEngine::initialize() {
    if (m_initialized) return;

    // Set global budget based on device tier
    double ramGb = static_cast<double>(m_platform->totalRamBytes())
                   / (1024.0 * 1024.0 * 1024.0);

    switch (m_platform->deviceTier()) {
    case core::interfaces::DeviceTier::Ultra:
        m_globalBudget = static_cast<qint64>(ramGb * 0.25 * 1024 * 1024 * 1024); // 25% of RAM
        break;
    case core::interfaces::DeviceTier::High:
        m_globalBudget = static_cast<qint64>(ramGb * 0.20 * 1024 * 1024 * 1024);
        break;
    case core::interfaces::DeviceTier::Mid:
        m_globalBudget = static_cast<qint64>(ramGb * 0.15 * 1024 * 1024 * 1024);
        break;
    case core::interfaces::DeviceTier::Low:
        m_globalBudget = static_cast<qint64>(ramGb * 0.10 * 1024 * 1024 * 1024);
        break;
    }

    // Register default categories with proportional budgets
    registerCategory(QStringLiteral("frame_pool"),  m_globalBudget * 40 / 100);
    registerCategory(QStringLiteral("thumbnails"),  m_globalBudget * 20 / 100);
    registerCategory(QStringLiteral("audio"),       m_globalBudget * 15 / 100);
    registerCategory(QStringLiteral("cache"),       m_globalBudget * 15 / 100);
    registerCategory(QStringLiteral("misc"),        m_globalBudget * 10 / 100);

    // Start pressure monitoring
    connect(&m_pressureTimer, &QTimer::timeout, this, &MemoryManagementEngine::checkPressure);
    m_pressureTimer.start(2000); // 2 second interval

    m_initialized = true;
}

void MemoryManagementEngine::shutdown() {
    if (!m_initialized) return;
    m_pressureTimer.stop();
    m_initialized = false;
}

bool MemoryManagementEngine::isInitialized() const {
    return m_initialized;
}

void MemoryManagementEngine::setGlobalBudgetBytes(qint64 bytes) {
    m_globalBudget = bytes;
}

qint64 MemoryManagementEngine::globalBudgetBytes() const {
    return m_globalBudget;
}

qint64 MemoryManagementEngine::currentUsageBytes() const {
    QMutexLocker lock(&m_mutex);
    qint64 total = 0;
    for (auto it = m_categories.constBegin(); it != m_categories.constEnd(); ++it) {
        total += it.value().usage;
    }
    return total;
}

double MemoryManagementEngine::usageRatio() const {
    if (m_globalBudget <= 0) return 0.0;
    return static_cast<double>(currentUsageBytes()) / static_cast<double>(m_globalBudget);
}

void MemoryManagementEngine::registerCategory(const QString& name, qint64 initialBudgetBytes) {
    QMutexLocker lock(&m_mutex);
    if (!m_categories.contains(name)) {
        m_categories.insert(name, {initialBudgetBytes, 0});
    }
}

void MemoryManagementEngine::setCategoryBudget(const QString& name, qint64 bytes) {
    QMutexLocker lock(&m_mutex);
    if (m_categories.contains(name)) {
        m_categories[name].budget = bytes;
    }
}

qint64 MemoryManagementEngine::categoryBudget(const QString& name) const {
    QMutexLocker lock(&m_mutex);
    auto it = m_categories.constFind(name);
    return it != m_categories.constEnd() ? it->budget : 0;
}

qint64 MemoryManagementEngine::categoryUsage(const QString& name) const {
    QMutexLocker lock(&m_mutex);
    auto it = m_categories.constFind(name);
    return it != m_categories.constEnd() ? it->usage : 0;
}

void MemoryManagementEngine::reportAllocation(const QString& category, qint64 bytes) {
    QMutexLocker lock(&m_mutex);
    auto it = m_categories.find(category);
    if (it == m_categories.end()) {
        m_categories.insert(category, {0, bytes});
        return;
    }
    it->usage += bytes;

    if (it->budget > 0 && it->usage > it->budget) {
        qint64 overage = it->usage - it->budget;
        lock.unlock();
        emit budgetExceeded(category, overage);
    }
}

void MemoryManagementEngine::reportDeallocation(const QString& category, qint64 bytes) {
    QMutexLocker lock(&m_mutex);
    auto it = m_categories.find(category);
    if (it != m_categories.end()) {
        it->usage = qMax(qint64(0), it->usage - bytes);
    }
}

MemoryManagementEngine::PressureLevel MemoryManagementEngine::currentPressure() const {
    return m_currentPressure;
}

qint64 MemoryManagementEngine::recommendedCacheSize(const QString& category) const {
    qint64 budget = categoryBudget(category);
    if (budget <= 0) return 0;

    switch (m_currentPressure) {
    case PressureLevel::Normal:   return budget;
    case PressureLevel::Warning:  return budget * 60 / 100;
    case PressureLevel::Critical: return budget * 30 / 100;
    }
    return budget;
}

QVariantMap MemoryManagementEngine::breakdown() const {
    QMutexLocker lock(&m_mutex);
    QVariantMap result;
    for (auto it = m_categories.constBegin(); it != m_categories.constEnd(); ++it) {
        QVariantMap cat;
        cat[QStringLiteral("budget")] = it->budget;
        cat[QStringLiteral("usage")] = it->usage;
        result[it.key()] = cat;
    }
    return result;
}

void MemoryManagementEngine::checkPressure() {
    if (!m_platform) return;

    qint64 available = m_platform->availableRamBytes();
    qint64 total = m_platform->totalRamBytes();
    double ratio = usageRatio();
    double freeRatio = (total > 0) ? static_cast<double>(available) / static_cast<double>(total)
                                   : 1.0;

    PressureLevel newLevel = PressureLevel::Normal;

    if (ratio > 0.95 || freeRatio < 0.05) {
        newLevel = PressureLevel::Critical;
    } else if (ratio > 0.80 || freeRatio < 0.15) {
        newLevel = PressureLevel::Warning;
    }

    if (newLevel != m_currentPressure) {
        m_currentPressure = newLevel;
        emit pressureChanged(newLevel);
    }
}

} // namespace gopost::core::engines
