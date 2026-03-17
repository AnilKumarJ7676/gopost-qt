#include <gtest/gtest.h>
#include "gopost/effects.h"
#include "gopost/types.h"

#include <cstdlib>
#include <cstring>

static GopostFrame make_frame(int w, int h) {
    GopostFrame f{};
    f.width = w;
    f.height = h;
    f.format = GOPOST_PIXEL_FORMAT_RGBA8;
    f.stride = w * 4;
    f.data_size = (uint32_t)(w * h * 4);
    f.data = static_cast<uint8_t*>(calloc(w * h * 4, 1));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            size_t off = ((size_t)y * w + x) * 4;
            f.data[off]     = (uint8_t)(x % 256);
            f.data[off + 1] = (uint8_t)(y % 256);
            f.data[off + 2] = (uint8_t)((x + y) % 256);
            f.data[off + 3] = 255;
        }
    }
    return f;
}

static void free_frame(GopostFrame& f) {
    free(f.data);
    f.data = nullptr;
}

TEST(ArtisticFilters, OilPaintRejectsNull) {
    EXPECT_EQ(gopost_effect_oil_paint(nullptr, 4, 50), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ArtisticFilters, OilPaintModifiesPixels) {
    auto f = make_frame(64, 64);
    auto orig = static_cast<uint8_t*>(malloc(f.data_size));
    memcpy(orig, f.data, f.data_size);

    EXPECT_EQ(gopost_effect_oil_paint(&f, 4, 50), GOPOST_OK);
    EXPECT_NE(memcmp(orig, f.data, f.data_size), 0);

    free(orig);
    free_frame(f);
}

TEST(ArtisticFilters, WatercolorRejectsNull) {
    EXPECT_EQ(gopost_effect_watercolor(nullptr, 5, 60), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ArtisticFilters, WatercolorModifiesPixels) {
    auto f = make_frame(64, 64);
    auto orig = static_cast<uint8_t*>(malloc(f.data_size));
    memcpy(orig, f.data, f.data_size);

    EXPECT_EQ(gopost_effect_watercolor(&f, 5, 60), GOPOST_OK);
    EXPECT_NE(memcmp(orig, f.data, f.data_size), 0);

    free(orig);
    free_frame(f);
}

TEST(ArtisticFilters, SketchRejectsNull) {
    EXPECT_EQ(gopost_effect_sketch(nullptr, 30, 80), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ArtisticFilters, SketchModifiesPixels) {
    auto f = make_frame(64, 64);
    auto orig = static_cast<uint8_t*>(malloc(f.data_size));
    memcpy(orig, f.data, f.data_size);

    EXPECT_EQ(gopost_effect_sketch(&f, 30, 80), GOPOST_OK);
    EXPECT_NE(memcmp(orig, f.data, f.data_size), 0);

    free(orig);
    free_frame(f);
}

TEST(ArtisticFilters, PixelateRejectsNull) {
    EXPECT_EQ(gopost_effect_pixelate(nullptr, 8), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ArtisticFilters, PixelateModifiesPixels) {
    auto f = make_frame(64, 64);
    auto orig = static_cast<uint8_t*>(malloc(f.data_size));
    memcpy(orig, f.data, f.data_size);

    EXPECT_EQ(gopost_effect_pixelate(&f, 8), GOPOST_OK);
    EXPECT_NE(memcmp(orig, f.data, f.data_size), 0);

    free(orig);
    free_frame(f);
}

TEST(ArtisticFilters, GlitchRejectsNull) {
    EXPECT_EQ(gopost_effect_glitch(nullptr, 30, 42), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ArtisticFilters, GlitchModifiesPixels) {
    auto f = make_frame(64, 64);
    auto orig = static_cast<uint8_t*>(malloc(f.data_size));
    memcpy(orig, f.data, f.data_size);

    EXPECT_EQ(gopost_effect_glitch(&f, 50, 42), GOPOST_OK);
    EXPECT_NE(memcmp(orig, f.data, f.data_size), 0);

    free(orig);
    free_frame(f);
}

TEST(ArtisticFilters, HalftoneRejectsNull) {
    EXPECT_EQ(gopost_effect_halftone(nullptr, 8, 45, 1.0f), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(ArtisticFilters, HalftoneModifiesPixels) {
    auto f = make_frame(64, 64);
    auto orig = static_cast<uint8_t*>(malloc(f.data_size));
    memcpy(orig, f.data, f.data_size);

    EXPECT_EQ(gopost_effect_halftone(&f, 8, 45, 1.0f), GOPOST_OK);
    EXPECT_NE(memcmp(orig, f.data, f.data_size), 0);

    free(orig);
    free_frame(f);
}

TEST(ArtisticFilters, HaltoneClampsDotSize) {
    auto f = make_frame(32, 32);
    EXPECT_EQ(gopost_effect_halftone(&f, 0.5f, 45, 1.0f), GOPOST_OK);
    EXPECT_EQ(gopost_effect_halftone(&f, 100.0f, 45, 1.0f), GOPOST_OK);
    free_frame(f);
}

TEST(ArtisticFilters, GlitchIsDeterministic) {
    auto f1 = make_frame(32, 32);
    auto f2 = make_frame(32, 32);

    EXPECT_EQ(gopost_effect_glitch(&f1, 50, 42), GOPOST_OK);
    EXPECT_EQ(gopost_effect_glitch(&f2, 50, 42), GOPOST_OK);
    EXPECT_EQ(memcmp(f1.data, f2.data, f1.data_size), 0);

    free_frame(f1);
    free_frame(f2);
}

TEST(ArtisticFilters, EffectRegistryContainsHalftone) {
    GopostEffectDef def{};
    EXPECT_EQ(gopost_effects_init(nullptr), GOPOST_OK);
    EXPECT_EQ(gopost_effects_find(nullptr, "gp.artistic.halftone", &def), GOPOST_OK);
    EXPECT_EQ(def.category, GOPOST_EFFECT_CAT_STYLIZE);
    EXPECT_EQ(def.param_count, 3);
    gopost_effects_shutdown(nullptr);
}

TEST(ArtisticFilters, ProcessDispatchesHalftone) {
    EXPECT_EQ(gopost_effects_init(nullptr), GOPOST_OK);

    auto in_f = make_frame(32, 32);
    auto out_f = make_frame(32, 32);

    GopostEffectInstance inst{};
    inst.instance_id = 1;
    strncpy(inst.effect_id, "gp.artistic.halftone", sizeof(inst.effect_id) - 1);
    inst.enabled = 1;
    inst.mix = 1.0f;
    inst.param_values[0] = 8.0f;
    inst.param_values[1] = 45.0f;
    inst.param_values[2] = 100.0f;

    EXPECT_EQ(gopost_effects_process(nullptr, &inst, &in_f, &out_f), GOPOST_OK);
    EXPECT_NE(memcmp(in_f.data, out_f.data, in_f.data_size), 0);

    gopost_effects_shutdown(nullptr);
    free_frame(in_f);
    free_frame(out_f);
}

TEST(ArtisticFilters, GpuShaderSourceAvailable) {
    const char* src = gopost_artistic_compute_shader_source();
    EXPECT_NE(src, nullptr);
#if defined(__APPLE__)
    EXPECT_GT(strlen(src), 100u);
#endif
}
