
## IE-6. GPU Rendering Pipeline (Shared + IE Extensions)

### 6.1 Shared GPU Infrastructure

The image editor shares the entire GPU rendering infrastructure from the video editor engine:

- **RenderGraph** — DAG of render passes, compiled each frame
- **IGpuContext** — Platform GPU abstraction (Metal / Vulkan / DX12 / GLES)
- **Shader Management** — SPIR-V cross-compilation to Metal IR, GLSL
- **Built-in Shader Library** — Compositing, color, blur, distortion, keying shaders

### 6.2 IE-Specific Render Graph Extensions

```cpp
namespace gp::rendering {

// Image editor render graph is built per-canvas-update rather than per-frame
// (since images are static, not video). Re-builds only when layers change.

class IERenderGraph : public RenderGraph {
public:
    // Build render graph from canvas layer tree
    void build_from_canvas(const canvas::Canvas& canvas,
                           const ViewportState& viewport,
                           RenderQuality quality);

    // Incremental rebuild: only rebuild passes for dirty layers
    void rebuild_dirty(const std::vector<int32_t>& dirty_layer_ids);

    // Tile-based execution: render one tile at a time
    void execute_tile(IGpuContext& gpu, Rect tile_region);

    // Full execution: render entire canvas (for export)
    void execute_full(IGpuContext& gpu);

    enum class RenderQuality {
        Interactive,    // Viewport resolution, simplified effects
        Preview,        // Half resolution, full effects
        Full,           // Native resolution, full quality
        Export,         // Native or higher resolution, max quality
    };

private:
    // Layer-specific render pass builders
    int32_t add_image_layer_pass(const canvas::Layer& layer, Rect region);
    int32_t add_text_layer_pass(const canvas::Layer& layer, Rect region);
    int32_t add_shape_layer_pass(const canvas::Layer& layer, Rect region);
    int32_t add_gradient_layer_pass(const canvas::Layer& layer, Rect region);
    int32_t add_brush_layer_pass(const canvas::Layer& layer, Rect region);
    int32_t add_effect_chain_pass(int32_t input_tex, const canvas::Layer& layer);
    int32_t add_layer_style_pass(int32_t content_tex, const canvas::LayerStyle& style);
    int32_t add_blend_pass(int32_t src_tex, int32_t dst_tex,
                           BlendMode mode, float opacity,
                           int32_t mask_tex = -1);
    int32_t add_adjustment_pass(int32_t input_tex,
                                 const canvas::AdjustmentData& adj);
};

} // namespace gp::rendering
```

### 6.3 IE-Specific Shader Library

In addition to all shared shaders (compositing, color, blur, distortion, keying), the image editor adds:

| Shader Category | Shaders |
|---|---|
| **Layer Style** | Drop shadow, inner shadow, outer glow, inner glow, bevel/emboss, satin, stroke, color overlay, gradient overlay, pattern overlay |
| **Brush** | Brush stamp compositing, brush stroke accumulation, pressure-based opacity/size, brush dynamics (scatter, jitter, spacing) |
| **Selection** | Marching ants rendering, selection feathering, selection to mask conversion |
| **Vector Rasterize** | Bezier path fill (signed distance field), path stroke (variable width), anti-aliased edge rendering |
| **Image Processing** | Healing brush (Poisson blending), clone stamp compositing, content-aware fill (PatchMatch), histogram equalization |
| **Transform** | Perspective warp (homography), mesh warp (bilinear/bicubic per cell), liquify brush (forward/reconstruct warp), content-aware scale (seam carving energy map) |
| **Pattern** | Seamless pattern tile, pattern overlay with scale/rotation |
| **Template** | Placeholder frame rendering, placeholder shape masking, element snap guides |
| **Print** | Soft proofing shader (simulate CMYK on sRGB display), gamut warning overlay |
| **Utility** | Checkerboard (transparency indicator), canvas border/shadow, grid/guide overlay, ruler tick rendering |

### 6.4 Texture Management for Large Images

```cpp
namespace gp::rendering {

// Large image texture management: images > GPU max texture size
// are stored as tiled mipmaps

class LargeTextureManager {
public:
    struct MipmapLevel {
        int level;
        int width, height;
        int tile_cols, tile_rows;
        int tile_size;                 // 2048 or 4096
        std::vector<GpuTexture> tiles; // Row-major order
    };

    // Import a large image, creating all mipmap levels
    int32_t import(const uint8_t* pixels, int width, int height,
                    PixelFormat format);

    // Get texture for a specific region at a specific LOD
    GpuTexture get_region(int32_t image_id, Rect region, int lod_level);

    // Get the appropriate LOD level for a given zoom factor
    int recommended_lod(int32_t image_id, float zoom) const;

    // Evict textures not used recently (LRU)
    void trim(size_t target_bytes);

private:
    struct ImageEntry {
        int32_t id;
        int original_width, original_height;
        std::vector<MipmapLevel> mipmap_chain;
        uint64_t last_access_time;
        size_t total_gpu_bytes;
    };

    std::unordered_map<int32_t, ImageEntry> images_;
    size_t total_gpu_usage_ = 0;
    size_t budget_;
};

} // namespace gp::rendering
```

### 6.5 Interactive Canvas Rendering

```cpp
namespace gp::rendering {

// Optimized rendering path for interactive editing:
// Only renders the visible viewport region at screen resolution

class CanvasRenderer {
public:
    CanvasRenderer(IGpuContext& gpu);

    // Render canvas into viewport-sized texture for Flutter
    GpuTexture render_viewport(const canvas::Canvas& canvas,
                                const ViewportState& viewport);

    // Overlay rendering (guides, selection, handles, etc.)
    GpuTexture render_overlay(const canvas::Canvas& canvas,
                               const ViewportState& viewport,
                               const EditingContext& edit_ctx);

    // Composited output: canvas + overlay
    GpuTexture render_composited(const canvas::Canvas& canvas,
                                  const ViewportState& viewport,
                                  const EditingContext& edit_ctx);

private:
    // Cached layer composites (invalidated on edit)
    std::unordered_map<int32_t, CachedLayerResult> layer_cache_;

    // Quick dirty-region rendering: only re-render changed layers
    void update_dirty_layers(const canvas::Canvas& canvas,
                              const std::vector<int32_t>& dirty_ids);
};

struct EditingContext {
    bool show_guides;
    bool show_grid;
    bool show_rulers;
    bool show_selection_marching_ants;
    bool show_layer_bounds;
    bool show_transform_handles;
    bool show_anchor_point;
    bool show_snap_lines;
    bool show_pixel_grid;          // When zoomed > 500%
    bool show_transparency_checker;

    // Active tool overlay
    ToolType active_tool;
    Vec2 cursor_position;
    float brush_preview_radius;
    bool is_drawing;
};

} // namespace gp::rendering
```

### 6.6 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 1 | Wk 1–2 | IE-051 to IE-053 | Metal, Vulkan, GLES backends |
| Sprint 2 | Wk 3–4 | IE-054, IE-056, IE-057, IE-058 | Render graph integration, tile rendering, LOD, large texture manager |
| Sprint 3 | Wk 5 | IE-055, IE-059, IE-060 | IE render graph extensions, interactive canvas renderer, overlay rendering |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-051 | As a developer, I want Metal backend for iOS/macOS so that IE renders with Metal | - Metal implementation of IGpuContext<br/>- Metal 3 on supported devices, Metal 2 fallback<br/>- Shader cross-compilation from SPIR-V to Metal IR<br/>- Full compositing pipeline runs on Metal | 5 | Sprint 1 | IE-008 |
| IE-052 | As a developer, I want Vulkan backend for Android/Windows so that IE renders with Vulkan | - Vulkan implementation of IGpuContext<br/>- Vulkan 1.1 (Android), 1.2 (Windows)<br/>- SPIR-V shaders compile and run<br/>- Full compositing pipeline runs on Vulkan | 5 | Sprint 1 | IE-009 |
| IE-053 | As a developer, I want GLES fallback backend so that IE runs without Vulkan | - OpenGL ES 3.2 implementation of IGpuContext<br/>- Fallback when Vulkan unavailable<br/>- GLSL shaders for compositing<br/>- Feature parity with Vulkan path | 5 | Sprint 1 | IE-010 |
| IE-054 | As a developer, I want render graph (shared integration) so that IE uses shared RenderGraph | - RenderGraph from shared rendering module integrated<br/>- DAG of render passes, compiled per frame/update<br/>- Pass dependencies and resource lifecycle<br/>- Works with Metal, Vulkan, GLES | 5 | Sprint 2 | IE-051, IE-052 |
| IE-055 | As a developer, I want IE render graph extensions so that canvas builds passes from layer tree | - IERenderGraph extends RenderGraph<br/>- build_from_canvas() creates passes from canvas<br/>- rebuild_dirty() for incremental updates<br/>- add_image_layer_pass, add_blend_pass, add_effect_chain_pass, etc. | 8 | Sprint 3 | IE-054, IE-026 |
| IE-056 | As a developer, I want tile rendering execution so that large canvases render in tiles | - execute_tile(gpu, tile_region) renders single tile<br/>- execute_full(gpu) for export<br/>- Tile passes built from TileGrid<br/>- Parallel tile execution via worker pool | 5 | Sprint 2 | IE-054, IE-012 |
| IE-057 | As a developer, I want LOD selection system so that zoom level chooses appropriate resolution | - recommended_lod(zoom) returns LOD level<br/>- Interactive: viewport resolution or 1/4<br/>- Full: native resolution<br/>- Proxy: max 2048px for low-memory | 3 | Sprint 2 | IE-058 |
| IE-058 | As a developer, I want large texture manager with mipmaps so that big images fit in GPU memory | - LargeTextureManager with import(), get_region(), recommended_lod()<br/>- Mipmap chain for each image<br/>- Tiled storage (2048/4096 per tile)<br/>- LRU eviction via trim() | 5 | Sprint 2 | IE-012 |
| IE-059 | As a developer, I want interactive canvas renderer so that viewport renders at 60 fps | - CanvasRenderer.render_viewport() at screen resolution<br/>- Cached layer composites, invalidated on edit<br/>- update_dirty_layers() for incremental render<br/>- Meets 60 fps for ≤50 layers on mid-range | 5 | Sprint 3 | IE-055 |
| IE-060 | As a developer, I want overlay rendering for guides, selection, handles so that editing UI is visible | - render_overlay() draws guides, grid, selection marching ants<br/>- Layer bounds, transform handles, anchor point<br/>- Snap lines, pixel grid at high zoom<br/>- Composited with canvas via render_composited() | 5 | Sprint 3 | IE-059, IE-032 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Core Foundation |
| **Sprint(s)** | IE-Sprint 1-2 (Weeks 1-4) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [05-composition-engine](05-composition-engine.md) |
| **Successor** | [07-effects-filter-system](07-effects-filter-system.md) |
| **Story Points Total** | 58 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-061 | As a C++ engine developer, I want shared render graph integration from VE so that IE uses the same render pipeline | - RenderGraph from shared rendering module integrated<br/>- DAG of render passes, compiled per update<br/>- Works with Metal, Vulkan, GLES | 5 | P0 | IE-021 |
| IE-062 | As a C++ engine developer, I want IE-specific tile-based rendering passes so that large canvases render in tiles | - IERenderGraph extends RenderGraph<br/>- execute_tile(), execute_full()<br/>- build_from_canvas(), rebuild_dirty() | 5 | P0 | IE-061 |
| IE-063 | As a C++ engine developer, I want large texture management (split >8K images into tiles) so that big images fit in GPU memory | - LargeTextureManager with import(), get_region()<br/>- Images >8K split into 2048/4096 tiles<br/>- Mipmap chain for LOD | 5 | P0 | IE-039 |
| IE-064 | As a C++ engine developer, I want tile dirty region tracking (only re-render changed tiles) so that we avoid unnecessary work | - invalidate_region() marks tiles dirty<br/>- dirty_tiles() returns only changed tiles<br/>- Incremental render uses dirty set | 5 | P0 | IE-062 |
| IE-065 | As a C++ engine developer, I want GPU texture cache for layer content so that layer composites are cached | - LayerCacheEntry per layer with version<br/>- invalidate_layer() marks cache stale<br/>- LRU eviction under memory pressure | 5 | P0 | IE-062 |
| IE-066 | As a C++ engine developer, I want layer composite texture management so that compositor output is efficiently managed | - Composite textures allocated from pool<br/>- Release on invalidation<br/>- No leak on layer remove | 3 | P0 | IE-065 |
| IE-067 | As a Flutter developer, I want canvas preview rendering to Flutter texture so that the canvas displays in the app | - render_viewport() outputs to Flutter texture<br/>- 60 fps for ≤50 layers<br/>- Texture update on canvas change | 5 | P0 | IE-062 |
| IE-068 | As a C++ engine developer, I want full-resolution render for export so that export uses full quality | - render_full() at native canvas resolution<br/>- Tile assembly for large canvases<br/>- <3s for 4K composite | 5 | P0 | IE-062 |
| IE-069 | As a C++ engine developer, I want shader cache for IE-specific shaders so that compilation is fast | - IE shaders (layer style, brush, selection) cached<br/>- Compile on first use, reuse thereafter<br/>- Cache persisted across sessions (optional) | 3 | P1 | IE-061 |
| IE-070 | As a C++ engine developer, I want mipmap generation for zoom levels so that zoomed-out view is fast | - Mipmap chain for layer textures<br/>- recommended_lod(zoom) for LOD selection<br/>- Smooth transition between LODs | 3 | P0 | IE-063 |
| IE-071 | As a C++ engine developer, I want GPU memory pressure handling (evict least-recently-used) so that we stay within budget | - trim(target_bytes) evicts LRU textures<br/>- Memory budget from IEMemoryBudget<br/>- Graceful degradation under pressure | 5 | P0 | IE-065 |
| IE-072 | As a C++ engine developer, I want IE shader compilation and validation so that shaders work on all backends | - IE shaders compile to Metal IR, SPIR-V, GLSL<br/>- Validation on load<br/>- Fallback for unsupported features | 3 | P0 | IE-061 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
