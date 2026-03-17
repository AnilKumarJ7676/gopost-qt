#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "gopost/gpu_context_impl.h"

// ── Mock GPU context for verifying call sequences ────────────────────────────

namespace gopost {

class MockGpuContext : public IGpuContext {
public:
    MOCK_METHOD(GopostGpuBackend, backend, (), (const, override));
    MOCK_METHOD(GopostError, initialize, (), (override));

    /* Legacy texture API */
    MOCK_METHOD(GopostError, createTexture,
        (uint32_t, uint32_t, GopostPixelFormat, void**), (override));
    MOCK_METHOD(GopostError, destroyTexture, (void*), (override));
    MOCK_METHOD(GopostError, uploadTexture, (void*, const uint8_t*, size_t), (override));

    /* Extended texture API */
    MOCK_METHOD(GopostError, createTextureWithDesc,
        (const GopostTextureDesc&, GopostTextureHandle*), (override));
    MOCK_METHOD(GopostError, destroyTextureByHandle, (GopostTextureHandle), (override));
    MOCK_METHOD(GopostError, uploadTextureByHandle,
        (GopostTextureHandle, const uint8_t*, size_t), (override));
    MOCK_METHOD(GopostError, readPixels,
        (GopostTextureHandle, uint8_t*, size_t), (override));

    /* Framebuffer */
    MOCK_METHOD(GopostError, createFramebuffer,
        (GopostTextureHandle, GopostFramebufferHandle*), (override));
    MOCK_METHOD(GopostError, destroyFramebuffer, (GopostFramebufferHandle), (override));

    /* Shader */
    MOCK_METHOD(GopostError, loadShader,
        (const GopostShaderDesc&, GopostShaderHandle*), (override));
    MOCK_METHOD(GopostError, destroyShader, (GopostShaderHandle), (override));

    /* Rendering pipeline */
    MOCK_METHOD(GopostError, beginFrame, (), (override));
    MOCK_METHOD(GopostError, endFrame, (), (override));
    MOCK_METHOD(GopostError, present, (), (override));
    MOCK_METHOD(GopostError, bindFramebuffer, (GopostFramebufferHandle), (override));
    MOCK_METHOD(GopostError, bindShader, (GopostShaderHandle), (override));
    MOCK_METHOD(GopostError, setUniform,
        (const char*, const GopostUniformValue&), (override));
    MOCK_METHOD(GopostError, bindTexture, (GopostTextureHandle, uint32_t), (override));
    MOCK_METHOD(GopostError, drawFullscreenQuad, (), (override));
    MOCK_METHOD(GopostError, clearFramebuffer, (float, float, float, float), (override));

    /* Shared texture */
    MOCK_METHOD(GopostError, createSharedTexture,
        (const GopostTextureDesc&, GopostTextureHandle*, void**), (override));
};

}  // namespace gopost

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

// ── Test 1: Verify compositor call sequence ──────────────────────────────────

TEST(GraphicsBackendMockTest, CompositorCallSequence) {
    gopost::MockGpuContext mock;
    InSequence seq;

    EXPECT_CALL(mock, beginFrame()).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, bindFramebuffer(_)).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, clearFramebuffer(0.f, 0.f, 0.f, 0.f)).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, bindShader(_)).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, bindTexture(_, 0)).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, setUniform(_, _)).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, drawFullscreenQuad()).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, endFrame()).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, present()).WillOnce(Return(GOPOST_OK));

    // Simulate a single GPU composite pass
    EXPECT_EQ(mock.beginFrame(), GOPOST_OK);
    EXPECT_EQ(mock.bindFramebuffer(1), GOPOST_OK);
    EXPECT_EQ(mock.clearFramebuffer(0.f, 0.f, 0.f, 0.f), GOPOST_OK);
    EXPECT_EQ(mock.bindShader(1), GOPOST_OK);
    EXPECT_EQ(mock.bindTexture(1, 0), GOPOST_OK);

    GopostUniformValue uv{};
    uv.type = GOPOST_UNIFORM_FLOAT;
    uv.value.f = 1.0f;
    EXPECT_EQ(mock.setUniform("opacity", uv), GOPOST_OK);

    EXPECT_EQ(mock.drawFullscreenQuad(), GOPOST_OK);
    EXPECT_EQ(mock.endFrame(), GOPOST_OK);
    EXPECT_EQ(mock.present(), GOPOST_OK);
}

// ── Test 2: Resource lifecycle (create/destroy pairing) ──────────────────────

TEST(GraphicsBackendMockTest, ResourceLifecycle) {
    gopost::MockGpuContext mock;

    GopostTextureHandle texHandle = 42;
    GopostFramebufferHandle fbHandle = 7;
    GopostShaderHandle shHandle = 3;

    EXPECT_CALL(mock, createTextureWithDesc(_, _))
        .WillOnce([&](const GopostTextureDesc&, GopostTextureHandle* h) {
            *h = texHandle;
            return GOPOST_OK;
        });
    EXPECT_CALL(mock, createFramebuffer(texHandle, _))
        .WillOnce([&](GopostTextureHandle, GopostFramebufferHandle* h) {
            *h = fbHandle;
            return GOPOST_OK;
        });
    EXPECT_CALL(mock, loadShader(_, _))
        .WillOnce([&](const GopostShaderDesc&, GopostShaderHandle* h) {
            *h = shHandle;
            return GOPOST_OK;
        });
    EXPECT_CALL(mock, destroyShader(shHandle)).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, destroyFramebuffer(fbHandle)).WillOnce(Return(GOPOST_OK));
    EXPECT_CALL(mock, destroyTextureByHandle(texHandle)).WillOnce(Return(GOPOST_OK));

    // Create resources
    GopostTextureDesc desc{};
    desc.width = 1920;
    desc.height = 1080;
    desc.format = GOPOST_PIXEL_FORMAT_RGBA8;
    desc.usage = GOPOST_TEXTURE_USAGE_RENDER_TARGET | GOPOST_TEXTURE_USAGE_SAMPLED;

    GopostTextureHandle h = GOPOST_NULL_HANDLE;
    EXPECT_EQ(mock.createTextureWithDesc(desc, &h), GOPOST_OK);
    EXPECT_EQ(h, texHandle);

    GopostFramebufferHandle fh = GOPOST_NULL_HANDLE;
    EXPECT_EQ(mock.createFramebuffer(h, &fh), GOPOST_OK);
    EXPECT_EQ(fh, fbHandle);

    GopostShaderDesc sdesc{};
    sdesc.stage = GOPOST_SHADER_STAGE_FRAGMENT;
    sdesc.format = GOPOST_SHADER_FORMAT_SOURCE;
    sdesc.data = "void main() {}";
    sdesc.data_size = 15;
    sdesc.entry_point = nullptr;

    GopostShaderHandle sh = GOPOST_NULL_HANDLE;
    EXPECT_EQ(mock.loadShader(sdesc, &sh), GOPOST_OK);
    EXPECT_EQ(sh, shHandle);

    // Destroy in reverse order
    EXPECT_EQ(mock.destroyShader(sh), GOPOST_OK);
    EXPECT_EQ(mock.destroyFramebuffer(fh), GOPOST_OK);
    EXPECT_EQ(mock.destroyTextureByHandle(h), GOPOST_OK);
}

// ── Test 3: Error propagation ────────────────────────────────────────────────

TEST(GraphicsBackendMockTest, ErrorPropagation) {
    gopost::MockGpuContext mock;

    EXPECT_CALL(mock, createTextureWithDesc(_, _))
        .WillOnce(Return(GOPOST_ERROR_OUT_OF_MEMORY));
    EXPECT_CALL(mock, loadShader(_, _))
        .WillOnce(Return(GOPOST_ERROR_SHADER_COMPILE));
    EXPECT_CALL(mock, beginFrame())
        .WillOnce(Return(GOPOST_ERROR_NOT_INITIALIZED));

    GopostTextureDesc desc{};
    desc.width = 8192;
    desc.height = 8192;
    desc.format = GOPOST_PIXEL_FORMAT_RGBA8;
    desc.usage = GOPOST_TEXTURE_USAGE_RENDER_TARGET;

    GopostTextureHandle h = GOPOST_NULL_HANDLE;
    EXPECT_EQ(mock.createTextureWithDesc(desc, &h), GOPOST_ERROR_OUT_OF_MEMORY);

    GopostShaderDesc sdesc{};
    sdesc.stage = GOPOST_SHADER_STAGE_FRAGMENT;
    sdesc.format = GOPOST_SHADER_FORMAT_SOURCE;
    sdesc.data = "invalid shader";
    sdesc.data_size = 14;

    GopostShaderHandle sh = GOPOST_NULL_HANDLE;
    EXPECT_EQ(mock.loadShader(sdesc, &sh), GOPOST_ERROR_SHADER_COMPILE);

    EXPECT_EQ(mock.beginFrame(), GOPOST_ERROR_NOT_INITIALIZED);
}
