
## IE-15. Transform and Warp System

### 15.1 Standard Transform

```cpp
namespace gp::transform {

class TransformTool {
public:
    enum Mode {
        Free,               // Scale + rotate + move
        Scale,              // Scale only (with/without aspect lock)
        Rotate,             // Rotate only
        Skew,               // Skew/shear
        Distort,            // Free distort (4 corners independent)
        Perspective,        // Perspective transform (vanishing point)
        Warp,               // Envelope warp (mesh-based)
    };

    struct TransformState {
        Vec2 position;
        Vec2 anchor_point;
        Vec2 scale;            // 1.0 = 100%
        float rotation;        // Degrees
        float skew_x, skew_y;  // Degrees

        // For Distort/Perspective
        Vec2 top_left, top_right, bottom_left, bottom_right;

        // For Warp
        MeshWarpGrid* warp_grid;

        Mat3 to_matrix() const;
        static TransformState from_matrix(const Mat3& matrix);
    };

    // Apply transform to layer
    void apply(canvas::Layer& layer, const TransformState& state);

    // Apply transform to selection content
    void apply_selection(canvas::Canvas& canvas, const selection::SelectionState& sel,
                          const TransformState& state);

    // Numeric entry (precise values)
    void set_position(canvas::Layer& layer, Vec2 position);
    void set_scale(canvas::Layer& layer, Vec2 scale);
    void set_rotation(canvas::Layer& layer, float degrees);
    void set_size(canvas::Layer& layer, int width, int height, bool constrain);

    // Flip
    void flip_horizontal(canvas::Layer& layer);
    void flip_vertical(canvas::Layer& layer);

    // Align (relative to canvas or selection)
    enum Alignment {
        AlignLeft, AlignCenter, AlignRight,
        AlignTop, AlignMiddle, AlignBottom,
        DistributeHorizontal, DistributeVertical,
    };
    void align_layers(std::vector<canvas::Layer*>& layers, Alignment alignment,
                       Rect reference);  // Canvas or selection bounds
};

} // namespace gp::transform
```

### 15.2 Mesh Warp (Photoshop Warp)

```cpp
namespace gp::transform {

struct MeshWarpGrid {
    int rows, cols;                  // Default 4x4 (Photoshop standard)
    std::vector<Vec2> control_points; // (rows+1) * (cols+1) points
    std::vector<Vec2> tangent_h;     // Horizontal tangent handles
    std::vector<Vec2> tangent_v;     // Vertical tangent handles

    // Preset warp shapes
    static MeshWarpGrid arc(float bend);
    static MeshWarpGrid arch(float bend);
    static MeshWarpGrid bulge(float bend);
    static MeshWarpGrid shell_lower(float bend);
    static MeshWarpGrid shell_upper(float bend);
    static MeshWarpGrid flag(float bend);
    static MeshWarpGrid wave(float bend);
    static MeshWarpGrid fish(float bend);
    static MeshWarpGrid rise(float bend);
    static MeshWarpGrid fisheye(float bend);
    static MeshWarpGrid inflate(float bend);
    static MeshWarpGrid squeeze(float bend);
    static MeshWarpGrid twist(float bend);
    static MeshWarpGrid custom(int rows, int cols);

    // Get warped position for a UV coordinate
    Vec2 evaluate(float u, float v) const;   // Bicubic interpolation
};

class MeshWarpProcessor {
public:
    // Apply mesh warp to texture (GPU compute shader)
    GpuTexture apply(IGpuContext& gpu, GpuTexture source,
                      const MeshWarpGrid& grid, int output_width, int output_height);

    // Interactive preview (lower quality for speed)
    GpuTexture preview(IGpuContext& gpu, GpuTexture source,
                        const MeshWarpGrid& grid);
};

} // namespace gp::transform
```

### 15.3 Liquify Tool

```cpp
namespace gp::transform {

class LiquifyTool {
public:
    enum BrushType {
        ForwardWarp,        // Push pixels in brush direction
        Reconstruct,        // Restore original position
        TwirlClockwise,     // Rotate pixels clockwise
        TwirlCounterCW,     // Rotate pixels counter-clockwise
        Pucker,             // Pull pixels toward center (shrink)
        Bloat,              // Push pixels away from center (expand)
        PushLeft,           // Move pixels perpendicular to stroke
        Freeze,             // Protect areas from warping
        Thaw,               // Remove freeze protection
    };

    struct BrushParams {
        float size;              // Brush radius
        float pressure;          // 0-100
        float density;           // Edge falloff (0-100)
        float rate;              // Speed of effect per second
    };

    // Initialize liquify session
    void begin(GpuTexture source, int width, int height);

    // Apply brush stroke
    void apply_brush(BrushType type, Vec2 position, Vec2 direction,
                      const BrushParams& params, float delta_time);

    // Face-aware liquify: auto-detect face and provide sliders
    struct FaceLiquifyParams {
        float forehead_height;   // -100 to 100
        float face_width;        // -100 to 100
        float chin_height;       // -100 to 100
        float jawline;           // -100 to 100
        float eye_size;          // -100 to 100
        float eye_height;        // -100 to 100
        float eye_width;         // -100 to 100
        float eye_tilt;          // -100 to 100
        float eye_distance;      // -100 to 100
        float nose_height;       // -100 to 100
        float nose_width;        // -100 to 100
        float mouth_width;       // -100 to 100
        float mouth_height;      // -100 to 100
        float upper_lip;         // -100 to 100
        float lower_lip;         // -100 to 100
        float smile;             // -100 to 100
    };
    void apply_face_liquify(const ai::FaceProcessor::FaceDetection& face,
                             const FaceLiquifyParams& params);

    // Get current result
    GpuTexture get_result(IGpuContext& gpu) const;

    // Undo last stroke
    void undo_stroke();

    // Reset all liquify
    void reset();

    // Freeze mask (prevent areas from being warped)
    void set_freeze_mask(GpuTexture mask);

private:
    // Displacement field: stores per-pixel offset
    struct DisplacementField {
        int width, height;
        std::vector<Vec2> offsets;   // Per-pixel displacement

        Vec2 sample(float x, float y) const; // Bilinear interpolation
        void apply_forward_warp(Vec2 center, Vec2 direction,
                                 float radius, float pressure, float density);
    };

    DisplacementField field_;
    GpuTexture source_;
    GpuTexture freeze_mask_;
    std::vector<DisplacementField> history_; // For undo
};

} // namespace gp::transform
```

### 15.4 Content-Aware Scale

```cpp
namespace gp::transform {

// Seam carving: resize image while preserving important content
class ContentAwareScale {
public:
    struct Params {
        bool protect_faces;          // Auto-detect and protect faces
        GpuTexture protect_mask;     // Custom protection mask
        GpuTexture remove_mask;      // Areas to preferentially remove
        float amount;                // 0-100% content-aware (0 = normal scale)
    };

    // Resize image content-aware
    GpuTexture scale(IGpuContext& gpu, GpuTexture source,
                      int target_width, int target_height,
                      const Params& params);

    // Preview with energy map visualization
    GpuTexture preview_energy(IGpuContext& gpu, GpuTexture source);

private:
    // Seam carving energy computation (Sobel gradient magnitude)
    GpuTexture compute_energy(IGpuContext& gpu, GpuTexture source,
                               GpuTexture protect, GpuTexture remove);

    // Find and remove lowest-energy seam (dynamic programming)
    void remove_seam_vertical(std::vector<float>& energy, int width, int height);
    void remove_seam_horizontal(std::vector<float>& energy, int width, int height);
};

} // namespace gp::transform
```

### 15.5 Perspective Correction

```cpp
namespace gp::transform {

class PerspectiveCorrection {
public:
    // Auto-detect and correct perspective (for architecture, documents)
    struct AutoResult {
        Mat3 correction_matrix;
        float confidence;
        std::vector<Vec2> detected_lines; // Vanishing point lines
    };

    AutoResult auto_correct(IGpuContext& gpu, GpuTexture image);

    // Manual 4-point perspective correction
    GpuTexture correct(IGpuContext& gpu, GpuTexture image,
                        Vec2 src_tl, Vec2 src_tr, Vec2 src_bl, Vec2 src_br,
                        int output_width, int output_height);

    // Adaptive wide-angle correction (lens distortion)
    GpuTexture correct_wide_angle(IGpuContext& gpu, GpuTexture image,
                                    float correction_amount,
                                    float focal_length_mm);

    // Upright correction (auto-level horizon)
    struct UprightMode {
        static constexpr int Auto = 0;
        static constexpr int Level = 1;        // Horizontal lines only
        static constexpr int Vertical = 2;     // Vertical lines only
        static constexpr int Full = 3;         // Both H + V
        static constexpr int Guided = 4;       // User-drawn guides
    };

    GpuTexture upright(IGpuContext& gpu, GpuTexture image, int mode);
};

} // namespace gp::transform
```

### 15.6 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 19 | Wk 35-36 | IE-183 to IE-195 | Mesh warp, perspective, content-aware scale, liquify |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-183 | As a designer, I want a transform tool with free/scale/rotate modes so that I can manipulate layers intuitively | - Free transform combines move, scale, rotate in one interaction<br/>- Scale mode supports aspect lock toggle<br/>- Rotate mode with anchor point control | 5 | Sprint 19 | — |
| IE-184 | As a designer, I want skew transform so that I can create angled text and shapes | - Skew X and Y independently<br/>- Numeric entry for precise skew angles<br/>- Live preview during drag | 3 | Sprint 19 | IE-183 |
| IE-185 | As a designer, I want distort transform with 4-corner independent control so that I can warp images freely | - Each corner moves independently<br/>- Preserves straight lines within quad<br/>- Numeric entry for corner positions | 5 | Sprint 19 | IE-183 |
| IE-186 | As a designer, I want perspective transform with vanishing point so that I can correct architectural photos | - 4-point perspective control<br/>- Vanishing point visualization<br/>- Output to rectangular bounds | 5 | Sprint 19 | IE-185 |
| IE-187 | As a designer, I want numeric transform entry so that I can apply precise values | - Position, scale, rotation, size fields<br/>- Constrain proportions option<br/>- Apply/cancel actions | 3 | Sprint 19 | IE-183 |
| IE-188 | As a designer, I want flip horizontal/vertical so that I can mirror layers quickly | - Flip horizontal preserves layer bounds<br/>- Flip vertical preserves layer bounds<br/>- Works on selection or layer | 2 | Sprint 19 | IE-183 |
| IE-189 | As a designer, I want layer alignment (left/center/right/distribute) so that I can align multiple layers | - Align to canvas or selection bounds<br/>- Distribute horizontal and vertical<br/>- Multi-layer selection support | 3 | Sprint 19 | IE-183 |
| IE-190 | As a designer, I want mesh warp grid (4x4 + preset shapes) so that I can apply Photoshop-style warps | - 4x4 default grid (Photoshop standard)<br/>- Preset shapes: arc, arch, bulge, flag, wave, etc.<br/>- Bend parameter for presets | 5 | Sprint 19 | — |
| IE-191 | As a developer, I want mesh warp GPU processor so that warp is real-time | - GPU compute shader for warp<br/>- Bicubic interpolation for smooth result<br/>- Interactive preview mode | 5 | Sprint 19 | IE-190 |
| IE-192 | As a retoucher, I want liquify tool (forward warp, twirl, pucker, bloat) so that I can reshape images | - Forward warp, twirl, pucker, bloat brushes<br/>- Brush size, pressure, density, rate params<br/>- Displacement field storage | 8 | Sprint 19 | — |
| IE-193 | As a retoucher, I want liquify freeze/thaw masks so that I can protect areas from warping | - Freeze brush protects areas<br/>- Thaw brush removes protection<br/>- Freeze mask stored per session | 3 | Sprint 19 | IE-192 |
| IE-194 | As a retoucher, I want face-aware liquify (slider-based) so that I can adjust facial features safely | - Face detection integration<br/>- Sliders: forehead, face width, chin, jaw, eyes, nose, mouth<br/>- -100 to 100 range per slider | 5 | Sprint 19 | IE-192 |
| IE-195 | As a designer, I want content-aware scale (seam carving) so that I can resize without distorting important content | - Seam carving energy (Sobel gradient)<br/>- Protect faces and custom masks<br/>- Remove mask for preferential removal | 8 | Sprint 19 | — |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 7: Transform, AI & Advanced |
| **Sprint(s)** | IE-Sprint 19-20 (Weeks 35-38) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [16-brush-engine](16-brush-engine.md) |
| **Successor** | [14-ai-features](14-ai-features.md) |
| **Story Points Total** | 75 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-226 | As a designer, I want free transform (scale, rotate, skew) so that I can manipulate layers intuitively | - Free transform combines move, scale, rotate in one interaction<br/>- Skew X and Y independently<br/>- Numeric entry for precise values | 5 | P0 | — |
| IE-227 | As a designer, I want perspective transform (4-corner pin) so that I can correct architectural photos | - 4-point perspective control<br/>- Each corner moves independently<br/>- Output to rectangular bounds | 5 | P0 | IE-226 |
| IE-228 | As a designer, I want flip horizontal/vertical so that I can mirror layers quickly | - Flip horizontal preserves layer bounds<br/>- Flip vertical preserves layer bounds<br/>- Works on selection or layer | 2 | P0 | IE-226 |
| IE-229 | As a designer, I want mesh warp (4x4 grid) so that I can apply Photoshop-style warps | - 4x4 default grid (Photoshop standard)<br/>- Control points with tangent handles<br/>- Bicubic interpolation for smooth result | 5 | P0 | — |
| IE-230 | As a designer, I want mesh warp preset shapes (arc, wave, bulge, shell, flag) so that I can apply common warps quickly | - Preset shapes: arc, arch, bulge, shell_lower, shell_upper, flag, wave<br/>- Bend parameter for presets<br/>- MeshWarpGrid::arc(), ::wave(), etc. | 5 | P0 | IE-229 |
| IE-231 | As a designer, I want mesh warp custom grid (user-defined subdivisions) so that I can create complex warps | - custom(rows, cols) for user-defined grid<br/>- (rows+1)*(cols+1) control points<br/>- GPU compute shader for warp | 5 | P0 | IE-229 |
| IE-232 | As a designer, I want perspective correction (auto-detect vanishing points) so that I can fix perspective automatically | - auto_correct() detects vanishing points<br/>- Returns correction matrix and confidence<br/>- detected_lines for visualization | 5 | P0 | IE-227 |
| IE-233 | As a designer, I want perspective correction (manual 4-point) so that I can precisely correct perspective | - correct() with src_tl, src_tr, src_bl, src_br<br/>- Output width/height specified<br/>- Homography transform applied | 5 | P0 | IE-227 |
| IE-234 | As a designer, I want content-aware scale (seam carving) so that I can resize without distorting important content | - Seam carving energy (Sobel gradient)<br/>- scale() with target dimensions<br/>- protect_faces, protect_mask, remove_mask params | 8 | P0 | — |
| IE-235 | As a designer, I want content-aware scale with protection mask so that I can preserve specific areas | - protect_mask for areas to preserve<br/>- remove_mask for preferential removal<br/>- Params integrated with scale() | 3 | P0 | IE-234 |
| IE-236 | As a retoucher, I want liquify: forward warp tool so that I can push pixels in brush direction | - ForwardWarp brush type<br/>- apply_brush() with position, direction, params<br/>- Displacement field storage | 5 | P0 | — |
| IE-237 | As a retoucher, I want liquify: twirl tool so that I can rotate pixels | - TwirlClockwise, TwirlCounterCW brush types<br/>- rate param for effect speed<br/>- Brush size, pressure, density | 3 | P0 | IE-236 |
| IE-238 | As a retoucher, I want liquify: pucker/bloat tools so that I can shrink or expand areas | - Pucker (pull toward center), Bloat (push away)<br/>- Shared BrushParams<br/>- Undo per stroke | 3 | P0 | IE-236 |
| IE-239 | As a retoucher, I want liquify: freeze/thaw mask so that I can protect areas from warping | - Freeze brush protects areas<br/>- Thaw brush removes protection<br/>- set_freeze_mask() API | 3 | P0 | IE-236 |
| IE-240 | As a retoucher, I want face-aware liquify (slider-based: eyes, nose, mouth, face shape, forehead) so that I can adjust facial features safely | - FaceLiquifyParams with sliders<br/>- forehead_height, face_width, chin_height, jawline, eye_size, etc.<br/>- apply_face_liquify() with face detection | 5 | P0 | IE-236 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
