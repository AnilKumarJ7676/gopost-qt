#include "gopost/effects.h"
#include "gopost/engine.h"

#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <mutex>
#include <new>

struct EffectEntry {
    GopostEffectDef def;
};

struct GopostEffectRegistry {
    std::vector<EffectEntry> effects;
    mutable std::mutex mu;
    bool initialized = false;
};

static GopostEffectRegistry g_registry;

static void register_builtin_adjustments();
static void register_builtin_presets();
static void register_builtin_artistic();

extern "C" {

GopostError gopost_effects_init(GopostEngine* engine) {
    (void)engine;
    std::lock_guard<std::mutex> lock(g_registry.mu);
    if (g_registry.initialized) return GOPOST_OK;

    register_builtin_adjustments();
    register_builtin_presets();
    register_builtin_artistic();

    g_registry.initialized = true;
    return GOPOST_OK;
}

GopostError gopost_effects_shutdown(GopostEngine* engine) {
    (void)engine;
    std::lock_guard<std::mutex> lock(g_registry.mu);
    g_registry.effects.clear();
    g_registry.initialized = false;
    return GOPOST_OK;
}

GopostError gopost_effects_get_count(GopostEngine* engine, int32_t* out_count) {
    (void)engine;
    if (!out_count) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_registry.mu);
    *out_count = (int32_t)g_registry.effects.size();
    return GOPOST_OK;
}

GopostError gopost_effects_get_def(GopostEngine* engine, int32_t index,
                                   GopostEffectDef* out_def) {
    (void)engine;
    if (!out_def) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_registry.mu);
    if (index < 0 || index >= (int32_t)g_registry.effects.size())
        return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_def = g_registry.effects[index].def;
    return GOPOST_OK;
}

GopostError gopost_effects_find(GopostEngine* engine, const char* effect_id,
                                GopostEffectDef* out_def) {
    (void)engine;
    if (!effect_id || !out_def) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_registry.mu);
    for (const auto& e : g_registry.effects) {
        if (strcmp(e.def.id, effect_id) == 0) {
            *out_def = e.def;
            return GOPOST_OK;
        }
    }
    return GOPOST_ERROR_INVALID_ARGUMENT;
}

GopostError gopost_effects_list_category(GopostEngine* engine,
                                         GopostEffectCategory category,
                                         GopostEffectDef* out_defs,
                                         int32_t max_count, int32_t* out_count) {
    (void)engine;
    if (!out_defs || !out_count || max_count <= 0)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_registry.mu);
    int32_t written = 0;
    for (const auto& e : g_registry.effects) {
        if (e.def.category == category && written < max_count) {
            out_defs[written++] = e.def;
        }
    }
    *out_count = written;
    return GOPOST_OK;
}

} // extern "C"

/*--- Registration helpers ---*/

static void add_param(GopostEffectDef& def, const char* id, const char* name,
                      GopostParamType type, float def_val, float min_v, float max_v) {
    if (def.param_count >= GOPOST_MAX_EFFECT_PARAMS) return;
    auto& p = def.params[def.param_count++];
    strncpy(p.id, id, sizeof(p.id) - 1);
    strncpy(p.display_name, name, sizeof(p.display_name) - 1);
    p.type = type;
    p.default_val = def_val;
    p.min_val = min_v;
    p.max_val = max_v;
}

static void reg(const GopostEffectDef& def) {
    g_registry.effects.push_back({def});
}

static void register_builtin_adjustments() {
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.adjust.brightness", sizeof(d.id) - 1);
        strncpy(d.display_name, "Brightness", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_ADJUSTMENT;
        add_param(d, "brightness", "Brightness", GOPOST_PARAM_FLOAT, 0, -100, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.adjust.contrast", sizeof(d.id) - 1);
        strncpy(d.display_name, "Contrast", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_ADJUSTMENT;
        add_param(d, "contrast", "Contrast", GOPOST_PARAM_FLOAT, 0, -100, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.adjust.saturation", sizeof(d.id) - 1);
        strncpy(d.display_name, "Saturation", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_ADJUSTMENT;
        add_param(d, "saturation", "Saturation", GOPOST_PARAM_FLOAT, 0, -100, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.adjust.exposure", sizeof(d.id) - 1);
        strncpy(d.display_name, "Exposure", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_ADJUSTMENT;
        add_param(d, "exposure", "Exposure (EV)", GOPOST_PARAM_FLOAT, 0, -2.0f, 2.0f);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.adjust.temperature", sizeof(d.id) - 1);
        strncpy(d.display_name, "Temperature", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_ADJUSTMENT;
        add_param(d, "temperature", "Temperature", GOPOST_PARAM_FLOAT, 0, -100, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.adjust.highlights", sizeof(d.id) - 1);
        strncpy(d.display_name, "Highlights", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_ADJUSTMENT;
        add_param(d, "highlights", "Highlights", GOPOST_PARAM_FLOAT, 0, -100, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.adjust.shadows", sizeof(d.id) - 1);
        strncpy(d.display_name, "Shadows", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_ADJUSTMENT;
        add_param(d, "shadows", "Shadows", GOPOST_PARAM_FLOAT, 0, -100, 100);
        reg(d);
    }
}

static void register_builtin_presets() {
    struct PresetInfo { const char* id; const char* name; };

    static const PresetInfo natural[] = {
        {"gp.preset.daylight",     "Daylight"},
        {"gp.preset.golden_hour",  "Golden Hour"},
        {"gp.preset.overcast",     "Overcast"},
        {"gp.preset.shade",        "Shade"},
        {"gp.preset.fluorescent",  "Fluorescent"},
        {"gp.preset.tungsten",     "Tungsten"},
        {"gp.preset.flash",        "Flash"},
        {"gp.preset.cloudy",       "Cloudy"},
    };
    static const PresetInfo portrait[] = {
        {"gp.preset.soft_skin",    "Soft Skin"},
        {"gp.preset.studio",       "Studio"},
        {"gp.preset.warm_glow",    "Warm Glow"},
        {"gp.preset.high_key",     "High Key"},
        {"gp.preset.low_key",      "Low Key"},
        {"gp.preset.beauty",       "Beauty"},
        {"gp.preset.magazine",     "Magazine"},
        {"gp.preset.headshot",     "Headshot"},
    };
    static const PresetInfo vintage[] = {
        {"gp.preset.polaroid",     "Polaroid"},
        {"gp.preset.kodachrome",   "Kodachrome"},
        {"gp.preset.fujifilm",     "Fujifilm"},
        {"gp.preset.agfa",         "Agfa Vista"},
        {"gp.preset.lomo",         "Lomo"},
        {"gp.preset.sepia",        "Sepia"},
        {"gp.preset.faded",        "Faded"},
        {"gp.preset.cross_process","Cross Process"},
    };
    static const PresetInfo cinematic[] = {
        {"gp.preset.teal_orange",  "Teal & Orange"},
        {"gp.preset.noir",         "Noir"},
        {"gp.preset.blockbuster",  "Blockbuster"},
        {"gp.preset.muted",        "Muted"},
        {"gp.preset.desaturated",  "Desaturated"},
        {"gp.preset.bleach_bypass","Bleach Bypass"},
        {"gp.preset.day_for_night","Day for Night"},
        {"gp.preset.indie",        "Indie"},
    };
    static const PresetInfo bw[] = {
        {"gp.preset.bw_classic",   "B&W Classic"},
        {"gp.preset.bw_selenium",  "Selenium"},
        {"gp.preset.bw_cyanotype", "Cyanotype"},
        {"gp.preset.bw_infrared",  "Infrared"},
        {"gp.preset.bw_high_contrast","High Contrast"},
        {"gp.preset.bw_silver",    "Silver"},
        {"gp.preset.bw_grain",     "Film Grain"},
        {"gp.preset.bw_noir",      "Noir"},
    };

    auto register_group = [](const PresetInfo* list, int count) {
        for (int i = 0; i < count; i++) {
            GopostEffectDef d{};
            strncpy(d.id, list[i].id, sizeof(d.id) - 1);
            strncpy(d.display_name, list[i].name, sizeof(d.display_name) - 1);
            d.category = GOPOST_EFFECT_CAT_PRESET;
            add_param(d, "intensity", "Intensity", GOPOST_PARAM_FLOAT, 100, 0, 100);
            reg(d);
        }
    };

    register_group(natural, 8);
    register_group(portrait, 8);
    register_group(vintage, 8);
    register_group(cinematic, 8);
    register_group(bw, 8);
}

static void register_builtin_artistic() {
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.artistic.oil_paint", sizeof(d.id) - 1);
        strncpy(d.display_name, "Oil Paint", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_STYLIZE;
        add_param(d, "radius", "Radius", GOPOST_PARAM_FLOAT, 4, 1, 10);
        add_param(d, "intensity", "Intensity", GOPOST_PARAM_FLOAT, 50, 0, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.artistic.watercolor", sizeof(d.id) - 1);
        strncpy(d.display_name, "Watercolor", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_STYLIZE;
        add_param(d, "brush_size", "Brush Size", GOPOST_PARAM_FLOAT, 5, 1, 15);
        add_param(d, "intensity", "Intensity", GOPOST_PARAM_FLOAT, 60, 0, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.artistic.sketch", sizeof(d.id) - 1);
        strncpy(d.display_name, "Sketch", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_STYLIZE;
        add_param(d, "threshold", "Threshold", GOPOST_PARAM_FLOAT, 30, 0, 100);
        add_param(d, "intensity", "Darkness", GOPOST_PARAM_FLOAT, 80, 0, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.artistic.pixelate", sizeof(d.id) - 1);
        strncpy(d.display_name, "Pixelate", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_STYLIZE;
        add_param(d, "block_size", "Block Size", GOPOST_PARAM_INT, 8, 2, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.artistic.glitch", sizeof(d.id) - 1);
        strncpy(d.display_name, "Glitch", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_STYLIZE;
        add_param(d, "amount", "Amount", GOPOST_PARAM_FLOAT, 30, 0, 100);
        add_param(d, "seed", "Seed", GOPOST_PARAM_FLOAT, 42, 0, 100);
        reg(d);
    }
    {
        GopostEffectDef d{};
        strncpy(d.id, "gp.artistic.halftone", sizeof(d.id) - 1);
        strncpy(d.display_name, "Halftone", sizeof(d.display_name) - 1);
        d.category = GOPOST_EFFECT_CAT_STYLIZE;
        add_param(d, "dot_size", "Dot Size", GOPOST_PARAM_FLOAT, 8, 2, 40);
        add_param(d, "angle", "Angle", GOPOST_PARAM_FLOAT, 45, 0, 180);
        add_param(d, "contrast", "Contrast", GOPOST_PARAM_FLOAT, 100, 0, 200);
        reg(d);
    }
}
