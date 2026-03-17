
## VE-6. GPU Rendering Pipeline

### 6.1 Render Graph Architecture

```cpp
namespace gp::rendering {

// Render graph: DAG of render passes compiled each frame
class RenderGraph {
public:
    struct TextureResource {
        int32_t id;
        int width, height;
        PixelFormat format;
        bool is_transient;      // Can be aliased and recycled
    };

    struct RenderPass {
        int32_t id;
        std::string name;
        std::vector<int32_t> inputs;     // Texture resources read
        std::vector<int32_t> outputs;    // Texture resources written
        std::function<void(GpuCommandBuffer&)> execute;
    };

    int32_t create_texture(int w, int h, PixelFormat format, bool transient = true);
    int32_t add_pass(const std::string& name,
                     std::vector<int32_t> inputs,
                     std::vector<int32_t> outputs,
                     std::function<void(GpuCommandBuffer&)> execute_fn);

    void compile();    // Topological sort, barrier insertion, resource aliasing
    void execute(IGpuContext& gpu);

private:
    std::vector<TextureResource> textures_;
    std::vector<RenderPass> passes_;
    std::vector<int32_t> execution_order_;    // Compiled order
    ResourceAliasingTable aliasing_;          // Transient texture reuse
};

} // namespace gp::rendering
```

### 6.2 GPU Abstraction Layer

```cpp
namespace gp::rendering {

enum class ShaderStage { Vertex, Fragment, Compute };
enum class PixelFormat {
    RGBA8, RGBA16F, RGBA32F,   // Color
    R8, R16F, R32F,            // Single channel
    NV12, YUV420P,             // Video formats
    Depth32F, Depth24Stencil8  // Depth/stencil
};

struct ShaderModule {
    ShaderStage stage;
    std::vector<uint8_t> bytecode;   // SPIR-V for Vulkan, Metal IR, GLSL source
};

struct PipelineDescriptor {
    ShaderModule vertex;
    ShaderModule fragment;
    VertexLayout vertex_layout;
    BlendState blend_state;
    DepthStencilState depth_stencil;
    RasterizerState rasterizer;
    std::vector<BindGroupLayout> bind_groups;
};

struct ComputePipelineDescriptor {
    ShaderModule compute;
    std::vector<BindGroupLayout> bind_groups;
    Vec3i workgroup_size;
};

class IGpuContext {
public:
    virtual ~IGpuContext() = default;

    virtual bool initialize(void* native_window = nullptr) = 0;
    virtual void shutdown() = 0;
    virtual GpuBackend backend() const = 0;  // Metal, Vulkan, DX12, GL

    // Resource creation
    virtual GpuTexture create_texture(const TextureDescriptor& desc) = 0;
    virtual GpuBuffer create_buffer(const BufferDescriptor& desc) = 0;
    virtual GpuSampler create_sampler(const SamplerDescriptor& desc) = 0;
    virtual GpuPipeline create_pipeline(const PipelineDescriptor& desc) = 0;
    virtual GpuComputePipeline create_compute_pipeline(const ComputePipelineDescriptor& desc) = 0;

    // Command recording
    virtual GpuCommandBuffer begin_commands() = 0;
    virtual void submit(GpuCommandBuffer cmd) = 0;
    virtual void wait_idle() = 0;

    // Texture upload/readback
    virtual void upload_texture(GpuTexture tex, const void* data, size_t size) = 0;
    virtual void readback_texture(GpuTexture tex, void* data, size_t size) = 0;

    // Interop: get native handle for Flutter Texture Registry
    virtual void* native_texture_handle(GpuTexture tex) = 0;

    // Capabilities
    virtual GpuCapabilities capabilities() const = 0;

    static std::unique_ptr<IGpuContext> create_for_platform();
};

struct GpuCapabilities {
    GpuBackend backend;
    std::string device_name;
    size_t vram_bytes;
    int max_texture_size;
    bool supports_compute;
    bool supports_float16;
    bool supports_float32_filtering;
    bool supports_bc_compression;
    bool supports_astc_compression;
    int max_compute_workgroup_size;
    int max_color_attachments;
};

} // namespace gp::rendering
```

### 6.3 Shader Management and Cross-Compilation

```
Shader Source (.glsl / .hlsl / .metal)
        │
        ▼
┌─────────────────┐
│ Shader Compiler  │ (offline, during build)
│ (glslc / DXC /  │
│  metal compiler) │
└────────┬────────┘
         │
    ┌────┴────┐
    │ SPIR-V  │  (canonical intermediate)
    └────┬────┘
         │
    ┌────┴─────────────────────────┐
    │                              │
    ▼                              ▼
┌──────────┐  ┌──────────┐  ┌──────────┐
│ Metal IR │  │ SPIR-V   │  │  GLSL    │
│ (Apple)  │  │ (Vulkan) │  │ (OpenGL) │
└──────────┘  └──────────┘  └──────────┘
```

All shaders are authored in GLSL (Vulkan dialect), compiled to SPIR-V during the build, and cross-compiled to Metal IR (via SPIRV-Cross) and GLSL ES (for OpenGL fallback). This ensures a single shader source with platform-specific binaries.

### 6.4 Built-in Shader Library

| Shader Category | Shaders |
|---|---|
| **Compositing** | Alpha blend, premultiply/unpremultiply, all 30+ blend modes |
| **Color** | Brightness/contrast, hue/sat, levels, curves, channel mixer, LUT application |
| **Blur** | Gaussian (separable), box, radial, zoom, directional, lens blur (bokeh) |
| **Distortion** | Warp, bulge, twirl, ripple, displacement map, lens correction |
| **Sharpen** | Unsharp mask, high-pass sharpen |
| **Edge** | Edge detect (Sobel, Canny), emboss, glow |
| **Keying** | Chroma key (YCbCr), luma key, difference key, spill suppression |
| **Matte** | Matte cleanup, matte choker, simple choker, refine edge |
| **Generate** | Noise (Perlin, simplex), gradient, grid, checkerboard, fractal |
| **Transform** | Affine transform, perspective warp, mesh warp, corner pin |
| **Temporal** | Motion blur (directional, radial), echo/trails |
| **Transition** | Cross dissolve, wipe (directional, radial, linear), push, zoom, glitch |
| **Utility** | Format conversion (NV12→RGBA, YUV→RGB), tone mapping, gamma correction |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Core Foundation |
| **Sprint(s)** | VE-Sprint 2 (Weeks 3-4) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [03-core-foundation](03-core-foundation.md) |
| **Successor** | [04-timeline-engine](04-timeline-engine.md) |
| **Story Points Total** | 64 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-073 | As a C++ engine developer, I want RenderGraph DAG with TextureResource and RenderPass so that we have a declarative render pipeline | - TextureResource: id, width, height, format, is_transient<br/>- RenderPass: inputs, outputs, execute fn<br/>- create_texture, add_pass API | 5 | P0 | VE-006 |
| VE-074 | As a C++ engine developer, I want RenderGraph compile (topological sort, barrier insertion) so that passes run in correct order | - Topological sort of passes<br/>- Barrier insertion between passes for resource transitions<br/>- execution_order_ output | 5 | P0 | VE-073 |
| VE-075 | As a C++ engine developer, I want RenderGraph resource aliasing for transient textures so that we reduce memory usage | - ResourceAliasingTable for transient reuse<br/>- No overlapping lifetimes<br/>- Compile validates aliasing | 5 | P0 | VE-074 |
| VE-076 | As a C++ engine developer, I want RenderGraph execute on IGpuContext so that we can run the graph on any backend | - execute(gpu) runs passes in order<br/>- Passes receive GpuCommandBuffer<br/>- Resource binding per pass | 3 | P0 | VE-074 |
| VE-077 | As a C++ engine developer, I want IGpuContext abstract interface so that we can swap GPU backends | - Pure virtual interface for all GPU ops<br/>- create_for_platform() factory<br/>- Backend enum (Metal, Vulkan, GL) | 5 | P0 | VE-004 |
| VE-078 | As a C++ engine developer, I want Metal backend (MTLDevice, MTLCommandQueue, CAMetalLayer) so that we run on iOS/macOS | - Metal implementation of IGpuContext<br/>- MTLDevice, MTLCommandQueue init<br/>- CAMetalLayer for display | 8 | P0 | VE-077 |
| VE-079 | As a C++ engine developer, I want Vulkan backend (VkDevice, VkQueue, VkSwapchain) so that we run on Android/Windows | - Vulkan implementation of IGpuContext<br/>- Device, queue, swapchain init<br/>- Sync objects (semaphores, fences) | 8 | P0 | VE-077 |
| VE-080 | As a C++ engine developer, I want OpenGL ES 3.2 fallback backend so that we have a fallback on older devices | - OpenGL ES 3.2 implementation<br/>- FBO for render targets<br/>- Shader compatibility | 5 | P1 | VE-077 |
| VE-081 | As a C++ engine developer, I want GpuTexture/GpuBuffer/GpuSampler creation so that we can allocate GPU resources | - create_texture, create_buffer, create_sampler<br/>- Descriptor structs for each<br/>- Resource lifetime management | 5 | P0 | VE-077 |
| VE-082 | As a C++ engine developer, I want GpuPipeline and ComputePipeline creation so that we can create render and compute pipelines | - PipelineDescriptor, ComputePipelineDescriptor<br/>- create_pipeline, create_compute_pipeline<br/>- Shader module binding | 5 | P0 | VE-081 |
| VE-083 | As a C++ engine developer, I want Command buffer recording and submission so that we can submit work to the GPU | - begin_commands, submit, wait_idle<br/>- Recording API for draw/dispatch<br/>- Per-backend implementation | 5 | P0 | VE-078, VE-079 |
| VE-084 | As a C++ engine developer, I want Texture upload/readback so that we can transfer data to/from GPU | - upload_texture, readback_texture<br/>- Staging buffer for upload<br/>- Async readback support | 3 | P0 | VE-081 |
| VE-085 | As a C++ engine developer, I want native_texture_handle for Flutter interop so that we can display frames in Flutter | - native_texture_handle(tex) returns platform handle<br/>- Metal: MTLTexture*, Vulkan: VkImage<br/>- Flutter TextureRegistry integration | 5 | P0 | VE-078, VE-079 |
| VE-086 | As a C++ engine developer, I want GpuCapabilities query so that we can adapt to device capabilities | - capabilities() returns GpuCapabilities<br/>- vram_bytes, max_texture_size, supports_compute<br/>- supports_float16, etc. | 2 | P0 | VE-077 |
| VE-087 | As a C++ engine developer, I want Shader cross-compilation pipeline (GLSL→SPIR-V→Metal IR→GLSL ES) so that we have one shader source | - GLSL Vulkan dialect input<br/>- glslc to SPIR-V<br/>- SPIRV-Cross to Metal IR and GLSL ES | 8 | P0 | VE-082 |
| VE-088 | As a C++ engine developer, I want Built-in shader library compilation and caching so that we ship precompiled shaders | - Shader library (blend, color, blur, etc.)<br/>- Compile at build time<br/>- Runtime cache for hot shaders | 5 | P0 | VE-087 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
