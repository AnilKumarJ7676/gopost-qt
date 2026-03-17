#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <algorithm>
#include <cmath>
#include <optional>

namespace gopost::video_editor {

// ── KeyframeProperty ────────────────────────────────────────────────────────

enum class KeyframeProperty : int {
    PositionX = 0,
    PositionY,
    Scale,
    Rotation,
    Opacity,
    Volume,
    Speed
};

inline QString keyframePropertyLabel(KeyframeProperty p) {
    switch (p) {
        case KeyframeProperty::PositionX: return QStringLiteral("Position X");
        case KeyframeProperty::PositionY: return QStringLiteral("Position Y");
        case KeyframeProperty::Scale:     return QStringLiteral("Scale");
        case KeyframeProperty::Rotation:  return QStringLiteral("Rotation");
        case KeyframeProperty::Opacity:   return QStringLiteral("Opacity");
        case KeyframeProperty::Volume:    return QStringLiteral("Volume");
        case KeyframeProperty::Speed:     return QStringLiteral("Speed");
    }
    return {};
}

inline double keyframePropertyDefaultValue(KeyframeProperty p) {
    switch (p) {
        case KeyframeProperty::Scale:
        case KeyframeProperty::Opacity:
        case KeyframeProperty::Volume:
        case KeyframeProperty::Speed:
            return 1.0;
        default:
            return 0.0;
    }
}

// ── KeyframeInterpolation ───────────────────────────────────────────────────

enum class KeyframeInterpolation : int {
    Linear = 0,
    Bezier,
    Hold,
    EaseIn,
    EaseOut,
    EaseInOut
};

inline QString keyframeInterpolationLabel(KeyframeInterpolation i) {
    switch (i) {
        case KeyframeInterpolation::Linear:    return QStringLiteral("Linear");
        case KeyframeInterpolation::Bezier:    return QStringLiteral("Bezier");
        case KeyframeInterpolation::Hold:      return QStringLiteral("Hold");
        case KeyframeInterpolation::EaseIn:    return QStringLiteral("Ease In");
        case KeyframeInterpolation::EaseOut:   return QStringLiteral("Ease Out");
        case KeyframeInterpolation::EaseInOut: return QStringLiteral("Ease In Out");
    }
    return {};
}

// ── Keyframe ────────────────────────────────────────────────────────────────

struct Keyframe {
    double time{0.0};
    double value{0.0};
    KeyframeInterpolation interpolation{KeyframeInterpolation::Linear};

    [[nodiscard]] Keyframe copyWith(
        std::optional<double> time_ = std::nullopt,
        std::optional<double> value_ = std::nullopt,
        std::optional<KeyframeInterpolation> interpolation_ = std::nullopt
    ) const {
        return {
            .time = time_.value_or(time),
            .value = value_.value_or(value),
            .interpolation = interpolation_.value_or(interpolation),
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        return {
            {QStringLiteral("time"), time},
            {QStringLiteral("value"), value},
            {QStringLiteral("interpolation"), static_cast<int>(interpolation)},
        };
    }

    [[nodiscard]] static Keyframe fromMap(const QJsonObject& m) {
        return {
            .time = m.value(QStringLiteral("time")).toDouble(),
            .value = m.value(QStringLiteral("value")).toDouble(),
            .interpolation = static_cast<KeyframeInterpolation>(
                m.value(QStringLiteral("interpolation")).toInt(0)),
        };
    }

    bool operator==(const Keyframe& o) const {
        return time == o.time && value == o.value;
    }
};

// ── KeyframeTrack ───────────────────────────────────────────────────────────

struct KeyframeTrack {
    KeyframeProperty property{KeyframeProperty::PositionX};
    QList<Keyframe> keyframes;

    [[nodiscard]] double evaluate(double t) const {
        if (keyframes.isEmpty()) return keyframePropertyDefaultValue(property);
        if (keyframes.size() == 1) return keyframes.first().value;

        if (t <= keyframes.first().time) return keyframes.first().value;
        if (t >= keyframes.last().time) return keyframes.last().value;

        for (int i = 0; i < keyframes.size() - 1; ++i) {
            const auto& a = keyframes[i];
            const auto& b = keyframes[i + 1];
            if (t >= a.time && t <= b.time) {
                if (a.interpolation == KeyframeInterpolation::Hold) return a.value;
                double frac = (t - a.time) / (b.time - a.time);
                double curved = applyCurve(frac, a.interpolation);
                return a.value + (b.value - a.value) * curved;
            }
        }
        return keyframes.last().value;
    }

    [[nodiscard]] KeyframeTrack addKeyframe(const Keyframe& kf) const {
        auto updated = keyframes;
        int idx = -1;
        for (int i = 0; i < updated.size(); ++i) {
            if (updated[i].time == kf.time) { idx = i; break; }
        }
        if (idx >= 0) {
            updated[idx] = kf;
        } else {
            updated.append(kf);
            std::sort(updated.begin(), updated.end(),
                [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
        }
        return {.property = property, .keyframes = updated};
    }

    [[nodiscard]] KeyframeTrack removeKeyframeAt(double time) const {
        QList<Keyframe> updated;
        for (const auto& k : keyframes) {
            if (k.time != time) updated.append(k);
        }
        return {.property = property, .keyframes = updated};
    }

    [[nodiscard]] KeyframeTrack moveKeyframe(double oldTime, double newTime, double newValue) const {
        auto updated = keyframes;
        for (auto& k : updated) {
            if (k.time == oldTime) {
                k = k.copyWith(newTime, newValue);
                break;
            }
        }
        std::sort(updated.begin(), updated.end(),
            [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
        return {.property = property, .keyframes = updated};
    }

private:
    [[nodiscard]] static double applyCurve(double t, KeyframeInterpolation interp) {
        switch (interp) {
            case KeyframeInterpolation::Linear:    return t;
            case KeyframeInterpolation::EaseIn:    return t * t;
            case KeyframeInterpolation::EaseOut:   return 1.0 - (1.0 - t) * (1.0 - t);
            case KeyframeInterpolation::EaseInOut:
                return t < 0.5 ? 2.0 * t * t : 1.0 - (-2.0 * t + 2.0) * (-2.0 * t + 2.0) / 2.0;
            case KeyframeInterpolation::Bezier:    return t * t * (3.0 - 2.0 * t);
            case KeyframeInterpolation::Hold:      return 0.0;
        }
        return t;
    }
};

// ── ClipKeyframes ───────────────────────────────────────────────────────────

struct ClipKeyframes {
    QList<KeyframeTrack> tracks;

    [[nodiscard]] bool isEmpty() const {
        if (tracks.isEmpty()) return true;
        return std::all_of(tracks.begin(), tracks.end(),
            [](const KeyframeTrack& t) { return t.keyframes.isEmpty(); });
    }

    [[nodiscard]] const KeyframeTrack* trackFor(KeyframeProperty prop) const {
        for (const auto& t : tracks) {
            if (t.property == prop) return &t;
        }
        return nullptr;
    }

    [[nodiscard]] double evaluate(KeyframeProperty prop, double time) const {
        const auto* track = trackFor(prop);
        return track ? track->evaluate(time) : keyframePropertyDefaultValue(prop);
    }

    [[nodiscard]] ClipKeyframes updateTrack(const KeyframeTrack& track) const {
        auto updated = tracks;
        int idx = -1;
        for (int i = 0; i < updated.size(); ++i) {
            if (updated[i].property == track.property) { idx = i; break; }
        }
        if (idx >= 0) {
            updated[idx] = track;
        } else {
            updated.append(track);
        }
        return {.tracks = updated};
    }

    [[nodiscard]] ClipKeyframes removeTrack(KeyframeProperty prop) const {
        QList<KeyframeTrack> updated;
        for (const auto& t : tracks) {
            if (t.property != prop) updated.append(t);
        }
        return {.tracks = updated};
    }

    [[nodiscard]] QJsonObject toMap() const {
        QJsonArray arr;
        for (const auto& t : tracks) {
            QJsonArray kfArr;
            for (const auto& k : t.keyframes) {
                kfArr.append(k.toMap());
            }
            arr.append(QJsonObject{
                {QStringLiteral("property"), static_cast<int>(t.property)},
                {QStringLiteral("keyframes"), kfArr},
            });
        }
        return {{QStringLiteral("tracks"), arr}};
    }

    [[nodiscard]] static ClipKeyframes fromMap(const QJsonObject& m) {
        ClipKeyframes result;
        const auto arr = m.value(QStringLiteral("tracks")).toArray();
        for (const auto& val : arr) {
            const auto obj = val.toObject();
            KeyframeTrack track;
            track.property = static_cast<KeyframeProperty>(
                obj.value(QStringLiteral("property")).toInt());
            const auto kfArr = obj.value(QStringLiteral("keyframes")).toArray();
            for (const auto& kv : kfArr) {
                track.keyframes.append(Keyframe::fromMap(kv.toObject()));
            }
            result.tracks.append(track);
        }
        return result;
    }
};

} // namespace gopost::video_editor
