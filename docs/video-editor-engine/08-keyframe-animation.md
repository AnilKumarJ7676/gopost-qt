
## VE-8. Keyframe Animation Engine

### 8.1 Keyframe Data Model

```cpp
namespace gp::animation {

enum class InterpolationType {
    Hold,           // No interpolation (step/constant)
    Linear,         // Linear interpolation
    Bezier,         // Cubic bezier (After Effects-style)
    EaseIn,         // Preset ease-in
    EaseOut,        // Preset ease-out
    EaseInOut,      // Preset ease-in-out
    Spring,         // Physics-based spring
    Bounce,         // Bounce easing
    Elastic,        // Elastic easing
};

struct Keyframe {
    Rational time;
    ParameterValue value;

    // Bezier handles (in/out tangent for graph editor)
    // Temporal handles: X axis is time, Y axis is value influence
    Vec2 tangent_in;         // Left handle (incoming)
    Vec2 tangent_out;        // Right handle (outgoing)
    bool tangent_broken;     // Independent in/out tangents

    // Spatial interpolation for position properties
    Vec2 spatial_tangent_in;   // Spatial bezier incoming
    Vec2 spatial_tangent_out;  // Spatial bezier outgoing

    InterpolationType interpolation;
    bool roving;              // Auto-adjust time for even spatial speed
};

struct KeyframeTrack {
    std::string property_id;  // "transform.position.x", "effects.blur.radius"
    ParameterType value_type;
    std::vector<Keyframe> keyframes;   // Sorted by time

    ParameterValue evaluate(Rational time) const;
    void add_keyframe(Keyframe kf);
    void remove_keyframe(int32_t index);
    void smooth_keyframes(int32_t start, int32_t end);  // Auto-bezier
    void easy_ease(int32_t index);    // Apply ease-in-out preset
    void easy_ease_in(int32_t index);
    void easy_ease_out(int32_t index);

    // Motion path (for 2D/3D position)
    std::vector<Vec2> compute_motion_path(Rational start, Rational end, int samples) const;
};

// Graph editor data for UI
struct CurveView {
    std::string property_id;
    Color4f curve_color;
    std::vector<Vec2> sampled_points;   // For rendering the curve
    Rect bounds;                         // Value range

    void fit_to_view();
    void set_value_range(float min_val, float max_val);
};

} // namespace gp::animation
```

### 8.2 Interpolation Engine

```cpp
namespace gp::animation {

class Interpolator {
public:
    // Evaluate bezier interpolation between two keyframes
    static float bezier(float t,
                        float v0, const Vec2& out_tangent,
                        float v1, const Vec2& in_tangent);

    // Solve for parameter t given timeline position (Newton-Raphson)
    static float solve_bezier_t(float timeline_pos,
                                float t0, const Vec2& out_tangent,
                                float t1, const Vec2& in_tangent,
                                float epsilon = 1e-6f);

    // Spatial bezier (for position motion paths)
    static Vec2 spatial_bezier(float t,
                               Vec2 p0, Vec2 cp0,
                               Vec2 p1, Vec2 cp1);

    // Arc-length parameterization for uniform motion along path
    static float reparameterize_by_arc_length(float t,
                                              const std::vector<float>& arc_lengths);

    // Spring physics interpolation
    static float spring(float t, float damping, float stiffness, float mass);

    // Preset easing curves
    static float ease_in_cubic(float t);
    static float ease_out_cubic(float t);
    static float ease_in_out_cubic(float t);
    static float ease_in_back(float t);
    static float ease_out_bounce(float t);
    static float ease_out_elastic(float t);
};

} // namespace gp::animation
```

### 8.3 Expression Engine (Lua-based)

```cpp
namespace gp::animation {

// Expressions allow procedural animation via scripting.
// Based on Lua for safety, performance, and cross-platform support.

class ExpressionEngine {
public:
    ExpressionEngine();
    ~ExpressionEngine();

    // Compile expression string for a property
    ExpressionHandle compile(const std::string& source,
                             const std::string& property_path);

    // Evaluate expression at a given time
    ParameterValue evaluate(ExpressionHandle handle,
                            Rational time,
                            const ExpressionContext& ctx);

    // Validate expression without executing
    ValidationResult validate(const std::string& source);

    void set_timeout_ms(int ms);   // Safety timeout per evaluation

private:
    lua_State* L_;
};

struct ExpressionContext {
    Rational time;
    Rational comp_duration;
    int32_t comp_width, comp_height;
    Rational frame_rate;
    int32_t frame_number;

    // Access to other properties and layers
    std::function<ParameterValue(const std::string& path)> get_property;
    std::function<ParameterValue(int32_t layer_id, const std::string& path)> get_layer_property;
};

// Built-in expression functions (exposed to Lua)
// - time, frame, fps
// - value (current keyframe value)
// - wiggle(frequency, amplitude, octaves, amp_mult, t)
// - loopOut(type, numkeyframes) / loopIn(...)
// - linear(t, tMin, tMax, val1, val2)
// - ease(t, tMin, tMax, val1, val2)
// - clamp(value, min, max)
// - random(min, max), gaussRandom(min, max)
// - degreesToRadians(deg), radiansToDegrees(rad)
// - length(vec), normalize(vec), dot(a, b), cross(a, b)
// - lookAt(from, to) — returns rotation
// - comp("name") — reference another composition
// - thisLayer, thisComp, thisProperty

} // namespace gp::animation
```

### 8.4 Common Expression Examples

```lua
-- Wiggle: random oscillation
wiggle(5, 30)  -- 5x per second, amplitude 30

-- Loop keyframes
loopOut("cycle")

-- Time-based rotation (spinner)
time * 360   -- 360 degrees per second

-- Bounce expression
local n = 0
if (numKeys > 0) then
    n = nearestKey(time).index
    if (key(n).time > time) then n = n - 1 end
end
if (n == 0) then
    return value
end
local t = time - key(n).time
local amp = velocityAtTime(key(n).time - 0.001)
local freq = 3
local decay = 5
return value + amp * (math.sin(freq * t * 2 * math.pi) / math.exp(decay * t))

-- Linking to another layer's property
thisComp.layer("Controller").effect("Slider")("Slider")

-- Auto-orient along motion path
local v = position.velocityAtTime(time)
math.deg(math.atan2(v[2], v[1]))
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 3: Effects & Color |
| **Sprint(s)** | VE-Sprint 8-9 (Weeks 15-18) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [07-effects-filter-system](07-effects-filter-system.md) |
| **Successor** | [09-motion-graphics](09-motion-graphics.md) |
| **Story Points Total** | 72 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-113 | As a C++ engine developer, I want Keyframe data model (time, value, tangents, interpolation type) so that we can store keyframe animation | - Keyframe struct: time, value, tangent_in/out, tangent_broken<br/>- spatial_tangent_in/out for position<br/>- InterpolationType enum | 5 | P0 | VE-031 |
| VE-114 | As a C++ engine developer, I want KeyframeTrack with sorted keyframes and evaluate(time) so that we can animate properties | - KeyframeTrack: property_id, value_type, keyframes<br/>- add_keyframe, remove_keyframe<br/>- evaluate(time) returns interpolated value | 5 | P0 | VE-113 |
| VE-115 | As a C++ engine developer, I want Hold/Linear/Bezier interpolation so that we support basic interpolation types | - Hold: constant until next keyframe<br/>- Linear: lerp between keyframes<br/>- Bezier: cubic bezier with tangent handles | 5 | P0 | VE-114 |
| VE-116 | As a C++ engine developer, I want Bezier tangent handles (in/out, broken/unified) so that we support After Effects-style curves | - tangent_in, tangent_out (Vec2)<br/>- tangent_broken for independent handles<br/>- Unified: mirror out from in | 3 | P0 | VE-115 |
| VE-117 | As a C++ engine developer, I want Spatial bezier for position motion paths so that we support curved motion | - spatial_tangent_in, spatial_tangent_out on Keyframe<br/>- spatial_bezier(t, p0, cp0, p1, cp1)<br/>- Vec2/Vec3 position support | 5 | P0 | VE-116 |
| VE-118 | As a C++ engine developer, I want Arc-length reparameterization for uniform motion so that motion speed is constant along path | - Compute arc lengths between keyframes<br/>- reparameterize_by_arc_length(t, arc_lengths)<br/>- Uniform motion along spatial path | 5 | P1 | VE-117 |
| VE-119 | As a C++ engine developer, I want Spring/Bounce/Elastic physics interpolation so that we support physics-based easing | - spring(t, damping, stiffness, mass)<br/>- bounce, elastic preset curves<br/>- Interpolator static methods | 5 | P1 | VE-115 |
| VE-120 | As a C++ engine developer, I want Preset easing curves (ease in/out cubic, back, bounce, elastic) so that we have common easings | - ease_in_cubic, ease_out_cubic, ease_in_out_cubic<br/>- ease_in_back, ease_out_bounce, ease_out_elastic<br/>- Exposed in InterpolationType | 3 | P0 | VE-115 |
| VE-121 | As a C++ engine developer, I want KeyframeTrack.smooth_keyframes (auto-bezier) so that we can auto-smooth keyframe tangents | - smooth_keyframes(start, end) computes auto-bezier<br/>- Tangent handles set for smooth curve<br/>- Preserves keyframe values | 3 | P1 | VE-116 |
| VE-122 | As a C++ engine developer, I want KeyframeTrack.easy_ease/easy_ease_in/easy_ease_out so that we can apply preset easing to keyframes | - easy_ease: ease-in-out on both sides<br/>- easy_ease_in: ease-in on exit<br/>- easy_ease_out: ease-out on enter | 2 | P1 | VE-120 |
| VE-123 | As a C++ engine developer, I want CurveView data for graph editor UI so that the Flutter UI can render the curve | - CurveView: property_id, curve_color, sampled_points, bounds<br/>- fit_to_view, set_value_range<br/>- Sampled for rendering | 3 | P1 | VE-114 |
| VE-124 | As a C++ engine developer, I want Motion path computation and visualization so that we can show position animation path | - compute_motion_path(start, end, samples) on KeyframeTrack<br/>- Returns vector of Vec2 for 2D position<br/>- For motion path overlay | 3 | P1 | VE-117 |
| VE-125 | As a C++ engine developer, I want ExpressionEngine (Lua state init/cleanup) so that we can run Lua expressions | - ExpressionEngine ctor/dtor<br/>- Lua state init, cleanup on destroy<br/>- Isolated state per engine instance | 5 | P0 | VE-114 |
| VE-126 | As a C++ engine developer, I want Expression compile and validate so that we can check expressions before evaluation | - compile(source, property_path) returns ExpressionHandle<br/>- validate(source) returns ValidationResult<br/>- Parse errors reported | 5 | P0 | VE-125 |
| VE-127 | As a C++ engine developer, I want Expression evaluate with ExpressionContext so that we can evaluate expressions at runtime | - evaluate(handle, time, ctx) returns ParameterValue<br/>- ExpressionContext: time, comp_duration, get_property<br/>- Thread-safe evaluation | 5 | P0 | VE-126 |
| VE-128 | As a C++ engine developer, I want Built-in expression functions (wiggle, loopOut, linear, ease, etc.) so that we have common expressions | - wiggle, loopOut, loopIn, linear, ease, clamp<br/>- random, gaussRandom, value, time, frame<br/>- length, normalize, dot, cross, lookAt | 5 | P0 | VE-127 |
| VE-129 | As a C++ engine developer, I want Expression layer/property references (thisLayer, thisComp) so that we can reference other data | - thisLayer, thisComp, thisProperty in context<br/>- get_layer_property(layer_id, path)<br/>- comp("name") for composition ref | 5 | P0 | VE-127 |
| VE-130 | As a C++ engine developer, I want Expression timeout safety so that runaway expressions don't block the engine | - set_timeout_ms(ms) on ExpressionEngine<br/>- Lua hook for execution limit<br/>- Returns error on timeout | 3 | P0 | VE-127 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
