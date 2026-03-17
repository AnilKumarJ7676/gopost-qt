#pragma once

#include <QString>
#include <QJsonObject>
#include <cstdint>

namespace gopost::video_editor {

// ── TransitionType ──────────────────────────────────────────────────────────

enum class TransitionType : int {
    None = 0,
    Fade,
    Dissolve,
    SlideLeft,
    SlideRight,
    SlideUp,
    SlideDown,
    WipeLeft,
    WipeRight,
    WipeUp,
    WipeDown,
    Zoom,
    Push,
    Reveal,
    Iris,
    ClockWipe,
    Blur,
    Glitch,
    Morph,
    Flash,
    Spin
};

inline QString transitionTypeLabel(TransitionType t) {
    switch (t) {
        case TransitionType::None:       return QStringLiteral("None");
        case TransitionType::Fade:       return QStringLiteral("Fade");
        case TransitionType::Dissolve:   return QStringLiteral("Dissolve");
        case TransitionType::SlideLeft:  return QStringLiteral("Slide Left");
        case TransitionType::SlideRight: return QStringLiteral("Slide Right");
        case TransitionType::SlideUp:    return QStringLiteral("Slide Up");
        case TransitionType::SlideDown:  return QStringLiteral("Slide Down");
        case TransitionType::WipeLeft:   return QStringLiteral("Wipe Left");
        case TransitionType::WipeRight:  return QStringLiteral("Wipe Right");
        case TransitionType::WipeUp:     return QStringLiteral("Wipe Up");
        case TransitionType::WipeDown:   return QStringLiteral("Wipe Down");
        case TransitionType::Zoom:       return QStringLiteral("Zoom");
        case TransitionType::Push:       return QStringLiteral("Push");
        case TransitionType::Reveal:     return QStringLiteral("Reveal");
        case TransitionType::Iris:       return QStringLiteral("Iris");
        case TransitionType::ClockWipe:  return QStringLiteral("Clock Wipe");
        case TransitionType::Blur:       return QStringLiteral("Blur");
        case TransitionType::Glitch:     return QStringLiteral("Glitch");
        case TransitionType::Morph:      return QStringLiteral("Morph");
        case TransitionType::Flash:      return QStringLiteral("Flash");
        case TransitionType::Spin:       return QStringLiteral("Spin");
    }
    return {};
}

// ── EasingCurve ─────────────────────────────────────────────────────────────

enum class EasingCurve : int {
    Linear = 0,
    EaseIn,
    EaseOut,
    EaseInOut,
    CubicBezier
};

inline QString easingCurveLabel(EasingCurve e) {
    switch (e) {
        case EasingCurve::Linear:      return QStringLiteral("Linear");
        case EasingCurve::EaseIn:      return QStringLiteral("Ease In");
        case EasingCurve::EaseOut:     return QStringLiteral("Ease Out");
        case EasingCurve::EaseInOut:   return QStringLiteral("Ease In Out");
        case EasingCurve::CubicBezier: return QStringLiteral("Cubic Bezier");
    }
    return {};
}

// ── ClipTransition ──────────────────────────────────────────────────────────

struct ClipTransition {
    TransitionType type{TransitionType::None};
    double durationSeconds{0.5};
    EasingCurve easing{EasingCurve::EaseInOut};

    [[nodiscard]] bool isNone() const { return type == TransitionType::None; }

    [[nodiscard]] ClipTransition copyWith(
        std::optional<TransitionType> type_ = std::nullopt,
        std::optional<double> durationSeconds_ = std::nullopt,
        std::optional<EasingCurve> easing_ = std::nullopt
    ) const {
        return {
            .type = type_.value_or(type),
            .durationSeconds = durationSeconds_.value_or(durationSeconds),
            .easing = easing_.value_or(easing),
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        return {
            {QStringLiteral("type"), static_cast<int>(type)},
            {QStringLiteral("durationSeconds"), durationSeconds},
            {QStringLiteral("easing"), static_cast<int>(easing)},
        };
    }

    [[nodiscard]] static ClipTransition fromMap(const QJsonObject& m) {
        return {
            .type = static_cast<TransitionType>(m.value(QStringLiteral("type")).toInt(0)),
            .durationSeconds = m.value(QStringLiteral("durationSeconds")).toDouble(0.5),
            .easing = static_cast<EasingCurve>(m.value(QStringLiteral("easing")).toInt(3)),
        };
    }

    bool operator==(const ClipTransition&) const = default;
};

} // namespace gopost::video_editor
