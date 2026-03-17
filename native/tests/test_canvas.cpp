#include <gtest/gtest.h>
#include "gopost/canvas.h"

TEST(Canvas, CreateRejectsNull) {
    EXPECT_EQ(gopost_canvas_create(nullptr, nullptr, nullptr), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, CreateRejectsInvalidSize) {
    GopostCanvasConfig cfg = {};
    cfg.width = -1;
    cfg.height = 100;
    GopostCanvas* canvas = nullptr;
    EXPECT_EQ(gopost_canvas_create((GopostEngine*)1, &cfg, &canvas), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, CreateRejectsOversized) {
    GopostCanvasConfig cfg = {};
    cfg.width = 20000;
    cfg.height = 20000;
    GopostCanvas* canvas = nullptr;
    EXPECT_EQ(gopost_canvas_create((GopostEngine*)1, &cfg, &canvas), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, DestroyRejectsNull) {
    EXPECT_EQ(gopost_canvas_destroy(nullptr), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, AddImageLayerRejectsNull) {
    int32_t id;
    EXPECT_EQ(gopost_canvas_add_image_layer(nullptr, nullptr, 0, 0, 0, &id), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, RemoveLayerRejectsNull) {
    EXPECT_EQ(gopost_canvas_remove_layer(nullptr, 1), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, SetVisibleRejectsNull) {
    EXPECT_EQ(gopost_layer_set_visible(nullptr, 1, 1), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, SetOpacityRejectsNull) {
    EXPECT_EQ(gopost_layer_set_opacity(nullptr, 1, 0.5f), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, RenderRejectsNull) {
    GopostFrame* frame = nullptr;
    EXPECT_EQ(gopost_canvas_render(nullptr, &frame), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, SetViewportRejectsNull) {
    EXPECT_EQ(gopost_canvas_set_viewport(nullptr, 0, 0, 1, 0), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(Canvas, SetTileSizeRejectsInvalid) {
    EXPECT_EQ(gopost_canvas_set_tile_size(nullptr, 2048), GOPOST_ERROR_INVALID_ARGUMENT);
}
