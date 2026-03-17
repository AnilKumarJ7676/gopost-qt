
## IE-8. Image Processing Pipeline

### 8.1 RAW Image Processing

```cpp
namespace gp::image {

// RAW processing pipeline (LibRaw + custom GPU stages)
// Supports: Canon CR2/CR3, Nikon NEF, Sony ARW, Fuji RAF, Adobe DNG, etc.

struct RawProcessingParams {
    // White Balance
    float temperature;         // 2000-10000K
    float tint;                // -150 to +150

    // Exposure
    float exposure;            // -5 to +5 EV
    float contrast;            // -100 to +100
    float highlights;          // -100 to +100
    float shadows;             // -100 to +100
    float whites;              // -100 to +100
    float blacks;              // -100 to +100

    // Presence
    float clarity;             // -100 to +100
    float dehaze;              // -100 to +100
    float vibrance;            // -100 to +100
    float saturation;          // -100 to +100

    // Tone Curve
    CurveData tone_curve;      // Custom parametric + point curve

    // Detail
    float sharpening_amount;   // 0-150
    float sharpening_radius;   // 0.5-3.0
    float sharpening_detail;   // 0-100
    float sharpening_masking;  // 0-100
    float noise_reduction_luminance; // 0-100
    float noise_reduction_color;     // 0-100
    float noise_reduction_detail;    // 0-100

    // Lens Corrections
    bool enable_profile_corrections;
    bool remove_chromatic_aberration;
    float distortion_correction;
    float vignette_correction;

    // Color
    std::string color_profile; // "Adobe Standard", "Camera Faithful", etc.
    HslAdjustments hsl;        // Per-hue H/S/L adjustments

    // Crop
    Rect crop_rect;
    float crop_angle;

    // Output
    ColorSpace output_space;   // sRGB, DisplayP3, AdobeRGB, ProPhotoRGB
    BitDepth output_depth;     // Uint8, Uint16, Float32
};

struct HslAdjustments {
    struct ChannelAdjust {
        float hue;             // -100 to +100
        float saturation;      // -100 to +100
        float luminance;       // -100 to +100
    };
    ChannelAdjust red, orange, yellow, green, aqua, blue, purple, magenta;
};

class RawProcessor {
public:
    RawProcessor();
    ~RawProcessor();

    // Decode RAW file to linear RGB (CPU, LibRaw)
    bool open(const std::string& path);
    bool open_buffer(const uint8_t* data, size_t size);

    // Get RAW metadata
    RawMetadata metadata() const;

    // Process RAW with given parameters (GPU-accelerated)
    GpuTexture process(IGpuContext& gpu, const RawProcessingParams& params);

    // Generate preview (fast, 1/4 resolution)
    GpuTexture preview(IGpuContext& gpu, const RawProcessingParams& params);

    // Get embedded JPEG thumbnail (instant)
    bool get_thumbnail(uint8_t** data, int* width, int* height);

private:
    LibRaw raw_processor_;
    float* linear_rgb_ = nullptr;    // Demosaiced linear RGB
    int width_, height_;
};

struct RawMetadata {
    std::string camera_make;
    std::string camera_model;
    std::string lens_model;
    float iso;
    float shutter_speed;
    float aperture;
    float focal_length;
    int width, height;
    std::string timestamp;
    GpsCoord gps;
    int orientation;                 // EXIF orientation
};

} // namespace gp::image
```

### 8.2 Retouching Tools

```cpp
namespace gp::image {

// Healing Brush: content-aware repair using Poisson image editing
class HealingBrush {
public:
    struct Params {
        float radius;
        float hardness;            // 0-1
        float opacity;             // 0-1
        HealingMode mode;          // Normal, Replace
        SampleMode sample;         // CurrentLayer, CurrentAndBelow, AllLayers
    };

    // Begin healing stroke
    void begin_stroke(Vec2 source_point, Vec2 dest_point);

    // Continue stroke (samples from source, blends at destination)
    void add_point(Vec2 dest_point, float pressure);

    // Finish stroke: apply Poisson blending for seamless result
    void end_stroke(IGpuContext& gpu, GpuTexture target_layer);

private:
    // Poisson blending solver (GPU compute shader)
    GpuTexture poisson_blend(IGpuContext& gpu,
                              GpuTexture source_patch,
                              GpuTexture target_region,
                              GpuTexture mask);
};

enum class HealingMode { Normal, Replace };
enum class SampleMode { CurrentLayer, CurrentAndBelow, AllLayers };

// Clone Stamp: pixel-accurate copying from source to destination
class CloneStamp {
public:
    struct Params {
        float radius;
        float hardness;
        float opacity;
        float flow;
        bool aligned;              // Source follows cursor offset
        SampleMode sample;
    };

    void set_source(Vec2 source_point);
    void begin_stroke(Vec2 dest_point);
    void add_point(Vec2 dest_point, float pressure);
    void end_stroke(IGpuContext& gpu, GpuTexture target_layer);
};

// Patch Tool: select area, drag to source, content-aware blend
class PatchTool {
public:
    enum Mode { Normal, ContentAware };

    // Define the patch selection (bezier path or lasso)
    void set_selection(const BezierPath& selection);

    // Set destination (where to sample from)
    void set_destination(Vec2 offset);

    // Apply patch with Poisson blending
    GpuTexture apply(IGpuContext& gpu, GpuTexture source_layer, Mode mode);
};

// Spot Healing: one-click blemish removal
class SpotHealing {
public:
    struct Params {
        float radius;
        SpotHealingType type;      // ContentAware, CreateTexture, ProximityMatch
    };

    GpuTexture apply(IGpuContext& gpu, GpuTexture source_layer,
                      Vec2 center, const Params& params);

private:
    // Content-aware fill for the spot region
    GpuTexture content_aware_fill(IGpuContext& gpu,
                                   GpuTexture source,
                                   GpuTexture mask,
                                   Rect region);
};

enum class SpotHealingType { ContentAware, CreateTexture, ProximityMatch };

// Red Eye Removal
class RedEyeRemoval {
public:
    struct Detection {
        Vec2 center;
        float radius;
        float confidence;
    };

    // Auto-detect red eyes in image
    std::vector<Detection> detect(IGpuContext& gpu, GpuTexture image);

    // Remove red eye at specified location
    GpuTexture remove(IGpuContext& gpu, GpuTexture image,
                       Vec2 center, float radius, float darkening = 0.5f);
};

} // namespace gp::image
```

### 8.3 HDR Imaging

```cpp
namespace gp::image {

// HDR merge: combine multiple exposures into a single HDR image
class HdrMerger {
public:
    struct BracketImage {
        GpuTexture texture;
        float exposure_value;       // EV offset from base
        bool is_base;
    };

    // Add exposure bracket images
    void add_bracket(const BracketImage& bracket);

    // Align brackets (handle slight camera movement)
    void align(IGpuContext& gpu);

    // Merge to 32-bit HDR
    GpuTexture merge(IGpuContext& gpu);

    // Tone map HDR to displayable LDR
    GpuTexture tone_map(IGpuContext& gpu, GpuTexture hdr,
                         const ToneMapParams& params);

    // Ghost removal (moving objects between brackets)
    void enable_deghosting(bool enabled, float threshold = 0.5f);

private:
    std::vector<BracketImage> brackets_;
    bool deghosting_ = true;
    float deghost_threshold_ = 0.5f;
};

struct ToneMapParams {
    ToneMapOperator op;          // Reinhard, ACES, Filmic, Mantiuk, Drago
    float exposure;              // Overall exposure adjustment
    float gamma;                 // Display gamma
    float saturation;            // Color saturation
    float detail;                // Local contrast detail
    float shadow;                // Shadow recovery
    float highlight;             // Highlight compression
};

enum class ToneMapOperator { Reinhard, ACES, Filmic, Mantiuk, Drago, Uncharted2, AgX };

// Single-image HDR look (simulated)
class PseudoHdr {
public:
    GpuTexture apply(IGpuContext& gpu, GpuTexture input,
                      float strength, float detail, float saturation);
};

} // namespace gp::image
```

### 8.4 Image Adjustments (Non-Destructive)

```cpp
namespace gp::image {

// Applied to image layers as non-destructive metadata
struct ImageAdjustments {
    // Basic
    float brightness = 0;        // -100 to 100
    float contrast = 0;          // -100 to 100
    float exposure = 0;          // -5 to 5 EV
    float highlights = 0;        // -100 to 100
    float shadows = 0;           // -100 to 100
    float whites = 0;            // -100 to 100
    float blacks = 0;            // -100 to 100

    // Color
    float temperature = 0;       // -100 to 100 (maps to Kelvin)
    float tint = 0;              // -100 to 100
    float vibrance = 0;          // -100 to 100
    float saturation = 0;        // -100 to 100

    // Detail
    float clarity = 0;           // -100 to 100
    float texture = 0;           // -100 to 100
    float dehaze = 0;            // -100 to 100
    float sharpness = 0;         // 0 to 100
    float noise_reduction = 0;   // 0 to 100

    // Vignette
    float vignette_amount = 0;   // -100 to 100
    float vignette_midpoint = 50;
    float vignette_roundness = 0;
    float vignette_feather = 50;

    // Grain
    float grain_amount = 0;      // 0 to 100
    float grain_size = 25;
    float grain_roughness = 50;

    bool has_adjustments() const;
    GpuTexture apply(IGpuContext& gpu, GpuTexture input) const;
};

} // namespace gp::image
```

### 8.5 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 9 | Wk 15-16 | IE-083 to IE-092 | Retouching tools, healing brush, clone stamp, patch, spot healing, red eye, image adjustments |
| Sprint 21 | Wk 39-40 | IE-079 to IE-082, IE-089 to IE-091 | RAW processor, RAW GPU pipeline, HDR bracket merger, HDR alignment, HDR tone mapping |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-079 | As a photographer, I want to open and decode RAW files (LibRaw) so that I can edit professional camera formats | - Support Canon CR2/CR3, Nikon NEF, Sony ARW, Fuji RAF, Adobe DNG<br/>- open() and open_buffer() decode to linear RGB<br/>- Extract RAW metadata | 5 | Sprint 21 | — |
| IE-080 | As a photographer, I want a RAW processing params model so that I can adjust white balance, exposure, and detail non-destructively | - RawProcessingParams struct with temperature, tint, exposure, contrast, highlights, shadows<br/>- Tone curve, sharpening, noise reduction params<br/>- Lens correction and color profile options | 3 | Sprint 21 | IE-079 |
| IE-081 | As a photographer, I want RAW processing on GPU so that edits apply in real time | - process() runs demosaic and adjustments on GPU<br/>- Output to GpuTexture in target color space<br/>- Support Uint8, Uint16, Float32 output | 5 | Sprint 21 | IE-079, IE-080 |
| IE-082 | As a photographer, I want a fast RAW preview (1/4 res) so that I can browse and cull quickly | - preview() generates 1/4 resolution in &lt;100ms<br/>- Uses same params as full process<br/>- Optional embedded JPEG thumbnail path | 3 | Sprint 21 | IE-081 |
| IE-083 | As a retoucher, I want a healing brush with begin/continue/end stroke so that I can seamlessly repair blemishes | - begin_stroke(source, dest), add_point(), end_stroke()<br/>- Samples from source, blends at destination<br/>- Supports SampleMode (CurrentLayer, AllLayers) | 5 | Sprint 9 | — |
| IE-084 | As a retoucher, I want Poisson blending (GPU compute) so that healing results are seamless | - poisson_blend() GPU compute shader<br/>- Seamless gradient-domain blending<br/>- Handles source patch, target region, mask | 5 | Sprint 9 | IE-083 |
| IE-085 | As a retoucher, I want a clone stamp tool so that I can pixel-accurate copy from source to destination | - set_source(), begin_stroke(), add_point(), end_stroke()<br/>- Aligned and non-aligned modes<br/>- Radius, hardness, opacity, flow params | 3 | Sprint 9 | — |
| IE-086 | As a retoucher, I want a patch tool so that I can select an area and content-aware blend from another region | - set_selection(BezierPath), set_destination()<br/>- Normal and ContentAware modes<br/>- Poisson blending for seamless result | 5 | Sprint 9 | IE-084 |
| IE-087 | As a retoucher, I want spot healing (one-click) so that I can remove small blemishes instantly | - apply(center, params) with ContentAware, CreateTexture, ProximityMatch<br/>- Radius-based spot selection<br/>- Single-click application | 3 | Sprint 9 | IE-084 |
| IE-088 | As a photographer, I want red eye removal (detect + fix) so that I can correct flash-induced red eye | - detect() returns center, radius, confidence for each eye<br/>- remove() applies darkening at specified location<br/>- Configurable darkening amount | 3 | Sprint 9 | — |
| IE-089 | As a photographer, I want HDR bracket merger so that I can combine multiple exposures into one HDR image | - add_bracket() for exposure brackets<br/>- merge() produces 32-bit HDR<br/>- Optional deghosting for moving objects | 5 | Sprint 21 | — |
| IE-090 | As a photographer, I want HDR alignment (camera movement) so that brackets align before merge | - align() handles slight camera movement between shots<br/>- Feature-based or optical flow alignment<br/>- Runs before merge() | 5 | Sprint 21 | IE-089 |
| IE-091 | As a photographer, I want HDR tone mapping (6 operators) so that I can display HDR on LDR screens | - Reinhard, ACES, Filmic, Mantiuk, Drago, Uncharted2, AgX operators<br/>- tone_map() with exposure, gamma, saturation, detail params<br/>- Ghost removal option | 5 | Sprint 21 | IE-089 |
| IE-092 | As a user, I want image adjustments (non-destructive) so that I can tweak brightness, contrast, color without committing | - ImageAdjustments struct (brightness, contrast, exposure, highlights, shadows, etc.)<br/>- has_adjustments(), apply() to GpuTexture<br/>- Stored as layer metadata | 5 | Sprint 9 | — |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 7: Transform, AI & Advanced |
| **Sprint(s)** | IE-Sprint 20-21 (Weeks 39-42) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [07-effects-filter-system](07-effects-filter-system.md) |
| **Successor** | [09-text-typography](09-text-typography.md) |
| **Story Points Total** | 68 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-099 | As a photographer, I want RAW image processing (LibRaw integration) so that I can edit professional camera formats | - LibRaw integration for CR2/CR3, NEF, ARW, RAF, DNG<br/>- open(), open_buffer() decode to linear RGB<br/>- RawMetadata extraction | 5 | P0 | — |
| IE-100 | As a photographer, I want RAW develop pipeline (white balance, exposure, highlights, shadows, clarity, vibrance) so that I can adjust RAW non-destructively | - RawProcessingParams with temperature, tint, exposure, contrast<br/>- Highlights, shadows, whites, blacks<br/>- Clarity, dehaze, vibrance, saturation | 5 | P0 | IE-099 |
| IE-101 | As a C++ engine developer, I want RAW demosaicing (AMaZE/DCB algorithm) so that RAW converts to RGB correctly | - AMaZE and DCB demosaic algorithms<br/>- GPU-accelerated demosaic<br/>- Output to linear RGB | 5 | P0 | IE-099 |
| IE-102 | As a photographer, I want RAW noise reduction so that I can reduce noise in high-ISO images | - Luminance and color noise reduction<br/>- Detail preservation params<br/>- GPU pipeline | 5 | P0 | IE-100 |
| IE-103 | As a photographer, I want RAW lens correction (auto-detect lens profile) so that I can fix lens distortion | - Auto-detect lens from metadata<br/>- Distortion, vignette, CA correction<br/>- enable_profile_corrections | 5 | P0 | IE-100 |
| IE-104 | As a retoucher, I want healing brush (Poisson blending) so that I can seamlessly repair blemishes | - HealingBrush with begin_stroke, add_point, end_stroke<br/>- Poisson blending GPU compute shader<br/>- Sample from source, blend at destination | 8 | P0 | — |
| IE-105 | As a retoucher, I want clone stamp tool so that I can pixel-accurate copy from source | - CloneStamp with set_source, begin_stroke, add_point, end_stroke<br/>- Aligned and non-aligned modes<br/>- Radius, hardness, opacity, flow | 3 | P0 | — |
| IE-106 | As a retoucher, I want patch tool (select source→drag to destination) so that I can content-aware blend | - PatchTool with set_selection, set_destination<br/>- Normal and ContentAware modes<br/>- Poisson blending for seamless result | 5 | P0 | IE-104 |
| IE-107 | As a retoucher, I want spot healing (auto-source selection) so that I can remove small blemishes in one click | - SpotHealing with ContentAware, CreateTexture, ProximityMatch<br/>- Radius-based selection<br/>- Single-click application | 3 | P0 | IE-104 |
| IE-108 | As a photographer, I want red eye removal (auto-detect + correct) so that I can fix flash red eye | - detect() returns center, radius, confidence<br/>- remove() with darkening amount<br/>- Auto-detect in portrait images | 3 | P0 | — |
| IE-109 | As a photographer, I want HDR merge (multi-exposure bracket alignment + merge) so that I can create HDR images | - HdrMerger with add_bracket, align, merge<br/>- 32-bit HDR output<br/>- Deghosting for moving objects | 8 | P0 | — |
| IE-110 | As a photographer, I want HDR tone mapping (local + global) so that I can display HDR on LDR screens | - Reinhard, ACES, Filmic, Mantiuk, Drago, AgX operators<br/>- tone_map() with exposure, gamma, saturation<br/>- Local and global options | 5 | P0 | IE-109 |
| IE-111 | As a photographer, I want panorama stitching (feature matching + blending) so that I can create panoramas | - Feature matching between images<br/>- Alignment and blending<br/>- Output to canvas | 8 | P1 | — |
| IE-112 | As a photographer, I want focus stacking (depth-of-field extension) so that I can extend DoF | - Align focus bracket images<br/>- Depth map from focus distance<br/>- Merge to extended DoF | 8 | P1 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
