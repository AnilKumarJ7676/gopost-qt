#include "gopost/project.h"
#include "gopost/canvas.h"
#include "gopost/engine.h"
#include "canvas/canvas_internal.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>

extern "C" {

GopostError gopost_project_save(
    GopostCanvas* canvas,
    uint8_t** out_data, size_t* out_size
) {
    if (!canvas || !out_data || !out_size) return GOPOST_ERROR_INVALID_ARGUMENT;

    int32_t layer_count = gopost_internal_canvas_layer_count(canvas);

    GopostProjectHeader header{};
    header.magic = GOPOST_PROJECT_MAGIC;
    header.version = GOPOST_PROJECT_VERSION;
    header.canvas_width = gopost_internal_canvas_width(canvas);
    header.canvas_height = gopost_internal_canvas_height(canvas);
    header.dpi = 72.0f;
    header.color_space = 0;
    header.bg_r = gopost_internal_canvas_bg_r(canvas);
    header.bg_g = gopost_internal_canvas_bg_g(canvas);
    header.bg_b = gopost_internal_canvas_bg_b(canvas);
    header.bg_a = gopost_internal_canvas_bg_a(canvas);
    header.transparent_bg = gopost_internal_canvas_transparent_bg(canvas);
    header.layer_count = layer_count;

    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(),
                  reinterpret_cast<uint8_t*>(&header),
                  reinterpret_cast<uint8_t*>(&header) + sizeof(header));

    for (int32_t i = 0; i < layer_count; i++) {
        GopostLayerInfo info{};
        GopostError err = gopost_internal_canvas_layer_at(canvas, i, &info);
        if (err != GOPOST_OK) continue;

        const uint8_t* pixels = nullptr;
        size_t pixel_size = 0;
        gopost_internal_canvas_layer_pixels(canvas, i, &pixels, &pixel_size);

        GopostProjectLayerEntry entry{};
        entry.layer_id = info.id;
        entry.layer_type = info.type;
        memcpy(entry.name, info.name, sizeof(entry.name));
        entry.opacity = info.opacity;
        entry.blend_mode = info.blend_mode;
        entry.visible = info.visible;
        entry.locked = info.locked;
        entry.tx = info.tx;
        entry.ty = info.ty;
        entry.sx = info.sx;
        entry.sy = info.sy;
        entry.rotation = info.rotation;
        entry.content_w = info.content_w;
        entry.content_h = info.content_h;
        entry.pixel_data_size = (pixels && pixel_size > 0) ? (uint32_t)pixel_size : 0;

        buffer.insert(buffer.end(),
                      reinterpret_cast<uint8_t*>(&entry),
                      reinterpret_cast<uint8_t*>(&entry) + sizeof(entry));

        if (pixels && pixel_size > 0) {
            buffer.insert(buffer.end(), pixels, pixels + pixel_size);
        }
    }

    *out_size = buffer.size();
    *out_data = static_cast<uint8_t*>(malloc(buffer.size()));
    if (!*out_data) return GOPOST_ERROR_OUT_OF_MEMORY;
    memcpy(*out_data, buffer.data(), buffer.size());

    return GOPOST_OK;
}

GopostError gopost_project_load(
    GopostEngine* engine,
    const uint8_t* data, size_t data_size,
    GopostCanvas** out_canvas
) {
    if (!engine || !data || !out_canvas) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (data_size < sizeof(GopostProjectHeader)) return GOPOST_ERROR_INVALID_ARGUMENT;

    const GopostProjectHeader* header =
        reinterpret_cast<const GopostProjectHeader*>(data);

    if (header->magic != GOPOST_PROJECT_MAGIC) return GOPOST_ERROR_INVALID_ARGUMENT;
    if (header->version > GOPOST_PROJECT_VERSION) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostCanvasConfig config{};
    config.width = header->canvas_width;
    config.height = header->canvas_height;
    config.dpi = header->dpi;
    config.color_space = static_cast<GopostColorSpace>(header->color_space);
    config.bg_r = header->bg_r;
    config.bg_g = header->bg_g;
    config.bg_b = header->bg_b;
    config.bg_a = header->bg_a;
    config.transparent_bg = header->transparent_bg;

    GopostCanvas* canvas = nullptr;
    GopostError err = gopost_canvas_create(engine, &config, &canvas);
    if (err != GOPOST_OK) return err;

    size_t offset = sizeof(GopostProjectHeader);

    for (int32_t i = 0; i < header->layer_count; i++) {
        if (offset + sizeof(GopostProjectLayerEntry) > data_size) break;

        const GopostProjectLayerEntry* entry =
            reinterpret_cast<const GopostProjectLayerEntry*>(data + offset);
        offset += sizeof(GopostProjectLayerEntry);

        if (entry->pixel_data_size > 0) {
            if (offset + entry->pixel_data_size > data_size) break;
            const uint8_t* pixels = data + offset;
            offset += entry->pixel_data_size;

            int32_t new_id = 0;
            if (entry->layer_type == GOPOST_LAYER_SOLID_COLOR) {
                gopost_canvas_add_solid_layer(canvas, 1, 1, 1, 1,
                    entry->content_w, entry->content_h, -1, &new_id);
            } else {
                gopost_canvas_add_image_layer(canvas, pixels,
                    entry->content_w, entry->content_h, -1, &new_id);
            }

            if (new_id > 0) {
                gopost_layer_set_name(canvas, new_id, entry->name);
                gopost_layer_set_opacity(canvas, new_id, entry->opacity);
                gopost_layer_set_blend_mode(canvas, new_id,
                    static_cast<GopostBlendMode>(entry->blend_mode));
                gopost_layer_set_visible(canvas, new_id, entry->visible);
                gopost_layer_set_locked(canvas, new_id, entry->locked);
                gopost_layer_set_transform(canvas, new_id,
                    entry->tx, entry->ty, entry->sx, entry->sy, entry->rotation);
            }
        }
    }

    *out_canvas = canvas;
    return GOPOST_OK;
}

GopostError gopost_project_save_to_file(GopostCanvas* canvas, const char* path) {
    if (!canvas || !path) return GOPOST_ERROR_INVALID_ARGUMENT;

    uint8_t* data = nullptr;
    size_t size = 0;
    GopostError err = gopost_project_save(canvas, &data, &size);
    if (err != GOPOST_OK) return err;

    FILE* f = fopen(path, "wb");
    if (!f) { free(data); return GOPOST_ERROR_IO; }

    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    free(data);

    return (written == size) ? GOPOST_OK : GOPOST_ERROR_IO;
}

GopostError gopost_project_load_from_file(
    GopostEngine* engine, const char* path, GopostCanvas** out_canvas
) {
    if (!engine || !path || !out_canvas) return GOPOST_ERROR_INVALID_ARGUMENT;

    FILE* f = fopen(path, "rb");
    if (!f) return GOPOST_ERROR_IO;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) { fclose(f); return GOPOST_ERROR_IO; }

    uint8_t* data = static_cast<uint8_t*>(malloc((size_t)size));
    if (!data) { fclose(f); return GOPOST_ERROR_OUT_OF_MEMORY; }

    size_t read = fread(data, 1, (size_t)size, f);
    fclose(f);

    if (read != (size_t)size) { free(data); return GOPOST_ERROR_IO; }

    GopostError err = gopost_project_load(engine, data, (size_t)size, out_canvas);
    free(data);
    return err;
}

void gopost_project_free(uint8_t* data) {
    free(data);
}

} // extern "C"
