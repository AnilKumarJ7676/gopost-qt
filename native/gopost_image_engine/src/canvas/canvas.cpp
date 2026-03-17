#include "gopost/canvas.h"
#include "gopost/engine.h"
#include "canvas/canvas_internal.h"

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <new>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <climits>

static constexpr size_t kMaxPixelAlloc = 512ULL * 1024 * 1024; // 512 MB guard

static inline bool pixel_alloc_safe(int32_t w, int32_t h, size_t* out_size) {
    if (w <= 0 || h <= 0) return false;
    size_t sz = (size_t)w * (size_t)h * 4;
    if (sz / 4 / (size_t)h != (size_t)w) return false; // overflow check
    if (sz > kMaxPixelAlloc) return false;
    *out_size = sz;
    return true;
}

struct LayerData {
    int32_t id;
    GopostLayerType type;
    std::string name;
    float opacity;
    GopostBlendMode blend_mode;
    bool visible;
    bool locked;

    float tx, ty;
    float sx, sy;
    float rotation;

    int32_t content_w;
    int32_t content_h;

    uint8_t* pixel_data;
    size_t pixel_data_size;

    std::vector<LayerData*> children;

    bool dirty;
};

struct ViewportState {
    float pan_x, pan_y;
    float zoom;
    float rotation;
};

struct GopostCanvas {
    GopostEngine* engine;
    GopostCanvasConfig config;

    std::vector<LayerData*> layers;
    int32_t next_layer_id;
    mutable std::mutex layer_mutex;

    ViewportState viewport;

    int32_t tile_size;
    std::atomic<bool> dirty;

    GopostFrameCallback frame_callback;
    void* callback_user_data;

    LayerData* find_layer(int32_t id) const {
        for (auto* l : layers) {
            if (l->id == id) return l;
            for (auto* c : l->children) {
                if (c->id == id) return c;
            }
        }
        return nullptr;
    }

    int find_layer_index(int32_t id) const {
        for (int i = 0; i < (int)layers.size(); i++) {
            if (layers[i]->id == id) return i;
        }
        return -1;
    }
};

static LayerData* create_layer(int32_t id, GopostLayerType type, const char* name) {
    auto* layer = new (std::nothrow) LayerData{};
    if (!layer) return nullptr;

    layer->id = id;
    layer->type = type;
    layer->name = name ? name : "";
    layer->opacity = 1.0f;
    layer->blend_mode = GOPOST_BLEND_NORMAL;
    layer->visible = true;
    layer->locked = false;
    layer->tx = 0; layer->ty = 0;
    layer->sx = 1.0f; layer->sy = 1.0f;
    layer->rotation = 0;
    layer->content_w = 0;
    layer->content_h = 0;
    layer->pixel_data = nullptr;
    layer->pixel_data_size = 0;
    layer->dirty = true;

    return layer;
}

static void destroy_layer(LayerData* layer) {
    if (!layer) return;
    for (auto* c : layer->children) destroy_layer(c);
    layer->children.clear();
    if (layer->pixel_data) {
        free(layer->pixel_data);
        layer->pixel_data = nullptr;
    }
    delete layer;
}

static LayerData* deep_copy_layer(const LayerData* src, int32_t new_id) {
    auto* dst = create_layer(new_id, src->type, src->name.c_str());
    if (!dst) return nullptr;

    dst->opacity = src->opacity;
    dst->blend_mode = src->blend_mode;
    dst->visible = src->visible;
    dst->locked = src->locked;
    dst->tx = src->tx; dst->ty = src->ty;
    dst->sx = src->sx; dst->sy = src->sy;
    dst->rotation = src->rotation;
    dst->content_w = src->content_w;
    dst->content_h = src->content_h;

    if (src->pixel_data && src->pixel_data_size > 0 &&
        src->pixel_data_size <= kMaxPixelAlloc) {
        dst->pixel_data = static_cast<uint8_t*>(malloc(src->pixel_data_size));
        if (dst->pixel_data) {
            memcpy(dst->pixel_data, src->pixel_data, src->pixel_data_size);
            dst->pixel_data_size = src->pixel_data_size;
        }
    }

    return dst;
}

extern "C" {

GopostError gopost_canvas_create(
    GopostEngine* engine,
    const GopostCanvasConfig* config,
    GopostCanvas** out_canvas
) {
    if (!engine || !config || !out_canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (config->width <= 0 || config->height <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (config->width > 16384 || config->height > 16384) return GOPOST_ERROR_INVALID_ARGUMENT;

    auto* canvas = new (std::nothrow) GopostCanvas{};
    if (!canvas) return GOPOST_ERROR_OUT_OF_MEMORY;

    canvas->engine = engine;
    canvas->config = *config;
    canvas->next_layer_id = 1;
    canvas->viewport = {0, 0, 1.0f, 0};
    canvas->tile_size = 2048;
    canvas->dirty.store(true);
    canvas->frame_callback = nullptr;
    canvas->callback_user_data = nullptr;

    *out_canvas = canvas;
    return GOPOST_OK;
}

GopostError gopost_canvas_destroy(GopostCanvas* canvas) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::vector<LayerData*> layers_to_free;
    {
        std::lock_guard<std::mutex> lock(canvas->layer_mutex);
        layers_to_free.swap(canvas->layers);
    }
    for (auto* l : layers_to_free) destroy_layer(l);

    delete canvas;
    return GOPOST_OK;
}

GopostError gopost_canvas_get_size(
    GopostCanvas* canvas,
    int32_t* out_width, int32_t* out_height, float* out_dpi
) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (out_width)  *out_width  = canvas->config.width;
    if (out_height) *out_height = canvas->config.height;
    if (out_dpi)    *out_dpi    = canvas->config.dpi;
    return GOPOST_OK;
}

GopostError gopost_canvas_resize(GopostCanvas* canvas, int32_t width, int32_t height) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    canvas->config.width = width;
    canvas->config.height = height;
    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_canvas_add_image_layer(
    GopostCanvas* canvas,
    const uint8_t* rgba_pixels, int32_t width, int32_t height,
    int32_t index,
    int32_t* out_layer_id
) {
    if (!canvas || !rgba_pixels || !out_layer_id) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (width <= 0 || height <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(canvas->layer_mutex);

    size_t data_size = 0;
    if (!pixel_alloc_safe(width, height, &data_size))
        return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t id = canvas->next_layer_id++;
    auto* layer = create_layer(id, GOPOST_LAYER_IMAGE, "Image");
    if (!layer) return GOPOST_ERROR_OUT_OF_MEMORY;

    layer->pixel_data = static_cast<uint8_t*>(malloc(data_size));
    if (!layer->pixel_data) {
        destroy_layer(layer);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }
    memcpy(layer->pixel_data, rgba_pixels, data_size);
    layer->pixel_data_size = data_size;
    layer->content_w = width;
    layer->content_h = height;

    if (index < 0 || index >= (int32_t)canvas->layers.size()) {
        canvas->layers.push_back(layer);
    } else {
        canvas->layers.insert(canvas->layers.begin() + index, layer);
    }

    canvas->dirty.store(true);
    *out_layer_id = id;
    return GOPOST_OK;
}

GopostError gopost_canvas_add_solid_layer(
    GopostCanvas* canvas,
    float r, float g, float b, float a,
    int32_t width, int32_t height,
    int32_t index,
    int32_t* out_layer_id
) {
    if (!canvas || !out_layer_id) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (width <= 0 || height <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(canvas->layer_mutex);

    size_t data_size = 0;
    if (!pixel_alloc_safe(width, height, &data_size))
        return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t id = canvas->next_layer_id++;
    auto* layer = create_layer(id, GOPOST_LAYER_SOLID_COLOR, "Solid Color");
    if (!layer) return GOPOST_ERROR_OUT_OF_MEMORY;

    layer->pixel_data = static_cast<uint8_t*>(malloc(data_size));
    if (!layer->pixel_data) {
        destroy_layer(layer);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    uint8_t cr = (uint8_t)(std::clamp(r, 0.0f, 1.0f) * 255);
    uint8_t cg = (uint8_t)(std::clamp(g, 0.0f, 1.0f) * 255);
    uint8_t cb = (uint8_t)(std::clamp(b, 0.0f, 1.0f) * 255);
    uint8_t ca = (uint8_t)(std::clamp(a, 0.0f, 1.0f) * 255);
    if (cr == cg && cg == cb && cb == ca) {
        memset(layer->pixel_data, cr, data_size);
    } else {
        uint32_t pixel = ((uint32_t)cr) | ((uint32_t)cg << 8) |
                         ((uint32_t)cb << 16) | ((uint32_t)ca << 24);
        auto* p32 = reinterpret_cast<uint32_t*>(layer->pixel_data);
        size_t count = data_size / 4;
        for (size_t i = 0; i < count; i++) p32[i] = pixel;
    }
    layer->pixel_data_size = data_size;
    layer->content_w = width;
    layer->content_h = height;

    if (index < 0 || index >= (int32_t)canvas->layers.size()) {
        canvas->layers.push_back(layer);
    } else {
        canvas->layers.insert(canvas->layers.begin() + index, layer);
    }

    canvas->dirty.store(true);
    *out_layer_id = id;
    return GOPOST_OK;
}

GopostError gopost_canvas_add_group_layer(
    GopostCanvas* canvas,
    const char* name,
    int32_t index,
    int32_t* out_layer_id
) {
    if (!canvas || !out_layer_id) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(canvas->layer_mutex);

    int32_t id = canvas->next_layer_id++;
    auto* layer = create_layer(id, GOPOST_LAYER_GROUP, name ? name : "Group");
    if (!layer) return GOPOST_ERROR_OUT_OF_MEMORY;

    layer->content_w = canvas->config.width;
    layer->content_h = canvas->config.height;

    if (index < 0 || index >= (int32_t)canvas->layers.size()) {
        canvas->layers.push_back(layer);
    } else {
        canvas->layers.insert(canvas->layers.begin() + index, layer);
    }

    canvas->dirty.store(true);
    *out_layer_id = id;
    return GOPOST_OK;
}

GopostError gopost_canvas_remove_layer(GopostCanvas* canvas, int32_t layer_id) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    int idx = canvas->find_layer_index(layer_id);
    if (idx < 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    destroy_layer(canvas->layers[idx]);
    canvas->layers.erase(canvas->layers.begin() + idx);
    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_canvas_reorder_layer(GopostCanvas* canvas, int32_t layer_id, int32_t new_index) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    int old_idx = canvas->find_layer_index(layer_id);
    if (old_idx < 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t clamped = std::clamp(new_index, (int32_t)0, (int32_t)(canvas->layers.size() - 1));

    auto* layer = canvas->layers[old_idx];
    canvas->layers.erase(canvas->layers.begin() + old_idx);
    canvas->layers.insert(canvas->layers.begin() + clamped, layer);

    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_canvas_duplicate_layer(GopostCanvas* canvas, int32_t layer_id, int32_t* out_new_id) {
    if (!canvas || !out_new_id) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    int idx = canvas->find_layer_index(layer_id);
    if (idx < 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t new_id = canvas->next_layer_id++;
    auto* copy = deep_copy_layer(canvas->layers[idx], new_id);
    if (!copy) return GOPOST_ERROR_OUT_OF_MEMORY;

    copy->name += " Copy";
    canvas->layers.insert(canvas->layers.begin() + idx + 1, copy);
    canvas->dirty.store(true);

    *out_new_id = new_id;
    return GOPOST_OK;
}

GopostError gopost_layer_set_visible(GopostCanvas* canvas, int32_t layer_id, int32_t visible) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    auto* l = canvas->find_layer(layer_id);
    if (!l) return GOPOST_ERROR_INVALID_ARGUMENT;
    l->visible = (visible != 0);
    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_layer_set_locked(GopostCanvas* canvas, int32_t layer_id, int32_t locked) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    auto* l = canvas->find_layer(layer_id);
    if (!l) return GOPOST_ERROR_INVALID_ARGUMENT;
    l->locked = (locked != 0);
    return GOPOST_OK;
}

GopostError gopost_layer_set_opacity(GopostCanvas* canvas, int32_t layer_id, float opacity) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    auto* l = canvas->find_layer(layer_id);
    if (!l) return GOPOST_ERROR_INVALID_ARGUMENT;
    l->opacity = std::clamp(opacity, 0.0f, 1.0f);
    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_layer_set_blend_mode(GopostCanvas* canvas, int32_t layer_id, GopostBlendMode mode) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    auto* l = canvas->find_layer(layer_id);
    if (!l) return GOPOST_ERROR_INVALID_ARGUMENT;
    l->blend_mode = mode;
    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_layer_set_name(GopostCanvas* canvas, int32_t layer_id, const char* name) {
    if (!canvas || !name) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    auto* l = canvas->find_layer(layer_id);
    if (!l) return GOPOST_ERROR_INVALID_ARGUMENT;
    l->name = name;
    return GOPOST_OK;
}

GopostError gopost_layer_set_transform(
    GopostCanvas* canvas, int32_t layer_id,
    float tx, float ty, float sx, float sy, float rotation
) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    auto* l = canvas->find_layer(layer_id);
    if (!l) return GOPOST_ERROR_INVALID_ARGUMENT;
    l->tx = tx; l->ty = ty;
    l->sx = sx; l->sy = sy;
    l->rotation = rotation;
    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_canvas_get_layer_count(GopostCanvas* canvas, int32_t* out_count) {
    if (!canvas || !out_count) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    *out_count = (int32_t)canvas->layers.size();
    return GOPOST_OK;
}

GopostError gopost_canvas_get_layer_info(GopostCanvas* canvas, int32_t layer_id, GopostLayerInfo* out_info) {
    if (!canvas || !out_info) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    auto* l = canvas->find_layer(layer_id);
    if (!l) return GOPOST_ERROR_INVALID_ARGUMENT;

    memset(out_info, 0, sizeof(GopostLayerInfo));
    out_info->id = l->id;
    out_info->type = l->type;
    strncpy(out_info->name, l->name.c_str(), sizeof(out_info->name) - 1);
    out_info->opacity = l->opacity;
    out_info->blend_mode = l->blend_mode;
    out_info->visible = l->visible ? 1 : 0;
    out_info->locked = l->locked ? 1 : 0;
    out_info->tx = l->tx; out_info->ty = l->ty;
    out_info->sx = l->sx; out_info->sy = l->sy;
    out_info->rotation = l->rotation;
    out_info->content_w = l->content_w;
    out_info->content_h = l->content_h;

    return GOPOST_OK;
}

GopostError gopost_canvas_get_layer_ids(GopostCanvas* canvas, int32_t* out_ids, int32_t max_count) {
    if (!canvas || !out_ids || max_count <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    int32_t n = std::min(max_count, (int32_t)canvas->layers.size());
    for (int32_t i = 0; i < n; i++) {
        out_ids[i] = canvas->layers[i]->id;
    }
    return GOPOST_OK;
}

GopostError gopost_canvas_set_viewport(
    GopostCanvas* canvas,
    float pan_x, float pan_y, float zoom, float rotation
) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    canvas->viewport = {pan_x, pan_y, zoom, rotation};
    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_canvas_get_viewport(
    GopostCanvas* canvas,
    float* out_pan_x, float* out_pan_y, float* out_zoom, float* out_rotation
) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (out_pan_x)    *out_pan_x    = canvas->viewport.pan_x;
    if (out_pan_y)    *out_pan_y    = canvas->viewport.pan_y;
    if (out_zoom)     *out_zoom     = canvas->viewport.zoom;
    if (out_rotation) *out_rotation = canvas->viewport.rotation;
    return GOPOST_OK;
}

GopostError gopost_canvas_set_tile_size(GopostCanvas* canvas, int32_t tile_size) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (tile_size != 256 && tile_size != 512 && tile_size != 1024 &&
        tile_size != 2048 && tile_size != 4096)
        return GOPOST_ERROR_INVALID_ARGUMENT;
    canvas->tile_size = tile_size;
    canvas->dirty.store(true);
    return GOPOST_OK;
}

GopostError gopost_canvas_get_dirty_tile_count(GopostCanvas* canvas, int32_t* out_count) {
    if (!canvas || !out_count) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (!canvas->dirty.load()) {
        *out_count = 0;
        return GOPOST_OK;
    }
    int32_t cols = (canvas->config.width  + canvas->tile_size - 1) / canvas->tile_size;
    int32_t rows = (canvas->config.height + canvas->tile_size - 1) / canvas->tile_size;
    *out_count = cols * rows;
    return GOPOST_OK;
}

GopostError gopost_canvas_set_frame_callback(
    GopostCanvas* canvas,
    GopostFrameCallback callback,
    void* user_data
) {
    if (!canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    canvas->frame_callback = callback;
    canvas->callback_user_data = user_data;
    return GOPOST_OK;
}

/*--- Internal accessors for compositor/tile_renderer (not public API) ---*/

int32_t gopost_internal_canvas_layer_count(GopostCanvas* canvas) {
    if (!canvas) return 0;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    return (int32_t)canvas->layers.size();
}

GopostError gopost_internal_canvas_layer_at(
    GopostCanvas* canvas, int32_t index, GopostLayerInfo* out_info
) {
    if (!canvas || !out_info) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    if (index < 0 || index >= (int32_t)canvas->layers.size())
        return GOPOST_ERROR_INVALID_ARGUMENT;

    auto* l = canvas->layers[index];
    memset(out_info, 0, sizeof(GopostLayerInfo));
    out_info->id = l->id;
    out_info->type = l->type;
    strncpy(out_info->name, l->name.c_str(), sizeof(out_info->name) - 1);
    out_info->opacity = l->opacity;
    out_info->blend_mode = l->blend_mode;
    out_info->visible = l->visible ? 1 : 0;
    out_info->locked = l->locked ? 1 : 0;
    out_info->tx = l->tx; out_info->ty = l->ty;
    out_info->sx = l->sx; out_info->sy = l->sy;
    out_info->rotation = l->rotation;
    out_info->content_w = l->content_w;
    out_info->content_h = l->content_h;
    return GOPOST_OK;
}

GopostError gopost_internal_canvas_layer_pixels(
    GopostCanvas* canvas, int32_t index,
    const uint8_t** out_data, size_t* out_data_size
) {
    if (!canvas || !out_data || !out_data_size) return GOPOST_ERROR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    if (index < 0 || index >= (int32_t)canvas->layers.size())
        return GOPOST_ERROR_INVALID_ARGUMENT;

    auto* l = canvas->layers[index];
    *out_data = l->pixel_data;
    *out_data_size = l->pixel_data_size;
    return GOPOST_OK;
}

int32_t gopost_internal_canvas_width(GopostCanvas* canvas) {
    return canvas ? canvas->config.width : 0;
}
int32_t gopost_internal_canvas_height(GopostCanvas* canvas) {
    return canvas ? canvas->config.height : 0;
}
float gopost_internal_canvas_bg_r(GopostCanvas* canvas) {
    return canvas ? canvas->config.bg_r : 0;
}
float gopost_internal_canvas_bg_g(GopostCanvas* canvas) {
    return canvas ? canvas->config.bg_g : 0;
}
float gopost_internal_canvas_bg_b(GopostCanvas* canvas) {
    return canvas ? canvas->config.bg_b : 0;
}
float gopost_internal_canvas_bg_a(GopostCanvas* canvas) {
    return canvas ? canvas->config.bg_a : 0;
}
int32_t gopost_internal_canvas_transparent_bg(GopostCanvas* canvas) {
    return canvas ? canvas->config.transparent_bg : 0;
}

GopostEngine* gopost_internal_canvas_get_engine(GopostCanvas* canvas) {
    return canvas ? canvas->engine : nullptr;
}

GopostEngine* gopost_canvas_get_engine(GopostCanvas* canvas) {
    return gopost_internal_canvas_get_engine(canvas);
}

GopostError gopost_internal_canvas_update_layer_pixels(
    GopostCanvas* canvas, int32_t layer_id,
    const uint8_t* rgba_pixels, int32_t width, int32_t height
) {
    if (!canvas || !rgba_pixels) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (width <= 0 || height <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(canvas->layer_mutex);
    auto* layer = canvas->find_layer(layer_id);
    if (!layer) return GOPOST_ERROR_INVALID_ARGUMENT;

    size_t data_size = 0;
    if (!pixel_alloc_safe(width, height, &data_size))
        return GOPOST_ERROR_INVALID_ARGUMENT;

    uint8_t* new_data = static_cast<uint8_t*>(malloc(data_size));
    if (!new_data) return GOPOST_ERROR_OUT_OF_MEMORY;

    memcpy(new_data, rgba_pixels, data_size);

    if (layer->pixel_data) free(layer->pixel_data);
    layer->pixel_data = new_data;
    layer->pixel_data_size = data_size;
    layer->content_w = width;
    layer->content_h = height;
    layer->dirty = true;
    canvas->dirty.store(true);

    return GOPOST_OK;
}

} // extern "C"
