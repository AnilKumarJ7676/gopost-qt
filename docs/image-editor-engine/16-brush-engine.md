
## IE-16. Brush Engine

### 16.1 Brush System Architecture

```cpp
namespace gp::brush {

struct BrushTip {
    enum Shape { Round, Square, Custom };
    Shape shape;

    float size;                  // Diameter in pixels (1-5000)
    float hardness;              // Edge falloff (0-1, 0=soft, 1=hard)
    float opacity;               // Per-stamp opacity (0-1)
    float flow;                  // Paint accumulation rate (0-1)
    float spacing;               // Distance between stamps (% of size, 1-1000)
    float angle;                 // Rotation of tip (0-360)
    float roundness;             // Aspect ratio (1-100%)

    // Custom tip (from image)
    GpuTexture custom_texture;
    int custom_width, custom_height;
};

struct BrushDynamics {
    // Each dynamic can be driven by: Off, Pressure, Tilt, Velocity, Direction, Random
    enum class Source { Off, Pressure, Tilt, Velocity, Direction, Random, Fade };

    struct DynamicProperty {
        Source source;
        float min_value;         // Minimum when input is 0
        float max_value;         // Maximum when input is 1
        float jitter;            // Random variation (0-1)
    };

    DynamicProperty size;        // Size variation
    DynamicProperty opacity;     // Opacity variation
    DynamicProperty flow;        // Flow variation
    DynamicProperty hardness;    // Hardness variation
    DynamicProperty angle;       // Angle variation
    DynamicProperty roundness;   // Roundness variation
    DynamicProperty scatter;     // Perpendicular scatter
    DynamicProperty count;       // Stamps per spacing step

    // Color dynamics
    struct ColorDynamic {
        Source source;
        float hue_jitter;       // 0-100%
        float saturation_jitter;
        float brightness_jitter;
        bool foreground_background; // Alternate between FG/BG colors
    } color;

    // Transfer
    float buildup;               // Wet-edge opacity buildup
    float smoothing;             // Stroke smoothing (0-100%)
    int smoothing_mode;          // 0=none, 1=stroke smoothing, 2=pulled string
};

struct BrushPreset {
    std::string id;
    std::string name;
    std::string category;        // "Basic", "Dry Media", "Wet Media", "Special", etc.
    std::string thumbnail_path;
    BrushTip tip;
    BrushDynamics dynamics;
    BlendMode blend_mode;
};

} // namespace gp::brush
```

### 16.2 Brush Tools

```cpp
namespace gp::brush {

enum class BrushToolType {
    Brush,              // Standard painting brush
    Pencil,             // Hard-edge pencil (no anti-aliasing)
    Eraser,             // Erase to transparency
    MixerBrush,         // Smudge/blend colors
    CloneStamp,         // Clone from source (see IE-8)
    HealingBrush,       // Content-aware heal (see IE-8)
    PatternStamp,       // Paint with pattern
    HistoryBrush,       // Paint from history state
    Dodge,              // Lighten (highlights/midtones/shadows)
    Burn,               // Darken
    Sponge,             // Saturate / desaturate
    Smudge,             // Push pixels (finger paint)
    Blur,               // Selective blur brush
    Sharpen,            // Selective sharpen brush
};

class BrushEngine {
public:
    BrushEngine(IGpuContext& gpu);

    // Configure brush
    void set_preset(const BrushPreset& preset);
    void set_color(Color4f foreground, Color4f background);
    void set_tool(BrushToolType tool);

    // Stroke input
    void begin_stroke(Vec2 position, float pressure = 1.0f, float tilt = 0.0f);
    void continue_stroke(Vec2 position, float pressure = 1.0f, float tilt = 0.0f);
    void end_stroke();

    // Render stroke to target layer
    GpuTexture render_stroke(GpuTexture target_layer);

    // Get live preview (during stroke)
    GpuTexture stroke_preview() const;

    // Undo last stroke
    void undo_stroke();

private:
    struct StrokePoint {
        Vec2 position;
        float pressure;
        float tilt;
        float velocity;
        float timestamp;
    };

    // Stroke interpolation: smooth input points
    std::vector<StrokePoint> interpolate_stroke(
        const std::vector<StrokePoint>& raw_points, float smoothing);

    // Catmull-Rom spline interpolation between points
    Vec2 catmull_rom(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) const;

    // Stamp a single brush tip onto the accumulation buffer
    void stamp(Vec2 position, float size, float opacity, float angle,
               float roundness, const BrushTip& tip);

    // GPU-accelerated stroke accumulation
    GpuTexture accumulation_buffer_;
    std::vector<StrokePoint> current_stroke_;
    BrushPreset current_preset_;
    BrushToolType current_tool_ = BrushToolType::Brush;
    Color4f fg_color_, bg_color_;
    IGpuContext& gpu_;
};

} // namespace gp::brush
```

### 16.3 Dodge, Burn, and Sponge Tools

```cpp
namespace gp::brush {

class DodgeBurnTool {
public:
    enum Range { Shadows, Midtones, Highlights };

    struct Params {
        float exposure;          // 0-100%
        Range range;
        float size;
        float hardness;
        bool protect_tones;      // Prevent clipping
    };

    // Dodge: lighten the specified tonal range
    void dodge(GpuTexture layer, Vec2 position, float pressure, const Params& params);

    // Burn: darken the specified tonal range
    void burn(GpuTexture layer, Vec2 position, float pressure, const Params& params);
};

class SpongeTool {
public:
    enum Mode { Saturate, Desaturate };

    struct Params {
        Mode mode;
        float flow;              // 0-100%
        float size;
        float hardness;
        bool vibrance;           // Intelligent saturation (protect skin tones)
    };

    void apply(GpuTexture layer, Vec2 position, float pressure, const Params& params);
};

} // namespace gp::brush
```

### 16.4 Built-in Brush Presets

| Category | Brushes |
|---|---|
| **Basic** | Hard Round, Soft Round, Hard Square, Soft Square, Airbrush Soft, Airbrush Pen |
| **Dry Media** | Pencil, Charcoal, Crayon, Pastel Soft, Pastel Hard, Chalk, Conte |
| **Wet Media** | Watercolor Wash, Watercolor Splatter, Ink Pen, Ink Brush, Marker, Felt Tip |
| **Natural** | Grass, Leaves, Clouds, Stars, Snow, Rain, Smoke, Fire |
| **Texture** | Grunge, Concrete, Paper, Fabric, Wood Grain, Metal, Stone |
| **Special** | Scatter Dots, Sparkle, Bokeh, Confetti, Halftone, Noise, Pixel |
| **Retouching** | Skin Smooth, Frequency Separation, Dodge Soft, Burn Soft |
| **Calligraphy** | Flat Calligraphy, Pointed Pen, Brush Pen, Gothic, Italic |

### 16.5 Pressure Simulation (for Non-Pressure Devices)

```cpp
namespace gp::brush {

// For devices without pressure-sensitive input (mouse, trackpad)
class PressureSimulator {
public:
    enum Mode {
        Constant,           // Fixed pressure
        VelocityBased,      // Faster = lighter
        DistanceBased,      // Fade in/out over distance
        TimeBased,          // Fade in over time
    };

    float simulate(Vec2 current, Vec2 previous, float time_delta, Mode mode);

private:
    float velocity_to_pressure(float velocity);
    float distance_accumulator_ = 0;
    float time_accumulator_ = 0;
};

} // namespace gp::brush
```

### 16.6 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 17 | Wk 31-32 | IE-196 to IE-208 | Brush engine, dynamics, tools, presets |
| Sprint 18 | Wk 33-34 | IE-209 to IE-212 | Retouching tools, pressure simulator, pattern stamp |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-196 | As a developer, I want brush tip model (round, square, custom) so that brushes support multiple shapes | - Round and square built-in shapes<br/>- Custom tip from image texture<br/>- Size, hardness, opacity, flow, spacing, angle, roundness | 5 | Sprint 17 | — |
| IE-197 | As a developer, I want brush dynamics model (pressure, tilt, velocity, scatter) so that strokes feel natural | - Source: Off, Pressure, Tilt, Velocity, Direction, Random, Fade<br/>- Min/max value and jitter per property<br/>- Color dynamics (hue, sat, brightness) | 5 | Sprint 17 | IE-196 |
| IE-198 | As a developer, I want brush stroke interpolation (Catmull-Rom) so that strokes are smooth | - Catmull-Rom spline between input points<br/>- Configurable smoothing (0-100%)<br/>- Pulled string mode option | 3 | Sprint 17 | IE-196 |
| IE-199 | As a developer, I want GPU brush stamp compositing so that painting is real-time | - GPU-accelerated stamp rendering<br/>- Blend mode support per stamp<br/>- Accumulation to target layer | 5 | Sprint 17 | IE-196 |
| IE-200 | As a developer, I want brush accumulation buffer so that flow builds correctly | - Per-stroke accumulation buffer<br/>- Flow-based paint buildup<br/>- Wet-edge opacity accumulation | 3 | Sprint 17 | IE-199 |
| IE-201 | As a user, I want paint brush tool so that I can paint with natural strokes | - Full brush dynamics support<br/>- Foreground/background color<br/>- Undo per stroke | 3 | Sprint 17 | IE-199, IE-200 |
| IE-202 | As a user, I want pencil tool (hard-edge) so that I can draw crisp lines | - No anti-aliasing<br/>- Hard edge at 100% hardness<br/>- Pressure for opacity/size | 2 | Sprint 17 | IE-201 |
| IE-203 | As a user, I want eraser tool so that I can remove pixels to transparency | - Erase to layer transparency<br/>- Brush dynamics for size/opacity<br/>- Works on all layer types | 2 | Sprint 17 | IE-201 |
| IE-204 | As a user, I want smudge/blend brush so that I can blend colors like finger painting | - Push pixels in stroke direction<br/>- Strength and flow params<br/>- Mixer brush behavior | 5 | Sprint 17 | IE-201 |
| IE-205 | As a retoucher, I want blur brush (selective) so that I can soften specific areas | - Selective blur per stroke<br/>- Brush size and strength<br/>- GPU-accelerated blur | 3 | Sprint 17 | IE-201 |
| IE-206 | As a retoucher, I want sharpen brush (selective) so that I can enhance edges locally | - Selective sharpen per stroke<br/>- Brush size and strength<br/>- Avoid oversharpening artifacts | 3 | Sprint 17 | IE-201 |
| IE-207 | As a retoucher, I want dodge tool (lighten) so that I can brighten highlights/midtones/shadows | - Shadows, midtones, highlights range<br/>- Exposure 0-100%<br/>- Protect tones option | 3 | Sprint 17 | IE-201 |
| IE-208 | As a retoucher, I want burn tool (darken) so that I can darken specific tonal ranges | - Shadows, midtones, highlights range<br/>- Exposure 0-100%<br/>- Protect tones option | 3 | Sprint 17 | IE-201 |
| IE-209 | As a retoucher, I want sponge tool (saturate/desaturate) so that I can adjust color intensity locally | - Saturate and desaturate modes<br/>- Flow 0-100%<br/>- Vibrance option (protect skin) | 3 | Sprint 18 | IE-201 |
| IE-210 | As a user, I want brush preset library (50+) so that I have varied brushes out of the box | - 50+ presets across categories<br/>- Basic, dry media, wet media, natural, texture, special, retouching, calligraphy<br/>- Thumbnail and metadata | 5 | Sprint 18 | IE-196 |
| IE-211 | As a user, I want pressure simulator (non-pressure devices) so that I can vary strokes with mouse/trackpad | - Constant, velocity-based, distance-based, time-based modes<br/>- Simulated 0-1 pressure output<br/>- Configurable per preset | 3 | Sprint 18 | IE-197 |
| IE-212 | As a user, I want pattern stamp tool so that I can paint with seamless patterns | - Pattern selection from library<br/>- Aligned or continuous tiling<br/>- Scale and opacity | 5 | Sprint 18 | IE-201 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 6: Selection, Masking & Brush |
| **Sprint(s)** | IE-Sprint 17-18 (Weeks 31-34) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [18-project-serialization](18-project-serialization.md) |
| **Successor** | [15-transform-warp](15-transform-warp.md) |
| **Story Points Total** | 80 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-241 | As a developer, I want brush tip system (round, square, custom texture) so that brushes support multiple shapes | - Round and square built-in shapes<br/>- Custom tip from image texture<br/>- Size, hardness, opacity, flow, spacing, angle, roundness | 5 | P0 | — |
| IE-242 | As a developer, I want brush size and hardness so that brushes have configurable dimensions | - size (diameter 1-5000 px)<br/>- hardness (0-1 edge falloff)<br/>- Live preview during adjustment | 3 | P0 | IE-241 |
| IE-243 | As a developer, I want brush opacity and flow so that paint buildup is controllable | - opacity (0-1 per-stamp)<br/>- flow (0-1 accumulation rate)<br/>- Wet-edge buildup support | 3 | P0 | IE-241 |
| IE-244 | As a developer, I want brush dynamics: pressure sensitivity (Apple Pencil, S-Pen, Wacom) so that strokes feel natural | - Source::Pressure for size, opacity, flow<br/>- min_value, max_value per property<br/>- Platform input integration | 5 | P0 | IE-241 |
| IE-245 | As a developer, I want brush dynamics: tilt so that angled strokes vary | - Source::Tilt for angle, roundness<br/>- azimuth and altitude from stylus<br/>- Natural brush behavior | 3 | P0 | IE-244 |
| IE-246 | As a developer, I want brush dynamics: velocity so that stroke speed affects output | - Source::Velocity for size, opacity<br/>- Faster stroke = lighter/thinner<br/>- velocity_to_pressure() | 3 | P0 | IE-244 |
| IE-247 | As a developer, I want brush dynamics: scatter/jitter so that strokes have organic variation | - scatter (perpendicular scatter)<br/>- jitter (random variation 0-1)<br/>- count (stamps per spacing step) | 3 | P0 | IE-241 |
| IE-248 | As a developer, I want brush spacing control so that stamp density is configurable | - spacing (1-1000% of size)<br/>- Distance between stamps<br/>- Smooth stroke interpolation | 2 | P0 | IE-241 |
| IE-249 | As a developer, I want brush rotation (fixed, direction, tilt-based) so that brush angle is controllable | - Fixed angle (0-360)<br/>- Direction (stroke direction)<br/>- Tilt-based from stylus | 3 | P0 | IE-241 |
| IE-250 | As a user, I want paint brush tool so that I can paint with natural strokes | - Full brush dynamics support<br/>- Foreground/background color<br/>- Undo per stroke | 3 | P0 | IE-241, IE-244 |
| IE-251 | As a user, I want pencil tool (hard-edge) so that I can draw crisp lines | - No anti-aliasing<br/>- Hard edge at 100% hardness<br/>- Pressure for opacity/size | 2 | P0 | IE-250 |
| IE-252 | As a user, I want eraser tool (brush eraser, background eraser) so that I can remove pixels | - Erase to layer transparency<br/>- Brush dynamics for size/opacity<br/>- Works on all layer types | 2 | P0 | IE-250 |
| IE-253 | As a user, I want smudge tool (finger painting) so that I can blend colors | - Push pixels in stroke direction<br/>- Strength and flow params<br/>- Mixer brush behavior | 5 | P0 | IE-250 |
| IE-254 | As a retoucher, I want blur tool (local blur) so that I can soften specific areas | - Selective blur per stroke<br/>- Brush size and strength<br/>- GPU-accelerated blur | 3 | P0 | IE-250 |
| IE-255 | As a retoucher, I want sharpen tool (local sharpen) so that I can enhance edges locally | - Selective sharpen per stroke<br/>- Brush size and strength<br/>- Avoid oversharpening artifacts | 3 | P0 | IE-250 |
| IE-256 | As a retoucher, I want dodge tool (lighten) so that I can brighten highlights/midtones/shadows | - Shadows, midtones, highlights range<br/>- Exposure 0-100%<br/>- Protect tones option | 3 | P0 | IE-250 |
| IE-257 | As a retoucher, I want burn tool (darken) so that I can darken specific tonal ranges | - Shadows, midtones, highlights range<br/>- Exposure 0-100%<br/>- Protect tones option | 3 | P0 | IE-250 |
| IE-258 | As a retoucher, I want sponge tool (saturate/desaturate) so that I can adjust color intensity locally | - Saturate and desaturate modes<br/>- Flow 0-100%<br/>- Vibrance option (protect skin) | 3 | P0 | IE-250 |
| IE-259 | As a user, I want brush preset library (50+ built-in) so that I have varied brushes out of the box | - 50+ presets across categories<br/>- Basic, dry media, wet media, natural, texture, special, retouching, calligraphy<br/>- Thumbnail and metadata | 5 | P0 | IE-241 |
| IE-260 | As a user, I want custom brush creation from image so that I can create unique brushes | - Create BrushTip from image texture<br/>- custom_texture, custom_width, custom_height<br/>- Save as preset | 5 | P0 | IE-241 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
