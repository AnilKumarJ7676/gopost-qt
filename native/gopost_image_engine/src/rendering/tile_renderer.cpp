#include "gopost/canvas.h"
#include "gopost/engine.h"

#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>

/*
 * S5-06: Tile-based rendering.
 *
 * The canvas is divided into a grid of tiles (default 2048x2048, 2px overlap).
 * Only "dirty" tiles are re-rendered. Visible tiles are computed from the
 * viewport and dispatched to worker threads.
 *
 * For Sprint 5, the tile renderer provides the structural framework.
 * Actual multi-threaded tile dispatch via the thread pool will be wired
 * in Sprint 6+ once the full effect chain requires parallel execution.
 */

struct TileCoord {
    int32_t col;
    int32_t row;
};

struct TileGrid {
    int32_t tile_size;
    int32_t overlap;
    int32_t cols;
    int32_t rows;
    int32_t canvas_w;
    int32_t canvas_h;
    std::vector<bool> dirty_flags;

    void init(int32_t w, int32_t h, int32_t ts) {
        canvas_w = w;
        canvas_h = h;
        tile_size = ts;
        overlap = 2;
        cols = (w + ts - 1) / ts;
        rows = (h + ts - 1) / ts;
        dirty_flags.assign((size_t)cols * rows, true);
    }

    void mark_all_dirty() {
        std::fill(dirty_flags.begin(), dirty_flags.end(), true);
    }

    void mark_clean(int32_t col, int32_t row) {
        if (col >= 0 && col < cols && row >= 0 && row < rows)
            dirty_flags[(size_t)row * cols + col] = false;
    }

    bool is_dirty(int32_t col, int32_t row) const {
        if (col < 0 || col >= cols || row < 0 || row >= rows) return false;
        return dirty_flags[(size_t)row * cols + col];
    }

    int32_t dirty_count() const {
        int32_t count = 0;
        for (bool d : dirty_flags) {
            if (d) count++;
        }
        return count;
    }

    void invalidate_region(float x, float y, float w, float h) {
        int32_t c0 = std::max(0, (int32_t)std::floor(x / tile_size));
        int32_t r0 = std::max(0, (int32_t)std::floor(y / tile_size));
        int32_t c1 = std::min(cols - 1, (int32_t)std::floor((x + w) / tile_size));
        int32_t r1 = std::min(rows - 1, (int32_t)std::floor((y + h) / tile_size));

        for (int32_t r = r0; r <= r1; r++) {
            for (int32_t c = c0; c <= c1; c++) {
                dirty_flags[(size_t)r * cols + c] = true;
            }
        }
    }

    GopostRect tile_rect(int32_t col, int32_t row) const {
        float x = (float)(col * tile_size - (col > 0 ? overlap : 0));
        float y = (float)(row * tile_size - (row > 0 ? overlap : 0));
        float w = (float)tile_size + (col > 0 ? overlap : 0) +
                  (col < cols - 1 ? overlap : 0);
        float h_val = (float)tile_size + (row > 0 ? overlap : 0) +
                      (row < rows - 1 ? overlap : 0);

        w = std::min(w, (float)canvas_w - x);
        h_val = std::min(h_val, (float)canvas_h - y);

        return {x, y, w, h_val};
    }

    std::vector<TileCoord> visible_tiles(
        float vp_x, float vp_y, float vp_w, float vp_h, float zoom
    ) const {
        float world_x = vp_x / zoom;
        float world_y = vp_y / zoom;
        float world_w = vp_w / zoom;
        float world_h = vp_h / zoom;

        int32_t c0 = std::max(0, (int32_t)std::floor(world_x / tile_size));
        int32_t r0 = std::max(0, (int32_t)std::floor(world_y / tile_size));
        int32_t c1 = std::min(cols - 1, (int32_t)std::ceil((world_x + world_w) / tile_size));
        int32_t r1 = std::min(rows - 1, (int32_t)std::ceil((world_y + world_h) / tile_size));

        std::vector<TileCoord> result;
        for (int32_t r = r0; r <= r1; r++) {
            for (int32_t c = c0; c <= c1; c++) {
                result.push_back({c, r});
            }
        }
        return result;
    }

    std::vector<TileCoord> dirty_tiles() const {
        std::vector<TileCoord> result;
        for (int32_t r = 0; r < rows; r++) {
            for (int32_t c = 0; c < cols; c++) {
                if (is_dirty(c, r)) result.push_back({c, r});
            }
        }
        return result;
    }
};

/*
 * The TileGrid is embedded within the GopostCanvas struct (see canvas.cpp).
 * This file provides the algorithmic core. Integration with canvas.cpp is
 * done via the public C API (gopost_canvas_set_tile_size,
 * gopost_canvas_get_dirty_tile_count) which forward to the canvas's internal
 * tile grid.
 *
 * For Sprint 5, we keep the tile grid as a standalone utility that the
 * compositor will instantiate per-render pass. Full integration with the
 * canvas's dirty region tracking comes in Sprint 6.
 */
