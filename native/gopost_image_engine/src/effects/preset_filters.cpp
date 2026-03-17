#include "gopost/effects.h"
#include "gopost/engine.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

/*
 * S6-04: 40 preset filters.
 *
 * Each preset is a combination of:
 *   - RGB channel curves (tone curves stored as 256-byte LUTs)
 *   - Saturation modifier
 *   - Tint shift (R/B)
 *   - Vignette amount
 *
 * The curves are computed procedurally at init time from a compact
 * parameter description, avoiding large embedded LUT data.
 */

struct PresetCurve {
    uint8_t r[256];
    uint8_t g[256];
    uint8_t b[256];
    float saturation;       /* 0 = no change, -1 = grayscale, +1 = 2x */
    float vignette;         /* 0 = none, 1 = strong */
    float tint_r, tint_b;   /* additive tint */
};

static constexpr int PRESET_COUNT = 40;
static PresetCurve g_presets[PRESET_COUNT];
static bool g_presets_initialized = false;

static inline uint8_t clamp8(float v) {
    int i = (int)(v + 0.5f);
    return (uint8_t)(i < 0 ? 0 : (i > 255 ? 255 : i));
}

/* Build a tone curve from black/white/gamma parameters */
static void build_curve(uint8_t* lut,
                        float black, float white, float gamma,
                        float lift, float gain) {
    for (int i = 0; i < 256; i++) {
        float t = i / 255.0f;
        t = std::clamp((t - black) / (white - black), 0.0f, 1.0f);
        t = powf(t, 1.0f / gamma);
        t = lift + t * (gain - lift);
        lut[i] = clamp8(t * 255.0f);
    }
}

static void build_linear(uint8_t* lut) {
    for (int i = 0; i < 256; i++) lut[i] = (uint8_t)i;
}

static void init_presets() {
    if (g_presets_initialized) return;

    for (int i = 0; i < PRESET_COUNT; i++) {
        build_linear(g_presets[i].r);
        build_linear(g_presets[i].g);
        build_linear(g_presets[i].b);
        g_presets[i].saturation = 0;
        g_presets[i].vignette = 0;
        g_presets[i].tint_r = 0;
        g_presets[i].tint_b = 0;
    }

    /* Natural (0-7) */
    // 0: Daylight — neutral warmth
    build_curve(g_presets[0].r, 0, 1, 1.0f, 0.02f, 1.0f);
    build_curve(g_presets[0].b, 0, 1, 1.0f, 0.0f, 0.95f);
    g_presets[0].saturation = 0.05f;

    // 1: Golden Hour — warm orange push
    build_curve(g_presets[1].r, 0, 1, 0.9f, 0.03f, 1.0f);
    build_curve(g_presets[1].g, 0, 1, 0.95f, 0.01f, 0.97f);
    build_curve(g_presets[1].b, 0, 1, 1.1f, 0.0f, 0.85f);
    g_presets[1].saturation = 0.15f;

    // 2: Overcast — slight cool desaturation
    g_presets[2].saturation = -0.15f;
    g_presets[2].tint_b = 5;

    // 3: Shade — cool blue lift
    build_curve(g_presets[3].b, 0, 1, 0.9f, 0.05f, 1.0f);
    g_presets[3].saturation = -0.05f;

    // 4: Fluorescent — green tint, slight desat
    build_curve(g_presets[4].g, 0, 1, 0.95f, 0.02f, 1.0f);
    g_presets[4].saturation = -0.1f;

    // 5: Tungsten — warm yellow, crushed blues
    build_curve(g_presets[5].r, 0, 1, 0.9f, 0.03f, 1.0f);
    build_curve(g_presets[5].b, 0, 1, 1.2f, 0.0f, 0.8f);
    g_presets[5].saturation = 0.05f;

    // 6: Flash — bright, slight contrast
    build_curve(g_presets[6].r, 0.02f, 0.98f, 1.1f, 0.0f, 1.0f);
    build_curve(g_presets[6].g, 0.02f, 0.98f, 1.1f, 0.0f, 1.0f);
    build_curve(g_presets[6].b, 0.02f, 0.98f, 1.1f, 0.0f, 1.0f);

    // 7: Cloudy — soft warmth
    build_curve(g_presets[7].r, 0, 1, 0.95f, 0.01f, 1.0f);
    build_curve(g_presets[7].b, 0, 1, 1.05f, 0.0f, 0.97f);

    /* Portrait (8-15) */
    // 8: Soft Skin — low contrast, warm, slight desat
    build_curve(g_presets[8].r, 0.02f, 0.96f, 0.9f, 0.03f, 0.98f);
    build_curve(g_presets[8].g, 0.02f, 0.96f, 0.9f, 0.02f, 0.97f);
    build_curve(g_presets[8].b, 0.02f, 0.96f, 0.95f, 0.01f, 0.94f);
    g_presets[8].saturation = -0.1f;

    // 9: Studio — neutral, punchy
    build_curve(g_presets[9].r, 0.03f, 0.97f, 1.05f, 0.0f, 1.0f);
    build_curve(g_presets[9].g, 0.03f, 0.97f, 1.05f, 0.0f, 1.0f);
    build_curve(g_presets[9].b, 0.03f, 0.97f, 1.05f, 0.0f, 1.0f);
    g_presets[9].saturation = 0.05f;

    // 10: Warm Glow
    build_curve(g_presets[10].r, 0, 1, 0.85f, 0.04f, 1.0f);
    build_curve(g_presets[10].b, 0, 1, 1.1f, 0.0f, 0.9f);
    g_presets[10].saturation = 0.1f;
    g_presets[10].vignette = 0.2f;

    // 11: High Key — bright, airy
    build_curve(g_presets[11].r, 0, 0.85f, 0.85f, 0.1f, 1.0f);
    build_curve(g_presets[11].g, 0, 0.85f, 0.85f, 0.1f, 1.0f);
    build_curve(g_presets[11].b, 0, 0.85f, 0.85f, 0.1f, 1.0f);
    g_presets[11].saturation = -0.15f;

    // 12: Low Key — dark, moody
    build_curve(g_presets[12].r, 0.05f, 1, 1.2f, 0.0f, 0.9f);
    build_curve(g_presets[12].g, 0.05f, 1, 1.2f, 0.0f, 0.9f);
    build_curve(g_presets[12].b, 0.05f, 1, 1.2f, 0.0f, 0.9f);
    g_presets[12].vignette = 0.4f;

    // 13: Beauty — soft, pink tint
    build_curve(g_presets[13].r, 0, 1, 0.9f, 0.02f, 1.0f);
    g_presets[13].saturation = -0.05f;
    g_presets[13].tint_r = 8;

    // 14: Magazine — punchy, saturated
    build_curve(g_presets[14].r, 0.04f, 0.96f, 1.1f, 0.0f, 1.0f);
    build_curve(g_presets[14].g, 0.04f, 0.96f, 1.1f, 0.0f, 1.0f);
    build_curve(g_presets[14].b, 0.04f, 0.96f, 1.1f, 0.0f, 1.0f);
    g_presets[14].saturation = 0.2f;

    // 15: Headshot
    build_curve(g_presets[15].r, 0.01f, 0.98f, 0.95f, 0.02f, 0.99f);
    build_curve(g_presets[15].g, 0.01f, 0.98f, 0.95f, 0.01f, 0.98f);
    build_curve(g_presets[15].b, 0.01f, 0.98f, 1.0f, 0.0f, 0.95f);

    /* Vintage (16-23) */
    // 16: Polaroid — warm, lifted blacks, faded
    build_curve(g_presets[16].r, 0, 1, 0.85f, 0.06f, 0.95f);
    build_curve(g_presets[16].g, 0, 1, 0.9f, 0.04f, 0.93f);
    build_curve(g_presets[16].b, 0, 1, 1.0f, 0.08f, 0.88f);
    g_presets[16].saturation = -0.1f;
    g_presets[16].vignette = 0.3f;

    // 17: Kodachrome — rich reds, deep shadows
    build_curve(g_presets[17].r, 0.02f, 0.95f, 0.9f, 0.0f, 1.0f);
    build_curve(g_presets[17].g, 0.02f, 0.98f, 1.0f, 0.0f, 0.95f);
    build_curve(g_presets[17].b, 0.05f, 0.95f, 1.1f, 0.0f, 0.85f);
    g_presets[17].saturation = 0.2f;

    // 18: Fujifilm — cool greens, muted
    build_curve(g_presets[18].g, 0, 1, 0.95f, 0.02f, 1.0f);
    build_curve(g_presets[18].b, 0, 1, 0.95f, 0.02f, 0.97f);
    g_presets[18].saturation = -0.05f;

    // 19: Agfa Vista — warm, boosted saturation
    build_curve(g_presets[19].r, 0, 1, 0.9f, 0.02f, 1.0f);
    build_curve(g_presets[19].b, 0, 1, 1.1f, 0.0f, 0.9f);
    g_presets[19].saturation = 0.25f;

    // 20: Lomo — high contrast, saturated, heavy vignette
    build_curve(g_presets[20].r, 0.05f, 0.92f, 1.15f, 0.0f, 1.0f);
    build_curve(g_presets[20].g, 0.05f, 0.92f, 1.15f, 0.0f, 1.0f);
    build_curve(g_presets[20].b, 0.05f, 0.92f, 1.15f, 0.0f, 1.0f);
    g_presets[20].saturation = 0.3f;
    g_presets[20].vignette = 0.5f;

    // 21: Sepia
    g_presets[21].saturation = -1.0f;
    g_presets[21].tint_r = 20;
    g_presets[21].tint_b = -15;

    // 22: Faded — lifted blacks, lowered whites
    build_curve(g_presets[22].r, 0, 1, 1.0f, 0.08f, 0.92f);
    build_curve(g_presets[22].g, 0, 1, 1.0f, 0.08f, 0.92f);
    build_curve(g_presets[22].b, 0, 1, 1.0f, 0.08f, 0.92f);
    g_presets[22].saturation = -0.2f;

    // 23: Cross Process — swapped channels, vivid
    build_curve(g_presets[23].r, 0, 1, 0.8f, 0.0f, 1.0f);
    build_curve(g_presets[23].g, 0, 1, 1.2f, 0.05f, 0.9f);
    build_curve(g_presets[23].b, 0, 1, 0.9f, 0.0f, 1.0f);
    g_presets[23].saturation = 0.15f;

    /* Cinematic (24-31) */
    // 24: Teal & Orange
    build_curve(g_presets[24].r, 0, 1, 0.9f, 0.0f, 1.0f);
    build_curve(g_presets[24].g, 0, 1, 1.0f, 0.0f, 0.92f);
    build_curve(g_presets[24].b, 0, 1, 0.85f, 0.08f, 0.9f);
    g_presets[24].saturation = 0.1f;

    // 25: Noir — high contrast B&W warm tint
    g_presets[25].saturation = -1.0f;
    build_curve(g_presets[25].r, 0.05f, 0.95f, 1.3f, 0.0f, 1.0f);
    build_curve(g_presets[25].g, 0.05f, 0.95f, 1.3f, 0.0f, 1.0f);
    build_curve(g_presets[25].b, 0.05f, 0.95f, 1.3f, 0.0f, 1.0f);
    g_presets[25].tint_r = 5;
    g_presets[25].vignette = 0.3f;

    // 26: Blockbuster
    build_curve(g_presets[26].r, 0.03f, 0.97f, 1.1f, 0.0f, 1.0f);
    build_curve(g_presets[26].b, 0.03f, 0.97f, 1.0f, 0.05f, 0.92f);
    g_presets[26].saturation = -0.1f;

    // 27: Muted
    build_curve(g_presets[27].r, 0, 1, 1.0f, 0.05f, 0.9f);
    build_curve(g_presets[27].g, 0, 1, 1.0f, 0.05f, 0.9f);
    build_curve(g_presets[27].b, 0, 1, 1.0f, 0.05f, 0.9f);
    g_presets[27].saturation = -0.25f;

    // 28: Desaturated
    g_presets[28].saturation = -0.5f;

    // 29: Bleach Bypass — low sat, high contrast
    build_curve(g_presets[29].r, 0.04f, 0.96f, 1.2f, 0.0f, 1.0f);
    build_curve(g_presets[29].g, 0.04f, 0.96f, 1.2f, 0.0f, 1.0f);
    build_curve(g_presets[29].b, 0.04f, 0.96f, 1.2f, 0.0f, 1.0f);
    g_presets[29].saturation = -0.4f;

    // 30: Day for Night — dark blue push
    build_curve(g_presets[30].r, 0.05f, 1, 1.4f, 0.0f, 0.6f);
    build_curve(g_presets[30].g, 0.05f, 1, 1.4f, 0.0f, 0.65f);
    build_curve(g_presets[30].b, 0, 1, 1.1f, 0.05f, 0.8f);
    g_presets[30].saturation = -0.3f;
    g_presets[30].vignette = 0.5f;

    // 31: Indie — warm, faded, lifted
    build_curve(g_presets[31].r, 0, 1, 0.9f, 0.06f, 0.96f);
    build_curve(g_presets[31].g, 0, 1, 0.95f, 0.04f, 0.94f);
    build_curve(g_presets[31].b, 0, 1, 1.05f, 0.06f, 0.9f);
    g_presets[31].saturation = -0.15f;

    /* B&W (32-39) */
    // 32: B&W Classic
    g_presets[32].saturation = -1.0f;

    // 33: Selenium — B&W + slight warm tint
    g_presets[33].saturation = -1.0f;
    g_presets[33].tint_r = 10;
    g_presets[33].tint_b = -5;

    // 34: Cyanotype — B&W + blue tint
    g_presets[34].saturation = -1.0f;
    g_presets[34].tint_b = 25;
    g_presets[34].tint_r = -5;

    // 35: Infrared — B&W + channel swap feel
    g_presets[35].saturation = -1.0f;
    build_curve(g_presets[35].r, 0, 1, 0.8f, 0.1f, 1.0f);
    build_curve(g_presets[35].g, 0, 1, 1.3f, 0.0f, 0.9f);

    // 36: High Contrast B&W
    g_presets[36].saturation = -1.0f;
    build_curve(g_presets[36].r, 0.08f, 0.92f, 1.3f, 0.0f, 1.0f);
    build_curve(g_presets[36].g, 0.08f, 0.92f, 1.3f, 0.0f, 1.0f);
    build_curve(g_presets[36].b, 0.08f, 0.92f, 1.3f, 0.0f, 1.0f);

    // 37: Silver
    g_presets[37].saturation = -1.0f;
    build_curve(g_presets[37].r, 0, 1, 1.05f, 0.03f, 0.97f);
    build_curve(g_presets[37].g, 0, 1, 1.05f, 0.03f, 0.97f);
    build_curve(g_presets[37].b, 0, 1, 1.05f, 0.03f, 0.97f);

    // 38: Film Grain B&W (grain applied in Flutter via noise overlay)
    g_presets[38].saturation = -1.0f;
    build_curve(g_presets[38].r, 0.02f, 0.98f, 1.1f, 0.0f, 1.0f);
    build_curve(g_presets[38].g, 0.02f, 0.98f, 1.1f, 0.0f, 1.0f);
    build_curve(g_presets[38].b, 0.02f, 0.98f, 1.1f, 0.0f, 1.0f);

    // 39: B&W Noir — deep blacks, vignette
    g_presets[39].saturation = -1.0f;
    build_curve(g_presets[39].r, 0.06f, 0.94f, 1.4f, 0.0f, 1.0f);
    build_curve(g_presets[39].g, 0.06f, 0.94f, 1.4f, 0.0f, 1.0f);
    build_curve(g_presets[39].b, 0.06f, 0.94f, 1.4f, 0.0f, 1.0f);
    g_presets[39].vignette = 0.45f;

    g_presets_initialized = true;
}

static void apply_saturation_shift(uint8_t* data, size_t px, float sat) {
    float scale = 1.0f + sat;
    for (size_t i = 0; i < px; i++) {
        size_t off = i * 4;
        float r = data[off] / 255.0f, g = data[off+1] / 255.0f, b = data[off+2] / 255.0f;
        float gray = 0.2126f * r + 0.7152f * g + 0.0722f * b;
        r = gray + (r - gray) * scale;
        g = gray + (g - gray) * scale;
        b = gray + (b - gray) * scale;
        data[off]   = clamp8(std::clamp(r, 0.0f, 1.0f) * 255.0f);
        data[off+1] = clamp8(std::clamp(g, 0.0f, 1.0f) * 255.0f);
        data[off+2] = clamp8(std::clamp(b, 0.0f, 1.0f) * 255.0f);
    }
}

static void apply_vignette(uint8_t* data, uint32_t w, uint32_t h, float amount) {
    float cx = w * 0.5f, cy = h * 0.5f;
    float max_dist = sqrtf(cx * cx + cy * cy);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            float dx = x - cx, dy = y - cy;
            float dist = sqrtf(dx * dx + dy * dy) / max_dist;
            float darken = 1.0f - amount * dist * dist;
            size_t off = ((size_t)y * w + x) * 4;
            data[off]   = clamp8(data[off]   * darken);
            data[off+1] = clamp8(data[off+1] * darken);
            data[off+2] = clamp8(data[off+2] * darken);
        }
    }
}

extern "C" {

GopostError gopost_preset_get_count(GopostEngine* engine, int32_t* out_count) {
    (void)engine;
    if (!out_count) return GOPOST_ERROR_INVALID_ARGUMENT;
    init_presets();
    *out_count = PRESET_COUNT;
    return GOPOST_OK;
}

GopostError gopost_preset_get_info(GopostEngine* engine, int32_t index,
                                   char* out_name, int32_t name_len,
                                   char* out_category, int32_t cat_len) {
    (void)engine;
    if (index < 0 || index >= PRESET_COUNT) return GOPOST_ERROR_INVALID_ARGUMENT;
    init_presets();

    static const char* names[PRESET_COUNT] = {
        "Daylight","Golden Hour","Overcast","Shade","Fluorescent","Tungsten","Flash","Cloudy",
        "Soft Skin","Studio","Warm Glow","High Key","Low Key","Beauty","Magazine","Headshot",
        "Polaroid","Kodachrome","Fujifilm","Agfa Vista","Lomo","Sepia","Faded","Cross Process",
        "Teal & Orange","Noir","Blockbuster","Muted","Desaturated","Bleach Bypass","Day for Night","Indie",
        "B&W Classic","Selenium","Cyanotype","Infrared","High Contrast","Silver","Film Grain","B&W Noir",
    };
    static const char* cats[5] = {"Natural","Portrait","Vintage","Cinematic","B&W"};

    if (out_name && name_len > 0)
        strncpy(out_name, names[index], (size_t)name_len - 1);
    if (out_category && cat_len > 0)
        strncpy(out_category, cats[index / 8], (size_t)cat_len - 1);

    return GOPOST_OK;
}

GopostError gopost_preset_apply(GopostEngine* engine,
                                GopostFrame* frame,
                                int32_t preset_index, float intensity) {
    (void)engine;
    if (!frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (preset_index < 0 || preset_index >= PRESET_COUNT)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    init_presets();
    intensity = std::clamp(intensity, 0.0f, 100.0f) / 100.0f;
    if (intensity < 0.001f) return GOPOST_OK;

    const PresetCurve& p = g_presets[preset_index];
    size_t px = (size_t)frame->width * frame->height;

    /* Apply LUT curves */
    for (size_t i = 0; i < px; i++) {
        size_t off = i * 4;
        uint8_t orig_r = frame->data[off];
        uint8_t orig_g = frame->data[off + 1];
        uint8_t orig_b = frame->data[off + 2];

        uint8_t new_r = p.r[orig_r];
        uint8_t new_g = p.g[orig_g];
        uint8_t new_b = p.b[orig_b];

        frame->data[off]     = clamp8(orig_r + (new_r - orig_r) * intensity);
        frame->data[off + 1] = clamp8(orig_g + (new_g - orig_g) * intensity);
        frame->data[off + 2] = clamp8(orig_b + (new_b - orig_b) * intensity);
    }

    /* Apply tint */
    if (p.tint_r != 0 || p.tint_b != 0) {
        float tr = p.tint_r * intensity;
        float tb = p.tint_b * intensity;
        for (size_t i = 0; i < px; i++) {
            size_t off = i * 4;
            frame->data[off]     = clamp8(frame->data[off]     + tr);
            frame->data[off + 2] = clamp8(frame->data[off + 2] + tb);
        }
    }

    /* Apply saturation */
    if (p.saturation != 0) {
        apply_saturation_shift(frame->data, px, p.saturation * intensity);
    }

    /* Apply vignette */
    if (p.vignette > 0) {
        apply_vignette(frame->data, frame->width, frame->height,
                       p.vignette * intensity);
    }

    return GOPOST_OK;
}

} // extern "C"
