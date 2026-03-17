#ifndef GOPOST_VIDEO_TRANSITION_RENDERER_HPP
#define GOPOST_VIDEO_TRANSITION_RENDERER_HPP

#include "gopost/types.h"
#include <cstdint>

namespace gopost {
namespace video {

enum class TransitionType {
    None = 0, Fade, Dissolve,
    SlideLeft, SlideRight, SlideUp, SlideDown,
    WipeLeft, WipeRight, WipeUp, WipeDown,
    Zoom, Push, Reveal, Iris, Clock, Blur, Glitch, Morph, Flash, Spin,
    COUNT
};

enum class EasingCurve { Linear = 0, EaseIn, EaseOut, EaseInOut, CubicBezier };

struct TransitionDesc {
    TransitionType type = TransitionType::None;
    double duration_seconds = 0.5;
    EasingCurve easing = EasingCurve::EaseInOut;
};

float apply_easing(float t, EasingCurve curve);

/**
 * Render a transition between two frames.
 * @param out    Output frame (must be allocated, same size as from/to)
 * @param from   Outgoing frame (may be null for fade-in)
 * @param to     Incoming frame (may be null for fade-out)
 * @param progress 0.0 = fully 'from', 1.0 = fully 'to'
 * @param type   Transition type
 */
void render_transition(GopostFrame* out,
                       const GopostFrame* from,
                       const GopostFrame* to,
                       float progress,
                       TransitionType type);

}  // namespace video
}  // namespace gopost

#endif
