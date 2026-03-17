
## IE-11. Template System for Image Editor

### 11.1 Template Data Model

```cpp
namespace gp::templates {

// A template is a pre-designed canvas with replaceable elements.
// Users customize text, images, colors, and fonts without editing from scratch.

struct ImageTemplate {
    std::string template_id;          // UUID
    std::string name;
    std::string description;
    std::string category;             // "Social Media", "Marketing", "Invitation", etc.
    std::vector<std::string> tags;
    std::string author_id;

    // Canvas definition
    canvas::CanvasSettings canvas_settings;
    std::vector<canvas::Layer> layers;

    // Replaceable elements
    std::vector<TemplatePlaceholder> placeholders;

    // Style controls (exposed to end users)
    std::vector<StyleControl> style_controls;

    // Color palette
    TemplatePalette palette;

    // Multiple pages (for multi-page designs like carousels)
    bool is_multi_page;
    std::vector<TemplatePage> pages;

    // Metadata
    int version;
    std::string created_at;
    std::string updated_at;
    std::string preview_url;          // Thumbnail URL for browsing
    int usage_count;
    float rating;
    bool is_premium;
};

} // namespace gp::templates
```

### 11.2 Placeholder System

```cpp
namespace gp::templates {

enum class PlaceholderType {
    Text,           // Replaceable text field
    Image,          // Replaceable photo/image
    Logo,           // Logo placeholder (image + smart fit)
    Color,          // Color swatch that affects multiple layers
    Font,           // Font selector that affects text group
    Sticker,        // Replaceable sticker/icon
    QRCode,         // Auto-generated QR code
    Barcode,        // Auto-generated barcode
};

struct TemplatePlaceholder {
    std::string id;                   // "headline", "photo_1", "logo"
    std::string label;                // User-facing label: "Main Photo"
    PlaceholderType type;
    int32_t layer_id;                 // The layer this placeholder controls
    bool required;                    // Must be filled before export

    // Text placeholder specifics
    struct TextConfig {
        std::string default_text;
        std::string hint_text;
        int max_characters;
        int min_font_size;            // For auto-shrink
        int max_font_size;
        bool allow_multiline;
        bool allow_font_change;
        bool allow_color_change;
        bool allow_style_change;
        std::vector<std::string> suggested_fonts;
    } text_config;

    // Image placeholder specifics
    struct ImageConfig {
        int min_width, min_height;    // Minimum source image resolution
        float aspect_ratio;           // 0 = any
        FitMode default_fit;          // Fill, Fit, etc.
        bool allow_filter;            // Allow applying filters to placed image
        bool allow_adjustment;        // Allow brightness/contrast etc.
        FrameShape frame_shape;       // Shape of the placeholder frame
        float corner_radius;
        BezierPath* custom_shape;     // Custom frame shape
    } image_config;

    // Logo placeholder specifics
    struct LogoConfig {
        int max_width, max_height;
        bool preserve_aspect;
        LogoPlacement placement;      // Center, TopLeft, etc.
    } logo_config;

    // Color placeholder specifics
    struct ColorConfig {
        Color4f default_color;
        std::vector<int32_t> affected_layer_ids;  // Layers whose color changes
        std::vector<ColorMapping> mappings;         // How color maps to each layer
    } color_config;
};

enum class LogoPlacement { Center, TopLeft, TopRight, BottomLeft, BottomRight, Custom };

struct ColorMapping {
    int32_t layer_id;
    std::string property_path;        // "fill.color", "stroke.color", "text.color"
    ColorTransform transform;         // Direct, Lighten, Darken, Tint
    float intensity;
};

enum class ColorTransform { Direct, Lighten10, Lighten20, Darken10, Darken20, Tint, Complement };

} // namespace gp::templates
```

### 11.3 Style Controls

```cpp
namespace gp::templates {

// Style controls are high-level parameters exposed to template users
// that affect multiple layers/properties at once

struct StyleControl {
    std::string id;
    std::string label;                // "Theme Color", "Font Style", "Layout"
    StyleControlType type;

    struct ColorControl {
        Color4f current_value;
        std::vector<Color4f> suggested_colors;
        std::vector<ColorMapping> mappings;
    } color;

    struct FontControl {
        std::string current_family;
        std::vector<std::string> suggested_families;
        std::vector<int32_t> affected_text_layers;
    } font;

    struct LayoutControl {
        int current_variant;
        std::vector<LayoutVariant> variants;
    } layout;

    struct SliderControl {
        float current_value;
        float min_value, max_value;
        std::string unit;             // "px", "%", "pt"
        std::vector<PropertyMapping> mappings;
    } slider;

    struct ToggleControl {
        bool current_value;
        std::vector<int32_t> show_layer_ids;   // Layers shown when ON
        std::vector<int32_t> hide_layer_ids;   // Layers hidden when ON
    } toggle;
};

enum class StyleControlType { Color, Font, Layout, Slider, Toggle, Palette };

struct LayoutVariant {
    std::string name;                 // "Centered", "Left Aligned", "Split"
    std::string thumbnail_path;
    std::vector<LayerTransformOverride> overrides;
};

struct LayerTransformOverride {
    int32_t layer_id;
    Transform2D transform;
    Rect content_bounds;
    bool visible;
};

struct PropertyMapping {
    int32_t layer_id;
    std::string property_path;
    float source_min, source_max;     // Input range from slider
    float target_min, target_max;     // Output range for property
};

} // namespace gp::templates
```

### 11.4 Template Palette System

```cpp
namespace gp::templates {

struct TemplatePalette {
    std::string name;
    Color4f primary;
    Color4f secondary;
    Color4f accent;
    Color4f background;
    Color4f text_primary;
    Color4f text_secondary;

    // Palette presets
    static std::vector<TemplatePalette> built_in_palettes();

    // Auto-generate palette from image
    static TemplatePalette from_image(GpuTexture image, int num_colors = 6);

    // Apply palette to template
    void apply_to_template(ImageTemplate& tmpl) const;
};

// Built-in palette categories
// - Modern & Clean (50+ palettes)
// - Warm & Cozy (30+ palettes)
// - Cool & Professional (30+ palettes)
// - Bold & Vibrant (40+ palettes)
// - Pastel & Soft (30+ palettes)
// - Dark & Moody (20+ palettes)
// - Seasonal (spring, summer, autumn, winter)
// - Brand-specific (user's saved brand colors)

} // namespace gp::templates
```

### 11.5 Collage System

```cpp
namespace gp::templates {

struct CollageLayout {
    std::string id;
    std::string name;
    int cell_count;
    float aspect_ratio;                // Output aspect ratio

    struct Cell {
        int index;
        Rect bounds;                   // Normalized 0-1 within canvas
        float corner_radius;
        BezierPath* custom_shape;      // For non-rectangular cells
        float padding;                 // Gap between cells
    };
    std::vector<Cell> cells;

    // Layout generation
    static CollageLayout grid(int rows, int cols, float gap = 0.02f);
    static CollageLayout mosaic(int image_count, float aspect_ratio = 1.0f);
    static CollageLayout freeform(int image_count);
    static CollageLayout magazine(int image_count);  // Editorial-style layout
};

class CollageEngine {
public:
    // Create collage from layout + images
    canvas::Canvas create_collage(const CollageLayout& layout,
                                   const std::vector<int32_t>& media_ids,
                                   const canvas::CanvasSettings& settings);

    // Smart auto-layout: choose best layout for N images
    CollageLayout auto_layout(int image_count, float aspect_ratio,
                               CollageStyle style = CollageStyle::Grid);

    // Swap images between cells
    void swap_cells(int cell_a, int cell_b);

    // Adjust single cell zoom/pan
    void adjust_cell(int cell_index, Vec2 pan, float zoom);

    // Change gap between cells
    void set_gap(float gap);

    // Change frame shape
    void set_frame_shape(int cell_index, FrameShape shape);

private:
    canvas::Canvas* canvas_ = nullptr;
    CollageLayout layout_;
};

enum class CollageStyle { Grid, Mosaic, Magazine, Freeform, Polaroid, FilmStrip };

} // namespace gp::templates
```

### 11.6 Element Library

```cpp
namespace gp::templates {

// Pre-made design elements that users can drag onto canvas

enum class ElementCategory {
    Stickers, Icons, Illustrations, Shapes, Frames, Borders,
    Dividers, Badges, Labels, Ribbons, Arrows, Decorations,
    Textures, Patterns, Overlays, Backgrounds, Mockups,
};

struct ElementEntry {
    std::string id;
    std::string name;
    ElementCategory category;
    std::vector<std::string> tags;
    std::string thumbnail_url;
    std::string asset_url;

    enum Format { SVG, PNG, WebP };
    Format format;
    bool is_premium;
    int width, height;              // For raster elements
    std::string color_hint;         // Dominant color for filtering
};

class ElementLibrary {
public:
    // Search and browse
    std::vector<ElementEntry> search(const std::string& query,
                                      ElementCategory category = {},
                                      int limit = 50, int offset = 0);
    std::vector<ElementEntry> trending(ElementCategory category, int limit = 20);
    std::vector<ElementEntry> recent(int limit = 20);

    // Add element to canvas as a layer
    int32_t add_to_canvas(canvas::Canvas& canvas, const std::string& element_id,
                           Vec2 position, float scale = 1.0f);

    // Download and cache element asset
    TaskHandle prefetch(const std::string& element_id);

    // User's saved elements
    void save_element(const std::string& element_id);
    std::vector<ElementEntry> saved_elements();

private:
    std::unordered_map<std::string, ElementEntry> cache_;
    std::string cache_dir_;
};

} // namespace gp::templates
```

### 11.7 Template Encryption (.gpit Format)

Template encryption follows the same secure pipeline as the video editor templates:

```
.gpit structure (Encrypted Image Template):

┌──────────────────────────────────────┐
│ Header                                │
│   Magic: "GPIT" (4 bytes)            │
│   Version: uint32                     │
│   Flags: uint32                       │
│   Template ID: UUID (16 bytes)        │
│   Content Hash: SHA-256 (32 bytes)    │
└──────────────────────────────────────┘
│ Encrypted Payload (AES-256-GCM)       │
│   ├─ Canvas Settings                  │
│   ├─ Layer Tree (MessagePack)         │
│   ├─ Placeholder Definitions          │
│   ├─ Style Controls                   │
│   ├─ Palette                          │
│   ├─ Embedded Assets (images, fonts)  │
│   └─ Thumbnails                       │
└──────────────────────────────────────┘
│ Signature (Ed25519)                   │
└──────────────────────────────────────┘
```

### 11.8 Template Creator Studio

```cpp
namespace gp::templates {

// Full authoring environment for template designers.
// Lets creators define placeholders, set constraints, test interactions,
// and publish production-ready .gpit packages.

class TemplateCreatorStudio {
public:
    // ---------- Authoring lifecycle ----------

    // Start a new blank template from canvas presets or custom dimensions
    ImageTemplate create_new(const canvas::CanvasSettings& settings,
                             const std::string& name,
                             const std::string& category);

    // Import an existing .gpimg project and convert into a template
    ImageTemplate import_from_project(const std::string& gpimg_path);

    // Duplicate & derive a new template from an existing one
    ImageTemplate fork_template(const std::string& source_template_id);

    // ---------- Placeholder authoring ----------

    // Promote any existing layer to a placeholder
    TemplatePlaceholder promote_to_placeholder(ImageTemplate& tmpl,
                                                int32_t layer_id,
                                                PlaceholderType type,
                                                const std::string& label);

    // Remove placeholder status from a layer (keep the layer itself)
    void demote_placeholder(ImageTemplate& tmpl, const std::string& placeholder_id);

    // ---------- Constraint editor ----------

    struct PlaceholderConstraints {
        // Positional lock
        bool lock_position;
        bool lock_size;
        bool lock_rotation;
        bool lock_aspect_ratio;

        // Content constraints
        int min_characters, max_characters;         // Text
        int min_image_width, min_image_height;      // Image
        float min_scale, max_scale;                 // Any layer
        float allowed_rotation_min, allowed_rotation_max;

        // Visibility
        bool always_visible;                        // Cannot be hidden by user
        bool initially_hidden;                      // Hidden until user provides content

        // Dependency: this placeholder only appears after another is filled
        std::string depends_on_placeholder_id;
    };

    void set_placeholder_constraints(ImageTemplate& tmpl,
                                      const std::string& placeholder_id,
                                      const PlaceholderConstraints& constraints);

    // ---------- Interactive preview / testing ----------

    // Simulate end-user experience: only placeholders are editable
    void enter_preview_mode(ImageTemplate& tmpl);
    void exit_preview_mode(ImageTemplate& tmpl);

    // Validate template completeness before publishing
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;    // Must fix
        std::vector<std::string> warnings;  // Should fix
        std::vector<std::string> tips;      // Nice to have
    };
    ValidationResult validate(const ImageTemplate& tmpl);

    // ---------- Packaging & publishing ----------

    // Export encrypted .gpit with all embedded assets
    bool package(const ImageTemplate& tmpl, const std::string& output_path,
                 const EncryptionKey& key);

    // Generate multiple preview thumbnails (grid, per-page, per-variant)
    struct PreviewSet {
        GpuTexture cover_thumbnail;         // 1080×1080 hero image
        GpuTexture wide_thumbnail;          // 1920×1080 for browsing
        std::vector<GpuTexture> page_thumbnails;  // One per page
        std::vector<GpuTexture> variant_thumbnails; // One per layout variant
    };
    PreviewSet generate_previews(const ImageTemplate& tmpl, IGpuContext& gpu);
};

} // namespace gp::templates
```

### 11.9 Smart Layout & Auto-Layout Engine

```cpp
namespace gp::templates {

// Constraint-based auto-layout so template elements reflow
// automatically when content changes (e.g. longer headline text,
// different image aspect ratio, canvas resize for another platform).

enum class LayoutConstraintEdge { Top, Right, Bottom, Left, CenterX, CenterY };
enum class LayoutSizeMode { Fixed, HugContents, FillParent, AspectRatio };
enum class LayoutDirection { Horizontal, Vertical };
enum class LayoutDistribution { Start, Center, End, SpaceBetween, SpaceAround, SpaceEvenly };
enum class LayoutAlign { Start, Center, End, Stretch, Baseline };

struct LayoutConstraint {
    int32_t layer_id;

    // Anchoring (pin edges to parent or sibling)
    struct Anchor {
        bool enabled;
        LayoutConstraintEdge edge;
        int32_t relative_to_layer_id;   // -1 = parent canvas
        LayoutConstraintEdge relative_edge;
        float offset;                   // px offset from anchor
    };
    std::vector<Anchor> anchors;

    // Sizing
    LayoutSizeMode width_mode;
    LayoutSizeMode height_mode;
    float fixed_width, fixed_height;
    float min_width, max_width;
    float min_height, max_height;
    float aspect_ratio;                 // Enforced when mode = AspectRatio

    // Margin & padding
    float margin_top, margin_right, margin_bottom, margin_left;
    float padding_top, padding_right, padding_bottom, padding_left;
};

struct AutoLayoutGroup {
    int32_t group_layer_id;             // The group this applies to
    LayoutDirection direction;
    LayoutDistribution distribution;
    LayoutAlign align;
    float gap;                          // Spacing between children
    bool wrap;                          // Wrap to next row/column
    bool reverse;                       // Reverse child order
    bool clip_contents;
    float padding_top, padding_right, padding_bottom, padding_left;
};

class SmartLayoutEngine {
public:
    // Attach constraints to layers
    void set_constraint(ImageTemplate& tmpl, const LayoutConstraint& constraint);
    void remove_constraint(ImageTemplate& tmpl, int32_t layer_id);

    // Convert a layer group into an auto-layout frame (Figma-style)
    void enable_auto_layout(ImageTemplate& tmpl, const AutoLayoutGroup& config);
    void disable_auto_layout(ImageTemplate& tmpl, int32_t group_layer_id);

    // Recompute all positions and sizes after content change
    void solve(ImageTemplate& tmpl);

    // Content-aware reflow: called when placeholder content changes
    void reflow_after_text_change(ImageTemplate& tmpl, const std::string& placeholder_id);
    void reflow_after_image_change(ImageTemplate& tmpl, const std::string& placeholder_id);

    // Resize entire template to new dimensions while preserving layout
    void resize_template(ImageTemplate& tmpl, int new_width, int new_height,
                          ResizeStrategy strategy = ResizeStrategy::ScaleAndReflow);

    enum class ResizeStrategy {
        ScaleAll,           // Uniformly scale everything
        ScaleAndReflow,     // Scale + reflow text and constrained elements
        ReflowOnly,         // Keep element sizes, reposition via constraints
        ContentAware,       // AI-assisted repositioning
    };

    // Snap & alignment helpers
    struct SnapResult {
        Vec2 snapped_position;
        std::vector<Guide> active_guides;
    };
    SnapResult snap_to_guides(const ImageTemplate& tmpl, int32_t layer_id, Vec2 proposed_position);

    // Distribute & align selected layers
    void align_layers(ImageTemplate& tmpl, const std::vector<int32_t>& layer_ids,
                       LayoutConstraintEdge edge);
    void distribute_layers(ImageTemplate& tmpl, const std::vector<int32_t>& layer_ids,
                            LayoutDirection direction, LayoutDistribution dist);

    // Smart spacing: equalize gaps between selected layers
    void equalize_spacing(ImageTemplate& tmpl, const std::vector<int32_t>& layer_ids,
                           LayoutDirection direction);
};

} // namespace gp::templates
```

### 11.10 Brand Kit System

```cpp
namespace gp::templates {

// Stores a user's or organization's brand identity (logos, colors, fonts)
// and can stamp any template with brand-consistent styling.

struct BrandKit {
    std::string kit_id;                 // UUID
    std::string name;                   // "Acme Corp Brand"
    std::string owner_id;

    // Logos
    struct BrandLogo {
        std::string id;
        std::string label;              // "Primary", "Icon", "Monochrome"
        std::string asset_url;
        enum Variant { FullColor, Monochrome, Reversed, Icon } variant;
        Color4f background_hint;        // Suggested background for this variant
        int width, height;
    };
    std::vector<BrandLogo> logos;

    // Color palette
    struct BrandColorSet {
        Color4f primary;
        Color4f secondary;
        Color4f accent;
        Color4f background;
        Color4f surface;
        Color4f text_on_primary;
        Color4f text_on_background;
        Color4f success, warning, error, info;
        std::vector<Color4f> extended_palette;  // Additional swatches
    } colors;

    // Typography
    struct BrandTypography {
        std::string heading_family;
        std::string heading_weight;     // "Bold", "SemiBold"
        std::string body_family;
        std::string body_weight;
        std::string accent_family;      // For callouts, quotes
        float heading_scale;            // Relative size multiplier
        float body_scale;
        float line_height_ratio;        // e.g. 1.5
        float letter_spacing;
    } typography;

    // Voice & tone guidelines (stored for AI generation)
    struct BrandVoice {
        std::string tone;               // "Professional", "Friendly", "Bold"
        std::string tagline;
        std::vector<std::string> keywords;
        std::string industry;
    } voice;

    // Graphic elements
    std::vector<std::string> pattern_ids;   // Brand patterns
    std::vector<std::string> icon_set_ids;  // Brand icon sets
    std::string watermark_asset_url;

    std::string created_at;
    std::string updated_at;
};

class BrandKitManager {
public:
    // CRUD
    BrandKit create_kit(const std::string& name);
    BrandKit get_kit(const std::string& kit_id);
    void update_kit(const BrandKit& kit);
    void delete_kit(const std::string& kit_id);
    std::vector<BrandKit> list_kits(const std::string& owner_id);

    // Apply brand to template
    void apply_to_template(ImageTemplate& tmpl, const BrandKit& kit,
                            BrandApplyMode mode = BrandApplyMode::Full);

    enum class BrandApplyMode {
        Full,           // Replace colors, fonts, and logo placeholders
        ColorsOnly,     // Only swap color palette
        FontsOnly,      // Only swap typography
        LogoOnly,       // Only fill logo placeholders
        Selective,      // Use BrandApplyOptions
    };

    struct BrandApplyOptions {
        bool apply_colors;
        bool apply_fonts;
        bool apply_logos;
        bool apply_patterns;
        bool apply_watermark;
        bool preserve_layout;           // Don't reflow after applying
    };
    void apply_selective(ImageTemplate& tmpl, const BrandKit& kit,
                          const BrandApplyOptions& options);

    // Extract brand from existing design
    BrandKit extract_from_template(const ImageTemplate& tmpl);
    BrandKit extract_from_image(IGpuContext& gpu, GpuTexture image);
    BrandKit extract_from_website(const std::string& url);  // Scrape colors & fonts

    // Brand consistency check
    struct BrandAuditResult {
        bool consistent;
        std::vector<std::string> color_violations;   // Off-brand colors
        std::vector<std::string> font_violations;    // Non-brand fonts
        std::vector<std::string> suggestions;
    };
    BrandAuditResult audit_template(const ImageTemplate& tmpl, const BrandKit& kit);
};

} // namespace gp::templates
```

### 11.11 AI Template Generation

```cpp
namespace gp::templates {

// Use AI to generate, suggest, and enhance templates automatically.
// Wraps ML inference for layout prediction, content-aware design,
// and generative asset creation.

class AITemplateGenerator {
public:
    // ---------- Text-to-template generation ----------

    struct GenerationPrompt {
        std::string description;            // "Instagram post for summer sale, vibrant colors"
        std::string category;               // "Social Media", "Marketing", "Event"
        canvas::CanvasSettings canvas;      // Target dimensions
        std::string mood;                   // "Professional", "Playful", "Minimalist", "Bold"
        std::string industry;               // "Food", "Fashion", "Tech", "Real Estate"
        std::vector<std::string> keywords;

        // Optional content to include
        std::string headline_text;
        std::string body_text;
        std::string cta_text;               // Call to action
        int32_t image_media_id;             // User's photo (-1 = generate placeholder)
        std::string logo_url;

        // Generation controls
        int num_variations;                 // How many variants to produce (1-8)
        float creativity;                   // 0.0 = conservative, 1.0 = experimental
    };

    struct GeneratedTemplate {
        ImageTemplate tmpl;
        float confidence;                   // Design quality score 0-1
        std::string rationale;              // Why this layout was chosen
        std::vector<std::string> design_principles_applied;
    };

    std::vector<GeneratedTemplate> generate(IGpuContext& gpu,
                                             const GenerationPrompt& prompt);

    // ---------- AI layout suggestions ----------

    // Given existing content, suggest optimal arrangements
    struct LayoutSuggestion {
        std::vector<LayerTransformOverride> transforms;
        float aesthetic_score;
        std::string layout_type;            // "Z-pattern", "F-pattern", "Centered", "Asymmetric"
    };

    std::vector<LayoutSuggestion> suggest_layouts(
        const ImageTemplate& tmpl, int num_suggestions = 5);

    // ---------- AI color suggestions ----------

    struct ColorSuggestion {
        TemplatePalette palette;
        float harmony_score;
        std::string harmony_type;           // "Complementary", "Analogous", "Triadic", "Split"
    };

    std::vector<ColorSuggestion> suggest_colors(
        const ImageTemplate& tmpl, int num_suggestions = 5);

    // AI-match colors to a reference image
    TemplatePalette match_colors_to_image(IGpuContext& gpu, GpuTexture reference);

    // ---------- AI font pairing ----------

    struct FontPairingSuggestion {
        std::string heading_family;
        std::string body_family;
        float compatibility_score;
        std::string style_category;         // "Modern", "Classic", "Playful"
    };

    std::vector<FontPairingSuggestion> suggest_font_pairings(
        const ImageTemplate& tmpl, int num_suggestions = 5);

    // ---------- AI content suggestions ----------

    struct ContentSuggestion {
        std::string placeholder_id;
        std::string suggested_text;         // AI-generated headline/body
        float relevance_score;
    };

    std::vector<ContentSuggestion> suggest_content(
        const ImageTemplate& tmpl, const std::string& context_description);

    // AI copywriting for template placeholders
    std::vector<std::string> generate_headlines(const std::string& topic,
                                                 const std::string& tone,
                                                 int count = 5);
    std::vector<std::string> generate_captions(const std::string& topic,
                                                const std::string& platform,
                                                int count = 5);

    // ---------- AI background generation ----------

    GpuTexture generate_background(IGpuContext& gpu,
                                    const std::string& description,
                                    int width, int height,
                                    const TemplatePalette& palette);

    // ---------- AI design score & critique ----------

    struct DesignCritique {
        float overall_score;                // 0-100
        float composition_score;
        float color_harmony_score;
        float typography_score;
        float whitespace_score;
        float hierarchy_score;
        std::vector<std::string> strengths;
        std::vector<std::string> improvements;
    };

    DesignCritique critique(const ImageTemplate& tmpl);
};

} // namespace gp::templates
```

### 11.12 Smart Content Replacement Engine

```cpp
namespace gp::templates {

// Intelligently handles what happens when a user drops content into
// a placeholder: face-aware cropping, subject detection, auto color
// adaptation, and content-aware repositioning.

class SmartContentReplacer {
public:
    // ---------- Smart image placement ----------

    struct SmartPlaceResult {
        Rect crop_rect;                     // Optimal crop within source
        Vec2 focal_point;                   // Detected subject center
        float recommended_zoom;
        FitMode fit_mode;
        bool face_detected;
        std::vector<Rect> face_rects;       // All detected faces
    };

    // Analyze source image and compute best placement for a placeholder
    SmartPlaceResult analyze_placement(IGpuContext& gpu,
                                        GpuTexture source_image,
                                        const TemplatePlaceholder& placeholder);

    // Place image into placeholder with smart cropping
    void smart_place(IGpuContext& gpu,
                      ImageTemplate& tmpl,
                      const std::string& placeholder_id,
                      int32_t source_media_id);

    // ---------- Face-aware operations ----------

    enum class FacePriority { Largest, Centered, All, None };

    struct FaceAwareCropConfig {
        FacePriority priority;
        float face_margin;                  // Extra space around faces (0.1 = 10%)
        float minimum_face_coverage;        // Min % of frame faces should occupy
        bool preserve_headroom;             // Keep space above heads
    };

    Rect face_aware_crop(IGpuContext& gpu, GpuTexture image,
                          float target_aspect_ratio,
                          const FaceAwareCropConfig& config = {});

    // ---------- Subject-aware operations ----------

    struct SubjectDetection {
        Rect subject_rect;
        Vec2 subject_center;
        float saliency_score;
        GpuTexture saliency_map;
    };

    SubjectDetection detect_subject(IGpuContext& gpu, GpuTexture image);

    Rect subject_aware_crop(IGpuContext& gpu, GpuTexture image,
                             float target_aspect_ratio);

    // ---------- Color adaptation ----------

    // Automatically adjust template colors to complement the placed image
    struct ColorAdaptation {
        TemplatePalette suggested_palette;
        float background_luminance;         // 0 = dark image, 1 = bright
        Color4f dominant_color;
        bool suggest_dark_text;             // Based on background brightness
    };

    ColorAdaptation analyze_color_adaptation(IGpuContext& gpu,
                                              GpuTexture placed_image,
                                              const ImageTemplate& tmpl);

    void auto_adapt_colors(IGpuContext& gpu,
                            ImageTemplate& tmpl,
                            const std::string& image_placeholder_id);

    // ---------- Text contrast enforcement ----------

    // Ensure text remains readable over any placed image
    struct ContrastFix {
        int32_t text_layer_id;
        Color4f original_color;
        Color4f adjusted_color;
        bool added_shadow;
        bool added_background_overlay;
        float contrast_ratio_before;
        float contrast_ratio_after;
    };

    std::vector<ContrastFix> enforce_text_contrast(
        IGpuContext& gpu, ImageTemplate& tmpl,
        float min_contrast_ratio = 4.5f);   // WCAG AA standard

    // ---------- Auto text resize ----------

    // Shrink or reflow text to fit its frame after content change
    void auto_fit_text(ImageTemplate& tmpl, const std::string& text_placeholder_id);

    // Multi-line balance: distribute text evenly across lines
    void balance_text_lines(ImageTemplate& tmpl, const std::string& text_placeholder_id);
};

} // namespace gp::templates
```

### 11.13 Template Theming Engine

```cpp
namespace gp::templates {

// Apply holistic visual themes to any template. A theme is a coordinated
// set of colors, typography, and decorative style (shadows, borders,
// shape treatments) that can be swapped in one click.

struct TemplateTheme {
    std::string theme_id;
    std::string name;
    std::string category;               // "Minimal", "Bold", "Elegant", "Retro", "Neon"

    // Color scheme
    TemplatePalette palette;

    // Typography overrides
    struct ThemeTypography {
        std::string heading_family;
        std::string heading_weight;
        std::string body_family;
        std::string body_weight;
        float heading_letter_spacing;
        TextTransform heading_transform; // None, Uppercase, Lowercase, Capitalize
    } typography;

    // Shape & decoration style
    struct ThemeDecoration {
        float global_corner_radius;     // 0 = sharp, 20 = very rounded
        float border_width;
        Color4f border_color;
        float shadow_blur;
        float shadow_distance;
        Color4f shadow_color;
        float shadow_opacity;
        bool use_gradients;             // Gradient fills on shapes
        float grain_amount;             // Film grain overlay (0 = none)
        float noise_amount;
    } decoration;

    // Background treatment
    struct ThemeBackground {
        enum Type { Solid, Gradient, Pattern, Image, Blur };
        Type type;
        Color4f solid_color;
        std::vector<GradientStop> gradient_stops;
        float gradient_angle;
        int32_t pattern_id;
        float pattern_opacity;
        GpuTexture background_image;
        float blur_radius;
    } background;

    // Text effect preset applied to headings
    std::string heading_text_effect_preset_id;

    // Preview thumbnail
    std::string thumbnail_url;
};

enum class TextTransform { None, Uppercase, Lowercase, Capitalize };

class TemplateThemeEngine {
public:
    // Built-in themes (100+)
    std::vector<TemplateTheme> built_in_themes();
    std::vector<TemplateTheme> themes_by_category(const std::string& category);

    // Apply theme to template
    void apply_theme(ImageTemplate& tmpl, const TemplateTheme& theme);

    // Selective application
    struct ThemeApplyOptions {
        bool apply_colors;
        bool apply_typography;
        bool apply_decoration;
        bool apply_background;
        bool apply_text_effects;
    };
    void apply_theme_selective(ImageTemplate& tmpl, const TemplateTheme& theme,
                                const ThemeApplyOptions& options);

    // Generate light/dark variant of current template
    ImageTemplate generate_dark_variant(const ImageTemplate& tmpl);
    ImageTemplate generate_light_variant(const ImageTemplate& tmpl);

    // Seasonal theme variants
    enum class Season { Spring, Summer, Autumn, Winter };
    ImageTemplate generate_seasonal_variant(const ImageTemplate& tmpl, Season season);

    // Extract theme from an existing template
    TemplateTheme extract_theme(const ImageTemplate& tmpl);

    // Custom theme creation
    TemplateTheme create_theme(const std::string& name,
                                const TemplatePalette& palette,
                                const TemplateTheme::ThemeTypography& typo,
                                const TemplateTheme::ThemeDecoration& deco);

    // Theme preview: render a template with a theme without committing
    GpuTexture preview_theme(IGpuContext& gpu,
                              const ImageTemplate& tmpl,
                              const TemplateTheme& theme);

    // Built-in theme categories:
    // - Minimal & Clean (15 themes)
    // - Bold & Vibrant (15 themes)
    // - Elegant & Luxury (10 themes)
    // - Retro & Vintage (10 themes)
    // - Neon & Cyberpunk (8 themes)
    // - Organic & Natural (8 themes)
    // - Corporate & Professional (10 themes)
    // - Playful & Fun (10 themes)
    // - Dark & Moody (8 themes)
    // - Seasonal (16 themes: 4 per season)
};

} // namespace gp::templates
```

### 11.14 Data-Driven Templates (Batch Merge)

```cpp
namespace gp::templates {

// Bulk-generate personalized designs from a single template
// and an external data source (CSV, JSON, API).
// Similar to mail-merge but for visual designs.

struct DataField {
    std::string column_name;            // CSV header or JSON key
    std::string placeholder_id;         // Template placeholder to fill
    FieldType type;
};

enum class FieldType {
    Text,           // Direct text replacement
    ImageUrl,       // URL to download and place
    ImagePath,      // Local file path
    Color,          // Hex color string
    Number,         // Formatted number
    Date,           // Formatted date
    QRContent,      // QR code data
    BarcodeContent, // Barcode data
    Boolean,        // Show/hide toggle layers
};

struct DataSource {
    enum Format { CSV, JSON, GoogleSheets, APIEndpoint };
    Format format;
    std::string path_or_url;
    std::string sheet_name;             // For Google Sheets
    std::string api_auth_header;        // For authenticated APIs
    std::vector<DataField> field_mappings;
    int row_count;                      // Total rows available
};

struct BatchMergeConfig {
    ImageTemplate tmpl;
    DataSource data;
    xport::ExportConfig export_config;  // Output format settings

    // Output
    std::string output_directory;
    std::string filename_pattern;       // e.g. "design_{row}_{name}" with column substitution

    // Range (partial runs)
    int start_row;                      // 0-based
    int end_row;                        // -1 = all rows

    // Behavior
    bool skip_errors;                   // Skip rows with missing/invalid data
    bool auto_adapt_colors;             // Auto-adapt template colors per image
    int max_concurrent;                 // Parallel generation threads
};

struct BatchMergeResult {
    int total_rows;
    int successful;
    int failed;
    int skipped;
    float total_duration_ms;
    std::vector<std::string> output_paths;
    std::vector<std::string> error_messages;  // Per-row error detail
};

class DataDrivenTemplateEngine {
public:
    // Parse data source and preview field mapping
    DataSource parse_data_source(const std::string& path_or_url,
                                  DataSource::Format format);

    // Auto-detect field mappings from column names → placeholder IDs
    std::vector<DataField> auto_map_fields(const DataSource& data,
                                            const ImageTemplate& tmpl);

    // Preview a single row merged into the template
    GpuTexture preview_row(IGpuContext& gpu,
                            const ImageTemplate& tmpl,
                            const DataSource& data,
                            int row_index);

    // Execute full batch merge
    BatchMergeResult execute(IGpuContext& gpu, const BatchMergeConfig& config,
                              std::function<void(int current, int total, float percent)> progress);

    // Cancel a running batch
    void cancel();

    // Data validation before merge
    struct DataValidation {
        bool valid;
        std::vector<std::string> missing_required;  // Required placeholders with no mapping
        std::vector<std::string> type_mismatches;   // Column type != placeholder type
        std::vector<int> rows_with_errors;           // Row indices with bad data
    };
    DataValidation validate(const BatchMergeConfig& config);

private:
    std::atomic<bool> cancelled_{false};
};

} // namespace gp::templates
```

### 11.15 Animated Template System

```cpp
namespace gp::templates {

// Per-element entrance/exit/loop animations for templates.
// Enables animated social posts (GIF, MP4, APNG, Lottie).
// Builds on the shared animation/keyframe system from VE.

enum class AnimationTrigger { OnLoad, OnDelay, AfterPrevious, WithPrevious, OnClick };

struct ElementAnimation {
    int32_t layer_id;
    AnimationTrigger trigger;
    float delay_ms;                     // Delay after trigger
    float duration_ms;

    // Entrance animation
    struct Entrance {
        bool enabled;
        EntranceType type;
        EasingFunction easing;
        float distance;                 // For slide animations (px)
        float start_scale;              // For zoom (0.0 = from nothing)
        float start_opacity;
    } entrance;

    // Exit animation
    struct Exit {
        bool enabled;
        ExitType type;
        EasingFunction easing;
        float distance;
        float end_scale;
        float end_opacity;
    } exit;

    // Emphasis / loop animation
    struct Emphasis {
        bool enabled;
        EmphasisType type;
        float intensity;                // Strength of the effect
        int loop_count;                 // 0 = infinite
        float loop_duration_ms;
    } emphasis;
};

enum class EntranceType {
    FadeIn, SlideFromLeft, SlideFromRight, SlideFromTop, SlideFromBottom,
    ZoomIn, BounceIn, FlipIn, RotateIn, TypewriterIn,
    WipeLeft, WipeRight, WipeUp, WipeDown,
    ScaleUp, ExpandFromCenter, DropIn, RiseUp,
    BlurIn, GlitchIn, PixelateIn,
};

enum class ExitType {
    FadeOut, SlideToLeft, SlideToRight, SlideToTop, SlideToBottom,
    ZoomOut, BounceOut, FlipOut, RotateOut,
    WipeLeft, WipeRight, WipeUp, WipeDown,
    ScaleDown, CollapseToCenter, FlyAway, SinkDown,
    BlurOut, GlitchOut, PixelateOut,
};

enum class EmphasisType {
    Pulse, Bounce, Shake, Wobble, Float, Spin, Glow, Flash,
    Swing, Tada, Jello, Heartbeat, RubberBand, Breathe,
};

enum class EasingFunction {
    Linear, EaseIn, EaseOut, EaseInOut,
    EaseInQuad, EaseOutQuad, EaseInOutQuad,
    EaseInCubic, EaseOutCubic, EaseInOutCubic,
    EaseInBack, EaseOutBack, EaseInOutBack,
    EaseInBounce, EaseOutBounce, EaseInOutBounce,
    EaseInElastic, EaseOutElastic, EaseInOutElastic,
    Spring, Custom,
};

struct AnimatedTemplateConfig {
    float total_duration_ms;            // Overall animation length
    float fps;                          // 24, 30, or 60
    bool loop;                          // Loop entire animation
    int loop_count;                     // 0 = infinite

    // Per-element animations
    std::vector<ElementAnimation> animations;

    // Page transitions (for multi-page templates)
    struct PageTransition {
        int from_page, to_page;
        TransitionType type;
        float duration_ms;
        EasingFunction easing;
    };
    std::vector<PageTransition> page_transitions;
};

enum class TransitionType {
    Cut, CrossDissolve, SlideLeft, SlideRight, SlideUp, SlideDown,
    Push, Reveal, Cover, Fade, Zoom, Flip, Cube, Morph,
};

class AnimatedTemplateEngine {
public:
    // Set animation config for a template
    void set_config(ImageTemplate& tmpl, const AnimatedTemplateConfig& config);
    AnimatedTemplateConfig get_config(const ImageTemplate& tmpl);

    // Add/update/remove per-element animation
    void set_animation(ImageTemplate& tmpl, const ElementAnimation& anim);
    void remove_animation(ImageTemplate& tmpl, int32_t layer_id);

    // Preview animation at a specific time
    GpuTexture render_frame(IGpuContext& gpu, const ImageTemplate& tmpl,
                             float time_ms);

    // Timeline scrubbing
    struct TimelineState {
        float current_time_ms;
        float total_duration_ms;
        std::vector<std::pair<int32_t, float>> layer_opacities;  // Per layer at this time
        std::vector<std::pair<int32_t, Transform2D>> layer_transforms;
    };
    TimelineState get_state_at(const ImageTemplate& tmpl, float time_ms);

    // Export animated template
    struct AnimatedExportConfig {
        enum Format { GIF, MP4_H264, MP4_H265, WebM_VP9, APNG, LottieJSON };
        Format format;
        int width, height;
        float fps;
        int quality;                    // 1-100
        int loop_count;                 // 0 = infinite (for GIF/APNG)
        bool optimize_palette;          // For GIF: reduce colors smartly
    };

    xport::ExportResult export_animated(IGpuContext& gpu,
                                         const ImageTemplate& tmpl,
                                         const AnimatedExportConfig& config,
                                         const std::string& output_path,
                                         std::function<void(float)> progress);

    // Animation presets (one-click animation sets for common use cases)
    struct AnimationPreset {
        std::string id;
        std::string name;
        std::string category;           // "Social Post", "Story", "Ad", "Presentation"
        AnimatedTemplateConfig config;
    };

    std::vector<AnimationPreset> built_in_presets();
    void apply_preset(ImageTemplate& tmpl, const std::string& preset_id);
};

} // namespace gp::templates
```

### 11.16 Smart Resize Engine (Multi-Platform Adaptation)

```cpp
namespace gp::templates {

// Resize a single template design to work across multiple social media
// platforms and aspect ratios. Content-aware repositioning keeps the
// design looking professional at every size.

struct PlatformTarget {
    std::string platform_name;          // "Instagram Post", "Facebook Cover"
    int width, height;
    float aspect_ratio;
    Rect safe_zone;                     // Normalized safe area (avoids platform UI)
    xport::SocialPlatform platform_enum;
};

struct SmartResizeResult {
    PlatformTarget target;
    ImageTemplate resized_template;
    float fidelity_score;               // 0-1 how well the resize preserved the design
    std::vector<std::string> warnings;  // Elements that needed adjustment
};

class SmartResizeEngine {
public:
    // All supported platform targets
    static std::vector<PlatformTarget> all_targets();

    // Resize to a single target
    SmartResizeResult resize(IGpuContext& gpu,
                              const ImageTemplate& source,
                              const PlatformTarget& target,
                              ResizeQuality quality = ResizeQuality::Balanced);

    // Batch resize to multiple targets at once
    std::vector<SmartResizeResult> resize_batch(
        IGpuContext& gpu,
        const ImageTemplate& source,
        const std::vector<PlatformTarget>& targets,
        std::function<void(int current, int total)> progress);

    // Resize to arbitrary dimensions
    SmartResizeResult resize_custom(IGpuContext& gpu,
                                     const ImageTemplate& source,
                                     int target_width, int target_height);

    enum class ResizeQuality {
        Fast,           // Scale-only, no content rearrangement
        Balanced,       // Scale + reposition text/elements
        HighQuality,    // AI-assisted content-aware repositioning
    };

    // Layer importance for resize decisions
    struct LayerImportance {
        int32_t layer_id;
        float importance;               // 0 = expendable, 1 = must keep visible
        bool can_scale;
        bool can_move;
        bool can_hide;                  // Can be hidden if no room
        Vec2 anchor_point;              // Preferred position (normalized 0-1)
    };

    void set_layer_importance(ImageTemplate& tmpl,
                               const std::vector<LayerImportance>& importances);

    // Content-aware resize strategy
    struct ResizeStrategy {
        // Text handling
        bool auto_resize_text;          // Reduce font size if needed
        float min_text_scale;           // Never go below this (e.g. 0.6 = 60%)
        bool allow_text_reflow;         // Rewrap text for new width

        // Image handling
        bool allow_recrop;              // Re-crop images for new aspect
        bool preserve_faces;            // Keep faces visible when re-cropping

        // Element handling
        bool allow_reposition;          // Move elements to fit
        bool allow_hide;                // Hide low-importance elements
        bool scale_decorations;         // Scale borders, dividers, etc.

        // Background
        bool extend_background;         // AI-extend background for wider aspect
        bool crop_background;           // Crop for narrower aspect
    };

    void set_strategy(const ResizeStrategy& strategy);

private:
    ResizeStrategy strategy_;

    // Internal: compute element positions for target size
    struct ResizePlan {
        std::vector<LayerTransformOverride> transforms;
        std::vector<int32_t> hidden_layers;
        std::vector<std::pair<int32_t, float>> font_scale_adjustments;
        std::vector<std::pair<int32_t, Rect>> recrop_adjustments;
    };

    ResizePlan compute_plan(const ImageTemplate& source,
                             const PlatformTarget& target);
};

} // namespace gp::templates
```

### 11.17 Template Marketplace & Publishing Pipeline

```cpp
namespace gp::templates {

// End-to-end pipeline for template creators to publish, version,
// distribute, and monetize their templates through the GoPost store.

struct TemplatePublication {
    std::string publication_id;
    std::string template_id;
    std::string creator_id;

    // Metadata
    std::string title;
    std::string description;
    std::string category;
    std::vector<std::string> tags;
    std::vector<std::string> target_platforms;  // "Instagram", "Facebook", etc.
    std::string language;               // Primary language

    // Pricing
    enum Tier { Free, Premium, Exclusive };
    Tier tier;
    float price;                        // 0 for free
    std::string currency;

    // Versioning
    int version_major, version_minor, version_patch;
    std::string changelog;
    std::string min_app_version;        // Minimum GoPost version required

    // Review status
    enum Status { Draft, Submitted, InReview, Approved, Rejected, Published, Deprecated };
    Status status;
    std::string reviewer_notes;
    std::string rejection_reason;

    // Analytics (read-only, populated by server)
    int64_t total_uses;
    int64_t total_downloads;
    float average_rating;
    int rating_count;
    int64_t revenue_cents;

    // Assets
    std::string gpit_url;              // Encrypted template file
    std::string cover_image_url;
    std::vector<std::string> preview_image_urls;
    std::string preview_video_url;     // Animated preview

    std::string created_at;
    std::string updated_at;
    std::string published_at;
};

class TemplatePublisher {
public:
    // Submit template for review
    TemplatePublication submit(const ImageTemplate& tmpl,
                                const std::string& title,
                                const std::string& description,
                                const std::vector<std::string>& tags,
                                TemplatePublication::Tier tier,
                                float price = 0.0f);

    // Update an existing published template (creates new version)
    TemplatePublication update_version(const std::string& publication_id,
                                        const ImageTemplate& tmpl,
                                        const std::string& changelog);

    // Deprecate a template (stop new downloads, existing users keep it)
    void deprecate(const std::string& publication_id);

    // Auto-validation before submission
    struct SubmissionCheck {
        bool passes;
        std::vector<std::string> errors;      // Must fix
        std::vector<std::string> warnings;    // Should fix
        bool has_minimum_placeholders;         // At least 1 text + 1 image
        bool has_preview_images;
        bool has_correct_dimensions;
        bool fonts_embeddable;                 // All fonts allow embedding
        float estimated_file_size_mb;
    };
    SubmissionCheck pre_check(const ImageTemplate& tmpl);

    // Generate marketplace listing assets automatically
    struct ListingAssets {
        GpuTexture cover_image;               // 1080×1080
        std::vector<GpuTexture> variations;   // Multiple color/content variants
        GpuTexture mockup_image;              // Template in device mockup
        std::string preview_video_path;       // Animated preview (if animated)
    };
    ListingAssets generate_listing_assets(IGpuContext& gpu,
                                           const ImageTemplate& tmpl);
};

class TemplateStore {
public:
    // Browse & discover
    struct SearchQuery {
        std::string keyword;
        std::string category;
        std::vector<std::string> tags;
        TemplatePublication::Tier tier;  // Free, Premium, or both
        std::string platform;           // "Instagram", etc. (empty = all)
        SortOrder sort;
        int limit, offset;
    };

    enum class SortOrder { Trending, Newest, TopRated, MostUsed, PriceAsc, PriceDesc };

    struct SearchResult {
        std::vector<TemplatePublication> items;
        int total_count;
    };

    SearchResult search(const SearchQuery& query);
    std::vector<TemplatePublication> trending(int limit = 20);
    std::vector<TemplatePublication> recommended_for_user(const std::string& user_id, int limit = 20);
    std::vector<TemplatePublication> by_creator(const std::string& creator_id, int limit = 50);

    // Download & use
    ImageTemplate download(const std::string& publication_id);

    // Rating & feedback
    void rate(const std::string& publication_id, int stars, const std::string& review);

    // Collections / curated lists
    struct TemplateCollection {
        std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> publication_ids;
        std::string cover_image_url;
    };
    std::vector<TemplateCollection> featured_collections();
};

} // namespace gp::templates
```

### 11.18 Template Accessibility Engine

```cpp
namespace gp::templates {

// Ensures templates produce accessible designs: sufficient contrast,
// alt text, logical reading order, and colorblind-safe palettes.

class TemplateAccessibilityEngine {
public:
    // ---------- Full accessibility audit ----------

    struct AccessibilityAudit {
        float overall_score;                // 0-100
        AccessibilityLevel level;           // A, AA, AAA
        std::vector<AccessibilityIssue> issues;
        std::vector<std::string> passed_checks;
    };

    enum class AccessibilityLevel { None, A, AA, AAA };

    struct AccessibilityIssue {
        enum Severity { Error, Warning, Info };
        Severity severity;
        std::string rule_id;                // "contrast-min", "alt-text", etc.
        std::string description;
        int32_t layer_id;                   // Affected layer (-1 = global)
        std::string fix_suggestion;
    };

    AccessibilityAudit audit(const ImageTemplate& tmpl);

    // ---------- Contrast checking ----------

    struct ContrastResult {
        int32_t text_layer_id;
        int32_t background_layer_id;
        float contrast_ratio;               // e.g. 4.5, 7.0
        bool passes_aa;                     // >= 4.5 for normal, >= 3.0 for large
        bool passes_aaa;                    // >= 7.0 for normal, >= 4.5 for large
        bool is_large_text;                 // >= 18pt or >= 14pt bold
        Color4f text_color;
        Color4f effective_background;
    };

    std::vector<ContrastResult> check_contrast(IGpuContext& gpu, const ImageTemplate& tmpl);

    // Auto-fix contrast issues
    void auto_fix_contrast(IGpuContext& gpu, ImageTemplate& tmpl,
                            AccessibilityLevel target = AccessibilityLevel::AA);

    // ---------- Colorblind simulation ----------

    enum class ColorblindType {
        Protanopia,     // Red-blind
        Deuteranopia,   // Green-blind
        Tritanopia,     // Blue-blind
        Achromatopsia,  // Total color blindness
    };

    GpuTexture simulate_colorblind(IGpuContext& gpu,
                                    const ImageTemplate& tmpl,
                                    ColorblindType type);

    // Check if template is distinguishable for colorblind users
    bool is_colorblind_safe(IGpuContext& gpu, const ImageTemplate& tmpl);

    // Suggest colorblind-safe alternatives for the palette
    TemplatePalette suggest_colorblind_safe_palette(const TemplatePalette& original);

    // ---------- Alt text & reading order ----------

    struct AltTextEntry {
        int32_t layer_id;
        std::string alt_text;
        bool is_decorative;             // True = skip in screen reader
    };

    void set_alt_text(ImageTemplate& tmpl, const std::vector<AltTextEntry>& entries);

    // AI-generate alt text for image layers
    std::string generate_alt_text(IGpuContext& gpu, GpuTexture image);

    // Set logical reading order (for accessible PDF export)
    void set_reading_order(ImageTemplate& tmpl, const std::vector<int32_t>& layer_order);

    // Auto-detect reading order from spatial layout
    std::vector<int32_t> detect_reading_order(const ImageTemplate& tmpl);

    // ---------- Typography accessibility ----------

    struct TypographyCheck {
        int32_t layer_id;
        bool font_size_ok;              // >= 12pt for body, >= 16pt recommended
        bool line_height_ok;            // >= 1.5x font size
        bool letter_spacing_ok;         // Not too tight
        bool word_spacing_ok;
        float actual_font_size_pt;
        float actual_line_height_ratio;
    };

    std::vector<TypographyCheck> check_typography(const ImageTemplate& tmpl);
};

} // namespace gp::templates
```

### 11.19 Template Versioning & Collaboration

```cpp
namespace gp::templates {

// Version control and team collaboration for template projects.
// Allows multiple designers to work on templates with conflict resolution.

struct TemplateVersion {
    std::string version_id;
    std::string template_id;
    int version_number;                 // Auto-incremented
    std::string author_id;
    std::string message;                // Commit-style message
    std::string created_at;

    // Snapshot of the template at this version
    std::string snapshot_url;           // Stored .gpit
    std::string thumbnail_url;

    // Diff from previous version
    struct VersionDiff {
        std::vector<int32_t> added_layers;
        std::vector<int32_t> removed_layers;
        std::vector<int32_t> modified_layers;
        std::vector<std::string> added_placeholders;
        std::vector<std::string> removed_placeholders;
        bool palette_changed;
        bool layout_changed;
    } diff;
};

struct TemplateComment {
    std::string comment_id;
    std::string author_id;
    std::string author_name;
    std::string content;
    std::string created_at;

    // Pin to specific location on canvas
    bool is_pinned;
    Vec2 pin_position;                  // Canvas coordinates
    int32_t pin_layer_id;              // Attached to specific layer (-1 = canvas)

    // Thread
    std::string parent_comment_id;      // Empty = top-level
    bool resolved;
};

class TemplateVersionControl {
public:
    // Save a new version
    TemplateVersion save_version(const ImageTemplate& tmpl,
                                  const std::string& message);

    // List version history
    std::vector<TemplateVersion> version_history(const std::string& template_id);

    // Restore to a previous version
    ImageTemplate restore_version(const std::string& version_id);

    // Compare two versions
    TemplateVersion::VersionDiff compare(const std::string& version_a,
                                          const std::string& version_b);

    // Auto-save (periodic background save)
    void enable_auto_save(const std::string& template_id, int interval_seconds = 60);
    void disable_auto_save(const std::string& template_id);
};

class TemplateCollaboration {
public:
    // Sharing & permissions
    enum class Permission { View, Comment, Edit, Admin };

    void share(const std::string& template_id,
               const std::string& user_id,
               Permission permission);
    void revoke(const std::string& template_id, const std::string& user_id);

    struct Collaborator {
        std::string user_id;
        std::string display_name;
        std::string avatar_url;
        Permission permission;
        bool is_online;
        Vec2 cursor_position;           // For real-time cursor tracking
    };

    std::vector<Collaborator> list_collaborators(const std::string& template_id);

    // Comments & annotations
    TemplateComment add_comment(const std::string& template_id,
                                 const TemplateComment& comment);
    void resolve_comment(const std::string& comment_id);
    void delete_comment(const std::string& comment_id);
    std::vector<TemplateComment> list_comments(const std::string& template_id,
                                                bool include_resolved = false);

    // Shared template libraries (team folders)
    struct SharedLibrary {
        std::string library_id;
        std::string name;
        std::string team_id;
        std::vector<std::string> template_ids;
        Permission default_permission;
    };

    SharedLibrary create_library(const std::string& name, const std::string& team_id);
    void add_to_library(const std::string& library_id, const std::string& template_id);
    std::vector<SharedLibrary> list_libraries(const std::string& team_id);

    // Locking: prevent edit conflicts on the same layer
    bool lock_layer(const std::string& template_id, int32_t layer_id,
                     const std::string& user_id);
    void unlock_layer(const std::string& template_id, int32_t layer_id);

    struct LockInfo {
        int32_t layer_id;
        std::string locked_by_user_id;
        std::string locked_at;
    };
    std::vector<LockInfo> active_locks(const std::string& template_id);
};

} // namespace gp::templates
```

### 11.20 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 13 | Wk 23-24 | IE-121 to IE-127 | Template data model, encrypted .gpit loader, placeholders |
| Sprint 14 | Wk 25-26 | IE-128 to IE-134 | Style controls, palette, collage layout engine |
| Sprint 15 | Wk 27-28 | IE-135 to IE-140 | Element library, template encryption, multi-page |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-121 | As a template creator, I want a template data model so that I can define pre-designed canvases with replaceable elements | - ImageTemplate: template_id, name, category, layers, placeholders, style_controls<br/>- CanvasSettings, TemplatePalette<br/>- is_multi_page, pages for carousels | 5 | Sprint 13 | — |
| IE-122 | As a template creator, I want an encrypted .gpit loader so that I can load protected templates | - Load .gpit file format<br/>- Parse header (Magic, Version, Template ID, Content Hash)<br/>- Validate signature before decrypt | 5 | Sprint 13 | — |
| IE-123 | As a template creator, I want .gpit decryption (AES-256-GCM) so that template content is protected | - AES-256-GCM decryption of payload<br/>- Verify content hash<br/>- Extract canvas, layers, placeholders, assets | 5 | Sprint 13 | IE-122 |
| IE-124 | As a template creator, I want text placeholders (editable, constraints) so that users can customize text | - PlaceholderType::Text with TextConfig<br/>- default_text, hint_text, max_characters, min/max_font_size<br/>- allow_font_change, allow_color_change, suggested_fonts | 5 | Sprint 13 | IE-121 |
| IE-125 | As a template creator, I want image placeholders (smart replace, fit modes) so that users can swap photos | - PlaceholderType::Image with ImageConfig<br/>- min_width, min_height, aspect_ratio, FitMode<br/>- allow_filter, allow_adjustment, frame_shape, corner_radius | 5 | Sprint 13 | IE-121 |
| IE-126 | As a template creator, I want logo placeholders so that users can add their logo | - PlaceholderType::Logo with LogoConfig<br/>- max_width, max_height, preserve_aspect<br/>- LogoPlacement: Center, TopLeft, etc. | 3 | Sprint 13 | IE-125 |
| IE-127 | As a template creator, I want color placeholders (multi-layer mapping) so that one color change updates many layers | - PlaceholderType::Color with ColorConfig<br/>- affected_layer_ids, ColorMapping per layer<br/>- ColorTransform: Direct, Lighten, Darken, Tint | 5 | Sprint 13 | IE-121 |
| IE-128 | As a template creator, I want QR code placeholders so that users can add custom QR codes | - PlaceholderType::QRCode<br/>- Editable URL/content<br/>- Auto-generated QR code rendering | 3 | Sprint 14 | IE-121 |
| IE-129 | As a template creator, I want font placeholders so that users can change fonts across text groups | - PlaceholderType::Font<br/>- suggested_families, affected_text_layers<br/>- Font selector UI integration | 3 | Sprint 14 | IE-124 |
| IE-130 | As a template creator, I want style control: color so that users can change theme colors | - StyleControlType::Color with ColorControl<br/>- current_value, suggested_colors, mappings<br/>- Apply to multiple layers via ColorMapping | 3 | Sprint 14 | IE-127 |
| IE-131 | As a template creator, I want style control: font so that users can change font style | - StyleControlType::Font with FontControl<br/>- current_family, suggested_families<br/>- affected_text_layers list | 3 | Sprint 14 | IE-129 |
| IE-132 | As a template creator, I want style control: layout variants so that users can switch layouts | - StyleControlType::Layout with LayoutControl<br/>- LayoutVariant with LayerTransformOverride<br/>- Thumbnail per variant | 5 | Sprint 14 | IE-121 |
| IE-133 | As a template creator, I want style control: slider so that users can adjust numeric values | - StyleControlType::Slider with SliderControl<br/>- min_value, max_value, unit<br/>- PropertyMapping to layer properties | 5 | Sprint 14 | IE-121 |
| IE-134 | As a template creator, I want style control: toggle so that users can show/hide elements | - StyleControlType::Toggle with ToggleControl<br/>- show_layer_ids, hide_layer_ids<br/>- Boolean visibility switch | 3 | Sprint 14 | IE-121 |
| IE-135 | As a template creator, I want a template palette (built-in 200+) so that templates have professional color schemes | - TemplatePalette::built_in_palettes() with 200+ palettes<br/>- Categories: Modern, Warm, Cool, Bold, Pastel, Dark, Seasonal<br/>- primary, secondary, accent, background, text colors | 5 | Sprint 14 | IE-121 |
| IE-136 | As a template creator, I want palette auto-generate from image so that I can create palettes from photos | - TemplatePalette::from_image(image, num_colors)<br/>- Color extraction (K-means or similar)<br/>- apply_to_template() | 5 | Sprint 14 | IE-135 |
| IE-137 | As a template creator, I want a collage layout engine (grid/mosaic/magazine) so that users can create photo collages | - CollageLayout: grid(), mosaic(), magazine(), freeform()<br/>- Cell bounds, corner_radius, padding<br/>- CollageEngine::create_collage() | 5 | Sprint 14 | IE-121 |
| IE-138 | As a designer, I want an element library (search/browse) so that I can find stickers, icons, and illustrations | - ElementLibrary::search(), trending(), recent()<br/>- ElementCategory: Stickers, Icons, Illustrations, etc.<br/>- ElementEntry with id, name, thumbnail_url, asset_url | 5 | Sprint 15 | — |
| IE-139 | As a designer, I want to add elements from the library to the canvas so that I can build designs quickly | - add_to_canvas(canvas, element_id, position, scale)<br/>- Creates layer from element asset<br/>- prefetch() for download/cache | 5 | Sprint 15 | IE-138 |
| IE-140 | As a template creator, I want multi-page templates (carousel) so that I can create multi-slide designs | - ImageTemplate.is_multi_page, TemplatePage vector<br/>- Carousel navigation between pages<br/>- Export all pages or selected | 5 | Sprint 15 | IE-121 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 5: Template System |
| **Sprint(s)** | IE-Sprint 13-15 (Weeks 23-28) |
| **Team** | C/C++ Engine Developer (2), Flutter Developer, Tech Lead |
| **Predecessor** | [10-vector-graphics.md](10-vector-graphics.md) |
| **Successor** | [12-masking-selection.md](12-masking-selection.md) |
| **Story Points Total** | 108 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-149 | As a template creator, I want a template data model so that I can define pre-designed canvases with replaceable elements | - ImageTemplate: template_id, name, category, layers, placeholders, style_controls<br/>- CanvasSettings, TemplatePalette<br/>- is_multi_page, pages for carousels | 5 | P0 | — |
| IE-150 | As a template creator, I want a template loader (.gpit decryption) so that I can load protected templates | - Load .gpit file format, parse header<br/>- AES-256-GCM decryption of payload<br/>- Validate signature, extract canvas, layers, placeholders, assets | 5 | P0 | — |
| IE-151 | As a template creator, I want asset extraction so that embedded images and fonts are available | - Extract embedded assets from .gpit payload<br/>- Media pool population from template<br/>- Font embedding and loading | 3 | P0 | IE-150 |
| IE-152 | As a template creator, I want image placeholders with fit modes so that users can swap photos | - PlaceholderType::Image with ImageConfig<br/>- Fit modes: Fill, Fit, Contain, Cover, etc.<br/>- min_width, min_height, aspect_ratio | 5 | P0 | IE-149 |
| IE-153 | As a template creator, I want text placeholders with constraints and auto-shrink so that users can customize text | - PlaceholderType::Text with TextConfig<br/>- max_characters, min/max_font_size for auto-shrink<br/>- allow_font_change, allow_color_change | 5 | P0 | IE-149 |
| IE-154 | As a template creator, I want color placeholders so that one color change updates many layers | - PlaceholderType::Color with ColorConfig<br/>- affected_layer_ids, ColorMapping per layer<br/>- ColorTransform: Direct, Lighten, Darken, Tint | 5 | P0 | IE-149 |
| IE-155 | As a template creator, I want logo placeholders so that users can add their logo | - PlaceholderType::Logo with LogoConfig<br/>- max_width, max_height, preserve_aspect<br/>- LogoPlacement: Center, TopLeft, etc. | 3 | P0 | IE-152 |
| IE-156 | As a template creator, I want QR code placeholders so that users can add custom QR codes | - PlaceholderType::QRCode<br/>- Editable URL/content<br/>- Auto-generated QR code rendering | 3 | P0 | IE-149 |
| IE-157 | As a template creator, I want a color palette system so that templates have professional color schemes | - TemplatePalette::built_in_palettes() with 200+ palettes<br/>- Categories: Modern, Warm, Cool, Bold, Pastel, Dark, Seasonal<br/>- primary, secondary, accent, background, text colors | 5 | P0 | IE-149 |
| IE-158 | As a template creator, I want auto-palette from image so that I can create palettes from photos | - TemplatePalette::from_image(image, num_colors)<br/>- Color extraction (K-means or similar)<br/>- apply_to_template() | 5 | P0 | IE-157 |
| IE-159 | As a template creator, I want style controls: color variant so that users can change theme colors | - StyleControlType::Color with ColorControl<br/>- current_value, suggested_colors, mappings<br/>- Apply to multiple layers via ColorMapping | 3 | P0 | IE-154 |
| IE-160 | As a template creator, I want style controls: font variant so that users can change font style | - StyleControlType::Font with FontControl<br/>- current_family, suggested_families<br/>- affected_text_layers list | 3 | P0 | IE-153 |
| IE-161 | As a template creator, I want style controls: layout variant so that users can switch layouts | - StyleControlType::Layout with LayoutControl<br/>- LayoutVariant with LayerTransformOverride<br/>- Thumbnail per variant | 5 | P0 | IE-149 |
| IE-162 | As a template creator, I want style controls: slider/toggle so that users can adjust values and show/hide elements | - StyleControlType::Slider with PropertyMapping<br/>- StyleControlType::Toggle with show/hide layer IDs<br/>- Numeric and boolean controls | 5 | P0 | IE-149 |
| IE-163 | As a template creator, I want collage grid layout so that users can create photo collages | - CollageLayout::grid(rows, cols, gap)<br/>- Cell bounds, corner_radius, padding<br/>- CollageEngine::create_collage() | 5 | P0 | IE-149 |
| IE-164 | As a template creator, I want collage mosaic layout so that users can create varied collages | - CollageLayout::mosaic(image_count, aspect_ratio)<br/>- Non-uniform cell sizing<br/>- Smart auto-layout | 5 | P0 | IE-163 |
| IE-165 | As a template creator, I want collage magazine/freeform layout so that users can create editorial designs | - CollageLayout::magazine(), freeform()<br/>- Editorial-style asymmetric layouts<br/>- Custom cell positioning | 5 | P0 | IE-163 |
| IE-166 | As a designer, I want an element library (stickers, icons, illustrations) so that I can find design elements | - ElementLibrary::search(), trending(), recent()<br/>- ElementCategory: Stickers, Icons, Illustrations<br/>- ElementEntry with id, name, thumbnail_url, asset_url | 5 | P0 | — |
| IE-167 | As a designer, I want a frame/border library so that I can add decorative frames to designs | - Frame and border elements in ElementLibrary<br/>- FrameShape variants<br/>- Add to canvas as layer | 3 | P0 | IE-166 |
| IE-168 | As a template creator, I want multi-page templates so that I can create multi-slide designs | - ImageTemplate.is_multi_page, TemplatePage vector<br/>- Carousel navigation between pages<br/>- Export all pages or selected | 5 | P0 | IE-149 |
| IE-169 | As a template creator, I want social media canvas presets (19 platforms) so that users get correct dimensions | - Canvas presets for Instagram, Facebook, Twitter, LinkedIn, etc.<br/>- 19 platform-specific dimensions<br/>- Aspect ratio and resolution presets | 5 | P0 | — |
| IE-170 | As a template creator, I want template preview generation so that templates display thumbnails in browsing | - Generate preview thumbnails for templates<br/>- Preview URL in ImageTemplate<br/>- Cached preview generation | 5 | P0 | IE-149 |

### Extended Template Features — User Stories (Sections 11.8–11.19)

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 16 | Wk 29-30 | IE-171 to IE-178 | Template Creator Studio, Smart Layout Engine |
| Sprint 17 | Wk 31-32 | IE-179 to IE-186 | Brand Kit, AI Template Generation |
| Sprint 18 | Wk 33-34 | IE-187 to IE-194 | Smart Content Replacement, Theming Engine |
| Sprint 19 | Wk 35-36 | IE-195 to IE-202 | Data-Driven Templates, Animated Templates |
| Sprint 20 | Wk 37-38 | IE-203 to IE-210 | Smart Resize, Marketplace & Publishing |
| Sprint 21 | Wk 39-40 | IE-211 to IE-218 | Accessibility, Versioning & Collaboration |

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-171 | As a template creator, I want a Creator Studio with authoring lifecycle (create new / import / fork) so that I have a dedicated authoring environment | - create_new() from CanvasSettings<br/>- import_from_project() from .gpimg<br/>- fork_template() to derive from existing | 5 | P0 | IE-149 |
| IE-172 | As a template creator, I want to promote/demote layers as placeholders so that I can visually author editable regions | - promote_to_placeholder(layer_id, type, label)<br/>- demote_placeholder(placeholder_id)<br/>- PlaceholderType selection during promote | 5 | P0 | IE-171 |
| IE-173 | As a template creator, I want a constraint editor (lock position/size/rotation, content limits) so that end users can't break the design | - PlaceholderConstraints: lock_position, lock_size, lock_rotation<br/>- min/max characters, min image resolution, scale range<br/>- depends_on_placeholder_id dependency chains | 5 | P0 | IE-172 |
| IE-174 | As a template creator, I want preview mode so that I can simulate the end-user editing experience | - enter_preview_mode() / exit_preview_mode()<br/>- Only placeholders editable in preview<br/>- Non-placeholder layers locked | 3 | P0 | IE-172 |
| IE-175 | As a template creator, I want template validation before publishing so that I catch errors early | - ValidationResult: errors, warnings, tips<br/>- Checks: missing required placeholders, orphan layers, font embedding<br/>- Must pass before package() | 3 | P0 | IE-171 |
| IE-176 | As a template creator, I want .gpit packaging with preview generation so that I can distribute templates | - package() encrypts to .gpit<br/>- PreviewSet: cover, wide, per-page, per-variant thumbnails<br/>- Embedded asset bundling | 5 | P0 | IE-175 |
| IE-177 | As a template creator, I want a constraint-based layout engine (anchors, sizing modes, margins) so that elements reflow when content changes | - LayoutConstraint with Anchor edges, LayoutSizeMode<br/>- Fixed, HugContents, FillParent, AspectRatio modes<br/>- Margin and padding per element | 8 | P0 | IE-149 |
| IE-178 | As a template creator, I want auto-layout groups (Figma-style) so that group children auto-arrange | - AutoLayoutGroup: direction, distribution, align, gap, wrap<br/>- enable_auto_layout() / disable_auto_layout()<br/>- solve() recomputes all positions | 8 | P0 | IE-177 |
| IE-179 | As a template creator, I want content-aware reflow so that layout adapts when text or images change | - reflow_after_text_change() / reflow_after_image_change()<br/>- resize_template() with ScaleAll, ScaleAndReflow, ReflowOnly, ContentAware<br/>- Smart spacing and alignment | 5 | P0 | IE-178 |
| IE-180 | As a template creator, I want layer alignment & distribution tools so that I can position elements precisely | - align_layers(layer_ids, edge)<br/>- distribute_layers(layer_ids, direction, distribution)<br/>- equalize_spacing() for uniform gaps<br/>- snap_to_guides() with SnapResult | 5 | P0 | IE-177 |
| IE-181 | As a brand owner, I want a Brand Kit data model (logos, colors, fonts, voice) so that I can store my brand identity | - BrandKit: logos (multi-variant), BrandColorSet, BrandTypography, BrandVoice<br/>- Pattern IDs, icon set IDs, watermark<br/>- CRUD operations: create, get, update, delete, list | 5 | P0 | — |
| IE-182 | As a brand owner, I want to apply my brand kit to any template so that all designs match my identity | - apply_to_template() with BrandApplyMode: Full, ColorsOnly, FontsOnly, LogoOnly<br/>- apply_selective() with BrandApplyOptions<br/>- Reflow after brand application | 5 | P0 | IE-181 |
| IE-183 | As a brand owner, I want brand extraction from existing designs/images/websites so that I can auto-populate brand kits | - extract_from_template(): extract colors, fonts, logo<br/>- extract_from_image(): dominant color extraction<br/>- extract_from_website(): scrape brand colors and fonts | 5 | P0 | IE-181 |
| IE-184 | As a brand owner, I want brand consistency auditing so that I can catch off-brand designs | - BrandAuditResult: color_violations, font_violations, suggestions<br/>- Detect off-palette colors and non-brand fonts<br/>- Actionable fix suggestions | 3 | P0 | IE-182 |
| IE-185 | As a user, I want AI text-to-template generation so that I can create designs from a description | - GenerationPrompt: description, category, mood, industry, keywords<br/>- headline_text, body_text, cta_text, image, logo<br/>- generate() returns multiple GeneratedTemplate with confidence scores | 8 | P1 | IE-149 |
| IE-186 | As a user, I want AI layout/color/font suggestions so that I can improve my template designs | - suggest_layouts() with aesthetic_score and layout_type<br/>- suggest_colors() with harmony_score and harmony_type<br/>- suggest_font_pairings() with compatibility_score | 5 | P1 | IE-185 |
| IE-187 | As a user, I want AI content suggestions and copywriting so that I can fill templates with relevant text | - suggest_content() for placeholder text<br/>- generate_headlines(topic, tone, count)<br/>- generate_captions(topic, platform, count) | 5 | P1 | IE-185 |
| IE-188 | As a user, I want AI design scoring and critique so that I get feedback on my designs | - DesignCritique: overall, composition, color, typography, whitespace, hierarchy scores<br/>- Strengths and improvements lists<br/>- Actionable improvement suggestions | 5 | P1 | IE-185 |
| IE-189 | As a user, I want smart image placement with face-aware and subject-aware cropping so that placed images look optimal | - analyze_placement() → SmartPlaceResult with focal point, face rects<br/>- smart_place() with automatic crop and zoom<br/>- face_aware_crop() and subject_aware_crop() | 5 | P0 | IE-152 |
| IE-190 | As a user, I want auto color adaptation when placing images so that the template palette adjusts to my photo | - analyze_color_adaptation() → suggested palette, dominant color, luminance<br/>- auto_adapt_colors() applies palette change<br/>- suggest_dark_text based on background brightness | 5 | P0 | IE-189 |
| IE-191 | As a user, I want automatic text contrast enforcement so that text stays readable over any image | - enforce_text_contrast(min_ratio=4.5) → ContrastFix per text layer<br/>- Auto-adjust color, add shadow, or add overlay<br/>- WCAG AA compliance | 5 | P0 | IE-190 |
| IE-192 | As a user, I want auto text fitting and line balancing so that text adapts to content length | - auto_fit_text() for placeholder shrink-to-fit<br/>- balance_text_lines() for even line distribution<br/>- Respects min_font_size constraint | 3 | P0 | IE-153 |
| IE-193 | As a user, I want 100+ built-in themes so that I can change the entire look of a template in one click | - TemplateTheme: palette, typography, decoration, background<br/>- 100+ themes across 10 categories<br/>- apply_theme() and apply_theme_selective() | 5 | P0 | IE-149 |
| IE-194 | As a user, I want light/dark/seasonal theme generation so that I can auto-generate variants of my design | - generate_dark_variant() / generate_light_variant()<br/>- generate_seasonal_variant(Season)<br/>- extract_theme() from existing template | 5 | P0 | IE-193 |
| IE-195 | As a user, I want theme preview without committing so that I can browse themes quickly | - preview_theme() renders template with theme<br/>- create_theme() for custom themes<br/>- Theme categories browsing | 3 | P0 | IE-193 |
| IE-196 | As a power user, I want CSV/JSON data source parsing with field mapping so that I can prepare batch jobs | - parse_data_source() for CSV, JSON, Google Sheets, API<br/>- auto_map_fields() auto-detects column→placeholder mapping<br/>- FieldType: Text, ImageUrl, ImagePath, Color, Number, Date, QR, Boolean | 5 | P0 | IE-149 |
| IE-197 | As a power user, I want batch merge execution (generate hundreds of designs from data) so that I can personalize at scale | - BatchMergeConfig: template, data, export config, output dir, filename pattern<br/>- execute() with progress callback<br/>- BatchMergeResult: successful, failed, skipped, output paths | 8 | P0 | IE-196 |
| IE-198 | As a power user, I want batch merge preview and validation so that I can verify before running | - preview_row() for single row preview<br/>- DataValidation: missing_required, type_mismatches, rows_with_errors<br/>- cancel() for running batches | 3 | P0 | IE-197 |
| IE-199 | As a user, I want per-element entrance/exit animations so that I can create animated social posts | - ElementAnimation: entrance (21 types), exit (21 types), emphasis (14 types)<br/>- AnimationTrigger: OnLoad, OnDelay, AfterPrevious, WithPrevious<br/>- EasingFunction: 20+ easing curves | 8 | P1 | IE-149 |
| IE-200 | As a user, I want page transitions for multi-page animated templates so that carousel slides transition smoothly | - PageTransition: TransitionType (14 types), duration, easing<br/>- AnimatedTemplateConfig: total_duration, fps, loop, page_transitions<br/>- render_frame() for timeline scrubbing | 5 | P1 | IE-199, IE-168 |
| IE-201 | As a user, I want animated template export (GIF/MP4/WebM/APNG/Lottie) so that I can share animated designs | - AnimatedExportConfig: GIF, MP4_H264, MP4_H265, WebM_VP9, APNG, LottieJSON<br/>- export_animated() with progress callback<br/>- optimize_palette for GIF, quality slider | 5 | P1 | IE-200 |
| IE-202 | As a user, I want animation presets (one-click) so that I can animate templates without manual work | - AnimationPreset: id, name, category, config<br/>- built_in_presets() library<br/>- apply_preset() to apply full animation set | 3 | P1 | IE-199 |
| IE-203 | As a user, I want smart resize to any platform (Instagram/Facebook/TikTok/etc.) so that one design works everywhere | - SmartResizeEngine::resize(source, PlatformTarget)<br/>- resize_batch() for multiple targets at once<br/>- SmartResizeResult with fidelity_score and warnings | 8 | P0 | IE-177 |
| IE-204 | As a user, I want content-aware resize strategy (face-preserving re-crop, text reflow, element hiding) so that resized designs look professional | - ResizeStrategy: auto_resize_text, allow_recrop, preserve_faces, allow_hide<br/>- LayerImportance: importance score, can_scale, can_move, can_hide<br/>- AI background extension for wider aspects | 5 | P0 | IE-203 |
| IE-205 | As a template creator, I want to submit templates to the marketplace with auto-validation so that I can publish professionally | - TemplatePublisher::submit() with title, description, tags, tier, price<br/>- SubmissionCheck: pre_check() validates before submission<br/>- TemplatePublication with status lifecycle (Draft → Published) | 5 | P0 | IE-176 |
| IE-206 | As a template creator, I want auto-generated marketplace listing assets so that my templates have professional previews | - generate_listing_assets(): cover, variations, mockup, video<br/>- update_version() for new versions<br/>- deprecate() to retire templates | 5 | P0 | IE-205 |
| IE-207 | As a user, I want template store search and discovery (trending, recommended, by creator) so that I can find templates | - TemplateStore::search(SearchQuery) with keyword, category, tags, tier, sort<br/>- trending(), recommended_for_user(), by_creator()<br/>- SortOrder: Trending, Newest, TopRated, MostUsed | 5 | P0 | IE-205 |
| IE-208 | As a user, I want template collections and ratings so that I can browse curated lists and rate templates | - rate() with stars and review<br/>- featured_collections() for curated lists<br/>- TemplateCollection with publication_ids | 3 | P0 | IE-207 |
| IE-209 | As a template creator, I want accessibility auditing (contrast, colorblind, alt text, typography) so that my designs are inclusive | - AccessibilityAudit: overall_score, level (A/AA/AAA), issues<br/>- check_contrast() per text layer with WCAG ratios<br/>- check_typography() for font size, line height, letter spacing | 5 | P1 | IE-149 |
| IE-210 | As a template creator, I want auto-fix contrast and colorblind simulation so that I can fix accessibility issues | - auto_fix_contrast() adjusts colors to meet target level<br/>- simulate_colorblind() for Protanopia, Deuteranopia, Tritanopia, Achromatopsia<br/>- suggest_colorblind_safe_palette() | 5 | P1 | IE-209 |
| IE-211 | As a template creator, I want alt text management and reading order so that exported PDFs are screen-reader friendly | - set_alt_text() per layer with decorative flag<br/>- generate_alt_text() via AI for image layers<br/>- set_reading_order() and detect_reading_order() | 5 | P1 | IE-209 |
| IE-212 | As a template creator, I want template versioning (save/restore/compare) so that I can track design evolution | - save_version() with message<br/>- version_history() and restore_version()<br/>- compare() two versions with VersionDiff | 5 | P1 | IE-149 |
| IE-213 | As a template creator, I want auto-save so that work is never lost | - enable_auto_save(template_id, interval_seconds)<br/>- Background periodic save<br/>- Recoverable from last auto-save | 3 | P0 | IE-212 |
| IE-214 | As a team member, I want template sharing with permissions (View/Comment/Edit/Admin) so that teams can collaborate | - share(template_id, user_id, permission)<br/>- revoke(), list_collaborators()<br/>- Permission levels: View, Comment, Edit, Admin | 5 | P1 | IE-212 |
| IE-215 | As a team member, I want pinned comments and annotations on templates so that I can give design feedback | - add_comment() with pin_position and pin_layer_id<br/>- Threaded replies (parent_comment_id)<br/>- resolve_comment() / delete_comment() | 3 | P1 | IE-214 |
| IE-216 | As a team lead, I want shared template libraries (team folders) so that teams share approved templates | - SharedLibrary: library_id, name, team_id, template_ids<br/>- create_library(), add_to_library(), list_libraries()<br/>- Default permission per library | 5 | P1 | IE-214 |
| IE-217 | As a team member, I want layer locking for conflict prevention so that two people don't edit the same element | - lock_layer() / unlock_layer()<br/>- active_locks() shows who holds which layer<br/>- Lock expiry for abandoned sessions | 3 | P1 | IE-214 |
| IE-218 | As a user, I want AI background generation so that I can create unique backgrounds from text prompts | - generate_background(description, width, height, palette)<br/>- Integrates with generative AI models<br/>- Palette-aware generation | 5 | P1 | IE-185 |

### Updated Story Points Summary

| Phase | Sprints | Weeks | Story Points |
|---|---|---|---|
| Core Template System (11.1–11.7) | Sprint 13-15 | Wk 23-28 | 108 |
| Creator Studio & Smart Layout (11.8–11.9) | Sprint 16 | Wk 29-30 | 47 |
| Brand Kit & AI Generation (11.10–11.11) | Sprint 17 | Wk 31-32 | 41 |
| Smart Content & Theming (11.12–11.13) | Sprint 18 | Wk 33-34 | 36 |
| Data-Driven & Animated Templates (11.14–11.15) | Sprint 19 | Wk 35-36 | 32 |
| Smart Resize & Marketplace (11.16–11.17) | Sprint 20 | Wk 37-38 | 36 |
| Accessibility & Collaboration (11.18–11.19) | Sprint 21 | Wk 39-40 | 39 |
| **Total Extended Template System** | **Sprint 13-21** | **Wk 23-40** | **339** |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
