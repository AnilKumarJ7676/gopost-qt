
## IE-9. Text and Typography Engine (Shared + IE Extensions)

### 9.1 Shared Text Infrastructure

The image editor shares the core text engine from the video editor:
- **TextData** — Rich text model with style runs
- **CharacterStyle** — Font, size, color, stroke, tracking
- **ParagraphStyle** — Alignment, direction, box text
- **FontManager** — FreeType + HarfBuzz layout, glyph atlas rasterization
- **TextLayout** — Glyph positioning, line breaks, bounding box

### 9.2 IE-Specific Text Features

```cpp
namespace gp::text {

// Extended text features for image editor / template creation

struct TextLayerData {
    TextData base_data;              // Shared rich text model

    // Text frame (bounding box for auto-fit)
    Rect text_frame;
    TextFrameAutoSize auto_size;     // None, Height, Width, Both, ShrinkToFit

    // Text warping (Photoshop text warp)
    TextWarpStyle warp_style;
    float warp_bend;                 // -100 to +100
    float warp_horizontal;           // -100 to +100
    float warp_vertical;             // -100 to +100

    // Text on shape (circle, wave, etc.)
    TextOnShapeType shape_type;
    float shape_radius;              // For circle
    float shape_amplitude;           // For wave
    float shape_frequency;           // For wave
    float text_offset;               // Start position along path

    // Template placeholder
    bool is_placeholder;             // Editable placeholder in templates
    std::string placeholder_label;   // "Headline", "Subtitle", "Body"
    std::string placeholder_hint;    // "Enter your text here"
    int max_characters;              // Limit for template fields

    // Per-character animation (for animated templates)
    TextAnimatorGroup* animators;    // Shared from VE
};

enum class TextFrameAutoSize {
    None,           // Fixed frame, text may overflow
    Height,         // Width fixed, height grows to fit
    Width,          // Height fixed, width grows to fit
    Both,           // Frame grows in both directions
    ShrinkToFit,    // Font size reduces to fit frame
};

enum class TextWarpStyle {
    None, Arc, ArcLower, ArcUpper, Arch, Bulge, ShellLower, ShellUpper,
    Flag, Wave, Fish, Rise, FishEye, Inflate, Squeeze, Twist,
};

enum class TextOnShapeType {
    None, Circle, Ellipse, Wave, Spiral, CustomPath,
};

} // namespace gp::text
```

### 9.3 Text Effects Engine

```cpp
namespace gp::text {

// Text-specific visual effects (beyond layer styles)
// These are rendered as part of the text rasterization pipeline

struct TextEffects {
    // 3D Text extrusion
    struct Extrude3D {
        bool enabled;
        float depth;              // Extrusion depth
        float bevel_width;
        float bevel_height;
        BevelType bevel_type;     // Round, Chisel, Flat
        Color4f front_color;
        Color4f extrude_color;
        Color4f bevel_color;
        Vec3 light_direction;
        float ambient;
        float specular;
    } extrude_3d;

    // Neon text
    struct NeonEffect {
        bool enabled;
        Color4f glow_color;
        float glow_size;
        float glow_intensity;
        float flicker_amount;     // For animated templates
        bool inner_glow;
    } neon;

    // Gradient text fill (beyond solid color)
    struct GradientFill {
        bool enabled;
        GradientType type;
        std::vector<GradientStop> stops;
        float angle;
        float scale;
    } gradient_fill;

    // Pattern fill
    struct PatternFill {
        bool enabled;
        int32_t pattern_id;
        float scale;
        Vec2 offset;
    } pattern_fill;

    // Image fill (clip image inside text)
    struct ImageFill {
        bool enabled;
        int32_t media_ref_id;
        FitMode fit;
        Vec2 offset;
        float scale;
    } image_fill;

    // Text outline (multiple outlines supported)
    struct Outline {
        bool enabled;
        Color4f color;
        float width;
        StrokePosition position;  // Outside, Inside, Center
        LineJoin join;            // Miter, Round, Bevel
    };
    std::vector<Outline> outlines;

    // Shadow (text-level, separate from layer style)
    struct TextShadow {
        bool enabled;
        Color4f color;
        Vec2 offset;
        float blur;
    };
    std::vector<TextShadow> shadows;

    // Background (text box background)
    struct Background {
        bool enabled;
        Color4f color;
        float opacity;
        float corner_radius;
        Vec2 padding;
        float border_width;
        Color4f border_color;
    } background;

    // Curved / arc text
    struct CurvedText {
        bool enabled;
        float curvature;          // -360 to +360 degrees
        CurveType type;           // Arc, Bridge, Valley
    } curved;

    // Letter spacing animation (typewriter effect for animated templates)
    struct TypewriterEffect {
        bool enabled;
        float speed;              // Characters per second
        float cursor_blink_rate;
    } typewriter;
};

enum class BevelType { Round, Chisel, Flat, Custom };
enum class CurveType { Arc, Bridge, Valley };

} // namespace gp::text
```

### 9.4 Text Style Presets (Template-Ready)

```cpp
namespace gp::text {

struct TextStylePreset {
    std::string id;
    std::string name;
    std::string category;          // "Heading", "Subtitle", "Body", "Quote", "Caption"
    std::string thumbnail_path;

    // Complete text style definition
    CharacterStyle character;
    ParagraphStyle paragraph;
    TextEffects effects;
    LayerStyle layer_style;        // Drop shadow, stroke, etc.
    TextWarpStyle warp;
    float warp_bend;

    // For templates: how this text adapts to content
    TextFrameAutoSize auto_size;
    int min_font_size;             // For ShrinkToFit
    int max_lines;
};

class TextStyleLibrary {
public:
    static TextStyleLibrary& instance();

    void register_preset(const TextStylePreset& preset);
    const TextStylePreset* find(const std::string& id) const;
    std::vector<const TextStylePreset*> list_by_category(const std::string& category) const;

    // Built-in presets (100+)
    void load_built_in_presets();

    // Apply preset to text layer
    void apply_to_layer(int32_t layer_id, const std::string& preset_id);
};

} // namespace gp::text
```

### 9.5 Font Management Extensions

```cpp
namespace gp::text {

// Extended font features for image editor
class FontManagerIE : public FontManager {
public:
    // Font pairing suggestions (for templates)
    struct FontPairing {
        std::string heading_family;
        std::string heading_style;
        std::string body_family;
        std::string body_style;
        std::string description;   // "Modern & Clean", "Elegant & Classic"
    };
    std::vector<FontPairing> suggest_pairings(const std::string& font_family) const;

    // Font categories for browsing
    enum FontCategory {
        Serif, SansSerif, Display, Handwriting, Monospace, Script, Decorative
    };
    std::vector<std::string> families_by_category(FontCategory category) const;

    // Recently used fonts
    void mark_used(const std::string& family);
    std::vector<std::string> recent_fonts(int max_count = 20) const;

    // Font preview rendering
    GpuTexture render_preview(const std::string& family, const std::string& style,
                               const std::string& sample_text, int size, IGpuContext& gpu);

    // Google Fonts integration (download on demand)
    struct GoogleFontEntry {
        std::string family;
        std::vector<std::string> styles;
        std::string category;
        float popularity;
        std::string download_url;
    };
    std::vector<GoogleFontEntry> search_google_fonts(const std::string& query) const;
    TaskHandle download_font(const GoogleFontEntry& entry);
};

} // namespace gp::text
```

### 9.6 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 10 | Wk 17-18 | IE-093 to IE-105 | Text engine, rich text, text effects, warp, presets |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-093 | As a designer, I want shared text engine integration so that text layers use the same layout and rendering as the video editor | - TextData, CharacterStyle, ParagraphStyle from shared module<br/>- FontManager + HarfBuzz layout<br/>- Glyph atlas rasterization | 5 | Sprint 10 | — |
| IE-094 | As a designer, I want rich text layers (multi-style runs) so that I can mix fonts, sizes, and colors in one block | - TextLayerData with style runs<br/>- Per-run CharacterStyle override<br/>- Rendered with correct glyph positioning | 5 | Sprint 10 | IE-093 |
| IE-095 | As a designer, I want paragraph style (alignment, direction, box text) so that I can control text layout | - ParagraphStyle: alignment, direction, box text<br/>- Line breaks and bounding box from TextLayout<br/>- RTL support | 3 | Sprint 10 | IE-093 |
| IE-096 | As a designer, I want text frame auto-size (shrink-to-fit) so that text fits within a bounding box | - TextFrameAutoSize: None, Height, Width, Both, ShrinkToFit<br/>- Font size reduces for ShrinkToFit<br/>- min_font_size, max_lines constraints | 5 | Sprint 10 | IE-094 |
| IE-097 | As a designer, I want text warp (16 preset shapes) so that I can curve and distort text | - TextWarpStyle: Arc, Bulge, Wave, Fish, etc. (16 presets)<br/>- warp_bend, warp_horizontal, warp_vertical params<br/>- Rendered in text rasterization pipeline | 5 | Sprint 10 | IE-094 |
| IE-098 | As a designer, I want text on shape (circle, wave, spiral) so that I can place text along paths | - TextOnShapeType: Circle, Ellipse, Wave, Spiral, CustomPath<br/>- shape_radius, shape_amplitude, shape_frequency<br/>- text_offset for start position | 5 | Sprint 10 | IE-094 |
| IE-099 | As a designer, I want text effects: gradient fill so that I can fill text with gradients | - GradientFill in TextEffects (type, stops, angle, scale)<br/>- Rendered as part of text pipeline<br/>- Linear and radial gradient support | 3 | Sprint 10 | IE-094 |
| IE-100 | As a designer, I want text effects: image fill so that I can clip an image inside text | - ImageFill with media_ref_id, fit, offset, scale<br/>- Rendered with correct clipping<br/>- Supports Fill, Fit, Stretch modes | 3 | Sprint 10 | IE-094 |
| IE-101 | As a designer, I want text effects: multiple outlines so that I can add stroke effects to text | - std::vector&lt;Outline&gt; with color, width, position, join<br/>- Outside, Inside, Center stroke position<br/>- Miter, Round, Bevel joins | 3 | Sprint 10 | IE-094 |
| IE-102 | As a designer, I want text effects: neon glow so that I can add glow to text | - NeonEffect: glow_color, glow_size, glow_intensity<br/>- inner_glow option<br/>- Optional flicker for animated templates | 3 | Sprint 10 | IE-094 |
| IE-103 | As a designer, I want text effects: 3D extrusion so that I can create dimensional text | - Extrude3D: depth, bevel, colors, light direction<br/>- BevelType: Round, Chisel, Flat<br/>- Ambient and specular lighting | 5 | Sprint 10 | IE-094 |
| IE-104 | As a designer, I want a text style preset library (100+) so that I can apply professional typography quickly | - TextStyleLibrary with 100+ built-in presets<br/>- Categories: Heading, Subtitle, Body, Quote, Caption<br/>- apply_to_layer() applies preset | 5 | Sprint 10 | IE-094 |
| IE-105 | As a template creator, I want text placeholders for templates so that users can edit predefined text fields | - is_placeholder, placeholder_label, placeholder_hint<br/>- max_characters constraint<br/>- Editable in template customization UI | 3 | Sprint 10 | IE-094 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 4: Text & Vector Graphics |
| **Sprint(s)** | IE-Sprint 10-11 (Weeks 17-20) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [08-image-processing](08-image-processing.md) |
| **Successor** | [10-vector-graphics](10-vector-graphics.md) |
| **Story Points Total** | 72 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-113 | As a C++ engine developer, I want shared text engine integration from VE (FreeType + HarfBuzz) so that text renders correctly | - TextData, CharacterStyle, ParagraphStyle from shared module<br/>- FontManager + HarfBuzz layout<br/>- Glyph atlas rasterization | 5 | P0 | — |
| IE-114 | As a designer, I want text layer creation and editing so that I can add and edit text on canvas | - TextLayer with TextLayerData<br/>- Create, edit, delete text layers<br/>- Cursor positioning, selection | 5 | P0 | IE-113 |
| IE-115 | As a designer, I want rich text: multi-style runs within single text box so that I can mix fonts and styles | - Style runs with per-run CharacterStyle<br/>- Font, size, color per run<br/>- Correct glyph positioning | 5 | P0 | IE-114 |
| IE-116 | As a designer, I want paragraph alignment (left/center/right/justify) so that I can control text layout | - ParagraphStyle: alignment, direction<br/>- Left, center, right, justify<br/>- RTL support | 2 | P0 | IE-114 |
| IE-117 | As a designer, I want box text with overflow handling so that text fits within bounds | - Text frame with overflow modes<br/>- Overflow hidden, scroll, ellipsis<br/>- Bounding box from TextLayout | 3 | P0 | IE-114 |
| IE-118 | As a designer, I want text fill: solid color so that I can set text color | - CharacterStyle fill color<br/>- Opacity support<br/>- Renders correctly | 2 | P0 | IE-114 |
| IE-119 | As a designer, I want text fill: gradient fill so that I can fill text with gradients | - GradientFill in TextEffects<br/>- Linear and radial<br/>- Angle, scale, stops | 3 | P0 | IE-114 |
| IE-120 | As a designer, I want text fill: image/pattern fill so that I can clip image inside text | - ImageFill, PatternFill in TextEffects<br/>- media_ref_id, pattern_id<br/>- Fit modes | 3 | P0 | IE-114 |
| IE-121 | As a designer, I want text outline (color, width, position) so that I can add stroke to text | - Outline with color, width, position (Outside/Inside/Center)<br/>- LineJoin: Miter, Round, Bevel<br/>- Multiple outlines support | 3 | P0 | IE-114 |
| IE-122 | As a designer, I want text shadow (color, distance, blur, angle) so that I can add shadow to text | - TextShadow with color, offset, blur<br/>- Multiple shadows<br/>- Renders in text pipeline | 3 | P0 | IE-114 |
| IE-123 | As a designer, I want text warp presets (arc, wave, flag, fish, inflate, etc.) so that I can curve text | - TextWarpStyle: Arc, Wave, Flag, Fish, Inflate, etc.<br/>- warp_bend, warp_horizontal, warp_vertical<br/>- 16+ presets | 5 | P0 | IE-114 |
| IE-124 | As a designer, I want text-on-path (circle, custom bezier) so that I can place text along paths | - TextOnShapeType: Circle, CustomPath<br/>- shape_radius, text_offset<br/>- Glyphs positioned along path | 5 | P0 | IE-114 |
| IE-125 | As a designer, I want text-on-shape (rectangle, ellipse) so that I can place text in shapes | - TextOnShapeType: Rectangle, Ellipse<br/>- Box text within shape bounds<br/>- Overflow handling | 3 | P0 | IE-114 |
| IE-126 | As a designer, I want text style presets (100+ pre-designed) so that I can apply professional typography | - TextStyleLibrary with 100+ presets<br/>- Categories: Heading, Subtitle, Body, Quote<br/>- apply_to_layer() | 5 | P0 | IE-114 |
| IE-127 | As a designer, I want per-character spacing (tracking/kerning) so that I can adjust letter spacing | - Tracking (global letter spacing)<br/>- Kerning (pair adjustment)<br/>- OpenType kerning support | 3 | P0 | IE-113 |
| IE-128 | As a designer, I want line spacing and paragraph spacing so that I can control vertical rhythm | - Line spacing (leading)<br/>- Paragraph spacing (before/after)<br/>- Unit support (pt, px, %) | 2 | P0 | IE-114 |
| IE-129 | As a designer, I want text auto-shrink to fit box so that text fits within bounds | - TextFrameAutoSize::ShrinkToFit<br/>- min_font_size, max_lines<br/>- Font size reduces to fit | 3 | P0 | IE-117 |
| IE-130 | As a designer, I want font management (system + bundled + Google Fonts download) so that I can use many fonts | - System fonts enumeration<br/>- Bundled fonts in app<br/>- Google Fonts search and download | 5 | P0 | IE-113 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
