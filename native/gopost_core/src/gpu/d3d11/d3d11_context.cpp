#include "gopost/gpu_context_impl.h"

#if defined(GOPOST_PLATFORM_WINDOWS)

namespace gopost {

class D3D11Context : public IGpuContext {
public:
    GopostGpuBackend backend() const override { return GOPOST_GPU_BACKEND_D3D11; }

    GopostError initialize() override {
        initialized_ = true;
        return GOPOST_OK;
    }

    /* ── Legacy texture API ───────────────────────────────────────────────── */

    GopostError createTexture(uint32_t /*width*/, uint32_t /*height*/,
        GopostPixelFormat /*format*/, void** outHandle) override {
        if (!initialized_) return GOPOST_ERROR_NOT_INITIALIZED;
        *outHandle = reinterpret_cast<void*>(static_cast<intptr_t>(nextTexH_++));
        return GOPOST_OK;
    }

    GopostError destroyTexture(void* /*handle*/) override {
        return GOPOST_OK;
    }

    GopostError uploadTexture(void* handle, const uint8_t* /*data*/, size_t /*size*/) override {
        if (!handle) return GOPOST_ERROR_INVALID_ARGUMENT;
        return GOPOST_OK;
    }

    /* ── Extended texture API ─────────────────────────────────────────────── */

    GopostError createTextureWithDesc(const GopostTextureDesc& /*desc*/,
        GopostTextureHandle* outHandle) override {
        if (!initialized_) return GOPOST_ERROR_NOT_INITIALIZED;
        *outHandle = nextTexH_++;
        return GOPOST_OK;
    }

    GopostError destroyTextureByHandle(GopostTextureHandle /*handle*/) override {
        return GOPOST_OK;
    }

    GopostError uploadTextureByHandle(GopostTextureHandle handle,
        const uint8_t* /*data*/, size_t /*size*/) override {
        if (handle == GOPOST_NULL_HANDLE) return GOPOST_ERROR_INVALID_ARGUMENT;
        return GOPOST_OK;
    }

    GopostError readPixels(GopostTextureHandle handle,
        uint8_t* /*outData*/, size_t /*bufferSize*/) override {
        if (handle == GOPOST_NULL_HANDLE) return GOPOST_ERROR_INVALID_ARGUMENT;
        return GOPOST_OK;
    }

    /* ── Framebuffer ──────────────────────────────────────────────────────── */

    GopostError createFramebuffer(GopostTextureHandle /*colorAttachment*/,
        GopostFramebufferHandle* outHandle) override {
        if (!initialized_) return GOPOST_ERROR_NOT_INITIALIZED;
        *outHandle = nextFbH_++;
        return GOPOST_OK;
    }

    GopostError destroyFramebuffer(GopostFramebufferHandle /*handle*/) override {
        return GOPOST_OK;
    }

    /* ── Shader ───────────────────────────────────────────────────────────── */

    GopostError loadShader(const GopostShaderDesc& /*desc*/,
        GopostShaderHandle* outHandle) override {
        if (!initialized_) return GOPOST_ERROR_NOT_INITIALIZED;
        *outHandle = nextShH_++;
        return GOPOST_OK;
    }

    GopostError destroyShader(GopostShaderHandle /*handle*/) override {
        return GOPOST_OK;
    }

    /* ── Rendering pipeline ───────────────────────────────────────────────── */

    GopostError beginFrame() override {
        if (!initialized_) return GOPOST_ERROR_NOT_INITIALIZED;
        return GOPOST_OK;
    }

    GopostError endFrame() override { return GOPOST_OK; }
    GopostError present() override { return GOPOST_OK; }

    GopostError bindFramebuffer(GopostFramebufferHandle /*handle*/) override {
        return GOPOST_OK;
    }

    GopostError bindShader(GopostShaderHandle /*handle*/) override {
        return GOPOST_OK;
    }

    GopostError setUniform(const char* /*name*/, const GopostUniformValue& /*value*/) override {
        return GOPOST_OK;
    }

    GopostError bindTexture(GopostTextureHandle /*handle*/, uint32_t /*slot*/) override {
        return GOPOST_OK;
    }

    GopostError drawFullscreenQuad() override { return GOPOST_OK; }

    GopostError clearFramebuffer(float /*r*/, float /*g*/, float /*b*/, float /*a*/) override {
        return GOPOST_OK;
    }

    /* ── Shared texture ───────────────────────────────────────────────────── */

    GopostError createSharedTexture(const GopostTextureDesc& /*desc*/,
        GopostTextureHandle* outHandle, void** nativeHandle) override {
        if (!initialized_) return GOPOST_ERROR_NOT_INITIALIZED;
        *outHandle = nextTexH_++;
        if (nativeHandle) *nativeHandle = nullptr;
        return GOPOST_OK;
    }

private:
    bool initialized_ = false;
    uint64_t nextTexH_ = 1;
    uint64_t nextFbH_ = 1;
    uint64_t nextShH_ = 1;
};

}  // namespace gopost

#endif
