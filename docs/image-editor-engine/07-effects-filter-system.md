
## IE-7. Effects and Filter System (Shared + IE Extensions)

### 7.1 Shared Effect Infrastructure

The image editor uses the identical effect graph, effect registry, and effect processor architecture from the video editor (VE-7). All 80+ video editor effects are available, plus image-specific additions.

### 7.2 IE-Specific Filters and Presets

#### 7.2.1 Photo Enhancement Presets (One-Tap Filters)

| Category | Filters | Description |
|---|---|---|
| **Natural** | Daylight, Golden Hour, Blue Hour, Overcast, Forest, Ocean, Sunrise, Sunset | Color temperature + tint + slight sat adjustments |
| **Portrait** | Soft Skin, Studio Light, Warm Portrait, Cool Portrait, High Key, Low Key, Dramatic, Ethereal | Skin-optimized tone + softening |
| **Vintage** | Film 70s, Film 80s, Film 90s, Polaroid, Kodachrome, Fuji Velvia, Ilford HP5, Tri-X 400, Cross Process | Film emulation via 3D LUT + grain |
| **Cinematic** | Teal & Orange, Muted, Blockbuster, Noir, Desaturated, Bleach Bypass, Film Noir, Matrix Green | Cinema color grading LUTs |
| **B&W** | Classic B&W, High Contrast, Soft B&W, Selenium, Cyanotype, Sepia, Infrared, Film Grain B&W | Desaturation + tone mapping |
| **Mood** | Dreamy, Dark Moody, Airy, Pastel, Neon, Cyberpunk, Retro Wave, Vaporwave | Creative color grading |
| **Food** | Warm Food, Bright Food, Dark Food, Rustic, Clean Food, Cafe Style | Food photography optimized |
| **Landscape** | Vivid Nature, HDR Look, Matte Landscape, Aerial, Desert, Arctic, Tropical | Landscape-optimized grading |

Each filter is implemented as a parameter preset applied to the shared color/effect pipeline:

```cpp
namespace gp::effects {

struct FilterPreset {
    std::string id;               // "gp.filter.vintage.kodachrome"
    std::string display_name;     // "Kodachrome"
    std::string category;         // "Vintage"
    std::string thumbnail_path;   // Preview image
    float default_intensity;      // 0.0 - 1.0 (default 0.75)

    // Preset is defined as a chain of effects with pre-set parameters
    struct EffectSlot {
        std::string effect_id;    // Reference to effect registry
        std::unordered_map<std::string, ParameterValue> params;
    };
    std::vector<EffectSlot> effect_chain;

    // Optional LUT (for film emulations)
    std::string lut_path;         // .cube file path (bundled)
    float lut_intensity;          // 0.0 - 1.0
};

class FilterPresetLibrary {
public:
    static FilterPresetLibrary& instance();

    void register_preset(const FilterPreset& preset);
    const FilterPreset* find(const std::string& id) const;
    std::vector<const FilterPreset*> list_by_category(const std::string& category) const;
    std::vector<std::string> categories() const;

    // Apply preset to a layer or canvas
    void apply_to_layer(int32_t layer_id, const std::string& preset_id, float intensity);
    void apply_to_canvas(const std::string& preset_id, float intensity); // As adjustment layer

private:
    std::vector<FilterPreset> presets_;
};

} // namespace gp::effects
```

#### 7.2.2 Image-Specific Effects (Beyond Shared VE Effects)

| Effect | Parameters | Description |
|---|---|---|
| **Clarity** | amount (-100 to 100), mask_luminosity | Mid-tone contrast enhancement |
| **Dehaze** | amount (-100 to 100) | Remove/add atmospheric haze |
| **Texture** | amount (-100 to 100) | Fine detail enhancement |
| **Shadow/Highlight Recovery** | shadow_amount, highlight_amount, color_correction, midtone_contrast, clip_black, clip_white | Recover detail in shadows and highlights |
| **Split Toning** | shadow_hue, shadow_saturation, highlight_hue, highlight_saturation, balance | Color tint shadows and highlights independently |
| **Grain** | amount, size, roughness, color | Film grain simulation |
| **Chromatic Aberration** | red_cyan, blue_yellow, amount, type (simulate/correct) | Add or remove color fringing |
| **Miniature / Tilt-Shift** | focus_position, blur_amount, transition_size, shape (linear/radial), angle | Selective focus for miniature effect |
| **Orton Effect** | amount, brightness, blur_radius | Dreamy glow (sharp + blurred overlay) |
| **Duotone** | shadow_color, highlight_color, contrast | Two-color tone mapping |
| **Color Splash** | selected_hue, tolerance, feather | Keep one color, desaturate everything else |
| **HDR Tone Map** | strength, natural_saturation, creative_saturation, detail, shadow, highlight | Single-image HDR look |
| **Bokeh Overlay** | shape (circle/hexagon/heart/star), density, size_min, size_max, brightness, color_variation | Decorative bokeh light overlay |
| **Light Leak** | preset (30+ options), intensity, blend_mode | Film light leak overlays |
| **Lens Flare** | position, brightness, scale, type (50mm/35mm/anamorphic), color_temperature | Lens flare generation |
| **Glitch** | shift_amount, color_channels, scan_lines, noise, rgb_shift | Digital glitch/distortion effect |
| **Pixelate (Mosaic)** | cell_size, shape (square/hexagon/triangle) | Intentional pixelation |
| **Oil Paint** | stylization, cleanliness, scale, bristle_detail | Oil painting simulation |
| **Watercolor** | wet_edge, flow, texture, transparency, smoothness | Watercolor simulation |
| **Pencil Sketch** | detail, darkness, edge_type (soft/hard) | Pencil drawing simulation |
| **Comic / Halftone** | dot_size, angle, type (dots/lines/diamond), color_mode | Print halftone simulation |
| **Neon Glow** | glow_size, glow_brightness, glow_color, edge_detection_threshold | Neon edge glow |
| **Double Exposure** | second_image, blend_mode, opacity, mask_type (luminance/gradient) | Composite two images |

### 7.3 Effect Application Modes

```cpp
namespace gp::effects {

// Effects can be applied in different modes in the image editor:
enum class EffectApplicationMode {
    // Non-destructive layer effect (stored as metadata, applied during render)
    LayerEffect,

    // Adjustment layer (affects all layers below)
    AdjustmentLayer,

    // Destructive apply (rasterizes effect into pixel data — cannot undo except via history)
    DestructiveApply,

    // Filter preview (temporary, for real-time preview before committing)
    Preview,

    // Brush-applied effect (apply effect only where brush paints)
    BrushApplied,
};

// Brush-applied effects: apply an effect through a brush stroke
// Used for selective retouching: dodge/burn, selective blur, selective sharpen
struct BrushAppliedEffect {
    std::string effect_id;
    std::unordered_map<std::string, ParameterValue> params;
    float brush_radius;
    float brush_hardness;
    float brush_opacity;
    float brush_flow;

    // Accumulated mask: builds up as user paints
    GpuTexture accumulation_mask;

    void apply_stroke(Vec2 position, float pressure);
    GpuTexture render(IGpuContext& gpu, GpuTexture source);
};

} // namespace gp::effects
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 3: Effects & Filters |
| **Sprint(s)** | IE-Sprint 7-9 (Weeks 11-16) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [06-gpu-rendering-pipeline](06-gpu-rendering-pipeline.md) |
| **Successor** | [08-image-processing](08-image-processing.md) |
| **Story Points Total** | 112 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-073 | As a C++ engine developer, I want shared effect graph integration from VE so that IE uses the same effect pipeline | - Effect graph from shared effects module integrated<br/>- Effect registry and processor architecture<br/>- 80+ VE effects available | 5 | P0 | IE-061 |
| IE-074 | As a C++ engine developer, I want non-destructive effect chain per layer so that effects can be stacked | - effects vector on Layer<br/>- Effect chain evaluated during render<br/>- Add, remove, reorder effects | 5 | P0 | IE-073 |
| IE-075 | As a designer, I want adjustment layers: brightness/contrast so that I can adjust tonality | - BrightnessContrast adjustment with params<br/>- Real-time preview<br/>- Non-destructive | 2 | P0 | IE-074 |
| IE-076 | As a designer, I want adjustment layers: levels so that I can adjust black/white/gamma | - Levels adjustment with input/output black/white<br/>- Per-channel or RGB<br/>- Histogram display | 3 | P0 | IE-074 |
| IE-077 | As a designer, I want adjustment layers: curves so that I can fine-tune tonality | - Curves adjustment with point curve<br/>- Per-channel or RGB<br/>- Smooth curve interpolation | 5 | P0 | IE-074 |
| IE-078 | As a designer, I want adjustment layers: hue/saturation so that I can adjust color | - HueSaturation with hue, saturation, lightness<br/>- Per-hue HSL (optional)<br/>- Colorize option | 3 | P0 | IE-074 |
| IE-079 | As a designer, I want adjustment layers: color balance so that I can shift shadows/midtones/highlights | - ColorBalance with shadows, midtones, highlights<br/>- Cyan-red, magenta-green, yellow-blue<br/>- Preserve luminosity option | 3 | P0 | IE-074 |
| IE-080 | As a designer, I want adjustment layers: black & white so that I can convert to B&W with control | - BlackAndWhite with per-channel mix<br/>- Tint option<br/>- Preset mixes | 3 | P0 | IE-074 |
| IE-081 | As a designer, I want adjustment layers: exposure and vibrance so that I can adjust exposure and vibrance | - Exposure adjustment (-5 to +5 EV)<br/>- Vibrance (preserves skin tones)<br/>- Saturation alternative | 2 | P0 | IE-074 |
| IE-082 | As a C++ engine developer, I want LUT engine integration (.cube/.3dl) so that I can apply 3D LUTs | - Load .cube and .3dl files<br/>- Trilinear interpolation<br/>- LUT intensity 0-100% | 5 | P0 | IE-073 |
| IE-083 | As a designer, I want filter preset library (100+ one-tap photo filters with thumbnails) so that I can apply looks quickly | - FilterPresetLibrary with 100+ presets<br/>- Categories: Natural, Portrait, Vintage, Cinematic, etc.<br/>- Thumbnail per preset | 8 | P0 | IE-082 |
| IE-084 | As a designer, I want blur effects: gaussian/directional/radial/lens blur so that I can blur images | - Gaussian blur with radius<br/>- Directional/motion blur<br/>- Radial and lens blur | 5 | P0 | IE-073 |
| IE-085 | As a designer, I want image-specific: clarity/dehaze/texture so that I can enhance detail | - Clarity (mid-tone contrast)<br/>- Dehaze (atmospheric)<br/>- Texture (fine detail) | 5 | P0 | IE-074 |
| IE-086 | As a designer, I want image-specific: shadow/highlight recovery so that I can recover detail | - Shadow recovery amount<br/>- Highlight recovery amount<br/>- Color correction, midtone contrast | 5 | P0 | IE-074 |
| IE-087 | As a designer, I want image-specific: split toning so that I can tint shadows and highlights | - Shadow hue/saturation<br/>- Highlight hue/saturation<br/>- Balance slider | 3 | P0 | IE-074 |
| IE-088 | As a designer, I want distortion: warp/bulge/twirl/displacement so that I can distort images | - Warp mesh distortion<br/>- Bulge, twirl, pinch<br/>- Displacement map | 5 | P0 | IE-073 |
| IE-089 | As a designer, I want stylize: glow/drop shadow/emboss/mosaic/halftone so that I can add stylized effects | - Glow (blur + blend)<br/>- Drop shadow, emboss<br/>- Mosaic, halftone | 5 | P0 | IE-073 |
| IE-090 | As a designer, I want creative: duotone so that I can create two-color images | - Shadow and highlight color selection<br/>- Contrast control<br/>- Preset duotones | 3 | P1 | IE-074 |
| IE-091 | As a designer, I want creative: color splash so that I can keep one color and desaturate rest | - Selected hue and tolerance<br/>- Feather for smooth transition<br/>- Real-time preview | 3 | P1 | IE-074 |
| IE-092 | As a designer, I want creative: glitch effect so that I can add digital glitch | - RGB shift, scan lines, noise<br/>- Configurable intensity<br/>- Preset glitch styles | 3 | P2 | IE-073 |
| IE-093 | As a designer, I want creative: oil paint so that I can simulate oil painting | - Stylization, cleanliness, scale params<br/>- Bristle detail<br/>- GPU-accelerated | 5 | P1 | IE-073 |
| IE-094 | As a designer, I want creative: watercolor so that I can simulate watercolor | - Wet edge, flow, texture params<br/>- Transparency, smoothness<br/>- GPU-accelerated | 5 | P1 | IE-073 |
| IE-095 | As a designer, I want film grain + vignette so that I can add film look | - Grain: amount, size, roughness, color<br/>- Vignette: amount, midpoint, roundness, feather<br/>- Non-destructive | 3 | P0 | IE-074 |
| IE-096 | As a designer, I want filter favorites and history so that I can reuse and undo filter choices | - Save filter to favorites<br/>- Filter history (last N applied)<br/>- Quick re-apply from history | 3 | P1 | IE-083 |
| IE-097 | As a designer, I want filter intensity slider (0-100%) so that I can control effect strength | - Intensity 0-100% per effect<br/>- Smooth blend with original<br/>- Real-time preview | 2 | P0 | IE-074 |
| IE-098 | As a QA engineer, I want filter preset thumbnails so that I can preview filters before applying | - Thumbnail generated per preset<br/>- Cached for performance<br/>- Display in filter picker UI | 3 | P1 | IE-083 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed

---
