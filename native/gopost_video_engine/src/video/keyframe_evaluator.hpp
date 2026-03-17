#ifndef GOPOST_VIDEO_KEYFRAME_EVALUATOR_HPP
#define GOPOST_VIDEO_KEYFRAME_EVALUATOR_HPP

#include <cstdint>
#include <vector>

namespace gopost {
namespace video {

enum class KeyframeProperty {
    PositionX = 0, PositionY, Scale, Rotation, Opacity, Volume, COUNT
};

enum class KeyframeInterpolation {
    Linear = 0, Bezier, Hold, EaseIn, EaseOut, EaseInOut
};

struct Keyframe {
    double time = 0;
    double value = 0;
    KeyframeInterpolation interpolation = KeyframeInterpolation::Linear;
};

struct KeyframeTrack {
    KeyframeProperty property = KeyframeProperty::PositionX;
    std::vector<Keyframe> keyframes;
};

struct ClipKeyframeState {
    std::vector<KeyframeTrack> tracks;
};

struct EvaluatedProperties {
    double position_x = 0;
    double position_y = 0;
    double scale = 1.0;
    double rotation = 0;
    double opacity = 1.0;
    double volume = 1.0;
};

double evaluate_track(const KeyframeTrack& track, double time, double default_value);
EvaluatedProperties evaluate_keyframes(const ClipKeyframeState& state, double clip_local_time);
float interpolate(float t, KeyframeInterpolation interp);

}  // namespace video
}  // namespace gopost

#endif
