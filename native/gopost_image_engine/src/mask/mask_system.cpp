#include "gopost/mask.h"
#include "gopost/canvas.h"
#include "canvas/canvas_internal.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace {

struct MaskKey {
    GopostCanvas* canvas;
    int32_t layer_id;

    bool operator==(const MaskKey& o) const {
        return canvas == o.canvas && layer_id == o.layer_id;
    }
};

struct MaskKeyHash {
    size_t operator()(const MaskKey& k) const {
        return std::hash<void*>()(static_cast<void*>(k.canvas)) ^
               (static_cast<size_t>(k.layer_id) << 16);
    }
};

struct MaskData {
    GopostMaskType type;
    bool enabled;
    std::vector<uint8_t> data;
    int32_t width;
    int32_t height;
};

static std::unordered_map<MaskKey, MaskData, MaskKeyHash> g_masks;
static std::mutex g_masks_mutex;

static int32_t find_layer_index(GopostCanvas* canvas, int32_t layer_id) {
    int32_t count = gopost_internal_canvas_layer_count(canvas);
    GopostLayerInfo info;
    for (int32_t i = 0; i < count; i++) {
        if (gopost_internal_canvas_layer_at(canvas, i, &info) == GOPOST_OK &&
            info.id == layer_id) {
            return i;
        }
    }
    return -1;
}

} // anonymous namespace

extern "C" {

GopostError gopost_layer_add_mask(GopostCanvas* canvas, int32_t layer_id, GopostMaskType type) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (type != GOPOST_MASK_RASTER) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostLayerInfo info;
    if (gopost_canvas_get_layer_info(canvas, layer_id, &info) != GOPOST_OK)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t w = info.content_w;
    int32_t h = info.content_h;
    if (w <= 0 || h <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    if (g_masks.find(key) != g_masks.end())
        return GOPOST_ERROR_ALREADY_INITIALIZED;

    MaskData md;
    md.type = type;
    md.enabled = true;
    md.width = w;
    md.height = h;
    md.data.resize(static_cast<size_t>(w) * static_cast<size_t>(h), 255);

    g_masks[key] = std::move(md);
    return GOPOST_OK;
}

GopostError gopost_layer_remove_mask(GopostCanvas* canvas, int32_t layer_id) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    auto it = g_masks.find(key);
    if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    g_masks.erase(it);
    return GOPOST_OK;
}

GopostError gopost_layer_has_mask(GopostCanvas* canvas, int32_t layer_id, int32_t* out_has_mask) {
    if (!canvas || !out_has_mask) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    *out_has_mask = (g_masks.find(key) != g_masks.end()) ? 1 : 0;
    return GOPOST_OK;
}

GopostError gopost_layer_get_mask_type(GopostCanvas* canvas, int32_t layer_id,
                                       GopostMaskType* out_type) {
    if (!canvas || !out_type) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    auto it = g_masks.find(key);
    if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    *out_type = it->second.type;
    return GOPOST_OK;
}

GopostError gopost_layer_invert_mask(GopostCanvas* canvas, int32_t layer_id) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    auto it = g_masks.find(key);
    if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    for (size_t i = 0; i < it->second.data.size(); i++)
        it->second.data[i] = static_cast<uint8_t>(255 - it->second.data[i]);

    return GOPOST_OK;
}

GopostError gopost_layer_set_mask_enabled(GopostCanvas* canvas, int32_t layer_id, int32_t enabled) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    auto it = g_masks.find(key);
    if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    it->second.enabled = (enabled != 0);
    return GOPOST_OK;
}

GopostError gopost_layer_get_mask_data(GopostCanvas* canvas, int32_t layer_id,
                                       uint8_t** out_data, int32_t* out_w, int32_t* out_h) {
    if (!canvas || !out_data || !out_w || !out_h) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    auto it = g_masks.find(key);
    if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    const MaskData& md = it->second;
    size_t size = md.data.size();
    if (size == 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    uint8_t* copy = static_cast<uint8_t*>(malloc(size));
    if (!copy) return GOPOST_ERROR_OUT_OF_MEMORY;

    memcpy(copy, md.data.data(), size);
    *out_data = copy;
    *out_w = md.width;
    *out_h = md.height;
    return GOPOST_OK;
}

GopostError gopost_layer_set_mask_data(GopostCanvas* canvas, int32_t layer_id,
                                       const uint8_t* data, int32_t w, int32_t h) {
    if (!canvas || !data) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (w <= 0 || h <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    auto it = g_masks.find(key);
    if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    size_t size = static_cast<size_t>(w) * static_cast<size_t>(h);
    it->second.data.assign(data, data + size);
    it->second.width = w;
    it->second.height = h;
    return GOPOST_OK;
}

GopostError gopost_layer_mask_paint(GopostCanvas* canvas, int32_t layer_id,
                                    float center_x, float center_y,
                                    float radius, float hardness,
                                    GopostMaskBrushMode mode, float opacity) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (radius <= 0.0f) return GOPOST_OK;
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    hardness = std::clamp(hardness, 0.0f, 1.0f);

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    auto it = g_masks.find(key);
    if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    MaskData& md = it->second;
    int32_t w = md.width;
    int32_t h = md.height;
    int32_t x0 = static_cast<int32_t>(std::floor(center_x - radius));
    int32_t y0 = static_cast<int32_t>(std::floor(center_y - radius));
    int32_t x1 = static_cast<int32_t>(std::ceil(center_x + radius));
    int32_t y1 = static_cast<int32_t>(std::ceil(center_y + radius));

    x0 = std::max(0, x0);
    y0 = std::max(0, y0);
    x1 = std::min(w, x1);
    y1 = std::min(h, y1);

    float radius_sq = radius * radius;
    float inner_radius = radius * hardness;
    float inner_radius_sq = inner_radius * inner_radius;
    float falloff = (radius_sq - inner_radius_sq) > 0.0001f
        ? (1.0f / (radius_sq - inner_radius_sq)) : 1.0f;

    for (int32_t py = y0; py < y1; py++) {
        for (int32_t px = x0; px < x1; px++) {
            float dx = static_cast<float>(px) - center_x + 0.5f;
            float dy = static_cast<float>(py) - center_y + 0.5f;
            float dist_sq = dx * dx + dy * dy;

            if (dist_sq > radius_sq) continue;

            float intensity;
            if (dist_sq <= inner_radius_sq) {
                intensity = 1.0f;
            } else {
                intensity = 1.0f - (dist_sq - inner_radius_sq) * falloff;
                intensity = std::max(0.0f, intensity);
            }
            intensity *= opacity;

            size_t idx = static_cast<size_t>(py) * static_cast<size_t>(w) + static_cast<size_t>(px);
            uint8_t& v = md.data[idx];

            if (mode == GOPOST_MASK_BRUSH_PAINT) {
                float target = 255.0f;
                v = static_cast<uint8_t>(std::min(255, static_cast<int>(
                    v + (target - v) * intensity + 0.5f)));
            } else {
                float target = 0.0f;
                v = static_cast<uint8_t>(std::max(0, static_cast<int>(
                    v - (v - target) * intensity + 0.5f)));
            }
        }
    }

    return GOPOST_OK;
}

GopostError gopost_layer_mask_fill(GopostCanvas* canvas, int32_t layer_id, uint8_t value) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_masks_mutex);
    MaskKey key{canvas, layer_id};
    auto it = g_masks.find(key);
    if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;

    memset(it->second.data.data(), value, it->second.data.size());
    return GOPOST_OK;
}

GopostError gopost_layer_apply_mask(GopostCanvas* canvas, int32_t layer_id) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t index = find_layer_index(canvas, layer_id);
    if (index < 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    MaskData md_copy;
    {
        std::lock_guard<std::mutex> lock(g_masks_mutex);
        MaskKey key{canvas, layer_id};
        auto it = g_masks.find(key);
        if (it == g_masks.end()) return GOPOST_ERROR_INVALID_ARGUMENT;
        if (!it->second.enabled) return GOPOST_OK;

        md_copy = it->second;
    }

    GopostLayerInfo info;
    if (gopost_internal_canvas_layer_at(canvas, index, &info) != GOPOST_OK)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    const uint8_t* px = nullptr;
    size_t px_size = 0;
    if (gopost_internal_canvas_layer_pixels(canvas, index, &px, &px_size) != GOPOST_OK)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    if (!px || px_size == 0 || info.content_w <= 0 || info.content_h <= 0)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    if (md_copy.width != info.content_w || md_copy.height != info.content_h)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    size_t pixel_count = static_cast<size_t>(info.content_w) * static_cast<size_t>(info.content_h);
    size_t buf_size = pixel_count * 4;
    uint8_t* modified = static_cast<uint8_t*>(malloc(buf_size));
    if (!modified) return GOPOST_ERROR_OUT_OF_MEMORY;

    for (size_t i = 0; i < pixel_count; i++) {
        float m = static_cast<float>(md_copy.data[i]) / 255.0f;
        modified[i * 4 + 0] = px[i * 4 + 0];
        modified[i * 4 + 1] = px[i * 4 + 1];
        modified[i * 4 + 2] = px[i * 4 + 2];
        modified[i * 4 + 3] = static_cast<uint8_t>(
            std::min(255, static_cast<int>(px[i * 4 + 3] * m + 0.5f)));
    }

    GopostError err = gopost_internal_canvas_update_layer_pixels(
        canvas, layer_id, modified, info.content_w, info.content_h);
    free(modified);
    return err;
}

} // extern "C"
