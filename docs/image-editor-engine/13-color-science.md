
## IE-13. Color Science (Shared + IE Extensions)

### 13.1 Shared Color Infrastructure

The image editor shares the complete color science module from the video editor:
- **Color space conversions** (sRGB, Linear, Rec.709, Rec.2020, DCI-P3, ACES)
- **3D LUT engine** (.cube, .3dl loading, trilinear interpolation)
- **HDR pipeline** (RGBA16F working space, PQ/HLG transfer functions)
- **Tone mapping operators** (Reinhard, ACES, Filmic, AgX)
- **Color wheels** (lift/gamma/gain, offset)
- **Color grading effects** (curves, levels, hue/sat, color balance, selective color)

### 13.2 IE-Specific Color Features

```cpp
namespace gp::color {

// Color picker (eyedropper) with averaging
class ColorPicker {
public:
    struct PickResult {
        Color4f color;              // Picked color in working space
        Color4f srgb;               // Converted to sRGB for display
        Vec3 hsv;                   // HSV representation
        Vec3 hsl;                   // HSL representation
        Vec3 lab;                   // CIELAB
        Vec4 cmyk;                  // CMYK (estimated)
        std::string hex;            // "#RRGGBB"
        std::string css_rgb;        // "rgb(R, G, B)"
    };

    PickResult pick(GpuTexture image, Vec2 point, SampleMode sample = SampleMode::PointSample);

    enum class SampleMode {
        PointSample,        // Single pixel
        Average3x3,         // 3x3 pixel average
        Average5x5,         // 5x5 pixel average
        Average11x11,       // 11x11 pixel average
        Average31x31,       // 31x31 pixel average
        Average51x51,       // 51x51 pixel average
        AverageCustom,      // Custom radius
    };
};

// ICC Color Profile Management
class ICCProfileManager {
public:
    static ICCProfileManager& instance();

    // Load ICC profile from file
    bool load_profile(const std::string& path, const std::string& name);

    // Built-in profiles
    const ICCProfile* srgb() const;
    const ICCProfile* display_p3() const;
    const ICCProfile* adobe_rgb() const;
    const ICCProfile* prophoto_rgb() const;
    const ICCProfile* fogra39() const;           // CMYK print standard (Europe)
    const ICCProfile* swop_v2() const;           // CMYK print standard (US)
    const ICCProfile* japan_color_2001() const;  // CMYK print (Japan)

    // Convert between profiles
    void convert(const float* src, float* dst, int pixel_count,
                  const ICCProfile* src_profile,
                  const ICCProfile* dst_profile,
                  RenderingIntent intent = RenderingIntent::Perceptual);

    // Soft proof: simulate print output on screen
    GpuTexture soft_proof(IGpuContext& gpu, GpuTexture image,
                           const ICCProfile* print_profile,
                           RenderingIntent intent);

    // Gamut warning: highlight out-of-gamut pixels
    GpuTexture gamut_warning(IGpuContext& gpu, GpuTexture image,
                              const ICCProfile* target_profile,
                              Color4f warning_color = {1, 0, 1, 0.5f});

    // Extract profile from image file (JPEG/TIFF/PNG ICC chunk)
    const ICCProfile* extract_from_image(const std::string& image_path);

    // Embed profile into exported image
    std::vector<uint8_t> profile_data(const ICCProfile* profile) const;

    enum class RenderingIntent { Perceptual, RelativeColorimetric, Saturation, AbsoluteColorimetric };
};

// Histogram computation and analysis
class HistogramAnalyzer {
public:
    struct Histogram {
        std::array<int, 256> luminance;
        std::array<int, 256> red;
        std::array<int, 256> green;
        std::array<int, 256> blue;
        int total_pixels;

        // Statistics
        float mean_luminance;
        float median_luminance;
        float std_dev;
        float shadows_percentage;     // % pixels in 0-63 range
        float midtones_percentage;    // % pixels in 64-191 range
        float highlights_percentage;  // % pixels in 192-255 range
        float clipped_blacks;         // % pixels at 0
        float clipped_whites;         // % pixels at 255
    };

    // Compute histogram from image or layer
    Histogram compute(GpuTexture image);

    // Compute histogram for selection region only
    Histogram compute_region(GpuTexture image, const selection::SelectionState& selection);

    // Auto-levels: compute optimal levels parameters from histogram
    struct AutoLevels {
        float input_black;
        float input_white;
        float gamma;
    };
    AutoLevels auto_levels(const Histogram& hist, float clip_percentage = 0.1f);

    // Auto-contrast: compute optimal brightness/contrast
    struct AutoContrast {
        float brightness;
        float contrast;
    };
    AutoContrast auto_contrast(const Histogram& hist);

    // Auto-color: compute per-channel auto levels
    struct AutoColor {
        AutoLevels red, green, blue;
    };
    AutoColor auto_color(const Histogram& hist);
};

} // namespace gp::color
```

### 13.3 Color Harmony Generator

```cpp
namespace gp::color {

// Generate harmonious color palettes for template design
class ColorHarmony {
public:
    enum Scheme {
        Complementary,      // Opposite on color wheel
        Analogous,          // Adjacent on color wheel
        Triadic,            // Three equidistant colors
        SplitComplementary, // Base + two adjacent to complement
        Tetradic,           // Four colors (two complementary pairs)
        Square,             // Four equidistant colors
        Monochromatic,      // Variations of one hue
    };

    struct Palette {
        std::vector<Color4f> colors;
        std::string scheme_name;
    };

    // Generate palette from base color
    static Palette generate(Color4f base, Scheme scheme, int variations = 5);

    // Extract palette from image (K-means clustering)
    static Palette extract_from_image(GpuTexture image, int num_colors = 6);

    // Suggest accessible color pairs (WCAG AA/AAA contrast)
    struct AccessiblePair {
        Color4f foreground;
        Color4f background;
        float contrast_ratio;
        bool passes_aa;          // >= 4.5:1 for normal text
        bool passes_aaa;         // >= 7:1 for normal text
    };
    static std::vector<AccessiblePair> accessible_pairs(const Palette& palette);

    // Color temperature adjustments
    static Color4f warm_shift(Color4f color, float amount);
    static Color4f cool_shift(Color4f color, float amount);
};

} // namespace gp::color
```

### 13.4 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 7 | Wk 11-12 | IE-156 to IE-159 | Shared color module, ICC profiles, soft proofing, gamut warning |
| Sprint 8 | Wk 13-14 | IE-160 to IE-165 | Histogram, auto-levels/contrast/color, color picker, harmony, accessibility, print |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-156 | As a colorist, I want shared color module integration so that I use the same color pipeline as the video editor | - Color space conversions (sRGB, Linear, Rec.709, DCI-P3, ACES)<br/>- 3D LUT engine, HDR pipeline<br/>- Tone mapping, color wheels, grading effects | 5 | Sprint 7 | — |
| IE-157 | As a colorist, I want ICC profile management (lcms2) so that I can work in color-managed workflows | - ICCProfileManager with load_profile()<br/>- Built-in: sRGB, Display P3, Adobe RGB, ProPhoto RGB, FOGRA39, SWOP<br/>- convert() between profiles with RenderingIntent | 5 | Sprint 7 | IE-156 |
| IE-158 | As a colorist, I want soft proofing so that I can simulate print output on screen | - soft_proof(image, print_profile, intent)<br/>- Simulates target profile on display<br/>- Perceptual, RelativeColorimetric, etc. | 5 | Sprint 7 | IE-157 |
| IE-159 | As a colorist, I want gamut warning overlay so that I can see out-of-gamut pixels | - gamut_warning(image, target_profile, warning_color)<br/>- Highlights out-of-gamut pixels<br/>- Configurable warning color (default magenta) | 3 | Sprint 7 | IE-157 |
| IE-160 | As a colorist, I want histogram computation (GPU) so that I can analyze image tonality | - HistogramAnalyzer::compute(image)<br/>- Luminance, R, G, B channels<br/>- Statistics: mean, median, std_dev, shadows/midtones/highlights %, clipped | 5 | Sprint 8 | IE-156 |
| IE-161 | As a user, I want auto-levels / auto-contrast / auto-color so that I can quickly improve image tonality | - auto_levels(hist) → input_black, input_white, gamma<br/>- auto_contrast(hist) → brightness, contrast<br/>- auto_color(hist) → per-channel AutoLevels | 5 | Sprint 8 | IE-160 |
| IE-162 | As a user, I want a color picker (eyedropper with averaging) so that I can sample colors from the image | - ColorPicker::pick(image, point, SampleMode)<br/>- PointSample, Average3x3, 5x5, 11x11, 31x31, 51x51<br/>- PickResult: color, srgb, hsv, hsl, lab, cmyk, hex, css_rgb | 3 | Sprint 8 | IE-156 |
| IE-163 | As a designer, I want a color harmony generator so that I can create harmonious palettes | - ColorHarmony::generate(base, scheme, variations)<br/>- Schemes: Complementary, Analogous, Triadic, SplitComplementary, Tetradic, Square, Monochromatic<br/>- extract_from_image() | 5 | Sprint 8 | IE-156 |
| IE-164 | As a designer, I want an accessible color pair checker so that I can ensure WCAG compliance | - ColorHarmony::accessible_pairs(palette)<br/>- AccessiblePair: foreground, background, contrast_ratio<br/>- passes_aa (≥4.5:1), passes_aaa (≥7:1) | 3 | Sprint 8 | IE-163 |
| IE-165 | As a user, I want print color conversion (RGB to CMYK) so that I can prepare images for print | - ICCProfileManager with fogra39(), swop_v2(), japan_color_2001()<br/>- convert() from RGB to CMYK profile<br/>- extract_from_image(), embed profile in export | 5 | Sprint 8 | IE-157 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 3: Effects & Filters |
| **Sprint(s)** | IE-Sprint 7-8 (Weeks 11-14) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [12-masking-selection.md](12-masking-selection.md) |
| **Successor** | [14-ai-features.md](14-ai-features.md) |
| **Story Points Total** | 58 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-191 | As a colorist, I want ICC color profile support so that I can work in color-managed workflows | - ICCProfileManager with load_profile()<br/>- Built-in: sRGB, Display P3, Adobe RGB, ProPhoto RGB, FOGRA39, SWOP<br/>- convert() between profiles with RenderingIntent | 5 | P0 | — |
| IE-192 | As a colorist, I want profile assignment/conversion so that I can convert between color spaces | - Assign profile to image/layer<br/>- Convert between any loaded profiles<br/>- Perceptual, RelativeColorimetric, Saturation, AbsoluteColorimetric intents | 5 | P0 | IE-191 |
| IE-193 | As a colorist, I want soft proofing (CMYK simulation) so that I can simulate print output on screen | - soft_proof(image, print_profile, intent)<br/>- Simulates target profile on display<br/>- CMYK preview for print preparation | 5 | P0 | IE-192 |
| IE-194 | As a colorist, I want a color space indicator so that I know the current working color space | - Display current color space in UI<br/>- Profile name and color model<br/>- Bit depth indicator | 2 | P1 | IE-191 |
| IE-195 | As a colorist, I want histogram (RGB/Lum/per-channel) so that I can analyze image tonality | - HistogramAnalyzer::compute(image)<br/>- Luminance, R, G, B channels<br/>- Statistics: mean, median, std_dev, shadows/midtones/highlights % | 5 | P0 | — |
| IE-196 | As a colorist, I want live histogram update so that I see changes in real time | - Histogram updates on layer/canvas change<br/>- Selection region histogram support<br/>- Efficient incremental update | 3 | P1 | IE-195 |
| IE-197 | As a user, I want a color picker (eyedropper, sample size) so that I can sample colors from the image | - ColorPicker::pick(image, point, SampleMode)<br/>- PointSample, Average3x3, 5x5, 11x11, 31x31, 51x51<br/>- PickResult: color, srgb, hsv, hsl, lab, cmyk, hex | 3 | P0 | — |
| IE-198 | As a designer, I want a color swatches panel so that I can save and reuse colors | - Color swatches storage and retrieval<br/>- Add/remove swatches from image<br/>- Swatch groups and organization | 3 | P1 | — |
| IE-199 | As a designer, I want color harmony tools so that I can create harmonious palettes | - ColorHarmony::generate(base, scheme, variations)<br/>- Schemes: Complementary, Analogous, Triadic, SplitComplementary, Tetradic, Square, Monochromatic<br/>- extract_from_image() | 5 | P0 | — |
| IE-200 | As a colorist, I want gamut warning overlay so that I can see out-of-gamut pixels | - gamut_warning(image, target_profile, warning_color)<br/>- Highlights out-of-gamut pixels<br/>- Configurable warning color (default magenta) | 3 | P0 | IE-192 |
| IE-201 | As a colorist, I want color depth support (8/16/32-bit) so that I can work with high bit depth images | - 8-bit, 16-bit, 32-bit float support<br/>- Conversion between bit depths<br/>- Pipeline preserves precision | 5 | P0 | — |
| IE-202 | As a colorist, I want a color-managed pipeline so that colors are consistent across display and export | - End-to-end color management<br/>- Working space to display conversion<br/>- Export with embedded profile | 5 | P0 | IE-191 |
| IE-203 | As a colorist, I want working color space selection so that I can choose the project color space | - Select working color space (sRGB, Adobe RGB, ProPhoto RGB, etc.)<br/>- Per-project or per-document setting<br/>- Affects all color operations | 3 | P0 | IE-191 |
| IE-204 | As a user, I want print color management (CMYK preview) so that I can prepare images for print | - ICCProfileManager with fogra39(), swop_v2(), japan_color_2001()<br/>- convert() from RGB to CMYK profile<br/>- Soft proof and gamut check for print | 5 | P0 | IE-193 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
