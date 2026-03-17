#include "gopost/gpu_context.h"
#include "gopost/gpu_context_impl.h"

#if defined(GOPOST_PLATFORM_IOS) || defined(GOPOST_PLATFORM_MACOS)
#include "metal/metal_context.cpp"
#endif

#if defined(GOPOST_PLATFORM_WINDOWS) || defined(GOPOST_PLATFORM_LINUX) || defined(GOPOST_PLATFORM_ANDROID)
#include "vulkan/vulkan_context.cpp"
#endif

#if defined(GOPOST_PLATFORM_WINDOWS)
#include "d3d11/d3d11_context.cpp"
#endif

#include "gles/gles_context.cpp"

namespace gopost {

/// Try to initialize a backend; returns nullptr on failure so the caller
/// can fall through to the next candidate in the preference chain.
template <typename T>
static std::unique_ptr<IGpuContext> tryBackend() {
    auto ctx = std::make_unique<T>();
    if (ctx->initialize() == GOPOST_OK) return ctx;
    return nullptr;
}

std::unique_ptr<IGpuContext> IGpuContext::createForPlatform() {
    // Platform preference chains with automatic fallback.

#if defined(GOPOST_PLATFORM_IOS) || defined(GOPOST_PLATFORM_MACOS)
    // Apple: Metal (only viable option — GLES deprecated)
    if (auto ctx = tryBackend<MetalContext>()) return ctx;

#elif defined(GOPOST_PLATFORM_WINDOWS)
    // Windows: D3D11 (broadest driver compat) → Vulkan → GLES
    if (auto ctx = tryBackend<D3D11Context>())  return ctx;
    if (auto ctx = tryBackend<VulkanContext>())  return ctx;

#elif defined(GOPOST_PLATFORM_LINUX)
    // Linux: Vulkan → GLES
    if (auto ctx = tryBackend<VulkanContext>())  return ctx;

#elif defined(GOPOST_PLATFORM_ANDROID)
    // Android: Vulkan (API 26+) → GLES (legacy devices)
    if (auto ctx = tryBackend<VulkanContext>())  return ctx;

#endif

    // Universal fallback: GLES (also covers WASM/WebGL)
    if (auto ctx = tryBackend<GLESContext>()) return ctx;

    // Last resort: uninitialized — will report error on first use
    return std::make_unique<GLESContext>();
}

}  // namespace gopost

extern "C" {

/* ── Existing C API (unchanged) ───────────────────────────────────────────── */

GopostError gopost_gpu_create_for_platform(GopostGpuContext** ctx) {
    if (!ctx) return GOPOST_ERROR_INVALID_ARGUMENT;

    auto* context = new (std::nothrow) GopostGpuContext{};
    if (!context) return GOPOST_ERROR_OUT_OF_MEMORY;

    // createForPlatform() already initializes via the tryBackend() chain.
    context->impl = gopost::IGpuContext::createForPlatform();
    if (!context->impl) {
        delete context;
        return GOPOST_ERROR_GPU_NOT_AVAILABLE;
    }

    *ctx = context;
    return GOPOST_OK;
}

GopostError gopost_gpu_destroy(GopostGpuContext* ctx) {
    if (!ctx) return GOPOST_ERROR_INVALID_ARGUMENT;
    delete ctx;
    return GOPOST_OK;
}

GopostGpuBackend gopost_gpu_get_backend(GopostGpuContext* ctx) {
    if (!ctx || !ctx->impl) return GOPOST_GPU_BACKEND_NONE;
    return ctx->impl->backend();
}

GopostError gopost_gpu_create_texture(GopostGpuContext* ctx, GopostTexture** texture,
    uint32_t width, uint32_t height, GopostPixelFormat format) {
    if (!ctx || !ctx->impl || !texture) return GOPOST_ERROR_INVALID_ARGUMENT;

    auto* tex = new (std::nothrow) GopostTexture{};
    if (!tex) return GOPOST_ERROR_OUT_OF_MEMORY;

    tex->width = width;
    tex->height = height;
    tex->format = format;

    GopostError err = ctx->impl->createTexture(width, height, format, &tex->handle);
    if (err != GOPOST_OK) {
        delete tex;
        return err;
    }

    *texture = tex;
    return GOPOST_OK;
}

GopostError gopost_gpu_destroy_texture(GopostGpuContext* ctx, GopostTexture* texture) {
    if (!ctx || !ctx->impl || !texture) return GOPOST_ERROR_INVALID_ARGUMENT;
    ctx->impl->destroyTexture(texture->handle);
    delete texture;
    return GOPOST_OK;
}

GopostError gopost_gpu_upload_texture(GopostGpuContext* ctx, GopostTexture* texture,
    const uint8_t* data, size_t data_size) {
    if (!ctx || !ctx->impl || !texture || !data) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->uploadTexture(texture->handle, data, data_size);
}

/* ── Extended texture API ─────────────────────────────────────────────────── */

GopostError gopost_gpu_create_texture_ex(GopostGpuContext* ctx,
    const GopostTextureDesc* desc, GopostTextureHandle* out_handle) {
    if (!ctx || !ctx->impl || !desc || !out_handle) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->createTextureWithDesc(*desc, out_handle);
}

GopostError gopost_gpu_destroy_texture_ex(GopostGpuContext* ctx,
    GopostTextureHandle handle) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->destroyTextureByHandle(handle);
}

GopostError gopost_gpu_upload_texture_ex(GopostGpuContext* ctx,
    GopostTextureHandle handle, const uint8_t* data, size_t data_size) {
    if (!ctx || !ctx->impl || !data) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->uploadTextureByHandle(handle, data, data_size);
}

GopostError gopost_gpu_read_pixels(GopostGpuContext* ctx,
    GopostTextureHandle handle, uint8_t* out_data, size_t buffer_size) {
    if (!ctx || !ctx->impl || !out_data) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->readPixels(handle, out_data, buffer_size);
}

/* ── Framebuffer ──────────────────────────────────────────────────────────── */

GopostError gopost_gpu_create_framebuffer(GopostGpuContext* ctx,
    GopostTextureHandle color_attachment, GopostFramebufferHandle* out_handle) {
    if (!ctx || !ctx->impl || !out_handle) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->createFramebuffer(color_attachment, out_handle);
}

GopostError gopost_gpu_destroy_framebuffer(GopostGpuContext* ctx,
    GopostFramebufferHandle handle) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->destroyFramebuffer(handle);
}

/* ── Shader ───────────────────────────────────────────────────────────────── */

GopostError gopost_gpu_load_shader(GopostGpuContext* ctx,
    const GopostShaderDesc* desc, GopostShaderHandle* out_handle) {
    if (!ctx || !ctx->impl || !desc || !out_handle) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->loadShader(*desc, out_handle);
}

GopostError gopost_gpu_destroy_shader(GopostGpuContext* ctx,
    GopostShaderHandle handle) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->destroyShader(handle);
}

/* ── Rendering pipeline ───────────────────────────────────────────────────── */

GopostError gopost_gpu_begin_frame(GopostGpuContext* ctx) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->beginFrame();
}

GopostError gopost_gpu_end_frame(GopostGpuContext* ctx) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->endFrame();
}

GopostError gopost_gpu_present(GopostGpuContext* ctx) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->present();
}

GopostError gopost_gpu_bind_framebuffer(GopostGpuContext* ctx,
    GopostFramebufferHandle handle) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->bindFramebuffer(handle);
}

GopostError gopost_gpu_bind_shader(GopostGpuContext* ctx,
    GopostShaderHandle handle) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->bindShader(handle);
}

GopostError gopost_gpu_set_uniform(GopostGpuContext* ctx,
    const char* name, const GopostUniformValue* value) {
    if (!ctx || !ctx->impl || !name || !value) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->setUniform(name, *value);
}

GopostError gopost_gpu_bind_texture(GopostGpuContext* ctx,
    GopostTextureHandle handle, uint32_t slot) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->bindTexture(handle, slot);
}

GopostError gopost_gpu_draw_fullscreen_quad(GopostGpuContext* ctx) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->drawFullscreenQuad();
}

GopostError gopost_gpu_clear_framebuffer(GopostGpuContext* ctx,
    float r, float g, float b, float a) {
    if (!ctx || !ctx->impl) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->clearFramebuffer(r, g, b, a);
}

/* ── Shared texture ───────────────────────────────────────────────────────── */

GopostError gopost_gpu_create_shared_texture(GopostGpuContext* ctx,
    const GopostTextureDesc* desc, GopostTextureHandle* out_handle,
    void** native_handle) {
    if (!ctx || !ctx->impl || !desc || !out_handle) return GOPOST_ERROR_INVALID_ARGUMENT;
    return ctx->impl->createSharedTexture(*desc, out_handle, native_handle);
}

}  /* extern "C" */
