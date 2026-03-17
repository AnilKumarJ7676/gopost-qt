#include "video_compositor.hpp"
#include "gopost/error.h"
#include "gopost/types.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

inline uint8_t blend_normal(uint8_t base, uint8_t over, float opacity) {
    float a = (over / 255.f) * opacity;
    return static_cast<uint8_t>(std::min(255.f, base * (1.f - a) + over * a));
}

inline float srgb_to_linear(uint8_t c) {
    float x = c / 255.f;
    return x <= 0.04045f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f);
}
inline uint8_t linear_to_srgb(float c) {
    if (c <= 0.f) return 0;
    if (c >= 1.f) return 255;
    return c <= 0.0031308f
        ? static_cast<uint8_t>(c * 12.92f * 255.f + 0.5f)
        : static_cast<uint8_t>((1.055f * std::pow(c, 1.f/2.4f) - 0.055f) * 255.f + 0.5f);
}

inline uint8_t blend_multiply(uint8_t b, uint8_t o, float opacity) {
    float fb = b / 255.f, fo = o / 255.f;
    float r = fb * (1.f - fo * opacity) + fb * fo * opacity;
    return linear_to_srgb(r);
}

inline uint8_t blend_screen(uint8_t b, uint8_t o, float opacity) {
    float fb = b / 255.f, fo = o / 255.f;
    float r = fb + (1.f - fb) * fo * opacity;
    return linear_to_srgb(std::min(1.f, r));
}

inline uint8_t blend_overlay(uint8_t b, uint8_t o, float opacity) {
    float fb = b / 255.f, fo = o / 255.f;
    float r = fb < 0.5f
        ? 2.f * fb * fo * opacity + fb * (1.f - fo * opacity)
        : 1.f - 2.f * (1.f - fb) * (1.f - fo) * opacity - (1.f - fb) * (1.f - fo * opacity);
    return linear_to_srgb(std::min(1.f, std::max(0.f, r)));
}

inline uint8_t blend_add(uint8_t b, uint8_t o, float opacity) {
    int r = b + static_cast<int>(o * opacity + 0.5f);
    return static_cast<uint8_t>(std::min(255, r));
}

}  // namespace

extern "C" {

int gopost_video_composite_blend(
    GopostFrame* base,
    const GopostFrame* overlay,
    float overlay_opacity,
    GopostVideoBlendMode mode) {
    if (!base || !overlay || !base->data || !overlay->data) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (base->format != GOPOST_PIXEL_FORMAT_RGBA8 && base->format != GOPOST_PIXEL_FORMAT_BGRA8) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (overlay->format != GOPOST_PIXEL_FORMAT_RGBA8 && overlay->format != GOPOST_PIXEL_FORMAT_BGRA8) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (base->width != overlay->width || base->height != overlay->height) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    const size_t n = (size_t)base->width * base->height * 4;
    uint8_t* dst = base->data;
    const uint8_t* src = overlay->data;
    overlay_opacity = std::max(0.f, std::min(1.f, overlay_opacity));

    for (size_t i = 0; i < n; i += 4) {
        uint8_t ba = dst[i + 3], br = dst[i], bg = dst[i + 1], bb = dst[i + 2];
        uint8_t oa = src[i + 3], or_ = src[i], og = src[i + 1], ob = src[i + 2];
        float o_alpha = (oa / 255.f) * overlay_opacity;
        if (o_alpha < 1.f / 255.f) continue;
        uint8_t nr, ng, nb, na;
        switch (mode) {
            case GOPOST_VIDEO_BLEND_NORMAL:
                nr = blend_normal(br, or_, o_alpha);
                ng = blend_normal(bg, og, o_alpha);
                nb = blend_normal(bb, ob, o_alpha);
                break;
            case GOPOST_VIDEO_BLEND_MULTIPLY:
                nr = blend_multiply(br, or_, o_alpha);
                ng = blend_multiply(bg, og, o_alpha);
                nb = blend_multiply(bb, ob, o_alpha);
                break;
            case GOPOST_VIDEO_BLEND_SCREEN:
                nr = blend_screen(br, or_, o_alpha);
                ng = blend_screen(bg, og, o_alpha);
                nb = blend_screen(bb, ob, o_alpha);
                break;
            case GOPOST_VIDEO_BLEND_OVERLAY:
                nr = blend_overlay(br, or_, o_alpha);
                ng = blend_overlay(bg, og, o_alpha);
                nb = blend_overlay(bb, ob, o_alpha);
                break;
            case GOPOST_VIDEO_BLEND_ADD:
                nr = blend_add(br, or_, o_alpha);
                ng = blend_add(bg, og, o_alpha);
                nb = blend_add(bb, ob, o_alpha);
                break;
            default:
                nr = blend_normal(br, or_, o_alpha);
                ng = blend_normal(bg, og, o_alpha);
                nb = blend_normal(bb, ob, o_alpha);
                break;
        }
        na = static_cast<uint8_t>(std::min(255, (int)(ba * (1.f - o_alpha) + 255 * o_alpha + 0.5f)));
        dst[i] = nr;
        dst[i + 1] = ng;
        dst[i + 2] = nb;
        dst[i + 3] = na;
    }
    return GOPOST_OK;
}

int gopost_video_frame_copy(GopostFrame* dest, const GopostFrame* source, int clear_dest_first) {
    if (!dest || !source || !dest->data || !source->data) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (dest->width != source->width || dest->height != source->height ||
        dest->format != source->format) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (clear_dest_first) {
        std::memset(dest->data, 0, dest->data_size);
    }
    size_t copy_size = std::min(dest->data_size, source->data_size);
    std::memcpy(dest->data, source->data, copy_size);
    return GOPOST_OK;
}

int gopost_video_frame_clear(GopostFrame* frame) {
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::memset(frame->data, 0, frame->data_size);
    return GOPOST_OK;
}

}  // extern "C"
