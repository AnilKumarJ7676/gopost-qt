
## VE-3. Core Foundation Layer

### 3.1 Custom Memory Allocator System

```cpp
namespace gp::core {

// Tiered allocator hierarchy
class AllocatorSystem {
public:
    static AllocatorSystem& instance();

    PoolAllocator& frame_pool();       // Fixed-size frame buffers (RGBA, NV12)
    PoolAllocator& texture_pool();     // GPU texture staging buffers
    SlabAllocator& small_object();     // < 256 bytes (effects, nodes, params)
    ArenaAllocator& per_frame();       // Scratch memory reset each frame
    BuddyAllocator& general();        // General-purpose variable-size

    MemoryStats stats() const;
    void set_budget(size_t total_bytes);
    void trim();                       // Release unused pages back to OS

private:
    struct PlatformBudget {
        size_t frame_pool;
        size_t texture_pool;
        size_t small_object;
        size_t per_frame_arena;
        size_t general;
    };
    PlatformBudget detect_budget();
};

// Lock-free pool for hot path (frame buffers)
class FramePoolAllocator {
public:
    FramePoolAllocator(size_t frame_size, size_t count, size_t alignment = 64);
    ~FramePoolAllocator();

    uint8_t* acquire();           // O(1), lock-free
    void release(uint8_t* ptr);   // O(1), lock-free
    size_t available() const;
    size_t capacity() const;

    void prefault();              // Touch all pages to avoid first-access faults

private:
    alignas(64) std::atomic<uint64_t> head_;
    alignas(64) std::atomic<uint64_t> tail_;
    uint8_t* pool_;
    size_t frame_size_;
    size_t count_;
    std::vector<uint8_t*> slots_;
};

// Per-frame arena for scratch allocations (reset every frame)
class ArenaAllocator {
public:
    explicit ArenaAllocator(size_t capacity);

    void* allocate(size_t size, size_t alignment = 16);
    void reset();   // O(1) — just resets the offset pointer

    template<typename T, typename... Args>
    T* create(Args&&... args);

private:
    uint8_t* base_;
    size_t offset_;
    size_t capacity_;
};

} // namespace gp::core
```

### 3.2 Math Library (SIMD-accelerated)

```cpp
namespace gp::math {

// All math types are 16-byte aligned for SIMD
struct alignas(16) Vec2 { float x, y; };
struct alignas(16) Vec3 { float x, y, z; };
struct alignas(16) Vec4 { float x, y, z, w; };
struct alignas(16) Mat3 { float m[9]; };
struct alignas(16) Mat4 { float m[16]; };
struct alignas(16) Quat { float x, y, z, w; };

struct Rect { float x, y, w, h; };
struct Color4f { float r, g, b, a; };

// Bezier curve for animation/masking
struct CubicBezier {
    Vec2 p0, p1, p2, p3;
    float evaluate_at(float t) const;
    float solve_for_x(float x, float epsilon = 1e-6f) const;  // Newton-Raphson
};

// Transform stack used by compositor
struct Transform2D {
    Vec2 position;
    Vec2 anchor_point;
    Vec2 scale;
    float rotation;       // degrees
    float skew_x, skew_y;
    float opacity;

    Mat3 to_matrix() const;
    static Transform2D interpolate(const Transform2D& a, const Transform2D& b, float t);
};

struct Transform3D {
    Vec3 position;
    Vec3 anchor_point;
    Vec3 scale;
    Vec3 rotation;   // Euler XYZ degrees
    Quat orientation; // Alternative quaternion representation
    float opacity;

    Mat4 to_matrix() const;
    static Transform3D interpolate(const Transform3D& a, const Transform3D& b, float t);
};

// SIMD dispatch (compile-time + runtime)
namespace simd {
    void mat4_multiply(const float* a, const float* b, float* out);
    void vec4_transform(const float* mat, const float* vec, float* out);
    void blend_pixels_rgba(const uint8_t* src, const uint8_t* dst,
                           uint8_t* out, size_t count, float opacity,
                           BlendMode mode);
    void premultiply_alpha(uint8_t* pixels, size_t count);
    void unpremultiply_alpha(uint8_t* pixels, size_t count);
    void convert_nv12_to_rgba(const uint8_t* y, const uint8_t* uv,
                               uint8_t* rgba, int width, int height);
}

} // namespace gp::math
```

### 3.3 Thread Pool and Task System

```cpp
namespace gp::core {

enum class TaskPriority { Critical, High, Normal, Low, Background };

struct TaskHandle {
    uint64_t id;
    bool is_complete() const;
    void wait();
    void cancel();
};

class TaskGraph {
public:
    TaskHandle schedule(std::function<void()> fn, TaskPriority priority = TaskPriority::Normal);
    TaskHandle schedule_after(TaskHandle dependency, std::function<void()> fn);
    TaskHandle schedule_parallel(std::vector<std::function<void()>> tasks);
    void wait_all(const std::vector<TaskHandle>& handles);
    void cancel_all();

private:
    struct alignas(64) WorkerThread {
        std::thread thread;
        LockFreeQueue<Task> local_queue;
        std::atomic<bool> active;
    };
    std::vector<WorkerThread> workers_;
    LockFreeQueue<Task> global_queue_;   // Work-stealing fallback
};

// Lock-free SPSC ring buffer for main thread <-> engine thread communication
template<typename T, size_t Capacity>
class SPSCRingBuffer {
public:
    bool try_push(const T& item);
    bool try_pop(T& out);
    size_t size() const;
    bool empty() const;

private:
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    std::array<T, Capacity> buffer_;
};

} // namespace gp::core
```

### 3.4 Event and Signal System

```cpp
namespace gp::core {

// Type-safe signal/slot for internal engine events
template<typename... Args>
class Signal {
public:
    using SlotId = uint64_t;
    using SlotFn = std::function<void(Args...)>;

    SlotId connect(SlotFn fn);
    void disconnect(SlotId id);
    void emit(Args... args);
    void clear();

private:
    std::unordered_map<SlotId, SlotFn> slots_;
    std::shared_mutex mutex_;
    std::atomic<SlotId> next_id_{0};
};

// Engine-wide events
struct EngineEvents {
    Signal<int64_t>               on_playhead_changed;    // position_us
    Signal<int, int>              on_timeline_modified;    // track_id, clip_id
    Signal<float>                 on_export_progress;      // 0.0 - 1.0
    Signal<int, RenderFrame*>     on_frame_rendered;       // timeline_id, frame
    Signal<int, ErrorCode>        on_error;                // module_id, error
    Signal<>                      on_undo;
    Signal<>                      on_redo;
    Signal<const char*>           on_auto_save;            // path
};

} // namespace gp::core
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Core Foundation |
| **Sprint(s)** | VE-Sprint 1 (Weeks 1-2) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [02-engine-architecture](02-engine-architecture.md) |
| **Successor** | [06-gpu-rendering-pipeline](06-gpu-rendering-pipeline.md) |
| **Story Points Total** | 55 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-015 | As a C++ engine developer, I want FramePoolAllocator with lock-free acquire/release and prefault so that frame buffers are allocated without contention | - O(1) acquire/release, lock-free<br/>- prefault() touches all pages to avoid first-access faults<br/>- Unit test: concurrent acquire/release, no leaks | 5 | P0 | VE-009 |
| VE-016 | As a C++ engine developer, I want ArenaAllocator with per-frame scratch and O(1) reset so that temporary allocations are fast | - allocate() and reset() in O(1)<br/>- Alignment support (16-byte default)<br/>- create<T>() template for placement new | 3 | P0 | VE-006 |
| VE-017 | As a C++ engine developer, I want SlabAllocator for small objects (<256B) so that we avoid fragmentation for effects and nodes | - Fixed-size slabs, <256B objects<br/>- Free list for reuse<br/>- Stats for monitoring | 3 | P0 | VE-006 |
| VE-018 | As a C++ engine developer, I want BuddyAllocator for general use so that we have variable-size allocations with coalescing | - Buddy algorithm implementation<br/>- Coalesce on free<br/>- Max allocation size documented | 5 | P1 | VE-006 |
| VE-019 | As a C++ engine developer, I want AllocatorSystem singleton with platform budget detection so that we respect memory limits | - detect_budget() per platform<br/>- set_budget(), trim(), stats()<br/>- Integrates all allocators | 5 | P0 | VE-015, VE-016, VE-017 |
| VE-020 | As a C++ engine developer, I want SIMD Vec2/Vec3/Vec4/Mat3/Mat4/Quat so that we have aligned math types for vectorization | - 16-byte alignment, correct layout<br/>- Basic ops: add, sub, mul, dot, cross<br/>- Quat slerp | 3 | P0 | VE-006 |
| VE-021 | As a C++ engine developer, I want SIMD mat4_multiply and vec4_transform so that transforms are accelerated | - mat4_multiply uses SSE/NEON when available<br/>- vec4_transform for point transform<br/>- Fallback scalar path | 3 | P0 | VE-020 |
| VE-022 | As a C++ engine developer, I want SIMD blend_pixels_rgba so that compositing is fast | - BlendMode enum support<br/>- SIMD 4/8 pixels at a time<br/>- Premultiplied alpha handling | 5 | P0 | VE-020 |
| VE-023 | As a C++ engine developer, I want SIMD premultiply/unpremultiply alpha so that we handle alpha correctly | - premultiply_alpha, unpremultiply_alpha<br/>- SIMD vectorized<br/>- Edge cases (zero alpha) handled | 2 | P0 | VE-022 |
| VE-024 | As a C++ engine developer, I want SIMD NV12→RGBA conversion so that video decode output is converted efficiently | - Y/UV plane handling<br/>- SIMD conversion<br/>- Correct color space (Rec709) | 5 | P0 | VE-020 |
| VE-025 | As a C++ engine developer, I want CubicBezier evaluate and solve so that we support animation and masking curves | - evaluate_at(t), solve_for_x (Newton-Raphson)<br/>- Numeric stability for edge cases<br/>- Unit tests for known curves | 3 | P0 | VE-020 |
| VE-026 | As a C++ engine developer, I want Transform2D/Transform3D interpolation so that keyframe animation works | - interpolate(a, b, t) for both types<br/>- to_matrix() for compositor<br/>- Handles anchor, skew | 3 | P0 | VE-020 |
| VE-027 | As a C++ engine developer, I want TaskGraph with priority scheduling so that we can schedule work with dependencies | - schedule, schedule_after, schedule_parallel<br/>- TaskPriority: Critical, High, Normal, Low, Background<br/>- wait_all, cancel_all | 5 | P0 | VE-008 |
| VE-028 | As a C++ engine developer, I want TaskGraph work-stealing thread pool so that CPU cores are utilized | - Worker threads with local queues<br/>- Work-stealing when idle<br/>- Configurable worker count | 5 | P0 | VE-027 |
| VE-029 | As a C++ engine developer, I want Signal/Slot event system (thread-safe) so that we can emit engine events from any thread | - Signal<T...> with connect, disconnect, emit<br/>- shared_mutex for thread safety<br/>- SlotId for disconnect | 3 | P0 | VE-006 |
| VE-030 | As a C++ engine developer, I want EngineEvents struct with all engine-wide signals so that Flutter can subscribe to engine events | - on_playhead_changed, on_timeline_modified, on_export_progress<br/>- on_frame_rendered, on_error, on_undo, on_redo, on_auto_save<br/>- All wired to engine components | 3 | P0 | VE-029 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
