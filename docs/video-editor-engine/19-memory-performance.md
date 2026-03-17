## VE-19. Memory and Performance Architecture

### 19.1 Frame Cache System

```cpp
namespace gp::cache {

// Multi-level frame cache for timeline scrubbing
class FrameCache {
public:
    FrameCache(size_t memory_budget);

    // Cache a rendered frame at a timeline position
    void put(int32_t timeline_id, Rational time, GpuTexture frame);

    // Retrieve a cached frame (null if miss)
    GpuTexture get(int32_t timeline_id, Rational time) const;

    // Invalidation
    void invalidate(int32_t timeline_id);                      // Entire timeline
    void invalidate_range(int32_t timeline_id, TimeRange range); // Partial
    void invalidate_track(int32_t timeline_id, int32_t track_id);
    void clear();

    // Stats
    size_t used_bytes() const;
    size_t budget_bytes() const;
    float hit_rate() const;

private:
    // LRU eviction with priority based on distance from playhead
    struct CacheEntry {
        GpuTexture texture;
        int32_t timeline_id;
        Rational time;
        int64_t last_access;
        int priority;          // Higher = more important to keep
    };
    std::list<CacheEntry> entries_;
    size_t used_;
    size_t budget_;
};

// Thumbnail cache for timeline scrubbing and media browser
class ThumbnailCache {
public:
    ThumbnailCache(const std::string& cache_dir, size_t memory_budget);

    // Get thumbnail (from memory, then disk, then generate on demand)
    GpuTexture get(int32_t media_ref_id, Rational time, int max_width);

    // Pre-generate thumbnails for a clip (async)
    TaskHandle pregenerate(int32_t media_ref_id, int count, int max_width);

    // Waveform data (cached to disk)
    WaveformData get_waveform(int32_t media_ref_id, int target_width);
    TaskHandle pregenerate_waveform(int32_t media_ref_id);

private:
    std::string cache_dir_;
    LRUCache<std::string, GpuTexture> memory_cache_;
};

} // namespace gp::cache
```

### 19.2 Memory Budget Allocation

| Component | Mobile (512 MB) | Desktop (2 GB) |
|---|---|---|
| Frame pool (decoded frames) | 128 MB (16x 1080p RGBA) | 512 MB (8x 4K RGBA) |
| GPU textures (render targets) | 96 MB | 384 MB |
| Frame cache (rendered frames) | 64 MB | 256 MB |
| Thumbnail cache | 32 MB | 64 MB |
| Audio buffers | 32 MB | 64 MB |
| Waveform cache | 16 MB | 32 MB |
| ML model inference | 64 MB | 128 MB |
| Effect parameters / project | 16 MB | 32 MB |
| General / overhead | 64 MB | 256 MB |
| **Total** | **512 MB** | **1,728 MB** |

### 19.3 Performance Optimization Strategies

| Strategy | Description |
|---|---|
| **Predictive decode** | When playing forward, pre-decode 3–5 frames ahead in a ring buffer |
| **Quality tiers** | Preview at 1/2 or 1/4 resolution; full res only for export and paused preview |
| **Effect chain batching** | Multiple simple effects merged into a single compute shader dispatch |
| **Lazy evaluation** | Only recompute layers that changed since last frame |
| **Dirty rectangles** | For static compositions, only re-render changed regions |
| **GPU double-buffering** | Render frame N+1 while displaying frame N |
| **Async readback** | GPU→CPU readback for export uses async copy to avoid stalls |
| **SIMD pixel ops** | NEON (ARM) and AVX2 (x86) for CPU-side pixel format conversions |
| **Texture atlas** | Pack small images (stickers, icons) into a single GPU texture |
| **Shader precompilation** | All shaders compiled during engine init (pipeline cache for Vulkan/Metal) |
| **Memory-mapped I/O** | Large media files accessed via mmap where beneficial (random seek) |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 7: Plugin & Polish |
| **Sprint(s)** | VE-Sprint 20-21 (Weeks 39-42) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [18-platform-integration](18-platform-integration.md) |
| **Successor** | [20-build-system](20-build-system.md) |
| **Story Points Total** | 88 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-304 | As a C++ engine developer, I want FrameCache with LRU eviction and playhead-distance priority so that scrubbing is smooth | - put(timeline_id, time, frame), get() returns cached or null<br/>- LRU eviction when over budget<br/>- Priority boosts frames near playhead | 5 | P0 | — |
| VE-305 | As a C++ engine developer, I want FrameCache invalidation (entire timeline, range, track) so that edits invalidate correctly | - invalidate(timeline_id) clears all<br/>- invalidate_range(timeline_id, range)<br/>- invalidate_track(timeline_id, track_id) | 3 | P0 | VE-304 |
| VE-306 | As a C++ engine developer, I want ThumbnailCache (memory→disk→generate-on-demand) so that media browser is efficient | - get() checks memory, then disk, then generates<br/>- LRU in-memory cache<br/>- Disk persistence for generated thumbnails | 5 | P0 | — |
| VE-307 | As a C++ engine developer, I want thumbnail pregeneration (async) so that timeline scrubbing is fast | - pregenerate(media_ref_id, count, max_width)<br/>- Async task, non-blocking<br/>- Progress callback optional | 3 | P1 | VE-306 |
| VE-308 | As a C++ engine developer, I want waveform cache (disk-backed) so that audio visualization loads quickly | - get_waveform() from cache or compute<br/>- Disk persistence for computed waveforms<br/>- Target width support | 5 | P1 | VE-306 |
| VE-309 | As a C++ engine developer, I want memory budget allocation enforcement (mobile 512MB, desktop 2GB) so that OOM is prevented | - Configurable budget per platform<br/>- Enforcement in FrameCache, ThumbnailCache<br/>- Graceful degradation when over budget | 5 | P0 | VE-304 |
| VE-310 | As a C++ engine developer, I want predictive decode (3-5 frames ahead ring buffer) so that playback is smooth | - Ring buffer of 3-5 decoded frames ahead<br/>- Pre-decode on play forward<br/>- Seek invalidates and refills | 5 | P0 | — |
| VE-311 | As a C++ engine developer, I want quality tiers (1/4, 1/2, 3/4, full resolution preview) so that performance scales | - ResolutionScale enum: Quarter, Half, ThreeQuarter, Full<br/>- Preview uses selected tier<br/>- Export always full | 3 | P0 | — |
| VE-312 | As a C++ engine developer, I want effect chain batching (merge simple effects into single dispatch) so that GPU usage is efficient | - Detect mergeable effect sequences<br/>- Single compute dispatch for batch<br/>- No visual difference | 5 | P1 | — |
| VE-313 | As a C++ engine developer, I want lazy evaluation (only recompute changed layers) so that render cost is minimized | - Dirty tracking per layer<br/>- Skip unchanged layers in composition<br/>- Invalidate on param/keyframe change | 5 | P1 | — |
| VE-314 | As a C++ engine developer, I want GPU double-buffering (render N+1 while display N) so that frame drops are reduced | - Double buffer for output texture<br/>- Render next frame while displaying current<br/>- Swap on vsync or frame complete | 3 | P1 | — |
| VE-315 | As a C++ engine developer, I want async GPU readback for export so that encoding isn't stalled | - Async copy GPU→CPU for export frames<br/>- Staging buffer with fence<br/>- No blocking on readback | 5 | P0 | — |
| VE-316 | As a C++ engine developer, I want SIMD pixel ops (NEON + AVX2 format conversion) so that CPU-side conversions are fast | - NEON for ARM (iOS, Android)<br/>- AVX2 for x86 (Windows, macOS Intel)<br/>- Format conversion (NV12→RGBA, etc.) | 5 | P2 | — |
| VE-317 | As a C++ engine developer, I want texture atlas for small images so that draw calls are reduced | - Pack small images (stickers, icons) into atlas<br/>- UV coordinate computation<br/>- Atlas size limit and eviction | 5 | P2 | — |
| VE-318 | As a C++ engine developer, I want shader precompilation and pipeline caching so that first-frame latency is low | - Compile all shaders at engine init<br/>- Vulkan pipeline cache, Metal library cache<br/>- Persist cache to disk across runs | 5 | P0 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
