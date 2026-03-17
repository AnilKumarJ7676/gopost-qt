#include "gopost/effects.h"
#include "gopost/canvas.h"
#include "gopost/engine.h"
#include "canvas/canvas_internal.h"

#include <cstring>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

struct LayerKey {
    GopostCanvas* canvas;
    int32_t layer_id;

    bool operator==(const LayerKey& o) const {
        return canvas == o.canvas && layer_id == o.layer_id;
    }
};

struct LayerKeyHash {
    size_t operator()(const LayerKey& k) const {
        size_t h1 = std::hash<void*>()(k.canvas);
        size_t h2 = std::hash<int32_t>()(k.layer_id);
        return h1 ^ (h2 << 1);
    }
};

static std::unordered_map<LayerKey, std::vector<GopostEffectInstance>, LayerKeyHash> g_layer_effects;
static std::mutex g_effects_mutex;
static std::atomic<int32_t> g_next_instance_id{1};

static GopostEffectInstance* find_effect_instance(GopostCanvas* canvas, int32_t layer_id,
                                                   int32_t instance_id) {
    LayerKey key{canvas, layer_id};
    auto it = g_layer_effects.find(key);
    if (it == g_layer_effects.end()) return nullptr;
    for (auto& inst : it->second) {
        if (inst.instance_id == instance_id) return &inst;
    }
    return nullptr;
}

static int find_param_index(const GopostEffectDef* def, const char* param_id) {
    if (!def || !param_id) return -1;
    for (int32_t i = 0; i < def->param_count; i++) {
        if (strcmp(def->params[i].id, param_id) == 0) return i;
    }
    return -1;
}

extern "C" {

GopostError gopost_layer_add_effect(GopostCanvas* canvas, int32_t layer_id,
                                    const char* effect_id,
                                    int32_t* out_instance_id) {
    if (!canvas || !effect_id || !out_instance_id) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostEngine* engine = gopost_internal_canvas_get_engine(canvas);
    if (!engine) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostLayerInfo info;
    if (gopost_canvas_get_layer_info(canvas, layer_id, &info) != GOPOST_OK)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostEffectDef def;
    if (gopost_effects_find(engine, effect_id, &def) != GOPOST_OK)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostEffectInstance inst{};
    inst.instance_id = g_next_instance_id++;
    strncpy(inst.effect_id, effect_id, sizeof(inst.effect_id) - 1);
    inst.effect_id[sizeof(inst.effect_id) - 1] = '\0';
    inst.enabled = 1;
    inst.mix = 1.0f;
    for (int32_t i = 0; i < def.param_count; i++) {
        inst.param_values[i] = def.params[i].default_val;
    }

    std::lock_guard<std::mutex> lock(g_effects_mutex);
    LayerKey key{canvas, layer_id};
    g_layer_effects[key].push_back(inst);
    *out_instance_id = inst.instance_id;
    return GOPOST_OK;
}

GopostError gopost_layer_remove_effect(GopostCanvas* canvas, int32_t layer_id,
                                       int32_t instance_id) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_effects_mutex);
    LayerKey key{canvas, layer_id};
    auto it = g_layer_effects.find(key);
    if (it == g_layer_effects.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    auto& vec = it->second;
    for (auto vit = vec.begin(); vit != vec.end(); ++vit) {
        if (vit->instance_id == instance_id) {
            vec.erase(vit);
            if (vec.empty()) g_layer_effects.erase(it);
            return GOPOST_OK;
        }
    }
    return GOPOST_ERROR_INVALID_ARGUMENT;
}

GopostError gopost_layer_get_effect_count(GopostCanvas* canvas, int32_t layer_id,
                                          int32_t* out_count) {
    if (!canvas || !out_count) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostLayerInfo info;
    if (gopost_canvas_get_layer_info(canvas, layer_id, &info) != GOPOST_OK)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_effects_mutex);
    LayerKey key{canvas, layer_id};
    auto it = g_layer_effects.find(key);
    if (it == g_layer_effects.end()) {
        *out_count = 0;
        return GOPOST_OK;
    }
    *out_count = (int32_t)it->second.size();
    return GOPOST_OK;
}

GopostError gopost_layer_get_effect(GopostCanvas* canvas, int32_t layer_id,
                                    int32_t index,
                                    GopostEffectInstance* out_instance) {
    if (!canvas || !out_instance) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_effects_mutex);
    LayerKey key{canvas, layer_id};
    auto it = g_layer_effects.find(key);
    if (it == g_layer_effects.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    const auto& vec = it->second;
    if (index < 0 || index >= (int32_t)vec.size()) return GOPOST_ERROR_INVALID_ARGUMENT;

    *out_instance = vec[index];
    return GOPOST_OK;
}

GopostError gopost_effect_set_enabled(GopostCanvas* canvas, int32_t layer_id,
                                      int32_t instance_id, int32_t enabled) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_effects_mutex);
    GopostEffectInstance* inst = find_effect_instance(canvas, layer_id, instance_id);
    if (!inst) return GOPOST_ERROR_INVALID_ARGUMENT;
    inst->enabled = (enabled != 0) ? 1 : 0;
    return GOPOST_OK;
}

GopostError gopost_effect_set_mix(GopostCanvas* canvas, int32_t layer_id,
                                  int32_t instance_id, float mix) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_effects_mutex);
    GopostEffectInstance* inst = find_effect_instance(canvas, layer_id, instance_id);
    if (!inst) return GOPOST_ERROR_INVALID_ARGUMENT;
    inst->mix = mix;
    return GOPOST_OK;
}

GopostError gopost_effect_set_param(GopostCanvas* canvas, int32_t layer_id,
                                    int32_t instance_id,
                                    const char* param_id, float value) {
    if (!canvas || !param_id) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostEngine* engine = gopost_internal_canvas_get_engine(canvas);
    if (!engine) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_effects_mutex);
    GopostEffectInstance* inst = find_effect_instance(canvas, layer_id, instance_id);
    if (!inst) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostEffectDef def;
    if (gopost_effects_find(engine, inst->effect_id, &def) != GOPOST_OK)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    int idx = find_param_index(&def, param_id);
    if (idx < 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    inst->param_values[idx] = value;
    return GOPOST_OK;
}

GopostError gopost_effect_get_param(GopostCanvas* canvas, int32_t layer_id,
                                    int32_t instance_id,
                                    const char* param_id, float* out_value) {
    if (!canvas || !param_id || !out_value) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostEngine* engine = gopost_internal_canvas_get_engine(canvas);
    if (!engine) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_effects_mutex);
    GopostEffectInstance* inst = find_effect_instance(canvas, layer_id, instance_id);
    if (!inst) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostEffectDef def;
    if (gopost_effects_find(engine, inst->effect_id, &def) != GOPOST_OK)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    int idx = find_param_index(&def, param_id);
    if (idx < 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    *out_value = inst->param_values[idx];
    return GOPOST_OK;
}

} /* extern "C" */
