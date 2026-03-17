#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <cstdint>
#include <optional>

#include "video_editor/domain/models/video_effect.h"
#include "video_editor/domain/models/video_keyframe.h"
#include "video_editor/domain/models/video_transition.h"

namespace gopost::video_editor {

// ── AppliedBadge ────────────────────────────────────────────────────────────

enum class AppliedBadge : int {
    Effects = 0,
    Color,
    Transition,
    Speed,
    Keyframes,
    Audio
};

inline uint32_t appliedBadgeColor(AppliedBadge b) {
    switch (b) {
        case AppliedBadge::Effects:    return 0xFFAB47BC;
        case AppliedBadge::Color:      return 0xFFFF7043;
        case AppliedBadge::Transition: return 0xFF26C6DA;
        case AppliedBadge::Speed:      return 0xFFFFCA28;
        case AppliedBadge::Keyframes:  return 0xFF42A5F5;
        case AppliedBadge::Audio:      return 0xFF66BB6A;
    }
    return 0;
}

inline QString appliedBadgeShortLabel(AppliedBadge b) {
    switch (b) {
        case AppliedBadge::Effects:    return QStringLiteral("FX");
        case AppliedBadge::Color:      return QStringLiteral("CLR");
        case AppliedBadge::Transition: return QStringLiteral("TR");
        case AppliedBadge::Speed:      return QStringLiteral("SPD");
        case AppliedBadge::Keyframes:  return QStringLiteral("KF");
        case AppliedBadge::Audio:      return QStringLiteral("AUD");
    }
    return {};
}

// ── Enums ───────────────────────────────────────────────────────────────────

enum class TrackType : int { Video = 0, Audio, Title, Effect, Subtitle };
enum class ClipSourceType : int { Video = 0, Image, Title, Color, Adjustment };
enum class ProxyStatus : int { None = 0, Generating, Ready, Failed };
enum class MarkerType : int { Chapter = 0, Comment, Todo, Sync };

// ── ClipAudioSettings ───────────────────────────────────────────────────────

struct ClipAudioSettings {
    double volume{1.0};
    double pan{0.0};
    double fadeInSeconds{0.0};
    double fadeOutSeconds{0.0};
    bool isMuted{false};

    [[nodiscard]] ClipAudioSettings copyWith(
        std::optional<double> volume_ = std::nullopt,
        std::optional<double> pan_ = std::nullopt,
        std::optional<double> fadeInSeconds_ = std::nullopt,
        std::optional<double> fadeOutSeconds_ = std::nullopt,
        std::optional<bool> isMuted_ = std::nullopt
    ) const {
        return {
            .volume = volume_.value_or(volume),
            .pan = pan_.value_or(pan),
            .fadeInSeconds = fadeInSeconds_.value_or(fadeInSeconds),
            .fadeOutSeconds = fadeOutSeconds_.value_or(fadeOutSeconds),
            .isMuted = isMuted_.value_or(isMuted),
        };
    }

    [[nodiscard]] QJsonObject toMap() const;
    [[nodiscard]] static ClipAudioSettings fromMap(const QJsonObject& m);

    bool operator==(const ClipAudioSettings&) const = default;
};

// ── TrackAudioSettings ──────────────────────────────────────────────────────

struct TrackAudioSettings {
    double volume{1.0};
    double pan{0.0};

    [[nodiscard]] TrackAudioSettings copyWith(
        std::optional<double> volume_ = std::nullopt,
        std::optional<double> pan_ = std::nullopt
    ) const {
        return {
            .volume = volume_.value_or(volume),
            .pan = pan_.value_or(pan),
        };
    }

    [[nodiscard]] QJsonObject toMap() const;
    [[nodiscard]] static TrackAudioSettings fromMap(const QJsonObject& m);

    bool operator==(const TrackAudioSettings&) const = default;
};

// ── VideoClip ───────────────────────────────────────────────────────────────

struct VideoClip {
    int id{0};
    int trackIndex{0};
    ClipSourceType sourceType{ClipSourceType::Video};
    QString sourcePath;
    std::optional<QString> proxyPath;
    ProxyStatus proxyStatus{ProxyStatus::None};
    QString displayName;
    double timelineIn{0.0};
    double timelineOut{0.0};
    double sourceIn{0.0};
    double sourceOut{0.0};
    double speed{1.0};
    double opacity{1.0};
    int blendMode{0};
    int effectHash{0};
    QList<VideoEffect> effects;
    ColorGrading colorGrading;
    PresetFilterId presetFilter{PresetFilterId::None};
    ClipTransition transitionIn;
    ClipTransition transitionOut;
    ClipKeyframes keyframes;
    ClipAudioSettings audio;
    std::optional<AdjustmentClipData> adjustmentData;

    [[nodiscard]] bool isAdjustmentLayer() const { return sourceType == ClipSourceType::Adjustment; }
    [[nodiscard]] double duration() const { return timelineOut - timelineIn; }

    [[nodiscard]] bool hasEffects() const;
    [[nodiscard]] bool hasColorGrading() const;
    [[nodiscard]] bool hasTransition() const;
    [[nodiscard]] bool hasSpeedChange() const;
    [[nodiscard]] bool hasKeyframes() const;
    [[nodiscard]] bool hasAudioMod() const;
    [[nodiscard]] bool hasProxy() const;

    [[nodiscard]] QList<AppliedBadge> appliedBadges() const;

    struct CopyWithOpts {
        std::optional<int> id;
        std::optional<int> trackIndex;
        std::optional<double> timelineIn;
        std::optional<double> timelineOut;
        std::optional<double> sourceIn;
        std::optional<double> sourceOut;
        std::optional<double> speed;
        std::optional<double> opacity;
        std::optional<int> blendMode;
        std::optional<QString> proxyPath;
        bool clearProxyPath{false};
        std::optional<ProxyStatus> proxyStatus;
        std::optional<QList<VideoEffect>> effects;
        std::optional<ColorGrading> colorGrading;
        std::optional<PresetFilterId> presetFilter;
        std::optional<ClipTransition> transitionIn;
        std::optional<ClipTransition> transitionOut;
        std::optional<ClipKeyframes> keyframes;
        std::optional<ClipAudioSettings> audio;
        std::optional<AdjustmentClipData> adjustmentData;
    };

    [[nodiscard]] VideoClip copyWith(const CopyWithOpts& opts) const;

    [[nodiscard]] QJsonObject toMap() const;
    [[nodiscard]] static VideoClip fromMap(const QJsonObject& m);

    bool operator==(const VideoClip& other) const { return id == other.id; }
};

// ── VideoTrack ──────────────────────────────────────────────────────────────

struct VideoTrack {
    int index{0};
    TrackType type{TrackType::Video};
    QString label;
    bool isVisible{true};
    bool isLocked{false};
    bool isMuted{false};
    bool isSolo{false};
    QList<VideoClip> clips;
    TrackAudioSettings audioSettings;

    struct CopyWithOpts {
        std::optional<QString> label;
        std::optional<bool> isVisible;
        std::optional<bool> isLocked;
        std::optional<bool> isMuted;
        std::optional<bool> isSolo;
        std::optional<QList<VideoClip>> clips;
        std::optional<TrackAudioSettings> audioSettings;
    };

    [[nodiscard]] VideoTrack copyWith(const CopyWithOpts& opts) const;

    [[nodiscard]] QJsonObject toMap() const;
    [[nodiscard]] static VideoTrack fromMap(const QJsonObject& m);
};

// ── TimelineMarker ──────────────────────────────────────────────────────────

struct TimelineMarker {
    int id{0};
    double positionSeconds{0.0};
    MarkerType type{MarkerType::Chapter};
    QString label;
    std::optional<QString> color;

    [[nodiscard]] QJsonObject toMap() const;
    [[nodiscard]] static TimelineMarker fromMap(const QJsonObject& m);
};

// ── VideoProject ────────────────────────────────────────────────────────────

struct VideoProject {
    int timelineId{0};
    double frameRate{30.0};
    int width{1920};
    int height{1080};
    QList<VideoTrack> tracks;
    QList<TimelineMarker> markers;

    [[nodiscard]] double duration() const;
    [[nodiscard]] QList<VideoClip> allClips() const;
    [[nodiscard]] const VideoClip* findClip(int clipId) const;

    struct CopyWithOpts {
        std::optional<int> width;
        std::optional<int> height;
        std::optional<QList<VideoTrack>> tracks;
        std::optional<QList<TimelineMarker>> markers;
    };

    [[nodiscard]] VideoProject copyWith(const CopyWithOpts& opts) const;

    [[nodiscard]] QJsonObject toMap() const;
    [[nodiscard]] static VideoProject fromMap(const QJsonObject& m);
};

} // namespace gopost::video_editor
