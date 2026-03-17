
## IE-12. Masking and Selection System

### 12.1 Selection Tools

```cpp
namespace gp::selection {

enum class SelectionTool {
    RectangularMarquee,    // Rectangle selection
    EllipticalMarquee,     // Ellipse/circle selection
    SingleRow,             // 1px tall full-width selection
    SingleColumn,          // 1px wide full-height selection
    Lasso,                 // Freehand selection
    PolygonalLasso,        // Straight-edge polygon selection
    MagneticLasso,         // Edge-snapping lasso (edge detection)
    MagicWand,             // Color-based flood fill selection
    QuickSelection,        // Brush-based smart selection (edge-aware)
    ObjectSelection,       // AI-powered object selection
    SubjectSelection,      // AI auto-select main subject
    ColorRange,            // Select by color range (HSL-based)
    FocusArea,             // Select in-focus areas (depth-based)
};

struct SelectionState {
    int32_t id;
    int width, height;               // Same as canvas dimensions
    std::vector<float> mask;         // Floating-point mask (0=unselected, 1=selected)
    bool has_selection;
    Rect bounding_box;               // Bounding box of selected region

    // Operations
    void set_all();                  // Select all
    void clear();                    // Deselect all
    void invert();                   // Invert selection
    void feather(float radius);      // Soften edges
    void expand(float pixels);       // Grow selection
    void contract(float pixels);     // Shrink selection
    void smooth(float radius);       // Remove jagged edges
    void border(float width);        // Select only the border region

    // Combine with another selection
    void combine(const SelectionState& other, SelectionMode mode);

    // Convert
    GpuTexture to_gpu_mask(IGpuContext& gpu) const;
    BezierPath to_path(float tolerance = 1.0f) const;  // Marching squares → bezier
    void from_path(const BezierPath& path);
    void from_layer_alpha(const canvas::Layer& layer);
};

enum class SelectionMode {
    New,          // Replace existing selection
    Add,          // Add to existing (Union)
    Subtract,     // Remove from existing
    Intersect,    // Keep only overlap
};

} // namespace gp::selection
```

### 12.2 Selection Tool Implementations

```cpp
namespace gp::selection {

// Magic Wand: flood-fill based on color similarity
class MagicWandTool {
public:
    struct Params {
        int tolerance;             // 0-255 color difference
        bool contiguous;           // Only connected pixels, or all similar
        bool anti_alias;
        SampleMode sample;         // CurrentLayer, AllLayers
    };

    SelectionState select(IGpuContext& gpu, GpuTexture image,
                           Vec2 click_point, const Params& params);
};

// Quick Selection: brush-based smart selection (finds edges)
class QuickSelectionTool {
public:
    struct Params {
        float brush_size;          // Brush radius
        bool auto_enhance;         // Auto refine edges
        SampleMode sample;
    };

    void begin_stroke(Vec2 start, bool add_mode);
    void continue_stroke(Vec2 point);
    void end_stroke();

    SelectionState get_selection() const;

private:
    // Edge-aware region growing using color gradients
    void grow_selection(IGpuContext& gpu, GpuTexture image,
                         Vec2 center, float radius);
};

// Magnetic Lasso: edge-snapping path selection
class MagneticLassoTool {
public:
    struct Params {
        float width;               // Detection width
        float contrast;            // Edge contrast threshold
        int frequency;             // Anchor point frequency
    };

    void begin(Vec2 start);
    void continue_path(Vec2 cursor);
    void add_anchor();             // Manual anchor point
    void delete_last_anchor();
    void close();

    SelectionState get_selection() const;

private:
    // Canny edge detection + dynamic programming to snap to edges
    std::vector<Vec2> detect_edge_path(GpuTexture image,
                                        Vec2 from, Vec2 to, float width);
};

// Color Range Selection (Photoshop-style)
class ColorRangeSelector {
public:
    struct Params {
        Color4f target_color;
        float fuzziness;           // 0-200
        SelectionPreview preview;  // None, Grayscale, BlackMatte, WhiteMatte
        bool localized_color_clusters;
        float range;               // For localized mode
    };

    // Sample color from image
    void sample(GpuTexture image, Vec2 point, SampleAction action);

    // Generate selection
    SelectionState generate(IGpuContext& gpu, GpuTexture image,
                             const Params& params);

    enum class SampleAction { Set, Add, Subtract };
    enum class SelectionPreview { None, Grayscale, BlackMatte, WhiteMatte, QuickMask };
};

} // namespace gp::selection
```

### 12.3 Refine Edge / Select and Mask

```cpp
namespace gp::selection {

// Photoshop "Select and Mask" equivalent
class RefineEdge {
public:
    struct Params {
        // Edge Detection
        float radius;              // Smart Radius size
        bool smart_radius;         // Auto-vary radius based on edge

        // Global Refinements
        float smooth;              // 0-100
        float feather;             // 0-250 px
        float contrast;            // 0-100%
        float shift_edge;          // -100% to +100%

        // Decontaminate Colors
        bool decontaminate;
        float decontaminate_amount; // 0-100%

        // Output
        RefineOutput output;
    };

    enum class RefineOutput {
        Selection,                  // Updated selection
        LayerMask,                  // Apply as layer mask
        NewLayer,                   // Copy to new layer
        NewLayerWithMask,           // Copy with mask
        NewDocument,                // Copy to new document
    };

    // Refine the selection interactively
    void set_base_selection(const SelectionState& selection);
    SelectionState refine(IGpuContext& gpu, GpuTexture image, const Params& params);

    // Refinement brush tools
    void refine_brush(Vec2 center, float radius, bool add);      // Add/remove from refinement zone
    void hair_brush(Vec2 center, float radius);                   // Hair/fur refinement mode
    void smooth_brush(Vec2 center, float radius);

    // Preview modes
    GpuTexture preview(IGpuContext& gpu, GpuTexture image,
                        PreviewMode mode) const;

    enum class PreviewMode {
        MarchingAnts, Overlay, OnBlack, OnWhite, BlackAndWhite, OnLayers
    };
};

} // namespace gp::selection
```

### 12.4 Layer Mask System

```cpp
namespace gp::selection {

// Layer masks (shared with composition module)
// See IE-5.5 for LayerMask and VectorMask structs

class LayerMaskTools {
public:
    // Create mask from selection
    static canvas::LayerMask from_selection(const SelectionState& selection);

    // Create mask from layer alpha
    static canvas::LayerMask from_alpha(const canvas::Layer& layer);

    // Paint on mask (brush tool in mask mode)
    void paint_mask(canvas::LayerMask& mask, Vec2 center,
                     float radius, float hardness, float opacity,
                     bool erase, float pressure = 1.0f);

    // Apply gradient to mask
    void gradient_mask(canvas::LayerMask& mask,
                        Vec2 start, Vec2 end,
                        GradientType type);

    // Fill mask region
    void fill_mask(canvas::LayerMask& mask,
                    const SelectionState* selection,  // nullptr = entire mask
                    float value);                      // 0=black, 1=white

    // Invert mask
    void invert_mask(canvas::LayerMask& mask);

    // Apply levels/curves to mask (refine mask contrast)
    void levels_mask(canvas::LayerMask& mask,
                      float input_black, float input_white,
                      float gamma, float output_black, float output_white);
};

} // namespace gp::selection
```

### 12.5 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 16 | Wk 29-30 | IE-141 to IE-150 | Selection tools, marquee, lasso, magic wand, quick select, selection ops |
| Sprint 18 | Wk 33-34 | IE-151 to IE-155 | Refine edge, layer mask from selection, mask painting |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-141 | As a user, I want a rectangular marquee so that I can select rectangular regions | - RectangularMarquee tool<br/>- SelectionState with mask, bounding_box<br/>- set_all(), clear(), invert() | 3 | Sprint 16 | — |
| IE-142 | As a user, I want an elliptical marquee so that I can select circular/oval regions | - EllipticalMarquee tool<br/>- Ellipse mask generation<br/>- Same SelectionState interface | 3 | Sprint 16 | IE-141 |
| IE-143 | As a user, I want a lasso tool so that I can make freehand selections | - Lasso tool with freehand path<br/>- Path to mask conversion<br/>- add_point on mouse move | 3 | Sprint 16 | IE-141 |
| IE-144 | As a user, I want a polygonal lasso so that I can make straight-edge selections | - PolygonalLasso with click-to-add-point<br/>- Double-click or click start to close<br/>- Straight segments between points | 3 | Sprint 16 | IE-141 |
| IE-145 | As a user, I want a magnetic lasso (edge-snapping) so that I can snap selections to edges | - MagneticLassoTool with width, contrast, frequency params<br/>- detect_edge_path() using Canny + dynamic programming<br/>- Snaps to high-contrast edges | 5 | Sprint 16 | IE-144 |
| IE-146 | As a user, I want a magic wand (flood fill) so that I can select by color similarity | - MagicWandTool with tolerance, contiguous, anti_alias<br/>- select(image, click_point, params)<br/>- Flood fill based on color difference | 5 | Sprint 16 | IE-141 |
| IE-147 | As a user, I want quick selection (edge-aware brush) so that I can paint selections that find edges | - QuickSelectionTool with brush_size, auto_enhance<br/>- begin_stroke(), continue_stroke(), end_stroke()<br/>- grow_selection() with edge-aware region growing | 5 | Sprint 16 | IE-141 |
| IE-148 | As a user, I want selection operations (feather/expand/contract/smooth) so that I can refine selection edges | - feather(radius), expand(pixels), contract(pixels), smooth(radius)<br/>- border(width) for border selection<br/>- In-place SelectionState modification | 5 | Sprint 16 | IE-141 |
| IE-149 | As a user, I want selection combine modes (add/subtract/intersect) so that I can build complex selections | - combine(other, SelectionMode)<br/>- Add (Union), Subtract, Intersect modes<br/>- New mode replaces existing | 3 | Sprint 16 | IE-141 |
| IE-150 | As a user, I want marching ants rendering so that I can visualize the selection boundary | - to_gpu_mask() for mask texture<br/>- Marching ants overlay shader<br/>- Configurable ant speed and spacing | 3 | Sprint 16 | IE-141 |
| IE-151 | As a user, I want color range selection so that I can select by color in HSL space | - ColorRangeSelector with target_color, fuzziness<br/>- sample() for Set/Add/Subtract<br/>- generate() produces SelectionState<br/>- localized_color_clusters, range | 5 | Sprint 18 | IE-146 |
| IE-152 | As a user, I want refine edge (smart radius) so that I can improve selection edges on complex subjects | - RefineEdge with radius, smart_radius<br/>- refine() with smooth, feather, contrast, shift_edge<br/>- decontaminate for color bleed | 5 | Sprint 18 | IE-147 |
| IE-153 | As a user, I want refine edge brush tools so that I can manually add/remove from refinement zone | - refine_brush(center, radius, add)<br/>- hair_brush() for hair/fur<br/>- smooth_brush()<br/>- Preview modes: Overlay, OnBlack, OnWhite, etc. | 5 | Sprint 18 | IE-152 |
| IE-154 | As a user, I want to create a layer mask from selection so that I can mask layers non-destructively | - LayerMaskTools::from_selection(selection)<br/>- Converts SelectionState to LayerMask<br/>- from_alpha() for layer alpha | 3 | Sprint 18 | IE-141 |
| IE-155 | As a user, I want layer mask painting (brush-on-mask) so that I can refine masks manually | - paint_mask() with radius, hardness, opacity, erase<br/>- gradient_mask(), fill_mask(), invert_mask()<br/>- levels_mask() for contrast refinement | 5 | Sprint 18 | IE-154 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 6: Selection, Masking & Brush |
| **Sprint(s)** | IE-Sprint 16-17 (Weeks 29-32) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [11-template-system](11-template-system.md) |
| **Successor** | [13-color-science](13-color-science.md) |
| **Story Points Total** | 95 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-171 | As a user, I want rectangular marquee selection tool so that I can select rectangular regions | - RectangularMarquee tool<br/>- SelectionState with mask, bounding_box<br/>- set_all(), clear(), invert() | 3 | P0 | — |
| IE-172 | As a user, I want elliptical marquee selection tool so that I can select circular/oval regions | - EllipticalMarquee tool<br/>- Ellipse mask generation<br/>- Same SelectionState interface | 3 | P0 | IE-171 |
| IE-173 | As a user, I want lasso selection tool (freehand) so that I can make freehand selections | - Lasso tool with freehand path<br/>- Path to mask conversion<br/>- add_point on mouse move | 3 | P0 | IE-171 |
| IE-174 | As a user, I want polygonal lasso selection tool so that I can make straight-edge selections | - PolygonalLasso with click-to-add-point<br/>- Double-click or click start to close<br/>- Straight segments between points | 3 | P0 | IE-171 |
| IE-175 | As a user, I want magnetic lasso (edge-snapping) so that I can snap selections to edges | - MagneticLassoTool with width, contrast, frequency<br/>- detect_edge_path() using Canny + DP<br/>- Snaps to high-contrast edges | 5 | P0 | IE-174 |
| IE-176 | As a user, I want magic wand (tolerance-based flood fill) so that I can select by color similarity | - MagicWandTool with tolerance, contiguous, anti_alias<br/>- select(image, click_point, params)<br/>- Flood fill based on color difference | 5 | P0 | IE-171 |
| IE-177 | As a user, I want quick selection (edge-aware brush) so that I can paint selections that find edges | - QuickSelectionTool with brush_size, auto_enhance<br/>- begin_stroke(), continue_stroke(), end_stroke()<br/>- grow_selection() edge-aware | 5 | P0 | IE-171 |
| IE-178 | As a user, I want color range selection (sample + tolerance) so that I can select by color in HSL | - ColorRangeSelector with target_color, fuzziness<br/>- sample() for Set/Add/Subtract<br/>- generate() produces SelectionState | 5 | P0 | IE-176 |
| IE-179 | As a user, I want selection operations: feather so that I can soften selection edges | - feather(radius) on SelectionState<br/>- Gaussian blur for soft edge<br/>- In-place modification | 2 | P0 | IE-171 |
| IE-180 | As a user, I want selection operations: expand/contract so that I can grow or shrink selection | - expand(pixels), contract(pixels)<br/>- Morphological dilation/erosion<br/>- In-place modification | 2 | P0 | IE-171 |
| IE-181 | As a user, I want selection operations: smooth so that I can remove jagged edges | - smooth(radius) on SelectionState<br/>- Edge smoothing algorithm<br/>- In-place modification | 2 | P0 | IE-171 |
| IE-182 | As a user, I want selection operations: invert so that I can invert selection | - invert() on SelectionState<br/>- Swap selected/unselected<br/>- In-place modification | 2 | P0 | IE-171 |
| IE-183 | As a user, I want selection operations: grow/similar so that I can expand to similar colors | - grow() expands to contiguous similar<br/>- similar() selects all similar in image<br/>- Tolerance-based | 3 | P0 | IE-176 |
| IE-184 | As a user, I want selection rendering (marching ants overlay) so that I can visualize the selection | - to_gpu_mask() for mask texture<br/>- Marching ants overlay shader<br/>- Configurable ant speed and spacing | 3 | P0 | IE-171 |
| IE-185 | As a user, I want selection to path conversion so that I can convert selection to bezier path | - to_path(tolerance) on SelectionState<br/>- Marching squares → bezier<br/>- Adjustable tolerance | 5 | P0 | IE-171 |
| IE-186 | As a user, I want path to selection conversion so that I can convert path to selection | - from_path(BezierPath) on SelectionState<br/>- Rasterize path to mask<br/>- Anti-aliased edge | 3 | P0 | IE-171 |
| IE-187 | As a user, I want refine edge / select and mask panel so that I can improve selection edges | - RefineEdge with radius, smart_radius<br/>- refine() with smooth, feather, contrast, shift_edge<br/>- decontaminate for color bleed | 5 | P0 | IE-177 |
| IE-188 | As a user, I want layer mask from selection so that I can mask layers non-destructively | - LayerMaskTools::from_selection(selection)<br/>- Converts SelectionState to LayerMask<br/>- from_alpha() for layer alpha | 3 | P0 | IE-171 |
| IE-189 | As a user, I want apply/disable/delete layer mask so that I can manage masks | - Apply mask (rasterize to layer)<br/>- Disable mask (temporarily)<br/>- Delete mask | 3 | P0 | IE-188 |
| IE-190 | As a user, I want mask density and feather controls so that I can refine mask effect | - density 0-100% on LayerMask<br/>- feather in pixels<br/>- Real-time preview | 3 | P0 | IE-188 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
