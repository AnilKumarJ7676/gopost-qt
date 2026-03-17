
## VE-7. Effects and Filter System

### 7.1 Effect Graph Architecture

```cpp
namespace gp::effects {

// Effects are structured as a directed acyclic graph (DAG), not a simple chain.
// This enables complex routing (split, merge, multi-input effects).

enum class ParameterType {
    Float, Vec2, Vec3, Vec4, Color, Int, Bool,
    Enum, Angle, Point2D, Point3D,
    Curve,          // Bezier curve (for Curves effect)
    Gradient,       // Color gradient
    LayerRef,       // Reference to another layer (for displacement maps)
    Image,          // Static image input (for textures, LUTs)
};

struct ParameterDef {
    std::string id;
    std::string display_name;
    ParameterType type;
    ParameterValue default_value;
    ParameterValue min_value;
    ParameterValue max_value;
    std::string group;          // UI grouping
    bool keyframeable;
};

struct EffectDefinition {
    std::string id;              // "gp.builtin.gaussian_blur"
    std::string display_name;    // "Gaussian Blur"
    std::string category;        // "Blur & Sharpen"
    std::string description;
    int32_t version;
    std::vector<ParameterDef> parameters;
    int32_t input_count;         // Number of input layers/textures
    bool gpu_accelerated;
    bool supports_compute;       // Can use compute shaders

    // Shader references
    std::string fragment_shader; // For render pipeline effects
    std::string compute_shader;  // For compute pipeline effects (preferred)
};

struct EffectInstance {
    int32_t id;
    std::string effect_def_id;
    bool enabled;
    float mix;                    // 0.0 - 1.0 (dry/wet blend)
    std::unordered_map<std::string, ParameterValue> parameters;
    std::unordered_map<std::string, KeyframeTrack> animated_params;
    MaskRef effect_mask;          // Optional mask to limit effect region

    ParameterValue evaluate_param(const std::string& param_id, Rational time) const;
};

// Effect processor: evaluates one effect on the GPU
class EffectProcessor {
public:
    virtual ~EffectProcessor() = default;

    virtual void prepare(IGpuContext& gpu, const EffectDefinition& def) = 0;
    virtual void process(IGpuContext& gpu,
                        GpuTexture input,
                        GpuTexture output,
                        const EffectInstance& instance,
                        Rational time,
                        const RenderContext& ctx) = 0;
    virtual void release(IGpuContext& gpu) = 0;
};

// Effect registry: all built-in and plugin effects
class EffectRegistry {
public:
    static EffectRegistry& instance();

    void register_effect(const EffectDefinition& def, std::unique_ptr<EffectProcessor> processor);
    const EffectDefinition* find(const std::string& id) const;
    std::vector<const EffectDefinition*> list_by_category(const std::string& category) const;
    std::vector<std::string> categories() const;

    EffectProcessor* get_processor(const std::string& id) const;
};

} // namespace gp::effects
```

### 7.2 Built-in Effects Catalog

#### 7.2.1 Color & Tone

| Effect | Parameters | GPU Shader |
|---|---|---|
| Brightness/Contrast | brightness (-150 to 150), contrast (-150 to 150), use_legacy | color_brightness_contrast.comp |
| Hue/Saturation | master_hue (-180 to 180), saturation (-100 to 100), lightness (-100 to 100), colorize, per-channel adjustments | color_hue_sat.comp |
| Levels | input_black, input_white, gamma, output_black, output_white (per channel + master) | color_levels.comp |
| Curves | RGB master curve + individual R, G, B curves (bezier spline, 256 LUT) | color_curves.comp |
| Color Balance | shadow/midtone/highlight tint (R, G, B offsets), preserve_luminosity | color_balance.comp |
| Channel Mixer | 3x3 matrix + constant offsets, monochrome mode | color_channel_mixer.comp |
| Selective Color | per-color adjustments (reds, yellows, greens, cyans, blues, magentas, whites, neutrals, blacks) | color_selective.comp |
| Vibrance | vibrance (-100 to 100) — intelligent saturation boost for undersaturated colors | color_vibrance.comp |
| Color Wheels (3-Way) | lift, gamma, gain wheels + master offset, temperature, tint | color_wheels.comp |
| Exposure | exposure (-5 to 5 EV), offset, gamma | color_exposure.comp |
| White Balance | temperature (2000K–10000K), tint (-150 to 150) | color_white_balance.comp |
| LUT (3D) | Load .cube / .3dl LUT files, intensity 0–100 | color_lut3d.comp |
| Film Grain | amount, size, roughness, color_amount, gaussian/uniform | generate_grain.comp |
| Vignette | amount, midpoint, roundness, feather | generate_vignette.comp |
| Posterize | levels (2–256) | color_posterize.comp |
| Threshold | threshold (0–255), smooth | color_threshold.comp |
| Invert | channels (RGB, R, G, B, A) | color_invert.comp |
| Tint | map_black_to, map_white_to, amount | color_tint.comp |

#### 7.2.2 Blur & Sharpen

| Effect | Parameters | Notes |
|---|---|---|
| Gaussian Blur | radius (0–250px), repeat_edge_pixels, blur_dimensions (H+V, H only, V only) | Two-pass separable |
| Box Blur | radius, iterations | Fast approximation |
| Directional Blur | angle, distance | Motion-style |
| Radial Blur | type (spin/zoom), amount, center | |
| Zoom Blur | center, amount | |
| Lens Blur (Bokeh) | radius, blade_count, blade_curvature, specular_brightness, specular_threshold, noise, depth_map | Physically-based |
| Bilateral Blur | radius, threshold | Edge-preserving |
| Compound Blur | blur_layer (reference), max_blur | Blur from luminance of another layer |
| Sharpen | amount (0–500) | Simple convolution |
| Unsharp Mask | amount, radius, threshold | Professional sharpening |
| Smart Sharpen | amount, radius, reduce_noise, remove (gaussian/lens/motion) | |

#### 7.2.3 Distort

| Effect | Parameters |
|---|---|
| Transform | position, scale, rotation, skew, anchor, opacity |
| Corner Pin | four corner positions (perspective warp) |
| Mesh Warp | grid_rows, grid_cols, vertex positions (free-form deformation) |
| Bezier Warp | top/bottom/left/right bezier curves |
| Bulge | center, radius, height | 
| Twirl | angle, radius, center |
| Spherize | radius, center, type (horizontal/vertical/both) |
| Ripple | radius, wavelength, amplitude, phase |
| Wave Warp | wave_type (sine/square/triangle/sawtooth), height, width, direction, phase, speed |
| Displacement Map | map_layer, max_horizontal, max_vertical, map_channels |
| Turbulent Displace | amount, size, complexity, evolution, type |
| Optics Compensation | FOV, reverse_correction, center (lens distortion) |
| Polar Coordinates | interpolation, type (rect_to_polar, polar_to_rect) |

#### 7.2.4 Keying & Matte

| Effect | Parameters |
|---|---|
| Keylight (Chroma Key) | screen_colour, screen_gain, screen_balance, clip_black/white, screen_shrink/grow, despill | Industry-standard green/blue screen keyer |
| Luma Key | key_type (key_out_brighter/darker), threshold, tolerance, edge_feather |
| Color Difference Key | key_color, partial_A, partial_B |
| Difference Key | reference_frame, matching_tolerance |
| Spill Suppressor | key_color, suppression_amount, color_accuracy |
| Simple Choker | choke_matte (px, negative = spread) |
| Matte Choker | geometric_softness_1/2, grey_level_softness_1/2, choke |
| Refine Edge | smooth, feather, contrast, shift_edge, decontaminate |

#### 7.2.5 Generate

| Effect | Parameters |
|---|---|
| Solid Color | color, opacity |
| Gradient Ramp | start_point, start_color, end_point, end_color, ramp_shape (linear/radial), scatter |
| 4-Color Gradient | colors[4], positions[4], blend, jitter |
| Checkerboard | anchor, size, feather, color, opacity, blending_mode |
| Grid | anchor, corner, border, feather, color, opacity, blending_mode |
| Fractal Noise | fractal_type, noise_type, contrast, brightness, overflow, transform, complexity, evolution, sub-settings, opacity, blending_mode |
| Cell Pattern | cell_pattern, invert, contrast, overflow, disperse, size, offset, tiling_options, evolution |

#### 7.2.6 Stylize

| Effect | Parameters |
|---|---|
| Glow | threshold, radius, intensity, color, composite (over/behind/none) |
| Drop Shadow | color, opacity, direction, distance, softness |
| Emboss | direction, relief, contrast, blend_with_original |
| Find Edges | invert, blend_with_original |
| Mosaic | horizontal_blocks, vertical_blocks |
| Strobe Light | strobe_duration, strobe_period, type, random_phase, blend_with_original |
| Motion Tile | tile_center, tile_width, tile_height, output_width, output_height, mirror_edges, phase, horizontal_phase_shift |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 3: Effects & Color |
| **Sprint(s)** | VE-Sprint 7-9 (Weeks 13-18) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [05-composition-engine](05-composition-engine.md) |
| **Successor** | [08-keyframe-animation](08-keyframe-animation.md) |
| **Story Points Total** | 96 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-089 | As a C++ engine developer, I want EffectDefinition and ParameterDef data models so that we can describe effects declaratively | - EffectDefinition: id, display_name, category, parameters, input_count<br/>- ParameterDef: id, type, default, min, max, keyframeable<br/>- ParameterType enum (Float, Vec2, Color, Curve, etc.) | 5 | P0 | VE-064 |
| VE-090 | As a C++ engine developer, I want EffectInstance with parameter map and animation so that we can apply effects with keyframes | - EffectInstance: effect_def_id, parameters map, animated_params<br/>- evaluate_param(param_id, time) resolves keyframes<br/>- mix (dry/wet), effect_mask support | 5 | P0 | VE-089 |
| VE-091 | As a C++ engine developer, I want EffectProcessor abstract class so that we have a uniform interface for effect execution | - prepare(gpu, def), process(gpu, input, output, instance, time, ctx)<br/>- release(gpu)<br/>- GPU-accelerated processing | 3 | P0 | VE-089 |
| VE-092 | As a C++ engine developer, I want EffectRegistry singleton (register/find/list) so that we can discover and use effects | - register_effect(def, processor)<br/>- find(id), list_by_category(category), categories()<br/>- get_processor(id) | 3 | P0 | VE-091 |
| VE-093 | As a C++ engine developer, I want Color effects: brightness/contrast compute shader so that we have basic color correction | - brightness (-150 to 150), contrast (-150 to 150)<br/>- color_brightness_contrast.comp<br/>- Real-time on mid-range GPU | 3 | P0 | VE-091 |
| VE-094 | As a C++ engine developer, I want Color effects: hue/saturation compute shader so that we support HSL adjustment | - master_hue, saturation, lightness, colorize<br/>- color_hue_sat.comp<br/>- Per-channel optional | 3 | P0 | VE-093 |
| VE-095 | As a C++ engine developer, I want Color effects: levels compute shader so that we support professional levels | - input_black/white, gamma, output_black/white<br/>- color_levels.comp<br/>- Per-channel + master | 5 | P0 | VE-093 |
| VE-096 | As a C++ engine developer, I want Color effects: curves (bezier spline→256 LUT) so that we support curve-based grading | - RGB master + R, G, B curves<br/>- Bezier to 256-entry LUT<br/>- color_curves.comp | 5 | P0 | VE-025 |
| VE-097 | As a C++ engine developer, I want Color effects: color balance + channel mixer so that we support advanced color | - Shadow/midtone/highlight tint<br/>- 3x3 matrix + offsets, monochrome<br/>- color_balance.comp, color_channel_mixer.comp | 5 | P0 | VE-093 |
| VE-098 | As a C++ engine developer, I want Color effects: selective color + vibrance so that we support targeted color adjustment | - Per-color (reds, yellows, etc.) adjustments<br/>- Vibrance intelligent saturation<br/>- color_selective.comp, color_vibrance.comp | 5 | P1 | VE-093 |
| VE-099 | As a C++ engine developer, I want Blur effects: gaussian separable (H+V passes) so that we have efficient blur | - Two-pass horizontal + vertical<br/>- radius 0-250px, repeat_edge_pixels<br/>- blur_dimensions option | 5 | P0 | VE-091 |
| VE-100 | As a C++ engine developer, I want Blur effects: directional + radial + zoom so that we have motion-style blurs | - Directional: angle, distance<br/>- Radial: spin/zoom, amount, center<br/>- Zoom: center, amount | 5 | P0 | VE-099 |
| VE-101 | As a C++ engine developer, I want Blur effects: lens blur (bokeh, physically-based) so that we support depth-of-field | - radius, blade_count, blade_curvature<br/>- specular_brightness, specular_threshold<br/>- Optional depth_map input | 8 | P1 | VE-099 |
| VE-102 | As a C++ engine developer, I want Distort effects: corner pin + mesh warp so that we support perspective and free-form warp | - Corner pin: 4 corner positions<br/>- Mesh warp: grid_rows, grid_cols, vertex positions<br/>- GPU shader implementation | 5 | P0 | VE-091 |
| VE-103 | As a C++ engine developer, I want Distort effects: bulge + twirl + ripple + wave warp so that we support organic distortion | - Bulge, twirl, ripple, wave warp<br/>- All with center, radius/amount params<br/>- wave_type for wave warp | 5 | P0 | VE-102 |
| VE-104 | As a C++ engine developer, I want Distort effects: displacement map + turbulent displace so that we support map-based distortion | - Displacement map: map_layer, max_horizontal, max_vertical<br/>- Turbulent displace: amount, size, complexity, evolution<br/>- Map channel selection | 5 | P1 | VE-102 |
| VE-105 | As a C++ engine developer, I want Keying: chroma key (YCbCr) + spill suppressor so that we support green screen | - Keylight-style chroma key in YCbCr<br/>- screen_colour, gain, balance, clip_black/white<br/>- Spill suppressor post-key | 8 | P0 | VE-091 |
| VE-106 | As a C++ engine developer, I want Keying: luma key + difference key so that we support luminance and difference keying | - Luma key: threshold, tolerance, edge_feather<br/>- Difference key: reference_frame, matching_tolerance<br/>- GPU shaders | 5 | P0 | VE-105 |
| VE-107 | As a C++ engine developer, I want Matte: choker + refine edge so that we can clean up key edges | - Simple choker: choke_matte (px)<br/>- Matte choker: geometric_softness, grey_level_softness<br/>- Refine edge: smooth, feather, contrast, decontaminate | 5 | P1 | VE-105 |
| VE-108 | As a C++ engine developer, I want Generate: fractal noise + gradient + checkerboard so that we have procedural content | - Fractal noise: fractal_type, contrast, evolution<br/>- Gradient: linear/radial, start/end, colors<br/>- Checkerboard: size, feather, color | 5 | P0 | VE-091 |
| VE-109 | As a C++ engine developer, I want Stylize: glow + drop shadow + emboss + mosaic so that we have stylization effects | - Glow: threshold, radius, intensity, color<br/>- Drop shadow: color, direction, distance, softness<br/>- Emboss, mosaic | 5 | P0 | VE-099 |
| VE-110 | As a C++ engine developer, I want Film grain + vignette so that we have finishing effects | - Film grain: amount, size, roughness, color_amount<br/>- Vignette: amount, midpoint, roundness, feather<br/>- generate_grain.comp, generate_vignette.comp | 3 | P1 | VE-091 |
| VE-111 | As a C++ engine developer, I want Effect masking (limit to mask region) so that effects can be constrained to a mask | - EffectInstance.effect_mask (MaskRef)<br/>- Apply mask before/after effect<br/>- Feather support | 5 | P1 | VE-090 |
| VE-112 | As a C++ engine developer, I want Effect mix (dry/wet blend) so that we can blend effect output with original | - mix 0.0-1.0 in EffectInstance<br/>- output = lerp(input, effect_output, mix)<br/>- All effects support mix | 2 | P0 | VE-090 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
