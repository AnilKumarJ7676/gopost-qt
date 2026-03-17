
## IE-4. Canvas Engine

### 4.1 Canvas Data Model

```cpp
namespace gp::canvas {

struct CanvasSettings {
    int32_t width, height;             // Canvas dimensions in pixels
    float dpi;                         // 72 (screen), 150, 300 (print)
    ColorSpace color_space;            // sRGB, DisplayP3, AdobeRGB, ProPhotoRGB
    BitDepth bit_depth;                // Uint8, Uint16, Float32
    Color4f background_color;
    bool transparent_background;

    // Preset formats
    static CanvasSettings instagram_post()    { return {1080, 1080, 72, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings instagram_story()   { return {1080, 1920, 72, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings facebook_post()     { return {1200, 630, 72, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings twitter_post()      { return {1600, 900, 72, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings youtube_thumbnail() { return {1280, 720, 72, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings tiktok_cover()      { return {1080, 1920, 72, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings pinterest_pin()     { return {1000, 1500, 72, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings linkedin_post()     { return {1200, 1200, 72, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings a4_print()          { return {2480, 3508, 300, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings letter_print()      { return {2550, 3300, 300, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings poster_24x36()      { return {7200, 10800, 300, ColorSpace::sRGB, BitDepth::Uint16, {1,1,1,1}, false}; }
    static CanvasSettings business_card()     { return {1050, 600, 300, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
    static CanvasSettings custom(int w, int h, float dpi) { return {w, h, dpi, ColorSpace::sRGB, BitDepth::Uint8, {1,1,1,1}, false}; }
};

enum class ColorSpace { sRGB, DisplayP3, AdobeRGB, ProPhotoRGB, ACES_AP0, ACES_AP1, Rec2020 };
enum class BitDepth { Uint8, Uint16, Float16, Float32 };

// The top-level canvas object
struct Canvas {
    int32_t id;
    CanvasSettings settings;
    std::string name;

    // Layer tree (ordered bottom-to-top)
    std::vector<std::unique_ptr<Layer>> layers;

    // Guides, grids, rulers
    std::vector<Guide> guides;
    GridSettings grid;
    bool snap_to_grid;
    bool snap_to_guides;
    bool snap_to_layers;

    // Viewport state
    ViewportState viewport;

    // Operations
    int32_t add_layer(std::unique_ptr<Layer> layer, int32_t index = -1);
    void remove_layer(int32_t layer_id);
    void reorder_layer(int32_t layer_id, int32_t new_index);
    Layer* find_layer(int32_t id) const;
    Layer* find_layer_recursive(int32_t id) const; // Searches groups
    std::vector<Layer*> visible_layers() const;
    Rect canvas_rect() const;
};

struct ViewportState {
    Vec2 pan;             // Canvas offset in screen coordinates
    float zoom;           // 1.0 = 100%
    float rotation;       // Canvas rotation (degrees)
    Rect visible_region;  // Canvas region currently visible on screen

    Mat3 canvas_to_screen() const;
    Mat3 screen_to_canvas() const;
};

struct Guide {
    enum Type { Horizontal, Vertical };
    Type type;
    float position;       // In canvas pixels
    Color4f color;
    bool locked;
};

struct GridSettings {
    float spacing_x, spacing_y;
    int subdivisions;
    Color4f color;
    float opacity;
    bool visible;
};

} // namespace gp::canvas
```

### 4.2 Layer System

```cpp
namespace gp::canvas {

enum class LayerType {
    Image,              // Raster image (photo, imported bitmap)
    SolidColor,         // Filled rectangle
    Gradient,           // Gradient fill
    Text,               // Rich text layer
    Shape,              // Vector shape (rectangle, ellipse, polygon, custom path)
    Group,              // Layer group/folder (contains child layers)
    Adjustment,         // Non-destructive adjustment (applies to layers below)
    SmartObject,        // Embedded linked/embedded document
    Sticker,            // Pre-made sticker element (from library)
    Frame,              // Template placeholder frame (smart replace)
    Pattern,            // Pattern fill layer
    Brush,              // Painted brush strokes (rasterized)
};

struct Layer {
    int32_t id;
    LayerType type;
    std::string name;
    std::string label_color;

    // Transform
    Transform2D transform;

    // Properties
    float opacity;             // 0.0 - 1.0
    BlendMode blend_mode;
    bool visible;
    bool locked;
    bool pass_through;         // For groups: blend through vs isolated

    // Bounds
    Rect content_bounds;       // Actual content bounds (may differ from canvas)

    // Masks
    LayerMask* layer_mask;           // Grayscale mask (brush-painted)
    VectorMask* vector_mask;         // Bezier path mask
    bool clipping_mask;              // Clipped to layer below

    // Effects / styles
    std::vector<EffectInstance> effects;   // Non-destructive effects (shared system)
    LayerStyle layer_style;                // Drop shadow, stroke, bevel, etc.

    // Keyframe tracks (for animated templates)
    std::vector<KeyframeTrack> keyframe_tracks;

    // Type-specific content
    ImageLayerData* image_data;
    TextData* text_data;
    ShapeData* shape_data;
    GroupData* group_data;
    SmartObjectData* smart_object_data;
    StickerData* sticker_data;
    FrameData* frame_data;
    AdjustmentData* adjustment_data;
    GradientData* gradient_data;
    PatternData* pattern_data;
    BrushLayerData* brush_data;
    SolidColorData* solid_data;
};

} // namespace gp::canvas
```

### 4.3 Layer Content Types

```cpp
namespace gp::canvas {

struct ImageLayerData {
    int32_t media_ref_id;          // Reference to imported image in media pool
    Rect crop_rect;                // Crop within source image
    ImageAdjustments adjustments;  // Non-destructive image adjustments
    bool is_embedded;              // Embedded vs linked
};

struct SolidColorData {
    Color4f color;
};

struct GradientData {
    GradientType type;             // Linear, Radial, Angular, Diamond, Reflected
    std::vector<GradientStop> stops;
    Vec2 start_point;              // Normalized (0-1) within layer bounds
    Vec2 end_point;
    float angle;                   // For linear/angular
    float scale;
    bool dither;                   // Reduce banding
};

struct GradientStop {
    float position;                // 0.0 - 1.0
    Color4f color;
    float midpoint;                // 0.0 - 1.0 (bias between this and next stop)
};

struct GroupData {
    std::vector<std::unique_ptr<Layer>> children;
    bool expanded;                 // UI state

    int32_t add_child(std::unique_ptr<Layer> layer, int32_t index = -1);
    void remove_child(int32_t layer_id);
    void reorder_child(int32_t layer_id, int32_t new_index);
};

struct SmartObjectData {
    enum Source { Embedded, Linked };
    Source source_type;
    std::string linked_path;       // For linked smart objects
    std::vector<uint8_t> embedded_data; // For embedded (.gpimg sub-document)
    int32_t original_width, original_height;
    bool needs_update;             // Linked source has changed

    // Smart object allows non-destructive transforms + filters
    // Original content is preserved; edits produce a transformed output
};

struct StickerData {
    std::string sticker_id;        // ID from sticker library
    std::string pack_id;           // Sticker pack ID
    std::vector<uint8_t> svg_data; // SVG source (if vector sticker)
    int32_t raster_ref_id;         // Rasterized version (if bitmap sticker)
    bool is_vector;
};

struct FrameData {
    std::string placeholder_id;     // Unique placeholder identifier
    std::string placeholder_label;  // "Photo 1", "Logo", "Background"
    FrameShape shape;               // Rectangle, Circle, RoundedRect, CustomPath
    float corner_radius;
    BezierPath* custom_shape;       // For CustomPath
    int32_t content_layer_id;       // The image/content filling this frame (-1 = empty)
    FitMode fit_mode;               // Fill, Fit, Stretch, Tile
    Color4f placeholder_color;      // Color shown when empty
};

enum class FrameShape { Rectangle, Circle, Ellipse, RoundedRect, Star, Polygon, CustomPath };
enum class FitMode { Fill, Fit, Stretch, Tile, Original };

struct PatternData {
    int32_t pattern_id;            // Reference to pattern in library
    float scale;
    Vec2 offset;
    float rotation;
};

struct BrushLayerData {
    std::vector<BrushStroke> strokes;
    int32_t raster_cache_id;       // Cached rasterized version
};

struct AdjustmentData {
    AdjustmentType type;
    std::unordered_map<std::string, ParameterValue> parameters;

    // Adjustment types: same as effect types but applied non-destructively
    // to all layers below (within the group, or entire canvas)
};

enum class AdjustmentType {
    BrightnessContrast, Levels, Curves, Exposure, Vibrance,
    HueSaturation, ColorBalance, BlackAndWhite, PhotoFilter,
    ChannelMixer, ColorLookup, Invert, Posterize, Threshold,
    SelectiveColor, ShadowHighlight, HDRToning, GradientMap
};

} // namespace gp::canvas
```

### 4.4 Layer Style System (Photoshop-class)

```cpp
namespace gp::canvas {

struct LayerStyle {
    bool enabled;

    struct DropShadow {
        bool enabled;
        BlendMode blend_mode;
        Color4f color;
        float opacity;           // 0-100
        float angle;             // 0-360 degrees
        float distance;          // pixels
        float spread;            // 0-100%
        float size;              // blur radius
        bool use_global_light;
        ContourCurve contour;
        float noise;
    } drop_shadow;

    struct InnerShadow {
        bool enabled;
        BlendMode blend_mode;
        Color4f color;
        float opacity;
        float angle;
        float distance;
        float choke;
        float size;
        bool use_global_light;
        ContourCurve contour;
    } inner_shadow;

    struct OuterGlow {
        bool enabled;
        BlendMode blend_mode;
        float opacity;
        float noise;
        GlowSource source;      // Color or Gradient
        Color4f color;
        std::vector<GradientStop> gradient;
        GlowTechnique technique; // Softer, Precise
        float spread;
        float size;
        ContourCurve contour;
        float range;
        float jitter;
    } outer_glow;

    struct InnerGlow {
        bool enabled;
        BlendMode blend_mode;
        float opacity;
        GlowSource source;
        Color4f color;
        GlowTechnique technique;
        InnerGlowSource glow_source; // Center, Edge
        float choke;
        float size;
        ContourCurve contour;
    } inner_glow;

    struct BevelEmboss {
        bool enabled;
        BevelStyle style;       // Outer, Inner, Emboss, PillowEmboss, StrokeEmboss
        BevelTechnique technique; // Smooth, ChiselHard, ChiselSoft
        float depth;
        BevelDirection direction; // Up, Down
        float size;
        float soften;
        float angle;
        float altitude;
        Color4f highlight_color;
        float highlight_opacity;
        BlendMode highlight_mode;
        Color4f shadow_color;
        float shadow_opacity;
        BlendMode shadow_mode;
        ContourCurve gloss_contour;
        ContourCurve contour;
    } bevel_emboss;

    struct Stroke {
        bool enabled;
        float size;              // pixels
        StrokePosition position; // Outside, Inside, Center
        BlendMode blend_mode;
        float opacity;
        StrokeFillType fill_type; // Color, Gradient, Pattern
        Color4f color;
        std::vector<GradientStop> gradient;
        int32_t pattern_id;
    } stroke;

    struct ColorOverlay {
        bool enabled;
        BlendMode blend_mode;
        Color4f color;
        float opacity;
    } color_overlay;

    struct GradientOverlay {
        bool enabled;
        BlendMode blend_mode;
        float opacity;
        GradientType gradient_type;
        std::vector<GradientStop> gradient;
        float angle;
        float scale;
        bool reverse;
        bool align_with_layer;
    } gradient_overlay;

    struct PatternOverlay {
        bool enabled;
        BlendMode blend_mode;
        float opacity;
        int32_t pattern_id;
        float scale;
        bool link_with_layer;
    } pattern_overlay;

    // Render order (bottom to top):
    // 1. Drop Shadow
    // 2. Outer Glow
    // 3. Fill (layer content)
    // 4. Inner Shadow
    // 5. Inner Glow
    // 6. Bevel and Emboss
    // 7. Satin
    // 8. Color Overlay
    // 9. Gradient Overlay
    // 10. Pattern Overlay
    // 11. Stroke
};

enum class BevelStyle { OuterBevel, InnerBevel, Emboss, PillowEmboss, StrokeEmboss };
enum class BevelTechnique { Smooth, ChiselHard, ChiselSoft };
enum class BevelDirection { Up, Down };
enum class GlowSource { ColorSource, GradientSource };
enum class GlowTechnique { Softer, Precise };
enum class InnerGlowSource { Center, Edge };
enum class StrokePosition { Outside, Inside, Center };
enum class StrokeFillType { Color, Gradient, Pattern };

struct ContourCurve {
    std::vector<Vec2> points;  // 0-1 range, defines falloff shape
    bool anti_aliased;
};

} // namespace gp::canvas
```

### 4.5 Media Pool

```cpp
namespace gp::canvas {

struct MediaPool {
    struct MediaEntry {
        int32_t id;
        std::string name;
        std::string original_path;
        MediaType type;                // Image, SVG, Font, Pattern, Sticker
        int32_t width, height;
        size_t file_size;
        std::string color_profile;     // ICC profile name
        BitDepth bit_depth;
        bool has_alpha;
        std::string hash;             // SHA-256 for deduplication

        // Cached data
        GpuTexture thumbnail;
        GpuTexture proxy;              // Downsampled version for preview
        bool proxy_loaded;
    };

    std::vector<MediaEntry> entries;

    int32_t import_image(const std::string& path);
    int32_t import_svg(const std::string& path);
    int32_t import_pattern(const std::string& path);
    void remove(int32_t media_id);
    const MediaEntry* find(int32_t media_id) const;
    GpuTexture get_texture(int32_t media_id, int max_size = 0); // 0 = full res
};

enum class MediaType { Image, SVG, Font, Pattern, Sticker, EmbeddedDocument };

}
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Core Foundation |
| **Sprint(s)** | IE-Sprint 2-3 (Weeks 3-5) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [03-core-foundation](03-core-foundation.md) |
| **Successor** | [05-composition-engine](05-composition-engine.md) |
| **Story Points Total** | 89 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-026 | As a C++ engine developer, I want CanvasSettings (dimensions, DPI, color profile, background) so that canvas configuration is complete | - width, height, dpi, color_space, bit_depth, background_color<br/>- Preset methods (instagram_post, a4_print, etc.)<br/>- transparent_background support | 3 | P0 | IE-025 |
| IE-027 | As a C++ engine developer, I want layer tree data model (ordered list, parent-child groups) so that layers can be composed | - std::vector of Layer pointers, bottom-to-top order<br/>- GroupLayer with children<br/>- add_layer, remove_layer, reorder_layer, find_layer | 5 | P0 | IE-026 |
| IE-028 | As a designer, I want ImageLayer with crop region so that I can use photos in compositions | - ImageLayerData with media_ref_id, crop_rect<br/>- ImageAdjustments for non-destructive edits<br/>- is_embedded vs linked | 3 | P0 | IE-027 |
| IE-029 | As a designer, I want SolidColorLayer so that I can add flat color fills | - SolidColorData with Color4f<br/>- Renders at layer bounds<br/>- Opacity and blend mode support | 2 | P0 | IE-027 |
| IE-030 | As a designer, I want GradientLayer (linear/radial/angular/diamond) so that I can add gradient fills | - GradientData with type, stops, start/end points<br/>- Linear, radial, angular, diamond, reflected<br/>- Dither option for banding reduction | 5 | P0 | IE-027 |
| IE-031 | As a designer, I want TextLayer so that I can add editable text | - TextLayerData with TextData from shared text module<br/>- Renders via FontManager<br/>- Style runs support | 5 | P0 | IE-027 |
| IE-032 | As a designer, I want ShapeLayer so that I can add vector shapes | - ShapeLayerData with ShapeGeometry, fill, stroke<br/>- Rectangle, ellipse, polygon, star, custom path<br/>- Rasterizes to texture for compositing | 5 | P0 | IE-027 |
| IE-033 | As a designer, I want GroupLayer so that I can organize layers in folders | - GroupData with children vector<br/>- add_child, remove_child, reorder_child<br/>- pass_through vs isolated blend | 3 | P0 | IE-027 |
| IE-034 | As a designer, I want AdjustmentLayer so that I can apply non-destructive adjustments | - AdjustmentData with type and parameters<br/>- Applies to all layers below<br/>- BrightnessContrast, Levels, Curves, etc. | 5 | P0 | IE-027 |
| IE-035 | As a designer, I want SmartObjectLayer so that I can scale non-destructively | - SmartObjectData with embedded/linked source<br/>- Original dimensions preserved<br/>- needs_update flag for linked | 5 | P0 | IE-027 |
| IE-036 | As a C++ engine developer, I want ViewportState (zoom, pan, rotation) so that canvas can be navigated | - pan, zoom, rotation, visible_region<br/>- canvas_to_screen(), screen_to_canvas() matrices<br/>- Integrates with renderer | 3 | P0 | IE-026 |
| IE-037 | As a C++ engine developer, I want MediaPool (import JPEG/PNG/WebP/HEIF/RAW) so that assets can be imported | - import_image(), import_svg(), import_pattern()<br/>- MediaEntry with thumbnail, proxy<br/>- Hash-based deduplication | 5 | P0 | IE-025 |
| IE-038 | As a C++ engine developer, I want image decode pipeline (async, proxy generation) so that large images load quickly | - Async decode on decode thread<br/>- Proxy generation for viewport<br/>- Completion callback to main | 5 | P0 | IE-037 |
| IE-039 | As a C++ engine developer, I want TileGrid system (tile size, dirty region tracking) so that large canvases render | - TileGrid with configurable tile size (2048)<br/>- invalidate_region(), visible_tiles(), dirty_tiles()<br/>- Overlap for seamless filters | 5 | P0 | IE-027 |
| IE-040 | As a Flutter developer, I want canvas create/destroy FFI API so that I can manage canvas lifecycle | - gp_ie_canvas_create(), gp_ie_canvas_destroy()<br/>- C API in api/ module<br/>- Flutter FFI bindings compile | 3 | P0 | IE-026 |
| IE-041 | As a Flutter developer, I want layer add/remove/reorder FFI API so that I can manipulate layers from Flutter | - gp_ie_layer_add(), gp_ie_layer_remove(), gp_ie_layer_reorder()<br/>- Layer ID returned for add<br/>- Thread-safe via command queue | 5 | P0 | IE-027, IE-040 |
| IE-042 | As a Flutter developer, I want interactive canvas renderer (viewport zoom/pan rendering) so that users can navigate | - render_viewport() at screen resolution<br/>- 60 fps for ≤50 layers<br/>- Zoom/pan updates trigger re-render | 5 | P0 | IE-036, IE-039 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
