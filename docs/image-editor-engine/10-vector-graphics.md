
## IE-10. Vector Graphics Engine

### 10.1 Shape Layer Data Model

```cpp
namespace gp::vector {

enum class ShapeType {
    Rectangle, RoundedRectangle, Ellipse, Polygon, Star,
    Line, Arrow, Triangle,
    CustomPath,      // Pen tool-drawn bezier path
    CompoundPath,    // Multiple sub-paths (like text outlines)
};

struct ShapeLayerData {
    ShapeType type;
    ShapeGeometry geometry;

    // Appearance
    ShapeFill fill;
    ShapeStroke stroke;
    std::vector<ShapeEffect> effects;  // Shape-specific effects

    // Transform (relative to layer transform)
    Vec2 shape_origin;
    float shape_rotation;
};

struct ShapeGeometry {
    // Rectangle / Rounded Rectangle
    float rect_width, rect_height;
    float corner_radius;                // Uniform or per-corner
    float corner_radius_tl, corner_radius_tr, corner_radius_bl, corner_radius_br;
    bool independent_corners;

    // Ellipse
    float ellipse_width, ellipse_height;

    // Polygon
    int polygon_sides;                  // 3-100
    float polygon_radius;
    float polygon_roundness;            // Corner rounding
    float polygon_inner_radius;         // 0 = regular polygon, >0 = star
    float polygon_inner_roundness;

    // Star
    int star_points;                    // 3-100
    float star_outer_radius;
    float star_inner_radius;
    float star_outer_roundness;
    float star_inner_roundness;

    // Line / Arrow
    Vec2 line_start, line_end;
    ArrowStyle start_arrow, end_arrow;

    // Custom path (pen tool)
    BezierPath custom_path;

    // Compound path
    std::vector<BezierPath> sub_paths;
    FillRule fill_rule;                 // EvenOdd, NonZero
};

struct ArrowStyle {
    ArrowType type;                     // None, Triangle, Stealth, Circle, Square, Diamond
    float width;
    float length;
};

enum class ArrowType { None, Triangle, Stealth, OpenArrow, Circle, Square, Diamond, Custom };
enum class FillRule { EvenOdd, NonZero };

} // namespace gp::vector
```

### 10.2 Shape Fill and Stroke

```cpp
namespace gp::vector {

struct ShapeFill {
    bool enabled;
    FillType type;

    // Solid color
    Color4f color;

    // Gradient
    GradientType gradient_type;
    std::vector<GradientStop> gradient_stops;
    Vec2 gradient_start, gradient_end;
    float gradient_angle;

    // Pattern
    int32_t pattern_id;
    float pattern_scale;
    Vec2 pattern_offset;

    // Image fill
    int32_t image_ref_id;
    FitMode image_fit;

    float opacity;
    BlendMode blend_mode;
};

enum class FillType { None, Solid, Gradient, Pattern, Image };

struct ShapeStroke {
    bool enabled;
    Color4f color;
    float width;                    // In pixels
    StrokePosition position;        // Center, Inside, Outside

    // Line cap and join
    LineCap cap;                    // Butt, Round, Square
    LineJoin join;                  // Miter, Round, Bevel
    float miter_limit;

    // Dash pattern
    bool dashed;
    float dash_length;
    float gap_length;
    float dash_offset;

    // Gradient stroke
    bool gradient_enabled;
    GradientType gradient_type;
    std::vector<GradientStop> gradient_stops;

    // Variable width (pressure-sensitive)
    bool variable_width;
    std::vector<WidthPoint> width_profile;

    float opacity;
};

enum class LineCap { Butt, Round, Square };
enum class LineJoin { Miter, Round, Bevel };

struct WidthPoint {
    float position;                 // 0-1 along path
    float width_left;               // Width on left side
    float width_right;              // Width on right side
};

} // namespace gp::vector
```

### 10.3 Boolean Operations

```cpp
namespace gp::vector {

enum class BooleanOp {
    Unite,        // Union (A + B)
    Subtract,     // Difference (A - B)
    Intersect,    // Intersection (A ∩ B)
    Exclude,      // Exclusive or (A XOR B)
    Divide,       // Split both shapes at intersections
};

class BooleanEngine {
public:
    // Perform boolean operation on two shapes
    static ShapeGeometry operate(const ShapeGeometry& a,
                                  const ShapeGeometry& b,
                                  BooleanOp op);

    // Perform boolean on multiple shapes
    static ShapeGeometry operate_multi(const std::vector<ShapeGeometry>& shapes,
                                        BooleanOp op);

    // Flatten compound path to simple paths
    static std::vector<BezierPath> flatten_compound(const ShapeGeometry& compound);

    // Expand stroke to filled shape
    static ShapeGeometry expand_stroke(const BezierPath& path,
                                        const ShapeStroke& stroke);

    // Offset path (expand or contract)
    static BezierPath offset_path(const BezierPath& path, float distance);

private:
    // Clipper2 library integration for robust boolean operations
    static Paths64 to_clipper(const BezierPath& path, float scale = 1000.0f);
    static BezierPath from_clipper(const Paths64& paths, float scale = 1000.0f);
};

} // namespace gp::vector
```

### 10.4 Pen Tool and Path Editing

```cpp
namespace gp::vector {

// Interactive pen tool state machine
class PenTool {
public:
    enum State {
        Idle,              // Not drawing
        PlacingPoint,      // About to place next point
        DraggingHandle,    // Adjusting bezier handles
        ClosingPath,       // Hovering over first point to close
        EditingNode,       // Editing existing path node
    };

    void begin_path();
    void add_point(Vec2 position);
    void drag_handle(Vec2 handle_position);    // While mouse held after placing point
    void close_path();
    void cancel();

    // Path editing
    void select_node(int32_t index);
    void move_node(int32_t index, Vec2 new_position);
    void move_handle_in(int32_t index, Vec2 new_position);
    void move_handle_out(int32_t index, Vec2 new_position);
    void delete_node(int32_t index);
    void insert_node(float t);                 // Insert at parameter t on path
    void toggle_smooth(int32_t index);         // Smooth ↔ corner
    void convert_to_curve(int32_t index);      // Convert straight segment to curve
    void convert_to_line(int32_t index);       // Convert curve to straight

    // Path operations
    void reverse_path();
    void join_paths(const BezierPath& other);  // Connect two open paths
    void split_path(int32_t node_index);       // Split at node
    void simplify_path(float tolerance);       // Reduce node count

    State state() const;
    const BezierPath& current_path() const;
    int32_t selected_node() const;

private:
    BezierPath path_;
    State state_ = Idle;
    int32_t selected_node_ = -1;
    Vec2 last_mouse_pos_;
};

// Curvature pen tool (simplified, auto-curves like Figma)
class CurvaturePenTool {
public:
    void add_point(Vec2 position);
    void close_path();

    // Automatically computes smooth bezier handles
    const BezierPath& current_path() const;

private:
    std::vector<Vec2> points_;
    BezierPath path_;
    void recompute_handles();  // Catmull-Rom → bezier conversion
};

} // namespace gp::vector
```

### 10.5 SVG Import / Export

```cpp
namespace gp::vector {

class SvgImporter {
public:
    struct ImportResult {
        std::vector<canvas::Layer> layers;     // SVG elements as layers
        int width, height;                      // SVG viewport size
        float dpi;
        std::vector<std::string> warnings;      // Unsupported features
    };

    ImportResult import_file(const std::string& path);
    ImportResult import_string(const std::string& svg_content);

    // Import settings
    struct ImportOptions {
        float target_dpi;           // Rasterize at this DPI (0 = keep vector)
        bool flatten_groups;        // Flatten SVG groups to individual layers
        bool embed_images;          // Embed referenced images
        bool convert_text;          // Convert text to paths
        int max_complexity;         // Max path nodes (0 = unlimited)
    };
    void set_options(const ImportOptions& options);

private:
    ImportOptions options_;
};

class SvgExporter {
public:
    struct ExportOptions {
        bool include_metadata;
        bool embed_fonts;           // Embed used fonts
        bool convert_effects;       // Rasterize effects to images
        float precision;            // Decimal places for coordinates
        bool minify;                // Remove whitespace
        bool responsive;            // Use viewBox instead of fixed width/height
    };

    std::string export_canvas(const canvas::Canvas& canvas,
                               const ExportOptions& options = {});
    std::string export_layer(const canvas::Layer& layer,
                              const ExportOptions& options = {});

    void export_to_file(const canvas::Canvas& canvas,
                         const std::string& path,
                         const ExportOptions& options = {});
};

} // namespace gp::vector
```

### 10.6 Shape Preset Library

| Category | Shapes |
|---|---|
| **Basic** | Rectangle, Rounded Rectangle, Circle, Ellipse, Triangle, Pentagon, Hexagon, Octagon, Star (4-12 point), Cross, Diamond, Heart, Arrow |
| **Arrows** | Right Arrow, Left Arrow, Up Arrow, Down Arrow, Chevron, Double Arrow, Curved Arrow, Circular Arrow, U-Turn Arrow |
| **Callouts** | Speech Bubble (rectangle), Speech Bubble (rounded), Thought Bubble, Cloud Callout, Line Callout, Explosion Callout |
| **Flowchart** | Process, Decision, Data, Terminator, Predefined Process, Internal Storage, Document, Multi-Document, Manual Operation |
| **Banners** | Ribbon, Wave Banner, Scroll Banner, Curved Banner, Starburst |
| **Decorative** | Frame, Badge, Shield, Crest, Wreath, Divider Line, Decorative Border, Corner Ornament |
| **Social** | Like (thumb up), Heart, Chat Bubble, Share, Bookmark, Pin, Bell, Camera, Music Note, Play Button, Hashtag |
| **Icons** | 500+ common icons (SVG paths bundled) for template design |

### 10.7 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 11 | Wk 19-20 | IE-106 to IE-115 | Shape data model, primitives, fill/stroke, pen tool |
| Sprint 12 | Wk 21-22 | IE-116 to IE-120 | Boolean ops, SVG import/export, shape presets, font management |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-106 | As a designer, I want a shape data model (all types) so that I can create and edit vector shapes | - ShapeLayerData with ShapeType enum (Rectangle, Ellipse, Polygon, Star, Line, CustomPath, etc.)<br/>- ShapeGeometry for all primitive params<br/>- ShapeFill, ShapeStroke, effects | 5 | Sprint 11 | — |
| IE-107 | As a designer, I want rectangle and rounded rect shapes so that I can add basic shapes to the canvas | - rect_width, rect_height, corner_radius<br/>- Independent per-corner radius support<br/>- Render with correct fill/stroke | 3 | Sprint 11 | IE-106 |
| IE-108 | As a designer, I want ellipse shapes so that I can add circles and ovals | - ellipse_width, ellipse_height in ShapeGeometry<br/>- Bezier approximation for rendering<br/>- Fill and stroke support | 2 | Sprint 11 | IE-106 |
| IE-109 | As a designer, I want polygon and star shapes so that I can add multi-sided shapes | - polygon_sides (3-100), polygon_radius, polygon_inner_radius for stars<br/>- star_points, star_outer/inner_radius<br/>- Roundness params for corners | 3 | Sprint 11 | IE-106 |
| IE-110 | As a designer, I want line and arrow shapes so that I can add connectors and arrows | - line_start, line_end in ShapeGeometry<br/>- ArrowStyle: type, width, length for start/end<br/>- ArrowType: Triangle, Stealth, Circle, etc. | 3 | Sprint 11 | IE-106 |
| IE-111 | As a designer, I want shape fill (solid, gradient, pattern, image) so that I can style shape interiors | - FillType: None, Solid, Gradient, Pattern, Image<br/>- Gradient stops, angle, pattern_scale<br/>- opacity, blend_mode | 5 | Sprint 11 | IE-106 |
| IE-112 | As a designer, I want shape stroke (color, dash, variable width) so that I can style shape outlines | - ShapeStroke: color, width, position (Center/Inside/Outside)<br/>- LineCap, LineJoin, miter_limit<br/>- dashed, variable_width, gradient stroke | 5 | Sprint 11 | IE-106 |
| IE-113 | As a designer, I want a pen tool (bezier creation) so that I can draw custom paths | - begin_path(), add_point(), drag_handle(), close_path()<br/>- State machine: Idle, PlacingPoint, DraggingHandle, ClosingPath<br/>- BezierPath output | 5 | Sprint 11 | IE-106 |
| IE-114 | As a designer, I want pen tool node editing so that I can refine paths after creation | - select_node(), move_node(), move_handle_in/out()<br/>- delete_node(), insert_node(t), toggle_smooth()<br/>- convert_to_curve/line, reverse_path, join_paths | 5 | Sprint 11 | IE-113 |
| IE-115 | As a designer, I want a curvature pen tool so that I can draw smooth curves easily | - add_point(), close_path() with auto-handles<br/>- Catmull-Rom to Bezier conversion<br/>- Figma-style simplified curve creation | 5 | Sprint 11 | IE-113 |
| IE-116 | As a designer, I want boolean operations (unite/subtract/intersect/exclude) so that I can combine shapes | - BooleanEngine::operate(a, b, BooleanOp)<br/>- Unite, Subtract, Intersect, Exclude, Divide<br/>- Clipper2 library integration | 5 | Sprint 12 | IE-106 |
| IE-117 | As a designer, I want SVG import (nanosvg) so that I can bring in vector assets | - SvgImporter::import_file(), import_string()<br/>- ImportResult: layers, viewport, warnings<br/>- ImportOptions: target_dpi, flatten_groups, embed_images | 5 | Sprint 12 | IE-106 |
| IE-118 | As a designer, I want SVG export so that I can export vector artwork | - SvgExporter::export_canvas(), export_layer()<br/>- ExportOptions: embed_fonts, precision, minify<br/>- export_to_file() | 5 | Sprint 12 | IE-106 |
| IE-119 | As a designer, I want a shape preset library (200+) so that I can add common shapes quickly | - 200+ presets: Basic, Arrows, Callouts, Flowchart, Banners, Decorative, Social, Icons<br/>- Category browsing<br/>- Add to canvas as ShapeLayerData | 5 | Sprint 12 | IE-106 |
| IE-120 | As a designer, I want font management (system + Google Fonts) so that I can use a wide range of typefaces | - FontManagerIE: system fonts + Google Fonts<br/>- search_google_fonts(), download_font()<br/>- Font categories, recent fonts, pairing suggestions | 5 | Sprint 12 | — |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 4: Text & Vector Graphics |
| **Sprint(s)** | IE-Sprint 11-12 (Weeks 21-24) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [09-text-typography.md](09-text-typography.md) |
| **Successor** | [11-template-system.md](11-template-system.md) |
| **Story Points Total** | 78 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-131 | As a designer, I want a rectangle shape tool with rounded corners so that I can add basic shapes to the canvas | - rect_width, rect_height, corner_radius in ShapeGeometry<br/>- Independent per-corner radius support<br/>- Render with correct fill/stroke | 3 | P0 | — |
| IE-132 | As a designer, I want an ellipse shape tool so that I can add circles and ovals | - ellipse_width, ellipse_height in ShapeGeometry<br/>- Bezier approximation for rendering<br/>- Fill and stroke support | 2 | P0 | — |
| IE-133 | As a designer, I want a polygon tool (3-12 sides) so that I can add multi-sided shapes | - polygon_sides (3-12), polygon_radius<br/>- polygon_roundness for corner rounding<br/>- Fill and stroke support | 3 | P0 | — |
| IE-134 | As a designer, I want a star tool (points, inner radius) so that I can add star shapes | - star_points, star_outer_radius, star_inner_radius<br/>- star_outer_roundness, star_inner_roundness<br/>- Configurable star geometry | 3 | P0 | — |
| IE-135 | As a designer, I want a line and arrow tool so that I can add connectors and arrows | - line_start, line_end in ShapeGeometry<br/>- ArrowStyle: type, width, length for start/end<br/>- ArrowType: Triangle, Stealth, Circle, etc. | 3 | P0 | — |
| IE-136 | As a designer, I want shape fill (solid/gradient/pattern/image) so that I can style shape interiors | - FillType: None, Solid, Gradient, Pattern, Image<br/>- Gradient stops, angle, pattern_scale<br/>- opacity, blend_mode | 5 | P0 | — |
| IE-137 | As a designer, I want shape stroke (color, width, dash, variable width) so that I can style shape outlines | - ShapeStroke: color, width, position (Center/Inside/Outside)<br/>- LineCap, LineJoin, miter_limit, dashed<br/>- variable_width, gradient stroke | 5 | P0 | — |
| IE-138 | As a designer, I want a pen tool (bezier path creation with handles) so that I can draw custom paths | - begin_path(), add_point(), drag_handle(), close_path()<br/>- State machine: Idle, PlacingPoint, DraggingHandle, ClosingPath<br/>- BezierPath output with handles | 5 | P0 | — |
| IE-139 | As a designer, I want a curvature pen tool so that I can draw smooth curves easily | - add_point(), close_path() with auto-handles<br/>- Catmull-Rom to Bezier conversion<br/>- Figma-style simplified curve creation | 5 | P0 | IE-138 |
| IE-140 | As a designer, I want to add/delete/convert anchor points so that I can refine paths after creation | - select_node(), move_node(), move_handle_in/out()<br/>- delete_node(), insert_node(t), toggle_smooth()<br/>- convert_to_curve/line | 5 | P0 | IE-138 |
| IE-141 | As a designer, I want a direct selection tool so that I can select and edit individual path nodes | - Direct selection mode for path editing<br/>- Node and handle manipulation<br/>- Multi-node selection support | 3 | P0 | IE-138 |
| IE-142 | As a designer, I want boolean unite (Clipper2) so that I can combine shapes | - BooleanEngine::operate(a, b, BooleanOp::Unite)<br/>- Clipper2 library integration<br/>- Robust polygon union | 5 | P0 | — |
| IE-143 | As a designer, I want boolean subtract so that I can cut shapes from each other | - BooleanEngine::operate(a, b, BooleanOp::Subtract)<br/>- A - B difference operation<br/>- Clipper2 integration | 5 | P0 | IE-142 |
| IE-144 | As a designer, I want boolean intersect so that I can get overlapping regions | - BooleanEngine::operate(a, b, BooleanOp::Intersect)<br/>- A ∩ B intersection<br/>- Clipper2 integration | 5 | P0 | IE-142 |
| IE-145 | As a designer, I want boolean exclude (XOR) so that I can create complex shapes | - BooleanEngine::operate(a, b, BooleanOp::Exclude)<br/>- A XOR B exclusive or<br/>- Clipper2 integration | 5 | P0 | IE-142 |
| IE-146 | As a designer, I want SVG import (nanosvg→layers) so that I can bring in vector assets | - SvgImporter::import_file(), import_string()<br/>- ImportResult: layers, viewport, warnings<br/>- nanosvg parsing to layer tree | 5 | P0 | — |
| IE-147 | As a designer, I want SVG export so that I can export vector artwork | - SvgExporter::export_canvas(), export_layer()<br/>- ExportOptions: embed_fonts, precision, minify<br/>- export_to_file() | 5 | P0 | — |
| IE-148 | As a designer, I want a shape preset library (200+ shapes) so that I can add common shapes quickly | - 200+ presets: Basic, Arrows, Callouts, Flowchart, Banners, Decorative, Social, Icons<br/>- Category browsing<br/>- Add to canvas as ShapeLayerData | 5 | P0 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
