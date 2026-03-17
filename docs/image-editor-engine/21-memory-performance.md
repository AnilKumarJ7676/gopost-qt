
## IE-21. Memory and Performance Strategy

### 21.1 Performance Targets

| Metric | Mobile (iPhone 14/Pixel 7) | Desktop (M2 Mac / i7 PC) |
|---|---|---|
| Canvas render (interactive, 20 layers) | 16 ms (60 fps) | 8 ms (120 fps) |
| Canvas render (full quality, 4K, 50 layers) | < 3 seconds | < 1.5 seconds |
| Filter application (single effect) | < 50 ms | < 20 ms |
| Effect chain (5 stacked) | < 200 ms | < 80 ms |
| Layer add/remove | < 5 ms | < 2 ms |
| Selection (magic wand, 12 MP) | < 100 ms | < 50 ms |
| Brush stroke (per stamp) | < 2 ms | < 1 ms |
| Image load (12 MP JPEG) | < 150 ms | < 80 ms |
| Image load (50 MP RAW) | < 800 ms | < 400 ms |
| Template load (encrypted .gpit) | < 400 ms | < 200 ms |
| Export (4K JPEG) | < 500 ms | < 200 ms |
| Export (4K PNG) | < 1.5 s | < 600 ms |
| Undo/redo | < 20 ms | < 10 ms |
| Auto-save (incremental) | < 200 ms | < 100 ms |

### 21.2 Memory Budgets

```
┌─────────────────────────────────────────────────────────────┐
│                    Mobile Memory Budget (384 MB)             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────┐ ┌──────────────────────────────┐  │
│  │  Tile Pool (64 MB)   │ │  Layer Cache (128 MB)        │  │
│  │  8 tiles × 2048²     │ │  Cached composites for       │  │
│  │  × 4 bytes/pixel     │ │  non-dirty layers            │  │
│  └──────────────────────┘ └──────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────┐ ┌──────────────────────────────┐  │
│  │  GPU Staging (48 MB) │ │  General Heap (112 MB)       │  │
│  │  Texture upload/     │ │  Undo history, effects,      │  │
│  │  readback buffers    │ │  fonts, ML models            │  │
│  └──────────────────────┘ └──────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────┐ ┌──────────────────────────────┐  │
│  │  Brush Pool (16 MB)  │ │  Small Object Slab (8 MB)   │  │
│  │  Stroke buffers      │ │  Effect nodes, params        │  │
│  └──────────────────────┘ └──────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────┐                                   │
│  │  Per-Op Arena (8 MB) │                                   │
│  │  Scratch (reset)     │                                   │
│  └──────────────────────┘                                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                   Desktop Memory Budget (1.5 GB)             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────┐ ┌──────────────────────────────┐  │
│  │  Tile Pool (256 MB)  │ │  Layer Cache (512 MB)        │  │
│  │  32 tiles × 2048²    │ │  Cached composites           │  │
│  └──────────────────────┘ └──────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────┐ ┌──────────────────────────────┐  │
│  │  GPU Staging (192 MB)│ │  General Heap (448 MB)       │  │
│  └──────────────────────┘ └──────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────┐ ┌──────────────────────────────┐  │
│  │  Brush Pool (64 MB)  │ │  Small Object Slab (32 MB)  │  │
│  └──────────────────────┘ └──────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────┐                                   │
│  │  Per-Op Arena (32 MB)│                                   │
│  └──────────────────────┘                                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 21.3 Tile-Based Rendering Optimization

```cpp
namespace gp::canvas {

class TileRenderOptimizer {
public:
    // Only render tiles that are visible + dirty
    struct RenderPlan {
        std::vector<Tile*> tiles_to_render;   // Dirty + visible
        std::vector<Tile*> tiles_from_cache;  // Clean, use cached
        std::vector<Tile*> tiles_to_evict;    // Off-screen, can free
        int total_gpu_passes;
        size_t estimated_gpu_memory;
    };

    RenderPlan plan(const TileGrid& grid, const ViewportState& viewport,
                     size_t gpu_memory_budget);

    // Progressive rendering: render visible tiles first, then surrounding
    struct ProgressivePlan {
        std::vector<Tile*> priority_1;   // Currently visible
        std::vector<Tile*> priority_2;   // Adjacent to visible (pre-fetch)
        std::vector<Tile*> priority_3;   // Rest (background rendering)
    };

    ProgressivePlan plan_progressive(const TileGrid& grid,
                                       const ViewportState& viewport);

    // LOD selection based on zoom level
    int select_lod(float zoom) const;
    // zoom >= 1.0 → LOD 0 (full resolution)
    // zoom >= 0.5 → LOD 1 (half resolution)
    // zoom >= 0.25 → LOD 2 (quarter resolution)
    // zoom < 0.25 → LOD 3 (eighth resolution)
};

} // namespace gp::canvas
```

### 21.4 Layer Cache Strategy

```cpp
namespace gp::cache {

class LayerCache {
public:
    LayerCache(size_t budget_bytes);

    // Cache a rendered layer composite
    void put(int32_t layer_id, uint64_t version, GpuTexture texture, Rect bounds);

    // Get cached result (nullptr if not cached or stale)
    GpuTexture get(int32_t layer_id, uint64_t version) const;

    // Invalidate a single layer
    void invalidate(int32_t layer_id);

    // Invalidate layer and all layers above it (for adjustment layers)
    void invalidate_above(int32_t layer_id, const std::vector<int32_t>& layer_order);

    // Evict least recently used entries until under budget
    void evict_to_budget();

    // Memory stats
    size_t used_bytes() const;
    size_t budget_bytes() const;
    int cached_count() const;
    float hit_rate() const;

private:
    struct CacheEntry {
        int32_t layer_id;
        uint64_t version;
        GpuTexture texture;
        Rect bounds;
        size_t size_bytes;
        uint64_t last_access;
    };

    std::unordered_map<int32_t, CacheEntry> entries_;
    size_t budget_;
    size_t used_ = 0;
    uint64_t access_counter_ = 0;
    mutable uint64_t hits_ = 0, misses_ = 0;
};

} // namespace gp::cache
```

### 21.5 Optimization Strategies

| Strategy | Description | Impact |
|---|---|---|
| **Dirty region tracking** | Only re-render tiles/layers that changed | 5-10x faster interactive editing |
| **Layer caching** | Cache composited layer results, invalidate on edit | Avoid re-rendering unchanged layers |
| **LOD rendering** | Use lower-resolution textures when zoomed out | Proportional memory + speed savings |
| **GPU compute shaders** | All effects as compute shaders (vs fragment) | 2-3x faster on modern GPUs |
| **SIMD processing** | NEON (ARM) / SSE4.2+AVX2 (x86) for CPU paths | 4-8x faster pixel processing |
| **Async decode** | Decode images on background thread | No main thread blocking |
| **Texture streaming** | Load only visible tiles into GPU memory | Handles images larger than VRAM |
| **Effect merging** | Merge compatible sequential effects into one pass | Reduce GPU draw calls |
| **Lazy rasterization** | Keep vector/text as vector until needed as raster | Memory savings for small layers |
| **Compressed undo** | LZ4 compress undo pixel data | 3-5x smaller undo history |
| **Incremental auto-save** | Only serialize changed layers | Fast background saves |
| **Brush accumulation** | Accumulate brush stamps on GPU, readback only at stroke end | Smooth 60fps brush painting |
| **Proxy images** | Auto-generate 1/4 res proxies for large images | Fast preview on mobile |
| **Shader warm-up** | Pre-compile commonly used shader pipelines at init | No first-use stalls |

### 21.6 Memory Pressure Response

```cpp
namespace gp::core {

class MemoryPressureHandler {
public:
    enum Level { Normal, Warning, Critical, Emergency };

    void on_memory_pressure(Level level);

private:
    void handle_warning();
    // Evict layer cache to 50%
    // Clear undo history beyond 50 steps
    // Downgrade LOD by 1 level

    void handle_critical();
    // Evict all layer caches
    // Clear undo history beyond 10 steps
    // Force LOD to lowest level
    // Release all proxy textures

    void handle_emergency();
    // Emergency save (auto-save to disk)
    // Release all GPU textures except current view
    // Clear undo history completely
    // Show user warning
};

} // namespace gp::core
```

### 21.7 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 23 | Wk 43-44 | IE-263 to IE-276 | Performance, memory optimization |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-263 | As a developer, I want a tile render optimizer that renders only visible + dirty tiles so that GPU time is minimized during interactive editing | - TileRenderOptimizer produces RenderPlan with tiles_to_render, tiles_from_cache, tiles_to_evict<br/>- Only dirty + visible tiles are queued for render<br/>- Plan respects gpu_memory_budget | 5 | Sprint 23 | — |
| IE-264 | As a developer, I want progressive rendering with priority tiers so that visible content appears first | - ProgressivePlan has priority_1 (visible), priority_2 (adjacent), priority_3 (background)<br/>- Visible tiles render before pre-fetch<br/>- Background rendering does not block interactive frame rate | 5 | Sprint 23 | IE-263 |
| IE-265 | As a developer, I want LOD auto-selection based on zoom level so that memory and render cost scale with view | - select_lod(zoom) returns 0–3; zoom ≥1→LOD0, ≥0.5→LOD1, ≥0.25→LOD2, <0.25→LOD3<br/>- Lower LOD used when zoomed out<br/>- Seamless transition between LODs | 3 | Sprint 23 | IE-263 |
| IE-266 | As a developer, I want a layer cache with LRU eviction and budget awareness so that composited layers are reused without re-render | - LayerCache respects budget_bytes<br/>- evict_to_budget() evicts LRU entries<br/>- put/get/invalidate work correctly | 5 | Sprint 23 | — |
| IE-267 | As a developer, I want layer cache invalidation that propagates on edit so that cached results stay correct | - invalidate(layer_id) clears single layer<br/>- invalidate_above() clears layer and dependents for adjustment layers<br/>- Edit operations trigger appropriate invalidation | 3 | Sprint 23 | IE-266 |
| IE-268 | As a developer, I want per-tile dirty region tracking so that only changed tiles are re-rendered | - Dirty state tracked per tile<br/>- Edit operations mark affected tiles dirty<br/>- Clear dirty after successful render | 5 | Sprint 23 | IE-263 |
| IE-269 | As a developer, I want effect chain batching that merges compatible passes so that GPU draw calls are reduced | - Compatible sequential effects merged into single pass<br/>- No visual difference vs. separate passes<br/>- Measurable reduction in draw calls | 5 | Sprint 23 | — |
| IE-270 | As a developer, I want lazy rasterization for vector/text layers so that memory is saved until raster is needed | - Vector/text kept as vector until composited or exported<br/>- Rasterization triggered on first composite need<br/>- Memory usage reduced for small vector layers | 5 | Sprint 23 | — |
| IE-271 | As a developer, I want compressed undo using LZ4 for pixel data so that undo history uses less memory | - Undo pixel data compressed with LZ4<br/>- Decompress on undo/redo with acceptable latency<br/>- 3–5x smaller undo history | 3 | Sprint 23 | — |
| IE-272 | As a developer, I want proxy image generation at 1/4 resolution so that large images preview quickly on mobile | - Auto-generate 1/4 res proxy for images above threshold<br/>- Proxy used for viewport when zoomed out<br/>- Full res used when zoomed in or exporting | 3 | Sprint 23 | — |
| IE-273 | As a developer, I want shader warm-up at init so that first-use stalls are eliminated | - Pre-compile commonly used shader pipelines at engine init<br/>- No first-use compilation delay for standard operations<br/>- Init time increase acceptable (<500 ms) | 3 | Sprint 23 | — |
| IE-274 | As a developer, I want a memory pressure handler with warning/critical/emergency levels so that the app degrades gracefully under low memory | - on_memory_pressure(Level) implemented for Warning, Critical, Emergency<br/>- Warning: evict cache 50%, clear undo beyond 50 steps, downgrade LOD<br/>- Critical: evict all caches, clear undo beyond 10, force lowest LOD<br/>- Emergency: auto-save, release GPU textures, clear undo, show warning | 5 | Sprint 23 | IE-266 |
| IE-275 | As a developer, I want a memory profiling pass on all platforms so that allocations are traceable | - Memory profiling instrumentation for all allocators<br/>- Reports available on iOS, Android, macOS, Windows<br/>- Integration with platform profilers | 5 | Sprint 23 | — |
| IE-276 | As a developer, I want a GPU memory leak audit using validation layers so that no GPU resources leak | - Metal/Vulkan validation layers enabled in debug<br/>- Zero GPU resource leaks on canvas destroy and session end<br/>- Audit documented for all GPU allocation paths | 5 | Sprint 23 | — |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 8: Export, Polish & Launch |
| **Sprint(s)** | IE-Sprint 23 (Weeks 43-44) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [19-plugin-architecture](19-plugin-architecture.md) |
| **Successor** | [24-testing-validation](24-testing-validation.md) |
| **Story Points Total** | 70 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-313 | As a developer, I want tile-based rendering (only render dirty tiles) so that GPU time is minimized | - TileRenderOptimizer produces RenderPlan<br/>- Only dirty + visible tiles queued<br/>- Plan respects gpu_memory_budget | 5 | P0 | — |
| IE-314 | As a developer, I want layer cache (cache composited layer results) so that unchanged layers are reused | - LayerCache with put/get/invalidate<br/>- LRU eviction, budget_bytes<br/>- hit_rate() metrics | 5 | P0 | — |
| IE-315 | As a developer, I want layer cache invalidation on edit so that cached results stay correct | - invalidate(layer_id), invalidate_above()<br/>- Edit operations trigger invalidation<br/>- Adjustment layer propagation | 3 | P0 | IE-314 |
| IE-316 | As a developer, I want memory budget enforcement (512MB mobile, 2GB desktop) so that memory stays bounded | - 512MB mobile, 2GB desktop budgets<br/>- Enforcement and eviction<br/>- Memory pressure response | 5 | P0 | IE-314 |
| IE-317 | As a developer, I want decoded image cache (LRU) so that decoded images are reused | - LRU cache for decoded images<br/>- Budget-aware eviction<br/>- Cache hit/miss metrics | 3 | P0 | — |
| IE-318 | As a developer, I want thumbnail/proxy generation for large images so that preview is fast | - Auto-generate 1/4 res proxy above threshold<br/>- Proxy for viewport when zoomed out<br/>- Full res for export | 3 | P0 | — |
| IE-319 | As a developer, I want GPU texture eviction under memory pressure so that the app degrades gracefully | - MemoryPressureHandler: Warning, Critical, Emergency<br/>- Evict caches, reduce LOD, release textures<br/>- Emergency save on critical | 5 | P0 | IE-316 |
| IE-320 | As a developer, I want brush stroke buffering (accumulate→flush) so that painting is smooth | - Accumulate stamps on GPU<br/>- Readback only at stroke end<br/>- 60fps during stroke | 5 | P0 | — |
| IE-321 | As a developer, I want undo memory management (compress old states) so that undo history is bounded | - LZ4 compress undo pixel data<br/>- 3-5x smaller history<br/>- Decompress on undo/redo | 3 | P0 | — |
| IE-322 | As a developer, I want history state pruning (limit memory) so that undo does not grow unbounded | - Limit undo steps (200)<br/>- Purge old states under memory pressure<br/>- total_history_memory() | 3 | P0 | IE-321 |
| IE-323 | As a developer, I want performance profiling pass (all 4 platforms) so that regressions are detected | - Profiling on iOS, Android, macOS, Windows<br/>- Frame time, allocation tracking<br/>- Integration with platform profilers | 5 | P0 | — |
| IE-324 | As a developer, I want memory leak detection (ASan full session) so that leaks are found | - ASan/LSan in debug builds<br/>- Full editing session test<br/>- Zero leaks tolerance | 5 | P0 | — |
| IE-325 | As a developer, I want frame time optimization (<16ms for interactive tools) so that 60fps is maintained | - Canvas render <16ms (mobile)<br/>- Brush stroke <2ms per stamp<br/>- Interactive tools meet targets | 5 | P0 | IE-313, IE-320 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
