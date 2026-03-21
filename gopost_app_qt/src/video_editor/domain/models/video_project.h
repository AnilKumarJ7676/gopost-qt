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
enum class ClipSourceType : int { Video = 0, Image, Title, Color, Adjustment, Audio };

// ── Preset clip color palette (16 colors) ───────────────────────────────────

inline constexpr int kClipColorPresetCount = 16;

inline QString clipColorPresetHex(int index) {
    static const char* kPresets[16] = {
        "#EF5350", "#EC407A", "#AB47BC", "#7E57C2",  // Red, Pink, Purple, Deep Purple
        "#5C6BC0", "#42A5F5", "#26C6DA", "#26A69A",  // Indigo, Blue, Cyan, Teal
        "#66BB6A", "#9CCC65", "#D4E157", "#FFEE58",  // Green, Light Green, Lime, Yellow
        "#FFA726", "#FF7043", "#8D6E63", "#78909C",  // Orange, Deep Orange, Brown, Blue Grey
    };
    if (index < 0 || index >= 16) return {};
    return QString::fromLatin1(kPresets[index]);
}

inline QString clipColorPresetName(int index) {
    static const char* kNames[16] = {
        "Red", "Pink", "Purple", "Deep Purple",
        "Indigo", "Blue", "Cyan", "Teal",
        "Green", "Light Green", "Lime", "Yellow",
        "Orange", "Deep Orange", "Brown", "Blue Grey",
    };
    if (index < 0 || index >= 16) return {};
    return QString::fromLatin1(kNames[index]);
}

inline QString autoColorForSourceType(ClipSourceType t) {
    switch (t) {
        case ClipSourceType::Video:      return QStringLiteral("#42A5F5"); // Blue
        case ClipSourceType::Image:      return QStringLiteral("#FFEE58"); // Yellow
        case ClipSourceType::Title:      return QStringLiteral("#AB47BC"); // Purple
        case ClipSourceType::Color:      return QStringLiteral("#66BB6A"); // Green
        case ClipSourceType::Adjustment: return QStringLiteral("#78909C"); // Blue Grey
        case ClipSourceType::Audio:      return QStringLiteral("#66BB6A"); // Green
    }
    return QStringLiteral("#78909C");
}
enum class ProxyStatus : int { None = 0, Generating, Ready, Failed };
enum class MarkerType : int { Chapter = 0, Comment, Todo, Sync };

// ── CrossfadeType ──────────────────────────────────────────────────────────

enum class CrossfadeType : int { Crossfade = 0, DipToBlack, WipeLeft, WipeRight, WipeUp, WipeDown };

// ── TimelineTransition (cross-clip transitions from overlap) ───────────────

struct TimelineTransition {
    int clipAId{0};
    int clipBId{0};
    double duration{0.0};
    CrossfadeType type{CrossfadeType::Crossfade};

    [[nodiscard]] QJsonObject toMap() const;
    [[nodiscard]] static TimelineTransition fromMap(const QJsonObject& m);

    bool operator==(const TimelineTransition&) const = default;
};

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
    bool isReversed{false};
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
    std::optional<int> linkedClipId;   // Feature 10: linked audio/video pair
    std::optional<QString> colorTag;   // user-assigned color hex (e.g. "#EF5350"), nullopt = auto
    QString customLabel;               // user-assigned overlay label (empty = none)

    // Transform properties
    double positionX{0.0};
    double positionY{0.0};
    double scaleX{1.0};
    double scaleY{1.0};
    double rotation{0.0};
    double cropLeft{0.0};
    double cropTop{0.0};
    double cropRight{0.0};
    double cropBottom{0.0};

    // Text content (for Title/Subtitle clips)
    QString textContent;
    QString fontFamily;
    double fontSize{24.0};
    QString fontColor{QStringLiteral("#FFFFFF")};
    int textAlignment{1};   // 0=left, 1=center, 2=right

    // Original source file duration (never changes after clip creation)
    double sourceDuration{0.0};

    // Freeze frame flag
    bool isFreezeFrame{false};

    // Disabled clip flag (skipped during playback, rendered dimmed)
    bool isDisabled{false};

    // Template placeholder flag
    bool isPlaceholder{false};
    QString placeholderLabel;  // hint for template users (e.g. "Main footage")

    // Magnetic timeline: connected clip (FCP-style)
    std::optional<int> connectedToPrimaryClipId;  // anchored to this primary storyline clip
    std::optional<double> syncPointOffset;         // seconds offset from primary clip's timelineIn

    [[nodiscard]] bool isAdjustmentLayer() const { return sourceType == ClipSourceType::Adjustment; }
    [[nodiscard]] double duration() const { return timelineOut - timelineIn; }

    [[nodiscard]] bool hasEffects() const;
    [[nodiscard]] bool hasColorGrading() const;
    [[nodiscard]] bool hasTransition() const;
    [[nodiscard]] bool hasSpeedChange() const;
    [[nodiscard]] bool hasSpeedRamp() const;
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
        std::optional<bool> isReversed;
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
        std::optional<int> linkedClipId;
        bool clearLinkedClipId{false};
        std::optional<QString> colorTag;
        bool clearColorTag{false};
        std::optional<QString> customLabel;
        // Transform
        std::optional<double> positionX;
        std::optional<double> positionY;
        std::optional<double> scaleX;
        std::optional<double> scaleY;
        std::optional<double> rotation;
        std::optional<double> cropLeft;
        std::optional<double> cropTop;
        std::optional<double> cropRight;
        std::optional<double> cropBottom;
        // Text
        std::optional<QString> textContent;
        std::optional<QString> fontFamily;
        std::optional<double> fontSize;
        std::optional<QString> fontColor;
        std::optional<int> textAlignment;
        // Source duration
        std::optional<double> sourceDuration;
        // Freeze frame
        std::optional<bool> isFreezeFrame;
        // Disabled clip
        std::optional<bool> isDisabled;
        // Placeholder
        std::optional<bool> isPlaceholder;
        std::optional<QString> placeholderLabel;
        // Magnetic timeline connected clip
        std::optional<int> connectedToPrimaryClipId;
        bool clearConnectedToPrimaryClipId{false};
        std::optional<double> syncPointOffset;
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
    std::optional<QString> color;   // user-assigned track color hex, nullopt = default
    double trackHeight{64.0};       // per-track height in pixels (min 28, max 200)
    bool isMagneticPrimary{false};  // true for primary storyline track (magnetic timeline)

    struct CopyWithOpts {
        std::optional<QString> label;
        std::optional<bool> isVisible;
        std::optional<bool> isLocked;
        std::optional<bool> isMuted;
        std::optional<bool> isSolo;
        std::optional<QList<VideoClip>> clips;
        std::optional<TrackAudioSettings> audioSettings;
        std::optional<QString> color;
        bool clearColor{false};
        std::optional<double> trackHeight;
        std::optional<bool> isMagneticPrimary;
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
    QString notes;                          // optional notes/description
    std::optional<double> endPositionSeconds; // if set, marker is a region (span)
    std::optional<int> clipId;              // if set, marker is attached to a clip

    [[nodiscard]] bool isRegion() const { return endPositionSeconds.has_value(); }
    [[nodiscard]] bool isClipMarker() const { return clipId.has_value(); }
    [[nodiscard]] double regionDuration() const {
        return isRegion() ? *endPositionSeconds - positionSeconds : 0.0;
    }

    [[nodiscard]] static QString defaultColorForType(MarkerType t) {
        switch (t) {
            case MarkerType::Chapter: return QStringLiteral("#4CAF50");
            case MarkerType::Comment: return QStringLiteral("#2196F3");
            case MarkerType::Todo:    return QStringLiteral("#FF9800");
            case MarkerType::Sync:    return QStringLiteral("#E91E63");
        }
        return QStringLiteral("#9E9E9E");
    }

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
    QList<TimelineTransition> transitions;  // Feature 9: cross-clip crossfade transitions

    [[nodiscard]] double duration() const;
    [[nodiscard]] QList<VideoClip> allClips() const;
    [[nodiscard]] const VideoClip* findClip(int clipId) const;

    struct CopyWithOpts {
        std::optional<int> width;
        std::optional<int> height;
        std::optional<QList<VideoTrack>> tracks;
        std::optional<QList<TimelineMarker>> markers;
        std::optional<QList<TimelineTransition>> transitions;
    };

    [[nodiscard]] VideoProject copyWith(const CopyWithOpts& opts) const;

    [[nodiscard]] QJsonObject toMap() const;
    [[nodiscard]] static VideoProject fromMap(const QJsonObject& m);
};

} // namespace gopost::video_editor
