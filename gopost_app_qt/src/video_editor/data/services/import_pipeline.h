#pragma once

#include <QSemaphore>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// Import guardrails — concurrency limits and safety thresholds
// ---------------------------------------------------------------------------
struct ImportGuardrails {
    static constexpr int kMaxBatchSize         = 50;   // max files per drag-drop
    static constexpr int kMaxConcurrentProbes  = 4;    // concurrent ffprobe processes
    static constexpr int kMaxConcurrentProxies = 2;    // concurrent proxy encodes
    static constexpr int kProbeTimeoutMs       = 10000; // 10s per-file probe timeout
    static constexpr double kMemoryThresholdPct = 0.8;  // throttle at 80% RAM
};

// Global semaphores for concurrency control (thread-safe singletons)
inline QSemaphore& probeSemaphore() {
    static QSemaphore sem(ImportGuardrails::kMaxConcurrentProbes);
    return sem;
}

inline QSemaphore& proxySemaphore() {
    static QSemaphore sem(ImportGuardrails::kMaxConcurrentProxies);
    return sem;
}

} // namespace gopost::video_editor
