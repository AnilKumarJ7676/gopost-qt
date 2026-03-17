#ifndef GOPOST_GPU_CONTEXT_H
#define GOPOST_GPU_CONTEXT_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GOPOST_GPU_BACKEND_NONE = 0,
    GOPOST_GPU_BACKEND_METAL = 1,
    GOPOST_GPU_BACKEND_VULKAN = 2,
    GOPOST_GPU_BACKEND_GLES = 3,
    GOPOST_GPU_BACKEND_WEBGL = 4,
    GOPOST_GPU_BACKEND_D3D11 = 5,
} GopostGpuBackend;

typedef struct GopostGpuContext GopostGpuContext;
typedef struct GopostTexture GopostTexture;
typedef struct GopostPipeline GopostPipeline;

GopostError gopost_gpu_create_for_platform(GopostGpuContext** ctx);
GopostError gopost_gpu_destroy(GopostGpuContext* ctx);
GopostGpuBackend gopost_gpu_get_backend(GopostGpuContext* ctx);

GopostError gopost_gpu_create_texture(GopostGpuContext* ctx, GopostTexture** texture,
    uint32_t width, uint32_t height, GopostPixelFormat format);
GopostError gopost_gpu_destroy_texture(GopostGpuContext* ctx, GopostTexture* texture);

GopostError gopost_gpu_upload_texture(GopostGpuContext* ctx, GopostTexture* texture,
    const uint8_t* data, size_t data_size);

/* ── Extended texture API (typed handles) ─────────────────────────────────── */

GopostError gopost_gpu_create_texture_ex(GopostGpuContext* ctx,
    const GopostTextureDesc* desc, GopostTextureHandle* out_handle);
GopostError gopost_gpu_destroy_texture_ex(GopostGpuContext* ctx,
    GopostTextureHandle handle);
GopostError gopost_gpu_upload_texture_ex(GopostGpuContext* ctx,
    GopostTextureHandle handle, const uint8_t* data, size_t data_size);
GopostError gopost_gpu_read_pixels(GopostGpuContext* ctx,
    GopostTextureHandle handle, uint8_t* out_data, size_t buffer_size);

/* ── Framebuffer ──────────────────────────────────────────────────────────── */

GopostError gopost_gpu_create_framebuffer(GopostGpuContext* ctx,
    GopostTextureHandle color_attachment, GopostFramebufferHandle* out_handle);
GopostError gopost_gpu_destroy_framebuffer(GopostGpuContext* ctx,
    GopostFramebufferHandle handle);

/* ── Shader ───────────────────────────────────────────────────────────────── */

GopostError gopost_gpu_load_shader(GopostGpuContext* ctx,
    const GopostShaderDesc* desc, GopostShaderHandle* out_handle);
GopostError gopost_gpu_destroy_shader(GopostGpuContext* ctx,
    GopostShaderHandle handle);

/* ── Rendering pipeline ───────────────────────────────────────────────────── */

GopostError gopost_gpu_begin_frame(GopostGpuContext* ctx);
GopostError gopost_gpu_end_frame(GopostGpuContext* ctx);
GopostError gopost_gpu_present(GopostGpuContext* ctx);
GopostError gopost_gpu_bind_framebuffer(GopostGpuContext* ctx,
    GopostFramebufferHandle handle);
GopostError gopost_gpu_bind_shader(GopostGpuContext* ctx,
    GopostShaderHandle handle);
GopostError gopost_gpu_set_uniform(GopostGpuContext* ctx,
    const char* name, const GopostUniformValue* value);
GopostError gopost_gpu_bind_texture(GopostGpuContext* ctx,
    GopostTextureHandle handle, uint32_t slot);
GopostError gopost_gpu_draw_fullscreen_quad(GopostGpuContext* ctx);
GopostError gopost_gpu_clear_framebuffer(GopostGpuContext* ctx,
    float r, float g, float b, float a);

/* ── Shared texture (zero-copy display) ───────────────────────────────────── */

GopostError gopost_gpu_create_shared_texture(GopostGpuContext* ctx,
    const GopostTextureDesc* desc, GopostTextureHandle* out_handle,
    void** native_handle);

#ifdef __cplusplus
}
#endif

#endif
