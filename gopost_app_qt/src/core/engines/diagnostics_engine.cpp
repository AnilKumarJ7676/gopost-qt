#include "diagnostics_engine.h"
#include "memory_management_engine.h"
#include "logging_engine.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>
#include <algorithm>

namespace gopost::core::engines {

// --- OperationHistogram ---

void DiagnosticsEngine::OperationHistogram::record(double ms) {
    durations[writeIndex] = ms;
    writeIndex = (writeIndex + 1) % kHistogramSize;
    count++;
    lastMs = ms;
    sumMs += ms;
    if (ms < minMs) minMs = ms;
    if (ms > maxMs) maxMs = ms;
}

double DiagnosticsEngine::OperationHistogram::avgMs() const {
    return count > 0 ? sumMs / count : 0.0;
}

double DiagnosticsEngine::OperationHistogram::p95Ms() const {
    int n = qMin(count, kHistogramSize);
    if (n == 0) return 0.0;

    std::array<double, kHistogramSize> sorted;
    std::copy_n(durations.begin(), n, sorted.begin());
    std::sort(sorted.begin(), sorted.begin() + n);

    int idx = static_cast<int>(n * 0.95);
    if (idx >= n) idx = n - 1;
    return sorted[idx];
}

// --- ScopedTimer ---

DiagnosticsEngine::ScopedTimer::ScopedTimer(DiagnosticsEngine* engine,
                                             const QString& operation)
    : m_engine(engine)
    , m_operation(operation) {
    m_timer.start();
}

DiagnosticsEngine::ScopedTimer::~ScopedTimer() {
    if (!m_cancelled && m_engine) {
        m_engine->recordDuration(m_operation, m_timer.elapsed());
    }
}

void DiagnosticsEngine::ScopedTimer::cancel() {
    m_cancelled = true;
}

// --- DiagnosticsEngine ---

DiagnosticsEngine::DiagnosticsEngine(MemoryManagementEngine* memory,
                                     LoggingEngine* logging,
                                     QObject* parent)
    : QObject(parent)
    , m_memory(memory)
    , m_logging(logging) {}

DiagnosticsEngine::~DiagnosticsEngine() {
    shutdown();
}

void DiagnosticsEngine::initialize() {
    if (m_initialized) return;

    // FPS update at 1Hz
    connect(&m_fpsTimer, &QTimer::timeout, this, &DiagnosticsEngine::updateFps);
    m_fpsTimer.start(1000);

    // Memory update at 2Hz
    connect(&m_memoryTimer, &QTimer::timeout, this, &DiagnosticsEngine::updateMemory);
    m_memoryTimer.start(500);

    m_initialized = true;

    if (m_logging) {
        m_logging->info(QStringLiteral("Diagnostics"),
                        QStringLiteral("Diagnostics engine initialized"));
    }
}

void DiagnosticsEngine::shutdown() {
    if (!m_initialized) return;
    m_fpsTimer.stop();
    m_memoryTimer.stop();
    m_initialized = false;
}

bool DiagnosticsEngine::isInitialized() const {
    return m_initialized;
}

void DiagnosticsEngine::recordDuration(const QString& operation, double milliseconds) {
    QMutexLocker lock(&m_mutex);
    m_histograms[operation].record(milliseconds);
}

void DiagnosticsEngine::frameRendered() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_frameTimestamps[m_frameWriteIndex] = now;
    m_frameWriteIndex = (m_frameWriteIndex + 1) % kFpsWindowSize;
    if (m_frameCount < kFpsWindowSize) m_frameCount++;
}

double DiagnosticsEngine::fps() const {
    return m_currentFps;
}

double DiagnosticsEngine::memoryUsageMb() const {
    if (!m_memory) return 0.0;
    return static_cast<double>(m_memory->currentUsageBytes()) / (1024.0 * 1024.0);
}

QVariantMap DiagnosticsEngine::memoryBreakdown() const {
    if (!m_memory) return {};
    return m_memory->breakdown();
}

QVariantMap DiagnosticsEngine::operationStats(const QString& operation) const {
    QMutexLocker lock(&m_mutex);
    auto it = m_histograms.constFind(operation);
    if (it == m_histograms.constEnd()) return {};

    const auto& h = it.value();
    QVariantMap result;
    result[QStringLiteral("count")] = h.totalCount();
    result[QStringLiteral("avgMs")] = h.avgMs();
    result[QStringLiteral("minMs")] = h.minMs;
    result[QStringLiteral("maxMs")] = h.maxMs;
    result[QStringLiteral("p95Ms")] = h.p95Ms();
    result[QStringLiteral("lastMs")] = h.lastMs;
    return result;
}

QVariantList DiagnosticsEngine::allOperationStats() const {
    QMutexLocker lock(&m_mutex);
    QVariantList result;
    for (auto it = m_histograms.constBegin(); it != m_histograms.constEnd(); ++it) {
        const auto& h = it.value();
        QVariantMap entry;
        entry[QStringLiteral("operation")] = it.key();
        entry[QStringLiteral("count")] = h.totalCount();
        entry[QStringLiteral("avgMs")] = h.avgMs();
        entry[QStringLiteral("minMs")] = h.minMs;
        entry[QStringLiteral("maxMs")] = h.maxMs;
        entry[QStringLiteral("p95Ms")] = h.p95Ms();
        entry[QStringLiteral("lastMs")] = h.lastMs;
        result.append(entry);
    }
    return result;
}

QString DiagnosticsEngine::exportReport() const {
    QJsonObject report;

    // FPS
    report[QStringLiteral("fps")] = m_currentFps;

    // Memory
    if (m_memory) {
        QJsonObject mem;
        mem[QStringLiteral("totalUsageMb")] = memoryUsageMb();
        mem[QStringLiteral("globalBudgetMb")] =
            static_cast<double>(m_memory->globalBudgetBytes()) / (1024.0 * 1024.0);
        mem[QStringLiteral("usageRatio")] = m_memory->usageRatio();
        mem[QStringLiteral("pressureLevel")] = static_cast<int>(m_memory->currentPressure());
        report[QStringLiteral("memory")] = mem;
    }

    // Operations
    QJsonArray ops;
    {
        QMutexLocker lock(&m_mutex);
        for (auto it = m_histograms.constBegin(); it != m_histograms.constEnd(); ++it) {
            const auto& h = it.value();
            QJsonObject op;
            op[QStringLiteral("operation")] = it.key();
            op[QStringLiteral("count")] = h.totalCount();
            op[QStringLiteral("avgMs")] = h.avgMs();
            op[QStringLiteral("minMs")] = h.minMs;
            op[QStringLiteral("maxMs")] = h.maxMs;
            op[QStringLiteral("p95Ms")] = h.p95Ms();
            ops.append(op);
        }
    }
    report[QStringLiteral("operations")] = ops;

    return QString::fromUtf8(
        QJsonDocument(report).toJson(QJsonDocument::Indented));
}

void DiagnosticsEngine::updateFps() {
    if (m_frameCount < 2) {
        m_currentFps = 0.0;
        emit fpsUpdated();
        return;
    }

    // Find oldest and newest timestamps in the window
    int oldest = (m_frameWriteIndex - m_frameCount + kFpsWindowSize) % kFpsWindowSize;
    int newest = (m_frameWriteIndex - 1 + kFpsWindowSize) % kFpsWindowSize;

    qint64 dt = m_frameTimestamps[newest] - m_frameTimestamps[oldest];
    if (dt > 0) {
        m_currentFps = static_cast<double>(m_frameCount - 1) * 1000.0 / static_cast<double>(dt);
    } else {
        m_currentFps = 0.0;
    }

    emit fpsUpdated();
}

void DiagnosticsEngine::updateMemory() {
    emit memoryUpdated();
}

} // namespace gopost::core::engines
