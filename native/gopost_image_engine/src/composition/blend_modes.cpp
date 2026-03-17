#include "gopost/canvas.h"

#include <cstdint>
#include <cmath>
#include <algorithm>

/*
 * Per-pixel blend modes. All operate on normalized [0,1] values internally.
 * Alpha compositing: result_a = src_a + dst_a*(1-src_a).
 * RGB blend uses src_a as mix factor: out_rgb = blended_rgb * src_a + dst_rgb * (1-src_a).
 */

static inline float to_f(uint8_t v) { return v / 255.0f; }
static inline uint8_t to_u8(float v) {
    v = std::clamp(v, 0.0f, 1.0f);
    return (uint8_t)(v * 255.0f + 0.5f);
}

/* RGB <-> HSL helpers */
static void rgb_to_hsl(float r, float g, float b, float& h, float& s, float& l) {
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

static void hsl_to_rgb(float h, float s, float l, float& r, float& g, float& b) {
    if (s == 0) { r = g = b = l; return; }
    float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;
    r = hue2rgb(p, q, h + 1.0f/3);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1.0f/3);
}

/* Color Dodge: b/(1-a), clamped to 1 */
static inline float color_dodge(float a, float b) {
    if (a >= 1.0f) return 1.0f;
    float r = b / (1.0f - a);
    return std::clamp(r, 0.0f, 1.0f);
}

/* Color Burn: 1-(1-b)/a, clamped to 0 */
static inline float color_burn(float a, float b) {
    if (a <= 0.0f) return 0.0f;
    float r = 1.0f - (1.0f - b) / a;
    return std::clamp(r, 0.0f, 1.0f);
}

/* Photoshop-style soft light */
static inline float soft_light(float a, float b) {
    if (a <= 0.5f)
        return 2.0f * a * b + a * a * (1.0f - 2.0f * b);
    return 2.0f * a * (1.0f - b) + std::sqrt(a) * (2.0f * b - 1.0f);
}

/* Simple hash for Dissolve (no coordinates available - use pixel values) */
static inline uint32_t pixel_hash(uint8_t sr, uint8_t sg, uint8_t sb, uint8_t sa,
                                  uint8_t dr, uint8_t dg, uint8_t db, uint8_t da) {
    return (sr * 7u + sg * 31u + sb * 131u + sa * 17u +
            dr * 67u + dg * 97u + db * 197u + da * 37u) & 0xFFu;
}

extern "C" {

void gopost_blend_pixel(GopostBlendMode mode,
                        uint8_t src_r, uint8_t src_g, uint8_t src_b, uint8_t src_a,
                        uint8_t dst_r, uint8_t dst_g, uint8_t dst_b, uint8_t dst_a,
                        uint8_t* out_r, uint8_t* out_g, uint8_t* out_b, uint8_t* out_a) {
    float sr = to_f(src_r), sg = to_f(src_g), sb = to_f(src_b), sa = to_f(src_a);
    float dr = to_f(dst_r), dg = to_f(dst_g), db = to_f(dst_b), da = to_f(dst_a);

    float mr, mg, mb;

    switch (mode) {
        case GOPOST_BLEND_NORMAL:
            mr = sr; mg = sg; mb = sb;
            break;
        case GOPOST_BLEND_MULTIPLY:
            mr = sr * dr; mg = sg * dg; mb = sb * db;
            break;
        case GOPOST_BLEND_SCREEN:
            mr = 1.0f - (1.0f - sr) * (1.0f - dr);
            mg = 1.0f - (1.0f - sg) * (1.0f - dg);
            mb = 1.0f - (1.0f - sb) * (1.0f - db);
            break;
        case GOPOST_BLEND_OVERLAY:
            mr = dr < 0.5f ? 2.0f * sr * dr : 1.0f - 2.0f * (1.0f - sr) * (1.0f - dr);
            mg = dg < 0.5f ? 2.0f * sg * dg : 1.0f - 2.0f * (1.0f - sg) * (1.0f - dg);
            mb = db < 0.5f ? 2.0f * sb * db : 1.0f - 2.0f * (1.0f - sb) * (1.0f - db);
            break;
        case GOPOST_BLEND_DARKEN:
            mr = std::min(sr, dr); mg = std::min(sg, dg); mb = std::min(sb, db);
            break;
        case GOPOST_BLEND_LIGHTEN:
            mr = std::max(sr, dr); mg = std::max(sg, dg); mb = std::max(sb, db);
            break;
        case GOPOST_BLEND_COLOR_DODGE:
            mr = color_dodge(sr, dr); mg = color_dodge(sg, dg); mb = color_dodge(sb, db);
            break;
        case GOPOST_BLEND_COLOR_BURN:
            mr = color_burn(sr, dr); mg = color_burn(sg, dg); mb = color_burn(sb, db);
            break;
        case GOPOST_BLEND_HARD_LIGHT:
            mr = sr < 0.5f ? 2.0f * sr * dr : 1.0f - 2.0f * (1.0f - sr) * (1.0f - dr);
            mg = sg < 0.5f ? 2.0f * sg * dg : 1.0f - 2.0f * (1.0f - sg) * (1.0f - dg);
            mb = sb < 0.5f ? 2.0f * sb * db : 1.0f - 2.0f * (1.0f - sb) * (1.0f - db);
            break;
        case GOPOST_BLEND_SOFT_LIGHT:
            mr = soft_light(sr, dr); mg = soft_light(sg, dg); mb = soft_light(sb, db);
            break;
        case GOPOST_BLEND_DIFFERENCE:
            mr = std::fabs(sr - dr); mg = std::fabs(sg - dg); mb = std::fabs(sb - db);
            break;
        case GOPOST_BLEND_EXCLUSION:
            mr = sr + dr - 2.0f * sr * dr;
            mg = sg + dg - 2.0f * sg * dg;
            mb = sb + db - 2.0f * sb * db;
            break;
        case GOPOST_BLEND_LINEAR_BURN:
            mr = std::clamp(sr + dr - 1.0f, 0.0f, 1.0f);
            mg = std::clamp(sg + dg - 1.0f, 0.0f, 1.0f);
            mb = std::clamp(sb + db - 1.0f, 0.0f, 1.0f);
            break;
        case GOPOST_BLEND_LINEAR_DODGE:
            mr = std::clamp(sr + dr, 0.0f, 1.0f);
            mg = std::clamp(sg + dg, 0.0f, 1.0f);
            mb = std::clamp(sb + db, 0.0f, 1.0f);
            break;
        case GOPOST_BLEND_VIVID_LIGHT:
            mr = sr < 0.5f ? color_burn(2.0f * sr, dr) : color_dodge(2.0f * sr - 1.0f, dr);
            mg = sg < 0.5f ? color_burn(2.0f * sg, dg) : color_dodge(2.0f * sg - 1.0f, dg);
            mb = sb < 0.5f ? color_burn(2.0f * sb, db) : color_dodge(2.0f * sb - 1.0f, db);
            break;
        case GOPOST_BLEND_LINEAR_LIGHT:
            mr = std::clamp(dr + 2.0f * sr - 1.0f, 0.0f, 1.0f);
            mg = std::clamp(dg + 2.0f * sg - 1.0f, 0.0f, 1.0f);
            mb = std::clamp(db + 2.0f * sb - 1.0f, 0.0f, 1.0f);
            break;
        case GOPOST_BLEND_PIN_LIGHT:
            mr = sr < 0.5f ? std::min(dr, 2.0f * sr) : std::max(dr, 2.0f * sr - 1.0f);
            mg = sg < 0.5f ? std::min(dg, 2.0f * sg) : std::max(dg, 2.0f * sg - 1.0f);
            mb = sb < 0.5f ? std::min(db, 2.0f * sb) : std::max(db, 2.0f * sb - 1.0f);
            break;
        case GOPOST_BLEND_HARD_MIX:
            mr = (sr + dr >= 1.0f) ? 1.0f : 0.0f;
            mg = (sg + dg >= 1.0f) ? 1.0f : 0.0f;
            mb = (sb + db >= 1.0f) ? 1.0f : 0.0f;
            break;
        case GOPOST_BLEND_DISSOLVE: {
            uint32_t h = pixel_hash(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a);
            float thresh = sa * 255.0f;
            if ((float)h < thresh) {
                mr = sr; mg = sg; mb = sb;
            } else {
                mr = dr; mg = dg; mb = db;
            }
            /* Dissolve: use picked pixel directly, no blend mix */
            float out_a_dissolve = sa + da * (1.0f - sa);
            *out_r = to_u8(mr);
            *out_g = to_u8(mg);
            *out_b = to_u8(mb);
            *out_a = to_u8(out_a_dissolve);
            return;
        }
        case GOPOST_BLEND_DIVIDE:
            mr = (sr < 1e-6f) ? 1.0f : std::clamp(dr / sr, 0.0f, 1.0f);
            mg = (sg < 1e-6f) ? 1.0f : std::clamp(dg / sg, 0.0f, 1.0f);
            mb = (sb < 1e-6f) ? 1.0f : std::clamp(db / sb, 0.0f, 1.0f);
            break;
        case GOPOST_BLEND_SUBTRACT:
            mr = std::clamp(dr - sr, 0.0f, 1.0f);
            mg = std::clamp(dg - sg, 0.0f, 1.0f);
            mb = std::clamp(db - sb, 0.0f, 1.0f);
            break;
        case GOPOST_BLEND_GRAIN_EXTRACT:
            mr = std::clamp(dr - sr + 0.5f, 0.0f, 1.0f);
            mg = std::clamp(dg - sg + 0.5f, 0.0f, 1.0f);
            mb = std::clamp(db - sb + 0.5f, 0.0f, 1.0f);
            break;
        case GOPOST_BLEND_GRAIN_MERGE:
            mr = std::clamp(sr + dr - 0.5f, 0.0f, 1.0f);
            mg = std::clamp(sg + dg - 0.5f, 0.0f, 1.0f);
            mb = std::clamp(sb + db - 0.5f, 0.0f, 1.0f);
            break;
        case GOPOST_BLEND_HUE:
        case GOPOST_BLEND_SATURATION:
        case GOPOST_BLEND_COLOR:
        case GOPOST_BLEND_LUMINOSITY: {
            float sh, ss, sl, dh, ds, dl;
            rgb_to_hsl(sr, sg, sb, sh, ss, sl);
            rgb_to_hsl(dr, dg, db, dh, ds, dl);
            float oh = dh, os = ds, ol = dl;
            if (mode == GOPOST_BLEND_HUE)        { oh = sh; }
            else if (mode == GOPOST_BLEND_SATURATION) { os = ss; }
            else if (mode == GOPOST_BLEND_COLOR)      { oh = sh; os = ss; }
            else if (mode == GOPOST_BLEND_LUMINOSITY) { ol = sl; }
            hsl_to_rgb(oh, os, ol, mr, mg, mb);
            break;
        }
        default:
            mr = sr; mg = sg; mb = sb;
            break;
    }

    /* Alpha compositing: result_a = src_a + dst_a*(1-src_a) */
    float result_a = sa + da * (1.0f - sa);

    /* Blend RGB using src_a as mix: out_rgb = blended * src_a + dst * (1-src_a) */
    float or_ = mr * sa + dr * (1.0f - sa);
    float og_ = mg * sa + dg * (1.0f - sa);
    float ob_ = mb * sa + db * (1.0f - sa);

    *out_r = to_u8(or_);
    *out_g = to_u8(og_);
    *out_b = to_u8(ob_);
    *out_a = to_u8(result_a);
}

} /* extern "C" */
