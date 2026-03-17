#include "transition_renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace gopost {
namespace video {

float apply_easing(float t, EasingCurve curve) {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (curve) {
        case EasingCurve::EaseIn:    return t * t;
        case EasingCurve::EaseOut:   return 1.0f - (1.0f - t) * (1.0f - t);
        case EasingCurve::EaseInOut: return t < 0.5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2.0f) / 2;
        case EasingCurve::CubicBezier: return t * t * (3.0f - 2.0f * t);
        default: return t;
    }
}

static inline uint8_t lerp8(uint8_t a, uint8_t b, float t) {
    return static_cast<uint8_t>(a + (b - a) * t);
}

static void crossfade(GopostFrame* out, const GopostFrame* from, const GopostFrame* to, float t) {
    const uint32_t total = out->width * out->height * 4;
    for (uint32_t i = 0; i < total; ++i) {
        uint8_t a = from ? from->data[i] : 0;
        uint8_t b = to ? to->data[i] : 0;
        out->data[i] = lerp8(a, b, t);
    }
}

static void wipe_horizontal(GopostFrame* out, const GopostFrame* from, const GopostFrame* to,
                             float t, bool left_to_right) {
    const uint32_t w = out->width, h = out->height;
    const uint32_t split = static_cast<uint32_t>(w * t);
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint32_t idx = (y * w + x) * 4;
            bool use_to = left_to_right ? (x < split) : (x >= w - split);
            const GopostFrame* src = use_to ? to : from;
            if (src) std::memcpy(out->data + idx, src->data + idx, 4);
            else std::memset(out->data + idx, 0, 4);
        }
    }
}

static void wipe_vertical(GopostFrame* out, const GopostFrame* from, const GopostFrame* to,
                            float t, bool top_to_bottom) {
    const uint32_t w = out->width, h = out->height;
    const uint32_t split = static_cast<uint32_t>(h * t);
    for (uint32_t y = 0; y < h; ++y) {
        bool use_to = top_to_bottom ? (y < split) : (y >= h - split);
        const GopostFrame* src = use_to ? to : from;
        uint32_t row = y * w * 4;
        if (src) std::memcpy(out->data + row, src->data + row, w * 4);
        else std::memset(out->data + row, 0, w * 4);
    }
}

static void slide(GopostFrame* out, const GopostFrame* from, const GopostFrame* to,
                  float t, int dx, int dy) {
    const int32_t w = out->width, h = out->height;
    const int32_t offX = static_cast<int32_t>(w * t * dx);
    const int32_t offY = static_cast<int32_t>(h * t * dy);
    std::memset(out->data, 0, w * h * 4);

    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            uint32_t dst = (y * w + x) * 4;
            // Draw 'from' shifted out
            int32_t fx = x - offX, fy = y - offY;
            if (from && fx >= 0 && fx < w && fy >= 0 && fy < h) {
                std::memcpy(out->data + dst, from->data + (fy * w + fx) * 4, 4);
            }
            // Draw 'to' sliding in
            int32_t tx = x - offX + w * dx, ty = y - offY + h * dy;
            if (to && tx >= 0 && tx < w && ty >= 0 && ty < h) {
                std::memcpy(out->data + dst, to->data + (ty * w + tx) * 4, 4);
            }
        }
    }
}

static void iris_transition(GopostFrame* out, const GopostFrame* from, const GopostFrame* to, float t) {
    const uint32_t w = out->width, h = out->height;
    const float cx = w * 0.5f, cy = h * 0.5f;
    const float maxR = std::sqrt(cx * cx + cy * cy);
    const float radius = maxR * t;
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint32_t idx = (y * w + x) * 4;
            float dx = x - cx, dy = y - cy;
            bool inside = std::sqrt(dx * dx + dy * dy) < radius;
            const GopostFrame* src = inside ? to : from;
            if (src) std::memcpy(out->data + idx, src->data + idx, 4);
            else std::memset(out->data + idx, 0, 4);
        }
    }
}

static void clock_wipe(GopostFrame* out, const GopostFrame* from, const GopostFrame* to, float t) {
    const uint32_t w = out->width, h = out->height;
    const float cx = w * 0.5f, cy = h * 0.5f;
    const float angle_threshold = t * 2.0f * 3.14159265f - 3.14159265f;
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint32_t idx = (y * w + x) * 4;
            float a = std::atan2(y - cy, x - cx);
            const GopostFrame* src = (a < angle_threshold) ? to : from;
            if (src) std::memcpy(out->data + idx, src->data + idx, 4);
            else std::memset(out->data + idx, 0, 4);
        }
    }
}

static void flash_transition(GopostFrame* out, const GopostFrame* from, const GopostFrame* to, float t) {
    const uint32_t total = out->width * out->height * 4;
    if (t < 0.5f) {
        float flash = t * 2.0f;
        if (from) std::memcpy(out->data, from->data, total);
        else std::memset(out->data, 0, total);
        for (uint32_t i = 0; i < total; i += 4) {
            for (int c = 0; c < 3; ++c)
                out->data[i + c] = static_cast<uint8_t>(std::min(255.0f, out->data[i + c] + flash * 255));
        }
    } else {
        float flash = (1.0f - t) * 2.0f;
        if (to) std::memcpy(out->data, to->data, total);
        else std::memset(out->data, 0, total);
        for (uint32_t i = 0; i < total; i += 4) {
            for (int c = 0; c < 3; ++c)
                out->data[i + c] = static_cast<uint8_t>(std::min(255.0f, out->data[i + c] + flash * 255));
        }
    }
}

void render_transition(GopostFrame* out, const GopostFrame* from, const GopostFrame* to,
                       float progress, TransitionType type) {
    if (!out || !out->data) return;
    float t = std::clamp(progress, 0.0f, 1.0f);

    switch (type) {
        case TransitionType::Fade:
        case TransitionType::Dissolve:
            crossfade(out, from, to, t);
            break;
        case TransitionType::WipeLeft:   wipe_horizontal(out, from, to, t, true); break;
        case TransitionType::WipeRight:  wipe_horizontal(out, from, to, t, false); break;
        case TransitionType::WipeUp:     wipe_vertical(out, from, to, t, true); break;
        case TransitionType::WipeDown:   wipe_vertical(out, from, to, t, false); break;
        case TransitionType::SlideLeft:  slide(out, from, to, t, -1, 0); break;
        case TransitionType::SlideRight: slide(out, from, to, t, 1, 0); break;
        case TransitionType::SlideUp:    slide(out, from, to, t, 0, -1); break;
        case TransitionType::SlideDown:  slide(out, from, to, t, 0, 1); break;
        case TransitionType::Push:       slide(out, from, to, t, -1, 0); break;
        case TransitionType::Zoom:
        case TransitionType::Reveal:
        case TransitionType::Morph:
        case TransitionType::Blur:
        case TransitionType::Spin:
        case TransitionType::Glitch:
            crossfade(out, from, to, t);
            break;
        case TransitionType::Iris:  iris_transition(out, from, to, t); break;
        case TransitionType::Clock: clock_wipe(out, from, to, t); break;
        case TransitionType::Flash: flash_transition(out, from, to, t); break;
        default:
            if (t < 0.5f && from) std::memcpy(out->data, from->data, out->width * out->height * 4);
            else if (to) std::memcpy(out->data, to->data, out->width * out->height * 4);
            break;
    }
}

}  // namespace video
}  // namespace gopost
