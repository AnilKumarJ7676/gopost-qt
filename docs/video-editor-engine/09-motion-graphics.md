
## VE-9. Motion Graphics and Shape Layer System

### 9.1 Shape Layer Data Model

```cpp
namespace gp::motion {

// After Effects-style shape layer: groups of vector shapes with operators

enum class ShapeType {
    Group,
    Rectangle,
    Ellipse,
    Polystar,       // Star or polygon
    Path,           // Bezier path
    Text,           // Text on a path
};

enum class ShapeOperatorType {
    Fill,
    Stroke,
    GradientFill,
    GradientStroke,
    TrimPaths,
    MergePaths,
    OffsetPaths,
    PuckerBloat,
    Repeater,
    RoundCorners,
    Twist,
    Wiggle,         // Wiggle paths (random displacement)
    ZigZag,
};

struct BezierPath {
    std::vector<Vec2> vertices;
    std::vector<Vec2> in_tangents;
    std::vector<Vec2> out_tangents;
    bool closed;

    float path_length() const;
    Vec2 point_at_parameter(float t) const;
    Vec2 tangent_at_parameter(float t) const;
    BezierPath subdivide(float t) const;
};

struct ShapeRectangle {
    Vec2 position;      // Keyframeable
    Vec2 size;          // Keyframeable
    float roundness;    // Keyframeable
};

struct ShapeEllipse {
    Vec2 position;
    Vec2 size;
};

struct ShapePolystar {
    enum Type { Star, Polygon };
    Type type;
    Vec2 position;
    int points;           // Keyframeable (integer)
    float rotation;       // Keyframeable
    float outer_radius;   // Keyframeable
    float outer_roundness;
    float inner_radius;   // Star only, keyframeable
    float inner_roundness;
};

struct ShapeFill {
    Color4f color;        // Keyframeable
    float opacity;        // Keyframeable
    FillRule rule;        // EvenOdd, NonZero
    BlendMode blend_mode;
};

struct ShapeStroke {
    Color4f color;
    float opacity;
    float width;
    LineCap cap;          // Butt, Round, Square
    LineJoin join;        // Miter, Round, Bevel
    float miter_limit;
    std::vector<float> dash_pattern;   // Dash, gap, dash, gap...
    float dash_offset;
    BlendMode blend_mode;
};

struct ShapeGradientFill {
    GradientType type;    // Linear, Radial
    Vec2 start_point;
    Vec2 end_point;
    std::vector<GradientStop> colors;   // Keyframeable
    float opacity;
    float highlight_length; // Radial only
    float highlight_angle;  // Radial only
};

struct ShapeTrimPaths {
    float start;         // 0–100%, keyframeable
    float end;           // 0–100%, keyframeable
    float offset;        // Degrees, keyframeable
    TrimMode mode;       // Simultaneously, Individually
};

struct ShapeRepeater {
    int copies;           // Keyframeable
    float offset;         // Keyframeable
    Transform2D transform; // Applied progressively to each copy
    CompositeOrder order; // Above, Below
    float start_opacity;
    float end_opacity;
};

struct ShapeGroup {
    std::string name;
    std::vector<ShapeItem> items;  // Shapes + operators, ordered
    Transform2D transform;
    BlendMode blend_mode;
};

// Discriminated union: either a shape or an operator
using ShapeItem = std::variant<
    ShapeGroup, BezierPath, ShapeRectangle, ShapeEllipse, ShapePolystar,
    ShapeFill, ShapeStroke, ShapeGradientFill, ShapeGradientStroke,
    ShapeTrimPaths, ShapeMergePaths, ShapeOffsetPaths, ShapePuckerBloat,
    ShapeRepeater, ShapeRoundCorners, ShapeTwist, ShapeWiggle, ShapeZigZag
>;

} // namespace gp::motion
```

### 9.2 Shape Rendering Pipeline

```
ShapeGroup
    │
    ├─ Collect paths (Rectangle → Path, Ellipse → Path, etc.)
    │
    ├─ Apply path operators in order:
    │   ├─ Round Corners → modify vertex positions
    │   ├─ Wiggle Paths → randomize vertices
    │   ├─ Zig Zag → add vertices on segments
    │   ├─ Pucker & Bloat → scale tangent handles
    │   ├─ Trim Paths → compute sub-path by arc length
    │   ├─ Offset Paths → dilate/erode by distance
    │   ├─ Merge Paths → boolean operations (add, subtract, intersect, XOR)
    │   └─ Twist → rotate vertices based on distance from center
    │
    ├─ Repeater (if present):
    │   └─ Duplicate entire group N times with progressive transform
    │
    ├─ Tessellate paths → triangle mesh (for GPU fill)
    │
    ├─ Apply fills (solid, gradient) → GPU fragment shader
    │
    └─ Apply strokes → GPU stroke shader (or CPU-tessellated quads)
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 5: Motion Graphics & Advanced |
| **Sprint(s)** | VE-Sprint 13-14 (Weeks 25-28) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [08-keyframe-animation](08-keyframe-animation.md) |
| **Successor** | [10-text-typography](10-text-typography.md) |
| **Story Points Total** | 72 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-131 | As a C++ engine developer, I want BezierPath with vertices/tangents/closed so that we have the core path representation | - vertices, in_tangents, out_tangents vectors<br/>- closed flag<br/>- path_length(), point_at_parameter(t), tangent_at_parameter(t) | 5 | P0 | VE-025 |
| VE-132 | As a C++ engine developer, I want path_length and point_at_parameter so that we can sample and measure paths | - path_length() via arc length integration<br/>- point_at_parameter(t) for t in [0,1]<br/>- subdivide(t) for path splitting | 3 | P0 | VE-131 |
| VE-133 | As a C++ engine developer, I want ShapeRectangle/ShapeEllipse/ShapePolystar to path conversion so that we can convert primitives to paths | - ShapeRectangle → BezierPath (with roundness)<br/>- ShapeEllipse → BezierPath (4 arcs)<br/>- ShapePolystar → BezierPath (star/polygon) | 5 | P0 | VE-131 |
| VE-134 | As a C++ engine developer, I want ShapeFill (solid color, fill rule) so that we can fill shapes | - ShapeFill: color, opacity, rule (EvenOdd, NonZero)<br/>- blend_mode<br/>- Fill rule for self-intersecting paths | 3 | P0 | VE-131 |
| VE-135 | As a C++ engine developer, I want ShapeStroke (width, cap, join, dash pattern) so that we can stroke paths | - width, color, opacity, cap (Butt, Round, Square)<br/>- join (Miter, Round, Bevel), miter_limit<br/>- dash_pattern, dash_offset | 5 | P0 | VE-131 |
| VE-136 | As a C++ engine developer, I want ShapeGradientFill (linear/radial) so that we support gradient fills | - Linear: start_point, end_point, gradient stops<br/>- Radial: highlight_length, highlight_angle<br/>- Opacity, blend_mode | 5 | P0 | VE-134 |
| VE-137 | As a C++ engine developer, I want ShapeTrimPaths (start/end/offset, simultaneously/individually) so that we support path trimming | - start, end (0-100%), offset (degrees)<br/>- TrimMode: Simultaneously, Individually<br/>- Animated for reveal effects | 5 | P0 | VE-132 |
| VE-138 | As a C++ engine developer, I want ShapeRepeater (copies with progressive transform) so that we can duplicate shapes | - copies, offset, transform (progressive)<br/>- start_opacity, end_opacity<br/>- CompositeOrder: Above, Below | 5 | P0 | VE-131 |
| VE-139 | As a C++ engine developer, I want ShapeMergePaths (boolean: add/subtract/intersect/XOR) so that we support path boolean ops | - MergePaths: add, subtract, intersect, XOR<br/>- Operates on multiple paths in group<br/>- Correct winding for fill | 8 | P0 | VE-131 |
| VE-140 | As a C++ engine developer, I want ShapeRoundCorners so that we can round path corners | - roundness parameter (radius or percent)<br/>- Modifies vertex positions<br/>- Applied before other operators | 3 | P0 | VE-131 |
| VE-141 | As a C++ engine developer, I want ShapeOffsetPaths so that we can dilate or erode paths | - offset distance (positive = expand, negative = shrink)<br/>- Miter/round join for corners<br/>- Handles self-intersection | 5 | P0 | VE-131 |
| VE-142 | As a C++ engine developer, I want ShapePuckerBloat so that we can scale tangent handles for pucker/bloat effect | - amount (negative = pucker, positive = bloat)<br/>- Scale tangent handles from center<br/>- Per-vertex or uniform | 3 | P1 | VE-131 |
| VE-143 | As a C++ engine developer, I want ShapeWiggle and ShapeZigZag path operators so that we can add organic variation | - ShapeWiggle: random displacement, frequency<br/>- ShapeZigZag: add vertices on segments, amplitude<br/>- Deterministic with seed | 5 | P1 | VE-131 |
| VE-144 | As a C++ engine developer, I want ShapeGroup container with ordered items so that we can nest shapes and operators | - ShapeGroup: name, items (ShapeItem variant), transform<br/>- blend_mode<br/>- Ordered evaluation | 5 | P0 | VE-131 |
| VE-145 | As a C++ engine developer, I want Shape rendering pipeline (collect→operators→tessellate→fill→stroke) so that we render shapes | - Collect paths from primitives<br/>- Apply operators in order (round, wiggle, trim, offset, merge)<br/>- Tessellate, fill, stroke | 8 | P0 | VE-133, VE-134, VE-135 |
| VE-146 | As a C++ engine developer, I want Vector tessellation to triangle mesh for GPU so that we can render filled paths | - Tessellate BezierPath to triangles<br/>- EvenOdd/NonZero fill rule<br/>- GPU buffer upload | 5 | P0 | VE-145 |
| VE-147 | As a C++ engine developer, I want Shape keyframe animation for all properties so that we can animate shape parameters | - All shape/operator properties keyframeable<br/>- KeyframeTrack integration<br/>- Evaluate at time | 5 | P0 | VE-114 |
| VE-148 | As a C++ engine developer, I want Repeater composite order (above/below) so that we control repeater draw order | - CompositeOrder: Above (draw copies on top), Below (draw copies under)<br/>- Affects compositing order of repeated items<br/>- Correct for layer stacking | 2 | P1 | VE-138 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
