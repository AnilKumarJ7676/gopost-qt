#include "video_effects.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace gopost {
namespace video {

static inline uint8_t clamp8(float v) {
    return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
}

static inline float srgb_to_linear(float s) {
    return s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
}

static inline float linear_to_srgb(float l) {
    return l <= 0.0031308f ? l * 12.92f : 1.055f * std::pow(l, 1.0f / 2.4f) - 0.055f;
}

// Converts RGB to HSL; H in [0,360), S,L in [0,1]
static void rgb_to_hsl(float r, float g, float b, float& h, float& s, float& l) {
    float mx = std::max({r, g, b}), mn = std::min({r, g, b});
    l = (mx + mn) * 0.5f;
    if (mx == mn) { h = s = 0; return; }
    float d = mx - mn;
    s = l > 0.5f ? d / (2.0f - mx - mn) : d / (mx + mn);
    if (mx == r) h = std::fmod((g - b) / d + 6.0f, 6.0f) * 60.0f;
    else if (mx == g) h = ((b - r) / d + 2.0f) * 60.0f;
    else h = ((r - g) / d + 4.0f) * 60.0f;
}

static float hue2rgb(float p, float q, float t) {
    if (t < 0) t += 1; if (t > 1) t -= 1;
    if (t < 1.0f/6) return p + (q - p) * 6.0f * t;
    if (t < 0.5f)   return q;
    if (t < 2.0f/3) return p + (q - p) * (2.0f/3 - t) * 6.0f;
    return p;
}

static void hsl_to_rgb(float h, float s, float l, float& r, float& g, float& b) {
    if (s == 0) { r = g = b = l; return; }
    float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;
    float hN = h / 360.0f;
    r = hue2rgb(p, q, hN + 1.0f/3);
    g = hue2rgb(p, q, hN);
    b = hue2rgb(p, q, hN - 1.0f/3);
}

void apply_color_grading(GopostFrame* frame, const ColorGrading& g) {
    if (!frame || !frame->data || g.is_default()) return;
    const uint32_t total = frame->width * frame->height;
    uint8_t* px = frame->data;

    const float bri = g.brightness / 100.0f;
    const float con = (g.contrast + 100.0f) / 100.0f;
    const float sat = (g.saturation + 100.0f) / 100.0f;
    const float exp_mul = std::pow(2.0f, g.exposure);
    const float temp = g.temperature / 100.0f;
    const float hue_shift = g.hue;
    const float hi = g.highlights / 100.0f;
    const float sh = g.shadows / 100.0f;

    for (uint32_t i = 0; i < total; ++i) {
        float r = px[0] / 255.0f;
        float gr = px[1] / 255.0f;
        float b = px[2] / 255.0f;

        // Exposure
        r *= exp_mul; gr *= exp_mul; b *= exp_mul;

        // Temperature: warm shifts red up / blue down
        if (temp != 0) {
            r += temp * 0.1f;
            b -= temp * 0.1f;
        }

        // Brightness
        r += bri; gr += bri; b += bri;

        // Contrast around midpoint 0.5
        r = (r - 0.5f) * con + 0.5f;
        gr = (gr - 0.5f) * con + 0.5f;
        b = (b - 0.5f) * con + 0.5f;

        // Highlights & shadows
        float lum = 0.2126f * r + 0.7152f * gr + 0.0722f * b;
        if (hi != 0 && lum > 0.5f) {
            float factor = (lum - 0.5f) * 2.0f * hi;
            r += factor; gr += factor; b += factor;
        }
        if (sh != 0 && lum < 0.5f) {
            float factor = (0.5f - lum) * 2.0f * sh;
            r += factor; gr += factor; b += factor;
        }

        // Saturation
        if (sat != 1.0f) {
            float gray = 0.2126f * r + 0.7152f * gr + 0.0722f * b;
            r = gray + (r - gray) * sat;
            gr = gray + (gr - gray) * sat;
            b = gray + (b - gray) * sat;
        }

        // Hue rotation
        if (hue_shift != 0) {
            float h, s, l;
            rgb_to_hsl(std::clamp(r, 0.f, 1.f), std::clamp(gr, 0.f, 1.f), std::clamp(b, 0.f, 1.f), h, s, l);
            h = std::fmod(h + hue_shift + 360.0f, 360.0f);
            hsl_to_rgb(h, s, l, r, gr, b);
        }

        px[0] = clamp8(r * 255.0f);
        px[1] = clamp8(gr * 255.0f);
        px[2] = clamp8(b * 255.0f);
        px += 4;
    }
}

static void apply_vignette(GopostFrame* frame, float strength) {
    if (!frame || !frame->data || strength <= 0) return;
    const float cx = frame->width * 0.5f;
    const float cy = frame->height * 0.5f;
    const float maxDist = std::sqrt(cx * cx + cy * cy);
    const float s = strength / 100.0f;
    uint8_t* px = frame->data;
    for (uint32_t y = 0; y < frame->height; ++y) {
        for (uint32_t x = 0; x < frame->width; ++x) {
            float dx = x - cx, dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy) / maxDist;
            float factor = 1.0f - dist * dist * s;
            if (factor < 0) factor = 0;
            px[0] = clamp8(px[0] * factor);
            px[1] = clamp8(px[1] * factor);
            px[2] = clamp8(px[2] * factor);
            px += 4;
        }
    }
}

static void apply_sepia(GopostFrame* frame, float strength) {
    if (!frame || !frame->data) return;
    const float s = std::clamp(strength / 100.0f, 0.0f, 1.0f);
    const uint32_t total = frame->width * frame->height;
    uint8_t* px = frame->data;
    for (uint32_t i = 0; i < total; ++i) {
        float r = px[0] / 255.0f, g = px[1] / 255.0f, b = px[2] / 255.0f;
        float sr = r * 0.393f + g * 0.769f + b * 0.189f;
        float sg = r * 0.349f + g * 0.686f + b * 0.168f;
        float sb = r * 0.272f + g * 0.534f + b * 0.131f;
        px[0] = clamp8((r + (sr - r) * s) * 255.0f);
        px[1] = clamp8((g + (sg - g) * s) * 255.0f);
        px[2] = clamp8((b + (sb - b) * s) * 255.0f);
        px += 4;
    }
}

static void apply_invert(GopostFrame* frame, float strength) {
    if (!frame || !frame->data) return;
    const float s = std::clamp(strength / 100.0f, 0.0f, 1.0f);
    const uint32_t total = frame->width * frame->height * 4;
    for (uint32_t i = 0; i < total; i += 4) {
        for (int c = 0; c < 3; ++c) {
            float orig = frame->data[i + c] / 255.0f;
            float inv = 1.0f - orig;
            frame->data[i + c] = clamp8((orig + (inv - orig) * s) * 255.0f);
        }
    }
}

static void apply_pixelate(GopostFrame* frame, float block_size) {
    if (!frame || !frame->data || block_size < 2) return;
    int bs = static_cast<int>(block_size);
    for (uint32_t by = 0; by < frame->height; by += bs) {
        for (uint32_t bx = 0; bx < frame->width; bx += bs) {
            uint32_t rSum = 0, gSum = 0, bSum = 0, count = 0;
            for (uint32_t y = by; y < std::min(by + (uint32_t)bs, frame->height); ++y) {
                for (uint32_t x = bx; x < std::min(bx + (uint32_t)bs, frame->width); ++x) {
                    uint8_t* p = frame->data + (y * frame->width + x) * 4;
                    rSum += p[0]; gSum += p[1]; bSum += p[2]; ++count;
                }
            }
            uint8_t ar = rSum / count, ag = gSum / count, ab = bSum / count;
            for (uint32_t y = by; y < std::min(by + (uint32_t)bs, frame->height); ++y) {
                for (uint32_t x = bx; x < std::min(bx + (uint32_t)bs, frame->width); ++x) {
                    uint8_t* p = frame->data + (y * frame->width + x) * 4;
                    p[0] = ar; p[1] = ag; p[2] = ab;
                }
            }
        }
    }
}

static void apply_posterize(GopostFrame* frame, float levels) {
    if (!frame || !frame->data || levels < 2) return;
    float l = std::clamp(levels, 2.0f, 256.0f);
    const uint32_t total = frame->width * frame->height * 4;
    for (uint32_t i = 0; i < total; i += 4) {
        for (int c = 0; c < 3; ++c) {
            float v = frame->data[i + c] / 255.0f;
            v = std::floor(v * l) / l;
            frame->data[i + c] = clamp8(v * 255.0f);
        }
    }
}

static void apply_grain(GopostFrame* frame, float strength) {
    if (!frame || !frame->data || strength <= 0) return;
    const float s = strength / 100.0f * 50.0f;
    const uint32_t total = frame->width * frame->height;
    uint8_t* px = frame->data;
    uint32_t seed = 42;
    for (uint32_t i = 0; i < total; ++i) {
        seed = seed * 1103515245u + 12345u;
        float noise = ((seed >> 16) & 0x7FFF) / 32767.0f * 2.0f - 1.0f;
        float n = noise * s;
        px[0] = clamp8(px[0] + n);
        px[1] = clamp8(px[1] + n);
        px[2] = clamp8(px[2] + n);
        px += 4;
    }
}

void apply_effect(GopostFrame* frame, const ClipEffect& effect) {
    if (!effect.enabled || effect.mix <= 0) return;
    switch (effect.type) {
        case VideoEffectType::Vignette:  apply_vignette(frame, effect.value * effect.mix); break;
        case VideoEffectType::Sepia:     apply_sepia(frame, effect.value * effect.mix); break;
        case VideoEffectType::Invert:    apply_invert(frame, effect.value * effect.mix); break;
        case VideoEffectType::Pixelate:  apply_pixelate(frame, effect.value); break;
        case VideoEffectType::Posterize: apply_posterize(frame, effect.value); break;
        case VideoEffectType::Grain:     apply_grain(frame, effect.value * effect.mix); break;
        default: break;
    }
}

void apply_clip_effects(GopostFrame* frame, const ClipEffectState& state) {
    if (!frame) return;
    if (!state.color_grading.is_default()) {
        apply_color_grading(frame, state.color_grading);
    }
    for (const auto& fx : state.effects) {
        apply_effect(frame, fx);
    }
}

}  // namespace video
}  // namespace gopost
