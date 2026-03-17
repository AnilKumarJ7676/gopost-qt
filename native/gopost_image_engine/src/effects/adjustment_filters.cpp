#include "gopost/effects.h"
#include "gopost/engine.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

/*
 * S6-03: Core adjustment filters — CPU processing path.
 *
 * All filters operate on RGBA8 pixel data in-place or src→dst.
 * GPU shader versions will be added in Sprint 7 for real-time preview.
 *
 * Supported adjustments:
 *   - Brightness (-100..+100)  → additive shift
 *   - Contrast (-100..+100)    → sigmoid curve around midpoint
 *   - Saturation (-100..+100)  → HSL saturation scale
 *   - Exposure (-2..+2 EV)     → multiplicative gain
 *   - Temperature (-100..+100) → warm/cool tint shift
 *   - Highlights (-100..+100)  → tone curve for bright regions
 *   - Shadows (-100..+100)     → tone curve for dark regions
 */

static inline uint8_t clamp8(float v) {
    int i = (int)(v + 0.5f);
    return (uint8_t)(i < 0 ? 0 : (i > 255 ? 255 : i));
}

static inline float srgb_to_linear(uint8_t v) {
    float s = v / 255.0f;
    return s <= 0.04045f ? s / 12.92f : powf((s + 0.055f) / 1.055f, 2.4f);
}

static inline uint8_t linear_to_srgb(float v) {
    float s = v <= 0.0031308f ? v * 12.92f : 1.055f * powf(v, 1.0f / 2.4f) - 0.055f;
    return clamp8(s * 255.0f);
}

static void rgb_to_hsl(float r, float g, float b,
                       float& h, float& s, float& l) {
    float mx = std::max({r, g, b});
    float mn = std::min({r, g, b});
    l = (mx + mn) * 0.5f;
    if (mx == mn) { h = s = 0; return; }
    float d = mx - mn;
    s = l > 0.5f ? d / (2.0f - mx - mn) : d / (mx + mn);
    if (mx == r)      h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    else if (mx == g) h = (b - r) / d + 2.0f;
    else              h = (r - g) / d + 4.0f;
    h /= 6.0f;
}

static float hue2rgb(float p, float q, float t) {
    if (t < 0) t += 1; if (t > 1) t -= 1;
    if (t < 1.0f/6) return p + (q - p) * 6 * t;
    if (t < 1.0f/2) return q;
    if (t < 2.0f/3) return p + (q - p) * (2.0f/3 - t) * 6;
    return p;
}

static void hsl_to_rgb(float h, float s, float l,
                       float& r, float& g, float& b) {
    if (s == 0) { r = g = b = l; return; }
    float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;
    r = hue2rgb(p, q, h + 1.0f/3);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1.0f/3);
}

static void apply_brightness(uint8_t* data, size_t pixel_count, float val) {
    float shift = val * 255.0f / 100.0f;
    for (size_t i = 0; i < pixel_count; i++) {
        size_t off = i * 4;
        data[off]     = clamp8(data[off]     + shift);
        data[off + 1] = clamp8(data[off + 1] + shift);
        data[off + 2] = clamp8(data[off + 2] + shift);
    }
}

static void apply_contrast(uint8_t* data, size_t pixel_count, float val) {
    float factor = (259.0f * (val + 255.0f)) / (255.0f * (259.0f - val));
    for (size_t i = 0; i < pixel_count; i++) {
        size_t off = i * 4;
        data[off]     = clamp8(factor * (data[off]     - 128) + 128);
        data[off + 1] = clamp8(factor * (data[off + 1] - 128) + 128);
        data[off + 2] = clamp8(factor * (data[off + 2] - 128) + 128);
    }
}

static void apply_saturation(uint8_t* data, size_t pixel_count, float val) {
    float scale = 1.0f + val / 100.0f;
    for (size_t i = 0; i < pixel_count; i++) {
        size_t off = i * 4;
        float r = data[off] / 255.0f;
        float g = data[off + 1] / 255.0f;
        float b = data[off + 2] / 255.0f;
        float h, s, l;
        rgb_to_hsl(r, g, b, h, s, l);
        s = std::clamp(s * scale, 0.0f, 1.0f);
        hsl_to_rgb(h, s, l, r, g, b);
        data[off]     = clamp8(r * 255.0f);
        data[off + 1] = clamp8(g * 255.0f);
        data[off + 2] = clamp8(b * 255.0f);
    }
}

static void apply_exposure(uint8_t* data, size_t pixel_count, float ev) {
    float gain = powf(2.0f, ev);
    for (size_t i = 0; i < pixel_count; i++) {
        size_t off = i * 4;
        float r = srgb_to_linear(data[off]);
        float g = srgb_to_linear(data[off + 1]);
        float b = srgb_to_linear(data[off + 2]);
        data[off]     = linear_to_srgb(r * gain);
        data[off + 1] = linear_to_srgb(g * gain);
        data[off + 2] = linear_to_srgb(b * gain);
    }
}

static void apply_temperature(uint8_t* data, size_t pixel_count, float val) {
    float t = val / 100.0f;
    float r_shift = t * 30.0f;
    float b_shift = -t * 30.0f;
    for (size_t i = 0; i < pixel_count; i++) {
        size_t off = i * 4;
        data[off]     = clamp8(data[off]     + r_shift);
        data[off + 2] = clamp8(data[off + 2] + b_shift);
    }
}

static void apply_highlights(uint8_t* data, size_t pixel_count, float val) {
    float amount = val / 100.0f;
    for (size_t i = 0; i < pixel_count; i++) {
        size_t off = i * 4;
        for (int c = 0; c < 3; c++) {
            float v = data[off + c] / 255.0f;
            if (v > 0.5f) {
                float highlight_mask = (v - 0.5f) * 2.0f;
                v += amount * highlight_mask * 0.3f;
            }
            data[off + c] = clamp8(std::clamp(v, 0.0f, 1.0f) * 255.0f);
        }
    }
}

static void apply_shadows(uint8_t* data, size_t pixel_count, float val) {
    float amount = val / 100.0f;
    for (size_t i = 0; i < pixel_count; i++) {
        size_t off = i * 4;
        for (int c = 0; c < 3; c++) {
            float v = data[off + c] / 255.0f;
            if (v < 0.5f) {
                float shadow_mask = 1.0f - v * 2.0f;
                v += amount * shadow_mask * 0.3f;
            }
            data[off + c] = clamp8(std::clamp(v, 0.0f, 1.0f) * 255.0f);
        }
    }
}

extern "C" {

GopostError gopost_effects_process(GopostEngine* engine,
                                   const GopostEffectInstance* instance,
                                   GopostFrame* in_frame,
                                   GopostFrame* out_frame) {
    (void)engine;
    if (!instance || !in_frame || !out_frame) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!in_frame->data || !out_frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (in_frame->width != out_frame->width || in_frame->height != out_frame->height)
        return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!instance->enabled) {
        memcpy(out_frame->data, in_frame->data, in_frame->data_size);
        return GOPOST_OK;
    }

    if (out_frame->data != in_frame->data) {
        memcpy(out_frame->data, in_frame->data, in_frame->data_size);
    }

    size_t pixel_count = (size_t)out_frame->width * out_frame->height;
    uint8_t* data = out_frame->data;
    float mix = std::clamp(instance->mix, 0.0f, 1.0f);

    uint8_t* orig = nullptr;
    if (mix < 1.0f) {
        orig = static_cast<uint8_t*>(malloc(out_frame->data_size));
        if (orig) memcpy(orig, data, out_frame->data_size);
    }

    const char* eid = instance->effect_id;

    if (strncmp(eid, "gp.adjust.brightness", 64) == 0) {
        apply_brightness(data, pixel_count, instance->param_values[0]);
    } else if (strncmp(eid, "gp.adjust.contrast", 64) == 0) {
        apply_contrast(data, pixel_count, instance->param_values[0]);
    } else if (strncmp(eid, "gp.adjust.saturation", 64) == 0) {
        apply_saturation(data, pixel_count, instance->param_values[0]);
    } else if (strncmp(eid, "gp.adjust.exposure", 64) == 0) {
        apply_exposure(data, pixel_count, instance->param_values[0]);
    } else if (strncmp(eid, "gp.adjust.temperature", 64) == 0) {
        apply_temperature(data, pixel_count, instance->param_values[0]);
    } else if (strncmp(eid, "gp.adjust.highlights", 64) == 0) {
        apply_highlights(data, pixel_count, instance->param_values[0]);
    } else if (strncmp(eid, "gp.adjust.shadows", 64) == 0) {
        apply_shadows(data, pixel_count, instance->param_values[0]);
    } else if (strncmp(eid, "gp.artistic.oil_paint", 64) == 0) {
        gopost_effect_oil_paint(out_frame, instance->param_values[0], instance->param_values[1]);
    } else if (strncmp(eid, "gp.artistic.watercolor", 64) == 0) {
        gopost_effect_watercolor(out_frame, instance->param_values[0], instance->param_values[1]);
    } else if (strncmp(eid, "gp.artistic.sketch", 64) == 0) {
        gopost_effect_sketch(out_frame, instance->param_values[0], instance->param_values[1]);
    } else if (strncmp(eid, "gp.artistic.pixelate", 64) == 0) {
        gopost_effect_pixelate(out_frame, (int)instance->param_values[0]);
    } else if (strncmp(eid, "gp.artistic.glitch", 64) == 0) {
        gopost_effect_glitch(out_frame, instance->param_values[0], instance->param_values[1]);
    } else if (strncmp(eid, "gp.artistic.halftone", 64) == 0) {
        gopost_effect_halftone(out_frame, instance->param_values[0],
                               instance->param_values[1], instance->param_values[2] / 100.0f);
    }

    if (orig && mix < 1.0f) {
        for (size_t i = 0; i < out_frame->data_size; i++) {
            data[i] = clamp8(orig[i] * (1.0f - mix) + data[i] * mix);
        }
        free(orig);
    }

    return GOPOST_OK;
}

} // extern "C"
