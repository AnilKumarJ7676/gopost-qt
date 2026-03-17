#include <gtest/gtest.h>
#include "gopost/gpu_context.h"

TEST(GpuContextTest, CreateAndDestroy) {
    GopostGpuContext* ctx = nullptr;
    GopostError err = gopost_gpu_create_for_platform(&ctx);
    ASSERT_EQ(err, GOPOST_OK);
    ASSERT_NE(ctx, nullptr);

    GopostGpuBackend backend = gopost_gpu_get_backend(ctx);
    EXPECT_NE(backend, GOPOST_GPU_BACKEND_NONE);

    err = gopost_gpu_destroy(ctx);
    EXPECT_EQ(err, GOPOST_OK);
}

TEST(GpuContextTest, CreateAndDestroyTexture) {
    GopostGpuContext* ctx = nullptr;
    ASSERT_EQ(gopost_gpu_create_for_platform(&ctx), GOPOST_OK);

    GopostTexture* texture = nullptr;
    GopostError err = gopost_gpu_create_texture(ctx, &texture, 1920, 1080, GOPOST_PIXEL_FORMAT_RGBA8);
    ASSERT_EQ(err, GOPOST_OK);
    ASSERT_NE(texture, nullptr);

    err = gopost_gpu_destroy_texture(ctx, texture);
    EXPECT_EQ(err, GOPOST_OK);

    gopost_gpu_destroy(ctx);
}

TEST(GpuContextTest, NullArguments) {
    EXPECT_EQ(gopost_gpu_create_for_platform(nullptr), GOPOST_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(gopost_gpu_destroy(nullptr), GOPOST_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(gopost_gpu_get_backend(nullptr), GOPOST_GPU_BACKEND_NONE);
}
