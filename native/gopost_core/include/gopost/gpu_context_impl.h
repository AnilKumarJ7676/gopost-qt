#pragma once
#include "gopost/gpu_context.h"
#include "gopost/types.h"
#include <memory>

namespace gopost {

class IGpuContext {
public:
    virtual ~IGpuContext() = default;

    /* ── Existing: backend identity & init ─────────────────────────────────── */
    virtual GopostGpuBackend backend() const = 0;
    virtual GopostError initialize() = 0;

    /* ── Existing: legacy texture API (void* handles) ─────────────────────── */
    virtual GopostError createTexture(uint32_t width, uint32_t height,
        GopostPixelFormat format, void** outHandle) = 0;
    virtual GopostError destroyTexture(void* handle) = 0;
    virtual GopostError uploadTexture(void* handle, const uint8_t* data, size_t size) = 0;

    /* ── Extended texture API (typed handles) ─────────────────────────────── */
    virtual GopostError createTextureWithDesc(const GopostTextureDesc& desc,
        GopostTextureHandle* outHandle) = 0;
    virtual GopostError destroyTextureByHandle(GopostTextureHandle handle) = 0;
    virtual GopostError uploadTextureByHandle(GopostTextureHandle handle,
        const uint8_t* data, size_t size) = 0;
    virtual GopostError readPixels(GopostTextureHandle handle,
        uint8_t* outData, size_t bufferSize) = 0;

    /* ── Framebuffer ──────────────────────────────────────────────────────── */
    virtual GopostError createFramebuffer(GopostTextureHandle colorAttachment,
        GopostFramebufferHandle* outHandle) = 0;
    virtual GopostError destroyFramebuffer(GopostFramebufferHandle handle) = 0;

    /* ── Shader ───────────────────────────────────────────────────────────── */
    virtual GopostError loadShader(const GopostShaderDesc& desc,
        GopostShaderHandle* outHandle) = 0;
    virtual GopostError destroyShader(GopostShaderHandle handle) = 0;

    /* ── Rendering pipeline ───────────────────────────────────────────────── */
    virtual GopostError beginFrame() = 0;
    virtual GopostError endFrame() = 0;
    virtual GopostError present() = 0;
    virtual GopostError bindFramebuffer(GopostFramebufferHandle handle) = 0;
    virtual GopostError bindShader(GopostShaderHandle handle) = 0;
    virtual GopostError setUniform(const char* name, const GopostUniformValue& value) = 0;
    virtual GopostError bindTexture(GopostTextureHandle handle, uint32_t slot) = 0;
    virtual GopostError drawFullscreenQuad() = 0;
    virtual GopostError clearFramebuffer(float r, float g, float b, float a) = 0;

    /* ── Shared texture (zero-copy display) ───────────────────────────────── */
    virtual GopostError createSharedTexture(const GopostTextureDesc& desc,
        GopostTextureHandle* outHandle, void** nativeHandle) = 0;

    static std::unique_ptr<IGpuContext> createForPlatform();
};

}  // namespace gopost

struct GopostGpuContext {
    std::unique_ptr<gopost::IGpuContext> impl;
};

struct GopostTexture {
    void* handle = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    GopostPixelFormat format = GOPOST_PIXEL_FORMAT_RGBA8;
};
