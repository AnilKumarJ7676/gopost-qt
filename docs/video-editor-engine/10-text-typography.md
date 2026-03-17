
## VE-10. Text and Typography Engine

### 10.1 Text Data Model

```cpp
namespace gp::text {

struct TextData {
    std::string source_text;               // Raw text content, keyframeable
    std::vector<TextStyleRun> style_runs;  // Ranges of styled text

    ParagraphStyle paragraph;
    PathTextOptions* path_options;          // Text on a path (optional)
    TextAnimatorGroup* animators;           // Per-character animation
};

struct TextStyleRun {
    int32_t start_index;    // Character index range start (inclusive)
    int32_t end_index;      // Character index range end (exclusive)
    CharacterStyle style;
};

struct CharacterStyle {
    std::string font_family;
    std::string font_style;        // Regular, Bold, Italic, BoldItalic
    float font_size;               // Points
    float tracking;                // Letter spacing (1/1000 em)
    float leading;                 // Line spacing (auto or absolute)
    float baseline_shift;

    Color4f fill_color;
    bool fill_enabled;
    Color4f stroke_color;
    float stroke_width;
    bool stroke_enabled;
    StrokePosition stroke_position; // Over, Under

    bool all_caps;
    bool small_caps;
    bool superscript;
    bool subscript;

    float horizontal_scale;        // 100% = normal
    float vertical_scale;
};

struct ParagraphStyle {
    TextAlignment alignment;        // Left, Center, Right, Justify
    TextDirection direction;        // LTR, RTL, Auto
    float first_line_indent;
    float space_before;
    float space_after;

    // Box text (constrained to rectangle)
    bool is_box_text;
    float box_width;
    float box_height;
    TextOverflow overflow;          // Visible, Hidden, Scroll
};

struct PathTextOptions {
    int32_t path_layer_id;         // Mask path or shape path to follow
    int32_t path_index;            // Which path on the layer
    float first_margin;            // Start offset along path
    float last_margin;             // End offset
    bool perpendicular_to_path;
    bool force_alignment;
    PathTextReverse reverse;
};

} // namespace gp::text
```

### 10.2 Text Animators (Per-Character Animation)

After Effects-style text animators allow applying transform, color, and blur properties progressively across characters, words, or lines using a range selector:

```cpp
namespace gp::text {

struct TextAnimator {
    std::string name;
    std::vector<TextRangeSelector> selectors;
    TextAnimatorProperties properties;
};

struct TextRangeSelector {
    SelectorType type;              // Range, Wiggly, Expression

    // Range selector
    float start;                    // 0–100%, keyframeable
    float end;                      // 0–100%, keyframeable
    float offset;                   // Keyframeable
    RangeShape shape;               // Square, Ramp Up, Ramp Down, Triangle, Round, Smooth
    RangeUnits units;               // Percentage, Index
    RangeBasis basis;               // Characters, Characters Excluding Spaces, Words, Lines
    TextOrder order;                // Normal, Reverse (for cascading animations)

    // Wiggly selector
    float wiggly_max_amount;
    float wiggly_min_amount;
    float wiggly_speed;             // Wiggles per second
    WigglyCorrelation correlation;  // 0–100%
    WigglyMode mode;                // Add, Intersect, Min, Max, Difference

    // Expression selector
    std::string expression;

    // Evaluate: returns a weight (0–1) for each character at a given time
    std::vector<float> evaluate(int character_count, Rational time) const;
};

struct TextAnimatorProperties {
    // All optional, only set properties are animated
    std::optional<Vec2> position;
    std::optional<Vec2> anchor_point;
    std::optional<Vec2> scale;
    std::optional<float> rotation;
    std::optional<float> skew;
    std::optional<float> skew_axis;
    std::optional<float> opacity;
    std::optional<Color4f> fill_color;
    std::optional<Color4f> stroke_color;
    std::optional<float> stroke_width;
    std::optional<float> tracking;
    std::optional<float> line_anchor;
    std::optional<float> line_spacing;
    std::optional<float> character_offset;
    std::optional<float> blur;
};

struct TextAnimatorGroup {
    std::vector<TextAnimator> animators;

    // Evaluate all animators and produce per-character transforms
    std::vector<CharacterTransform> evaluate(int char_count, Rational time) const;
};

struct CharacterTransform {
    Vec2 position;
    Vec2 scale;
    float rotation;
    float opacity;
    Color4f fill_color;
    Color4f stroke_color;
    float blur;
};

} // namespace gp::text
```

### 10.3 Font Management

```cpp
namespace gp::text {

class FontManager {
public:
    static FontManager& instance();

    // Load fonts from bundled assets or system
    void register_font(const std::string& family, const std::string& style,
                       const uint8_t* data, size_t size);
    void load_system_fonts();

    // Font resolution
    FTFace* resolve(const std::string& family, const std::string& style) const;
    std::vector<std::string> available_families() const;
    std::vector<std::string> styles_for_family(const std::string& family) const;

    // Text layout (HarfBuzz + FreeType)
    TextLayout layout(const TextData& data, int comp_width, int comp_height) const;

    // Rasterize glyphs to GPU texture atlas
    GpuTexture rasterize_to_atlas(const TextLayout& layout, IGpuContext& gpu);

private:
    FT_Library ft_library_;
    hb_buffer_t* hb_buffer_;
    std::unordered_map<std::string, std::vector<FontEntry>> fonts_;
    GlyphAtlas glyph_atlas_;
};

struct TextLayout {
    std::vector<GlyphRun> runs;
    Rect bounding_box;
    std::vector<Vec2> character_positions;
    std::vector<float> character_advances;
    std::vector<int32_t> line_breaks;
};

} // namespace gp::text
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 4: Transitions/Text/Audio |
| **Sprint(s)** | VE-Sprint 11-12 (Weeks 21-24) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [09-motion-graphics](09-motion-graphics.md) |
| **Successor** | [11-audio-engine](11-audio-engine.md) |
| **Story Points Total** | 72 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-149 | As a C++ engine developer, I want TextData model (source text, style runs, paragraph) so that we have the core text structure | - TextData: source_text, style_runs (TextStyleRun), paragraph<br/>- PathTextOptions, TextAnimatorGroup optional<br/>- source_text keyframeable | 5 | P0 | VE-057 |
| VE-150 | As a C++ engine developer, I want CharacterStyle (font, size, tracking, fill, stroke, caps, scale) so that we can style characters | - font_family, font_style, font_size, tracking, leading<br/>- fill_color, stroke_color, stroke_width, stroke_position<br/>- all_caps, small_caps, superscript, subscript, horizontal/vertical scale | 5 | P0 | VE-149 |
| VE-151 | As a C++ engine developer, I want ParagraphStyle (alignment, direction, box text, overflow) so that we support paragraph formatting | - alignment (Left, Center, Right, Justify), direction (LTR, RTL)<br/>- first_line_indent, space_before, space_after<br/>- is_box_text, box_width, box_height, overflow | 3 | P0 | VE-149 |
| VE-152 | As a C++ engine developer, I want FreeType library init and font loading so that we can load and rasterize fonts | - FT_Library init, FT_Face load from file/memory<br/>- Glyph metrics, kerning<br/>- Platform font path resolution | 5 | P0 | VE-006 |
| VE-153 | As a C++ engine developer, I want HarfBuzz text shaping integration so that we support complex scripts | - hb_buffer_t for text, hb_shape<br/>- Script, direction, language support<br/>- Glyph clusters, positions | 8 | P0 | VE-152 |
| VE-154 | As a C++ engine developer, I want TextLayout computation (glyph runs, bounding box, positions) so that we have layout for rendering | - GlyphRun per style run<br/>- bounding_box, character_positions, character_advances<br/>- line_breaks for multi-line | 5 | P0 | VE-153 |
| VE-155 | As a C++ engine developer, I want Glyph atlas rasterization to GPU texture so that we can render text on GPU | - Rasterize glyphs to atlas texture<br/>- Atlas packing (bin packing or fixed grid)<br/>- Upload to GpuTexture | 5 | P0 | VE-152, VE-081 |
| VE-156 | As a C++ engine developer, I want FontManager (register, resolve, system fonts, available families) so that we can manage fonts | - register_font, load_system_fonts<br/>- resolve(family, style) returns FTFace<br/>- available_families, styles_for_family | 5 | P0 | VE-152 |
| VE-157 | As a C++ engine developer, I want Rich text multi-style rendering so that we can render text with multiple styles | - Multiple TextStyleRun in one TextData<br/>- Layout merges runs, applies styles per run<br/>- Correct glyph positioning across runs | 5 | P0 | VE-154, VE-155 |
| VE-158 | As a C++ engine developer, I want PathTextOptions (text-on-path) so that we can place text along a path | - path_layer_id, path_index, first_margin, last_margin<br/>- perpendicular_to_path, force_alignment, reverse<br/>- Glyph positions along path | 5 | P1 | VE-131, VE-154 |
| VE-159 | As a C++ engine developer, I want TextAnimator with range selectors so that we can animate text per-character | - TextAnimator: selectors, properties<br/>- TextRangeSelector: start, end, offset, shape, units, basis, order<br/>- evaluate(character_count, time) → weights | 8 | P0 | VE-149 |
| VE-160 | As a C++ engine developer, I want TextRangeSelector (range, wiggly, expression types) so that we support different selector kinds | - Range: start/end/offset, shape (Square, Ramp, etc.)<br/>- Wiggly: max/min amount, speed, correlation<br/>- Expression: expression string | 5 | P0 | VE-159 |
| VE-161 | As a C++ engine developer, I want TextAnimatorProperties (position, scale, rotation, opacity, colors, blur) so that we can animate character properties | - Optional: position, scale, rotation, opacity<br/>- fill_color, stroke_color, stroke_width, tracking<br/>- blur, line_anchor, line_spacing | 5 | P0 | VE-159 |
| VE-162 | As a C++ engine developer, I want TextAnimatorGroup evaluate → per-character transforms so that we apply animators to text | - evaluate(char_count, time) → vector of CharacterTransform<br/>- Combine multiple animators<br/>- Per-character position, scale, rotation, opacity, etc. | 5 | P0 | VE-159 |
| VE-163 | As a C++ engine developer, I want Wiggly selector (random per-character) so that we support random text animation | - Wiggly selector with max/min amount, speed<br/>- correlation for consistency<br/>- mode: Add, Intersect, etc. | 3 | P1 | VE-160 |
| VE-164 | As a C++ engine developer, I want Expression selector so that we can use expressions for range selection | - Expression selector evaluates to weight per character<br/>- ExpressionContext with character index, total<br/>- Integrates with ExpressionEngine | 5 | P1 | VE-127, VE-160 |
| VE-165 | As a C++ engine developer, I want Text warp (arc, wave, flag presets) so that we can warp text geometry | - Arc, wave, flag warp presets<br/>- Parameters: bend, horizontal/vertical distortion<br/>- Apply to character positions | 5 | P1 | VE-154 |
| VE-166 | As a C++ engine developer, I want Font download and caching so that we can load remote fonts and cache them | - Download font from URL<br/>- Cache to disk (platform path)<br/>- Register in FontManager after download | 3 | P1 | VE-156 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
