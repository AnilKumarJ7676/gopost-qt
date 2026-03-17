#pragma once

#include <QImage>
#include <QString>
#include <array>
#include <optional>

namespace gopost::video_editor {

// ── PlaybackStatus ──────────────────────────────────────────────────────────

enum class PlaybackStatus : int { Stopped = 0, Playing, Paused, Seeking };

// ── Shuttle speeds ──────────────────────────────────────────────────────────

inline constexpr std::array<double, 9> kShuttleSpeeds = {-8, -4, -2, -1, 0, 1, 2, 4, 8};
inline constexpr int kShuttleStop = 4; // index of 0 (paused) in the array

// ── PlaybackState ───────────────────────────────────────────────────────────

struct PlaybackState {
    PlaybackStatus status{PlaybackStatus::Stopped};
    double positionSeconds{0.0};
    double durationSeconds{0.0};
    QImage previewFrame;
    std::optional<QString> activeVideoPath;

    /// Current JKL shuttle speed index into kShuttleSpeeds.
    int shuttleIndex{kShuttleStop};

    /// True while the user is scrubbing (dragging the playhead or ruler).
    bool isScrubbing{false};

    /// In-point set by user (I key), nullopt if unset.
    std::optional<double> inPoint;

    /// Out-point set by user (O key), nullopt if unset.
    std::optional<double> outPoint;

    [[nodiscard]] bool isPlaying() const { return status == PlaybackStatus::Playing; }
    [[nodiscard]] bool isStopped() const { return status == PlaybackStatus::Stopped; }
    [[nodiscard]] double shuttleSpeed() const { return kShuttleSpeeds[shuttleIndex]; }
    [[nodiscard]] bool isShuttling() const { return shuttleIndex != kShuttleStop; }

    struct CopyWithOpts {
        std::optional<PlaybackStatus> status;
        std::optional<double> positionSeconds;
        std::optional<double> durationSeconds;
        std::optional<QImage> previewFrame;
        bool clearFrame{false};
        std::optional<QString> activeVideoPath;
        bool clearVideoPath{false};
        std::optional<int> shuttleIndex;
        std::optional<bool> isScrubbing;
        std::optional<double> inPoint;
        bool clearInPoint{false};
        std::optional<double> outPoint;
        bool clearOutPoint{false};
    };

    [[nodiscard]] PlaybackState copyWith(const CopyWithOpts& o) const {
        PlaybackState s;
        s.status = o.status.value_or(status);
        s.positionSeconds = o.positionSeconds.value_or(positionSeconds);
        s.durationSeconds = o.durationSeconds.value_or(durationSeconds);
        s.previewFrame = o.clearFrame ? QImage{} : (o.previewFrame.has_value() ? *o.previewFrame : previewFrame);
        s.activeVideoPath = o.clearVideoPath ? std::nullopt :
            (o.activeVideoPath.has_value() ? o.activeVideoPath : activeVideoPath);
        s.shuttleIndex = o.shuttleIndex.value_or(shuttleIndex);
        s.isScrubbing = o.isScrubbing.value_or(isScrubbing);
        s.inPoint = o.clearInPoint ? std::nullopt : (o.inPoint.has_value() ? o.inPoint : inPoint);
        s.outPoint = o.clearOutPoint ? std::nullopt : (o.outPoint.has_value() ? o.outPoint : outPoint);
        return s;
    }
};

} // namespace gopost::video_editor
