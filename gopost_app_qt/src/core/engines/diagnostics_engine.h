#pragma once

#include "core_engine.h"

#include <QObject>
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <array>

namespace gopost::core::engines {

class MemoryManagementEngine;
class LoggingEngine;

class DiagnosticsEngine : public QObject, public CoreEngine {
    Q_OBJECT
    Q_PROPERTY(double fps READ fps NOTIFY fpsUpdated)
    Q_PROPERTY(double memoryUsageMb READ memoryUsageMb NOTIFY memoryUpdated)
public:
    explicit DiagnosticsEngine(MemoryManagementEngine* memory,
                               LoggingEngine* logging,
                               QObject* parent = nullptr);
    ~DiagnosticsEngine() override;

    // CoreEngine
    void initialize() override;
    void shutdown() override;
    [[nodiscard]] bool isInitialized() const override;
    [[nodiscard]] QString engineName() const override { return QStringLiteral("Diagnostics"); }

    // Scoped timer — RAII helper for measuring operation durations
    class ScopedTimer {
    public:
        ScopedTimer(DiagnosticsEngine* engine, const QString& operation);
        ~ScopedTimer();
        void cancel();
    private:
        DiagnosticsEngine* m_engine;
        QString m_operation;
        QElapsedTimer m_timer;
        bool m_cancelled = false;
    };

    // Manual timing
    void recordDuration(const QString& operation, double milliseconds);

    // FPS tracking
    void frameRendered();
    [[nodiscard]] double fps() const;

    // Memory snapshots (from MemoryManagementEngine)
    [[nodiscard]] double memoryUsageMb() const;
    Q_INVOKABLE QVariantMap memoryBreakdown() const;

    // Operation histograms
    Q_INVOKABLE QVariantMap operationStats(const QString& operation) const;
    Q_INVOKABLE QVariantList allOperationStats() const;

    // Export diagnostic report as JSON
    Q_INVOKABLE QString exportReport() const;

signals:
    void fpsUpdated();
    void memoryUpdated();

private slots:
    void updateFps();
    void updateMemory();

private:
    static constexpr int kHistogramSize = 256;

    struct OperationHistogram {
        std::array<double, kHistogramSize> durations{};
        int count = 0;
        int writeIndex = 0;
        double minMs = std::numeric_limits<double>::max();
        double maxMs = 0.0;
        double sumMs = 0.0;
        double lastMs = 0.0;

        void record(double ms);
        [[nodiscard]] double avgMs() const;
        [[nodiscard]] double p95Ms() const;
        [[nodiscard]] int totalCount() const { return count; }
    };

    MemoryManagementEngine* m_memory = nullptr;
    LoggingEngine* m_logging = nullptr;
    bool m_initialized = false;

    // FPS tracking
    static constexpr int kFpsWindowSize = 60;
    std::array<qint64, kFpsWindowSize> m_frameTimestamps{};
    int m_frameWriteIndex = 0;
    int m_frameCount = 0;
    double m_currentFps = 0.0;
    QTimer m_fpsTimer;

    // Memory update timer
    QTimer m_memoryTimer;

    // Operation histograms
    mutable QMutex m_mutex;
    QHash<QString, OperationHistogram> m_histograms;
};

} // namespace gopost::core::engines
