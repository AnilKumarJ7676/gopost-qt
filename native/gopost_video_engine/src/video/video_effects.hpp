#ifndef GOPOST_VIDEO_EFFECTS_HPP
#define GOPOST_VIDEO_EFFECTS_HPP

#include "gopost/types.h"
#include <vector>

namespace gopost {
namespace video {

enum class VideoEffectType {
    Brightness = 0, Contrast, Saturation, Exposure, Temperature, Tint,
    Highlights, Shadows, Vibrance, HueRotate,
    GaussianBlur, RadialBlur, TiltShift, Sharpen,
    Pixelate, Glitch, Chromatic,
    Vignette, Grain, Sepia, Invert, Posterize,
    COUNT
};

struct ClipEffect {
    VideoEffectType type = VideoEffectType::Brightness;
    float value = 0.0f;
    bool enabled = true;
    float mix = 1.0f;
};

struct ColorGrading {
    float brightness = 0;
    float contrast = 0;
    float saturation = 0;
    float exposure = 0;
    float temperature = 0;
    float tint = 0;
    float highlights = 0;
    float shadows = 0;
    float vibrance = 0;
    float hue = 0;

    bool is_default() const {
        return brightness == 0 && contrast == 0 && saturation == 0 &&
               exposure == 0 && temperature == 0 && tint == 0 &&
               highlights == 0 && shadows == 0 && vibrance == 0 && hue == 0;
    }
};

struct ClipEffectState {
    std::vector<ClipEffect> effects;
    ColorGrading color_grading;
    int32_t preset_filter_id = 0;
};

void apply_color_grading(GopostFrame* frame, const ColorGrading& grading);
void apply_effect(GopostFrame* frame, const ClipEffect& effect);
void apply_clip_effects(GopostFrame* frame, const ClipEffectState& state);

}  // namespace video
}  // namespace gopost

#endif
