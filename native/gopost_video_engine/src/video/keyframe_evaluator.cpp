#include "keyframe_evaluator.hpp"
#include <algorithm>
#include <cmath>

namespace gopost {
namespace video {

float interpolate(float t, KeyframeInterpolation interp) {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (interp) {
        case KeyframeInterpolation::EaseIn:    return t * t;
        case KeyframeInterpolation::EaseOut:   return 1.0f - (1.0f - t) * (1.0f - t);
        case KeyframeInterpolation::EaseInOut:
            return t < 0.5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2.0f) / 2;
        case KeyframeInterpolation::Bezier:    return t * t * (3.0f - 2.0f * t);
        case KeyframeInterpolation::Hold:      return 0.0f;
        default: return t;
    }
}

static double default_for_property(KeyframeProperty prop) {
    switch (prop) {
        case KeyframeProperty::Scale:
        case KeyframeProperty::Opacity:
        case KeyframeProperty::Volume:
            return 1.0;
        default:
            return 0.0;
    }
}

double evaluate_track(const KeyframeTrack& track, double time, double default_value) {
    const auto& kfs = track.keyframes;
    if (kfs.empty()) return default_value;
    if (kfs.size() == 1) return kfs[0].value;
    if (time <= kfs.front().time) return kfs.front().value;
    if (time >= kfs.back().time) return kfs.back().value;

    for (size_t i = 0; i < kfs.size() - 1; ++i) {
        const auto& a = kfs[i];
        const auto& b = kfs[i + 1];
        if (time >= a.time && time <= b.time) {
            if (a.interpolation == KeyframeInterpolation::Hold) return a.value;
            float t = static_cast<float>((time - a.time) / (b.time - a.time));
            float curved = interpolate(t, a.interpolation);
            return a.value + (b.value - a.value) * curved;
        }
    }
    return kfs.back().value;
}

EvaluatedProperties evaluate_keyframes(const ClipKeyframeState& state, double clip_local_time) {
    EvaluatedProperties props;
    for (const auto& track : state.tracks) {
        double def = default_for_property(track.property);
        double val = evaluate_track(track, clip_local_time, def);
        switch (track.property) {
            case KeyframeProperty::PositionX: props.position_x = val; break;
            case KeyframeProperty::PositionY: props.position_y = val; break;
            case KeyframeProperty::Scale:     props.scale = val; break;
            case KeyframeProperty::Rotation:  props.rotation = val; break;
            case KeyframeProperty::Opacity:   props.opacity = val; break;
            case KeyframeProperty::Volume:    props.volume = val; break;
            default: break;
        }
    }
    return props;
}

}  // namespace video
}  // namespace gopost
