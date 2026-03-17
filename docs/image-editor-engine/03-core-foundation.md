
## IE-3. Core Foundation Layer (Shared with Video Editor)

The image editor engine shares the entire `core/` module with the video editor engine (`libgopost_ve`). This section documents the IE-specific extensions and configurations applied to the shared foundation.

### 3.1 Shared Core Modules (Identical to VE-3)

The following are used as-is from the shared core:

- **AllocatorSystem** — Tiered memory allocators (pool, slab, arena, buddy)
- **FramePoolAllocator** — Lock-free pool for image tile buffers (replaces video frame buffers)
- **ArenaAllocator** — Per-operation scratch memory
- **Math library** — Vec2/3/4, Mat3/4, Quat, CubicBezier, Transform2D/3D, SIMD dispatch
- **TaskGraph** — Thread pool with work-stealing, task dependencies
- **SPSCRingBuffer** — Lock-free queues for main↔engine communication
- **Signal system** — Type-safe signal/slot for internal events
- **Logging** — Structured logging with per-module severity

### 3.2 IE-Specific Memory Configuration

```cpp
namespace gp::core {

struct IEMemoryBudget {
    // Mobile (iPhone / Android flagship)
    static constexpr PlatformBudget mobile() {
        return {
            .tile_pool      = 64 * 1024 * 1024,   // 64 MB — 8 tiles × 2048² × 4 bytes
            .texture_pool   = 48 * 1024 * 1024,   // 48 MB — GPU staging
            .brush_pool     = 16 * 1024 * 1024,   // 16 MB — brush stroke buffers
            .small_object   = 8 * 1024 * 1024,    // 8 MB — effects, nodes, params
            .per_op_arena   = 8 * 1024 * 1024,    // 8 MB — scratch per operation
            .layer_cache    = 128 * 1024 * 1024,   // 128 MB — cached layer composites
            .general        = 112 * 1024 * 1024,   // 112 MB — everything else
        };                                          // Total: 384 MB
    }

    // Desktop (macOS / Windows)
    static constexpr PlatformBudget desktop() {
        return {
            .tile_pool      = 256 * 1024 * 1024,  // 256 MB — 32 tiles × 2048² × 4 bytes
            .texture_pool   = 192 * 1024 * 1024,  // 192 MB — GPU staging
            .brush_pool     = 64 * 1024 * 1024,   // 64 MB — brush stroke buffers
            .small_object   = 32 * 1024 * 1024,   // 32 MB — effects, nodes
            .per_op_arena   = 32 * 1024 * 1024,   // 32 MB — scratch per operation
            .layer_cache    = 512 * 1024 * 1024,   // 512 MB — cached layer composites
            .general        = 448 * 1024 * 1024,   // 448 MB — everything else
        };                                          // Total: 1536 MB (1.5 GB)
    }
};

} // namespace gp::core
```

### 3.3 IE-Specific Math Extensions

```cpp
namespace gp::math {

// Pixel coordinate with sub-pixel precision
struct PixelCoord {
    float x, y;
    int ix() const { return static_cast<int>(std::floor(x)); }
    int iy() const { return static_cast<int>(std::floor(y)); }
    float frac_x() const { return x - std::floor(x); }
    float frac_y() const { return y - std::floor(y); }
};

// Polygon for selection regions
struct Polygon {
    std::vector<Vec2> vertices;
    bool is_closed;

    bool contains(Vec2 point) const;     // Point-in-polygon test
    Rect bounding_box() const;
    float area() const;
    Polygon offset(float distance) const; // Expand/contract
    Polygon simplify(float tolerance) const; // Douglas-Peucker
};

// Bezier path for pen tool and vector shapes
struct BezierPath {
    struct Node {
        Vec2 position;
        Vec2 handle_in;
        Vec2 handle_out;
        bool smooth;        // Auto-smooth tangent handles
        bool corner;        // Break tangent continuity
    };
    std::vector<Node> nodes;
    bool closed;

    Vec2 evaluate(float t) const;
    Vec2 tangent_at(float t) const;
    float arc_length() const;
    std::vector<Vec2> flatten(float tolerance = 0.5f) const;
    Rect bounding_box() const;
    bool hit_test(Vec2 point, float tolerance = 2.0f) const;

    // Boolean operations
    static BezierPath unite(const BezierPath& a, const BezierPath& b);
    static BezierPath intersect(const BezierPath& a, const BezierPath& b);
    static BezierPath subtract(const BezierPath& a, const BezierPath& b);
    static BezierPath exclude(const BezierPath& a, const BezierPath& b);
};

// Color space conversion (used heavily in image processing)
namespace colorspace {
    Vec3 srgb_to_linear(Vec3 srgb);
    Vec3 linear_to_srgb(Vec3 linear);
    Vec3 srgb_to_lab(Vec3 srgb);
    Vec3 lab_to_srgb(Vec3 lab);
    Vec3 srgb_to_hsv(Vec3 srgb);
    Vec3 hsv_to_srgb(Vec3 hsv);
    Vec3 srgb_to_hsl(Vec3 srgb);
    Vec3 hsl_to_srgb(Vec3 hsl);
    Vec4 rgb_to_cmyk(Vec3 rgb);
    Vec3 cmyk_to_rgb(Vec4 cmyk);
}

// SIMD extensions for image processing
namespace simd {
    void bilinear_sample_rgba(const uint8_t* src, int src_w, int src_h,
                               float u, float v, uint8_t* out_pixel);
    void bicubic_sample_rgba(const uint8_t* src, int src_w, int src_h,
                              float u, float v, uint8_t* out_pixel);
    void box_downsample_2x(const uint8_t* src, int w, int h,
                            uint8_t* dst);  // Mipmap generation
    void alpha_composite_row(const uint8_t* src, uint8_t* dst,
                              int width, float opacity, BlendMode mode);
    void apply_lut_1d(const uint8_t* src, uint8_t* dst,
                       int count, const uint8_t* lut);
    void gaussian_blur_horizontal(const float* src, float* dst,
                                   int width, int height,
                                   const float* kernel, int radius);
    void gaussian_blur_vertical(const float* src, float* dst,
                                 int width, int height,
                                 const float* kernel, int radius);
}

} // namespace gp::math
```

### 3.4 IE-Specific Engine Events

```cpp
namespace gp::core {

struct IEEngineEvents {
    Signal<int32_t>             on_layer_added;        // layer_id
    Signal<int32_t>             on_layer_removed;      // layer_id
    Signal<int32_t>             on_layer_modified;     // layer_id
    Signal<int32_t, int32_t>    on_layer_reordered;    // layer_id, new_index
    Signal<Rect>                on_canvas_dirty;       // dirty region
    Signal<int32_t>             on_selection_changed;   // selection_id
    Signal<float, float>        on_viewport_changed;    // zoom, rotation
    Signal<float>               on_export_progress;     // 0.0 - 1.0
    Signal<int, ErrorCode>      on_error;               // module_id, error
    Signal<>                    on_undo;
    Signal<>                    on_redo;
    Signal<const char*>         on_auto_save;           // path
    Signal<int32_t, float>      on_brush_stroke;        // layer_id, pressure
    Signal<const char*>         on_template_loaded;     // template_id
};

} // namespace gp::core
```

### 3.5 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 1 | Wk 1–2 | IE-016 to IE-023 | IE memory budget, allocator tuning, SIMD math, pixel utils, bezier path, color space, events, logging |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-016 | As a developer, I want IE memory budget configuration so that mobile and desktop stay within NFR limits | - IEMemoryBudget::mobile() returns 384 MB total budget<br/>- IEMemoryBudget::desktop() returns 1.5 GB total budget<br/>- Per-pool allocations match documented breakdown<br/>- Budget applied at engine init | 2 | Sprint 1 | IE-007 |
| IE-017 | As a developer, I want allocator tuning so that tile pool and layer cache use IE budgets | - FramePoolAllocator sized from IEMemoryBudget::tile_pool<br/>- Layer cache uses layer_cache budget<br/>- ArenaAllocator uses per_op_arena for scratch<br/>- No over-allocation beyond budget | 3 | Sprint 1 | IE-016 |
| IE-018 | As a developer, I want SIMD math extensions for bilinear and bicubic sampling so that image resampling is fast | - simd::bilinear_sample_rgba implemented (NEON/SSE/AVX)<br/>- simd::bicubic_sample_rgba implemented<br/>- simd::box_downsample_2x for mipmaps<br/>- Fallback scalar path when SIMD unavailable | 5 | Sprint 1 | IE-007 |
| IE-019 | As a developer, I want pixel coordinate utilities so that sub-pixel and integer coords are handled correctly | - PixelCoord struct with ix(), iy(), frac_x(), frac_y()<br/>- Polygon struct with contains(), bounding_box(), area()<br/>- Point-in-polygon and offset/simplify for selection<br/>- Unit tests for edge cases | 2 | Sprint 1 | IE-007 |
| IE-020 | As a developer, I want bezier path library so that pen tool and vector shapes can use paths | - BezierPath with Node, evaluate(), tangent_at(), flatten()<br/>- hit_test(), bounding_box(), arc_length()<br/>- Boolean ops: unite, intersect, subtract, exclude<br/>- Closed path support | 5 | Sprint 1 | IE-007 |
| IE-021 | As a developer, I want color space conversions so that image processing uses correct color math | - colorspace::srgb_to_linear, linear_to_srgb<br/>- srgb_to_lab, lab_to_srgb, srgb_to_hsv, hsv_to_srgb<br/>- rgb_to_cmyk, cmyk_to_rgb for print<br/>- Unit tests against known reference values | 3 | Sprint 1 | IE-007 |
| IE-022 | As a developer, I want IE event system so that UI can react to layer and canvas changes | - IEEngineEvents with on_layer_added, on_layer_modified, on_canvas_dirty, etc.<br/>- Signal/slot connections from engine to Flutter bridge<br/>- Thread-safe emission from engine thread<br/>- No memory leaks on disconnect | 3 | Sprint 1 | IE-007 |
| IE-023 | As a developer, I want logging integration so that IE modules log to shared logger | - Per-module severity (canvas, composition, rendering, etc.)<br/>- Structured log format (timestamp, module, level, message)<br/>- Log levels configurable at runtime<br/>- Flutter can receive log stream for dev tools | 2 | Sprint 1 | IE-007 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Core Foundation |
| **Sprint(s)** | IE-Sprint 1 (Weeks 1-2) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [02-engine-architecture](02-engine-architecture.md) |
| **Successor** | [04-canvas-engine](04-canvas-engine.md) |
| **Story Points Total** | 42 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-017 | As a C++ engine developer, I want shared core integration (link core/, rendering/, platform/ from libgopost_ve) so that we reuse VE foundation | - libgopost_ie links core/, rendering/, platform/ from libgopost_ve<br/>- No duplicate symbols<br/>- AllocatorSystem, TaskGraph, IGpuContext accessible | 5 | P0 | IE-004 |
| IE-018 | As a C++ engine developer, I want IE-specific memory budget configuration (mobile 512MB, desktop 2GB) so that we stay within NFR limits | - IEMemoryBudget::mobile() returns 512MB total<br/>- IEMemoryBudget::desktop() returns 2GB total<br/>- Per-pool breakdown documented and applied at init | 3 | P0 | IE-017 |
| IE-019 | As a C++ engine developer, I want IE-specific allocator tuning (tile buffers, brush stroke buffers) so that allocations are optimized | - Tile pool sized for 8+ tiles at 2048²<br/>- Brush stroke buffers from brush_pool<br/>- No over-allocation beyond budget | 3 | P0 | IE-018 |
| IE-020 | As a C++ engine developer, I want CMake build system with shared core dependency so that IE builds correctly | - CMake finds and links libgopost_ve or shared static lib<br/>- Platform-specific flags (Metal, Vulkan, GLES)<br/>- Build succeeds on all target platforms | 5 | P0 | IE-017 |
| IE-021 | As a C++ engine developer, I want GPU backend verification (Metal/Vulkan/GLES) on all platforms so that rendering works everywhere | - Metal verified on iOS/macOS<br/>- Vulkan verified on Android/Windows<br/>- GLES fallback verified on Android<br/>- Basic clear/draw test passes per backend | 5 | P0 | IE-020 |
| IE-022 | As a C++ engine developer, I want platform abstraction verification so that OS-specific code is isolated | - Platform module abstracts file I/O, threading, GPU detection<br/>- Thermal monitoring hooks available<br/>- Unit tests for platform APIs | 3 | P0 | IE-017 |
| IE-023 | As a C++ engine developer, I want math library integration test so that Vec2/3/4, Mat3/4, BezierPath work correctly | - Math types compile and link from IE<br/>- Unit tests for transforms, bezier evaluation<br/>- SIMD dispatch verified | 3 | P0 | IE-017 |
| IE-024 | As a C++ engine developer, I want thread pool integration test so that TaskGraph works for IE workloads | - TaskGraph initialized with N = CPU cores<br/>- Tile worker tasks execute correctly<br/>- No deadlocks or race conditions | 3 | P0 | IE-017 |
| IE-025 | As a C++ engine developer, I want event system integration test so that IEEngineEvents work from Flutter | - Signal/slot connections from engine to bridge<br/>- Thread-safe emission from engine thread<br/>- on_layer_added, on_canvas_dirty etc. fire correctly | 3 | P0 | IE-017 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
