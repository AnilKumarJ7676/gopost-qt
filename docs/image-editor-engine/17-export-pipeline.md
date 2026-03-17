
## IE-17. Export Pipeline

### 17.1 Supported Export Formats

| Format | Extension | Color Depth | Alpha | ICC Profile | Metadata | Use Case |
|---|---|---|---|---|---|---|
| JPEG | .jpg, .jpeg | 8-bit | No | Yes | EXIF, IPTC, XMP | Photos, web, social |
| PNG | .png | 8/16-bit | Yes | Yes | Minimal | Graphics, transparency |
| WebP | .webp | 8-bit | Yes | No | EXIF | Web optimization |
| AVIF | .avif | 8/10/12-bit | Yes | Yes | EXIF | Next-gen web |
| HEIF/HEIC | .heif, .heic | 8/10-bit | Yes | Yes | EXIF | Apple ecosystem |
| TIFF | .tif, .tiff | 8/16/32-bit | Yes | Yes | Full EXIF/IPTC/XMP | Print, archival |
| PDF | .pdf | 8-bit | Yes | Yes (CMYK) | Full | Print, documents |
| SVG | .svg | N/A (vector) | N/A | N/A | Minimal | Vector export |
| PSD | .psd | 8/16-bit | Yes | Yes | Layers preserved | Adobe interchange |
| GIF | .gif | 8-bit (indexed) | 1-bit | No | Minimal | Simple animation |

### 17.2 Export Configuration

```cpp
namespace gp::xport {

struct ExportConfig {
    ImageFormat format;
    int width, height;                  // Output dimensions (0 = canvas size)
    float scale_factor;                 // 1.0 = native, 2.0 = 2x, etc.
    float dpi;                          // 72 (screen), 150, 300, 600 (print)

    // Format-specific options
    struct JpegOptions {
        int quality;                    // 1-100
        bool progressive;
        ChromaSubsampling chroma;       // 444, 422, 420
        bool optimize_huffman;
    } jpeg;

    struct PngOptions {
        PngBitDepth bit_depth;          // Uint8, Uint16
        int compression_level;          // 0-9 (0=fast, 9=small)
        bool interlaced;
    } png;

    struct WebpOptions {
        int quality;                    // 1-100 (lossy)
        bool lossless;
        int effort;                     // 0-6 (0=fast, 6=small)
    } webp;

    struct TiffOptions {
        TiffBitDepth bit_depth;         // Uint8, Uint16, Float32
        TiffCompression compression;    // None, LZW, ZIP, JPEG
        bool layers;                    // Preserve layers (for PSD compat)
    } tiff;

    struct PdfOptions {
        PdfColorMode color_mode;        // RGB, CMYK
        bool embed_fonts;
        bool rasterize_effects;         // Rasterize GPU effects for PDF compat
        float raster_dpi;               // DPI for rasterized elements
        PdfStandard standard;           // None, PDF_X1A, PDF_X3, PDF_X4
    } pdf;

    struct PsdOptions {
        bool preserve_layers;           // Export as layered PSD
        PsdBitDepth bit_depth;          // Uint8, Uint16
        bool maximize_compatibility;    // Include flattened composite
    } psd;

    // Color management
    ColorSpace output_color_space;      // sRGB (default), DisplayP3, AdobeRGB, CMYK
    bool embed_icc_profile;
    const ICCProfile* target_profile;   // For CMYK or custom profile

    // Metadata
    bool include_exif;
    bool include_iptc;
    bool include_xmp;
    bool strip_location;                // Remove GPS data for privacy

    // Watermark
    struct Watermark {
        bool enabled;
        std::string text;
        GpuTexture image;              // Image watermark (alternative to text)
        Vec2 position;                  // Normalized 0-1
        float opacity;
        float size;                     // Relative to output size
        float rotation;
    } watermark;
};

enum class ImageFormat { JPEG, PNG, WebP, AVIF, HEIF, TIFF, PDF, SVG, PSD, GIF };
enum class ChromaSubsampling { CS_444, CS_422, CS_420 };
enum class PdfColorMode { RGB, CMYK, Grayscale };
enum class PdfStandard { None, PDF_X1A, PDF_X3, PDF_X4 };

} // namespace gp::xport
```

### 17.3 Export Engine

```cpp
namespace gp::xport {

class ExportEngine {
public:
    ExportEngine(IGpuContext& gpu);

    // Single image export
    ExportResult export_image(const canvas::Canvas& canvas,
                               const ExportConfig& config,
                               const std::string& output_path);

    // Export to memory buffer (for sharing, clipboard)
    ExportResult export_to_buffer(const canvas::Canvas& canvas,
                                   const ExportConfig& config,
                                   std::vector<uint8_t>& out_buffer);

    // Batch export (same canvas, multiple formats/sizes)
    struct BatchItem {
        ExportConfig config;
        std::string output_path;
    };
    std::vector<ExportResult> export_batch(const canvas::Canvas& canvas,
                                            const std::vector<BatchItem>& items,
                                            std::function<void(int, int, float)> progress);

    // Export selection only
    ExportResult export_selection(const canvas::Canvas& canvas,
                                   const selection::SelectionState& selection,
                                   const ExportConfig& config,
                                   const std::string& output_path);

    // Export single layer
    ExportResult export_layer(const canvas::Canvas& canvas,
                               int32_t layer_id,
                               const ExportConfig& config,
                               const std::string& output_path);

    // Export for social media (auto-configure format, size, quality)
    ExportResult export_for_platform(const canvas::Canvas& canvas,
                                      SocialPlatform platform,
                                      const std::string& output_path);

private:
    // Render final composite at export resolution
    GpuTexture render_export(const canvas::Canvas& canvas,
                              int width, int height, float dpi);

    // Apply color space conversion
    GpuTexture convert_color_space(GpuTexture image,
                                    ColorSpace from, ColorSpace to,
                                    const ICCProfile* profile);

    // Encode to specific format
    bool encode_jpeg(GpuTexture image, const ExportConfig& config,
                      const std::string& path);
    bool encode_png(GpuTexture image, const ExportConfig& config,
                     const std::string& path);
    bool encode_webp(GpuTexture image, const ExportConfig& config,
                      const std::string& path);
    bool encode_tiff(GpuTexture image, const ExportConfig& config,
                      const std::string& path);
    bool encode_pdf(const canvas::Canvas& canvas, const ExportConfig& config,
                     const std::string& path);
    bool encode_psd(const canvas::Canvas& canvas, const ExportConfig& config,
                     const std::string& path);

    IGpuContext& gpu_;
};

struct ExportResult {
    bool success;
    std::string output_path;
    size_t file_size;
    int width, height;
    float duration_ms;
    std::string error_message;
};

} // namespace gp::xport
```

### 17.4 Social Media Export Presets

```cpp
namespace gp::xport {

enum class SocialPlatform {
    Instagram_Post,         // 1080x1080, JPEG q90
    Instagram_Story,        // 1080x1920, JPEG q90
    Instagram_Reel_Cover,   // 1080x1920, JPEG q90
    Instagram_Carousel,     // 1080x1080 or 1080x1350, JPEG q90
    Facebook_Post,          // 1200x630, JPEG q85
    Facebook_Cover,         // 820x312, JPEG q85
    Facebook_Story,         // 1080x1920, JPEG q85
    Facebook_Ad,            // 1200x628, JPEG q90
    Twitter_Post,           // 1600x900, JPEG q85
    Twitter_Header,         // 1500x500, JPEG q85
    LinkedIn_Post,          // 1200x1200 or 1200x627, JPEG q85
    LinkedIn_Cover,         // 1584x396, JPEG q85
    Pinterest_Pin,          // 1000x1500, JPEG q85
    TikTok_Cover,           // 1080x1920, JPEG q90
    YouTube_Thumbnail,      // 1280x720, JPEG q90
    YouTube_Banner,         // 2560x1440, JPEG q85
    Snapchat_Story,         // 1080x1920, JPEG q85
    WhatsApp_Status,        // 1080x1920, JPEG q80
    Telegram_Sticker,       // 512x512, WebP lossless
};

struct SocialExportPreset {
    SocialPlatform platform;
    std::string name;
    int width, height;
    ImageFormat format;
    int quality;
    float max_file_size_mb;     // Platform file size limits
    bool needs_safe_zone;        // Account for UI overlays
    Rect safe_zone;              // Safe area rectangle (normalized)
};

const SocialExportPreset& get_social_preset(SocialPlatform platform);

} // namespace gp::xport
```

### 17.5 Print Export Pipeline

```cpp
namespace gp::xport {

class PrintExporter {
public:
    struct PrintConfig {
        float width_mm, height_mm;       // Physical size in millimeters
        float bleed_mm;                  // Bleed area (typically 3mm)
        float dpi;                       // 300 default for print
        PrintColorMode color_mode;       // CMYK, RGB
        const ICCProfile* output_profile; // CMYK profile
        RenderingIntent intent;
        bool crop_marks;                 // Add crop marks
        bool registration_marks;         // Add registration marks
        bool color_bars;                 // Add color calibration bars
        bool slug_area;                  // Include slug information
    };

    enum class PrintColorMode { CMYK, RGB, Grayscale };

    // Export print-ready PDF
    ExportResult export_print_pdf(const canvas::Canvas& canvas,
                                   const PrintConfig& config,
                                   const std::string& path);

    // Export print-ready TIFF (for prepress)
    ExportResult export_print_tiff(const canvas::Canvas& canvas,
                                    const PrintConfig& config,
                                    const std::string& path);

    // Preflight check: validate print readiness
    struct PreflightResult {
        bool passed;
        std::vector<PreflightWarning> warnings;
        std::vector<PreflightError> errors;
    };

    struct PreflightWarning {
        std::string message;
        int32_t layer_id;              // Affected layer (-1 = canvas-level)
        PreflightIssue issue;
    };

    struct PreflightError {
        std::string message;
        int32_t layer_id;
        PreflightIssue issue;
    };

    enum class PreflightIssue {
        LowResolution,                 // Image below DPI threshold
        RGBColor,                      // RGB in CMYK workflow
        TransparencyOverprint,         // Transparency may not print correctly
        FontNotEmbedded,
        ImageNotEmbedded,
        OutOfGamut,                    // Colors outside printable gamut
        MissingBleed,
        TextTooSmall,                  // < 6pt may not reproduce
        HairlineStroke,                // < 0.25pt stroke
    };

    PreflightResult preflight(const canvas::Canvas& canvas,
                               const PrintConfig& config);
};

} // namespace gp::xport
```

### 17.6 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 22 | Wk 41-42 | IE-213 to IE-228 | Export pipeline, social presets, print |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-213 | As a developer, I want JPEG encoder (libjpeg-turbo) so that JPEG export is fast and high quality | - libjpeg-turbo integration<br/>- Quality 1-100, progressive, chroma subsampling<br/>- EXIF/IPTC/XMP metadata support | 3 | Sprint 22 | — |
| IE-214 | As a developer, I want PNG encoder (libpng) so that lossless export with alpha is supported | - libpng integration<br/>- 8-bit and 16-bit depth<br/>- Compression level 0-9 | 3 | Sprint 22 | — |
| IE-215 | As a developer, I want WebP encoder (libwebp) so that modern web-optimized export is available | - libwebp integration<br/>- Lossy and lossless modes<br/>- Effort 0-6 for compression | 3 | Sprint 22 | — |
| IE-216 | As a developer, I want AVIF encoder so that next-gen web export is supported | - AVIF encoding (libavif or similar)<br/>- 8/10/12-bit support<br/>- Quality and speed tradeoff | 5 | Sprint 22 | — |
| IE-217 | As a developer, I want HEIF encoder (Apple platforms) so that Apple ecosystem export works | - HEIF/HEIC on iOS/macOS<br/>- 8/10-bit, alpha support<br/>- EXIF metadata | 5 | Sprint 22 | — |
| IE-218 | As a developer, I want TIFF encoder (8/16/32-bit) so that print and archival export is supported | - 8, 16, 32-bit float<br/>- LZW, ZIP, JPEG compression<br/>- Full EXIF/IPTC/XMP | 5 | Sprint 22 | — |
| IE-219 | As a developer, I want PDF export engine (libharu) so that print-ready PDFs can be generated | - libharu integration<br/>- RGB, CMYK, grayscale<br/>- PDF/X-1a, X-3, X-4 standards | 5 | Sprint 22 | — |
| IE-220 | As a developer, I want SVG exporter so that vector export is available | - Raster to vector path conversion or embedded raster<br/>- Layer structure preservation where applicable<br/>- Minimal metadata | 5 | Sprint 22 | — |
| IE-221 | As a developer, I want PSD export (layered) so that Adobe interchange works | - Layered PSD format<br/>- 8/16-bit support<br/>- Maximize compatibility option | 5 | Sprint 22 | — |
| IE-222 | As a developer, I want export config model so that all format options are centralized | - ExportConfig struct with format-specific options<br/>- Color management, metadata, watermark<br/>- Validation before export | 3 | Sprint 22 | — |
| IE-223 | As a user, I want single image export so that I can save my work | - Export to file path<br/>- Format and quality selection<br/>- Progress feedback | 3 | Sprint 22 | IE-222, IE-213 |
| IE-224 | As a user, I want export to memory buffer so that I can share or paste without saving | - ExportResult with buffer data<br/>- Clipboard integration path<br/>- Format selection | 3 | Sprint 22 | IE-223 |
| IE-225 | As a user, I want batch export (multi-format) so that I can export one canvas to many formats/sizes | - Multiple BatchItems per canvas<br/>- Progress callback (current, total, percent)<br/>- Parallel or sequential execution | 5 | Sprint 22 | IE-223 |
| IE-226 | As a user, I want social media presets (19 platforms) so that I can export for Instagram, Facebook, etc. | - 19 platform presets (Instagram, Facebook, Twitter, LinkedIn, Pinterest, TikTok, YouTube, etc.)<br/>- Auto dimensions, format, quality<br/>- Safe zone for overlays | 5 | Sprint 22 | IE-223 |
| IE-227 | As a user, I want print export (CMYK + ICC + crop marks) so that I can create print-ready files | - CMYK conversion with ICC profile<br/>- Crop marks, registration marks, color bars<br/>- Bleed and slug area | 8 | Sprint 22 | IE-219, IE-218 |
| IE-228 | As a user, I want preflight check system so that I can validate print readiness before export | - PreflightResult with warnings and errors<br/>- Low resolution, RGB in CMYK, out of gamut, etc.<br/>- Layer-level issue reporting | 5 | Sprint 22 | IE-227 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 8: Export, Polish & Launch |
| **Sprint(s)** | IE-Sprint 22-23 (Weeks 41-44) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [14-ai-features](14-ai-features.md) |
| **Successor** | [19-plugin-architecture](19-plugin-architecture.md) |
| **Story Points Total** | 90 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-261 | As a developer, I want JPEG export (quality slider, progressive, EXIF preservation) so that photos export correctly | - libjpeg-turbo integration<br/>- Quality 1-100, progressive, chroma subsampling<br/>- EXIF/IPTC/XMP metadata support | 3 | P0 | — |
| IE-262 | As a developer, I want PNG export (8-bit, 16-bit, alpha channel) so that lossless export with alpha is supported | - libpng integration<br/>- 8-bit and 16-bit depth<br/>- Compression level 0-9, alpha channel | 3 | P0 | — |
| IE-263 | As a developer, I want WebP export (lossy/lossless, quality) so that modern web-optimized export is available | - libwebp integration<br/>- Lossy and lossless modes<br/>- Effort 0-6 for compression | 3 | P0 | — |
| IE-264 | As a developer, I want AVIF export (quality, speed preset) so that next-gen web export is supported | - AVIF encoding (libavif or similar)<br/>- Quality and speed tradeoff<br/>- 8/10/12-bit support | 5 | P0 | — |
| IE-265 | As a developer, I want HEIF export (quality) so that Apple ecosystem export works | - HEIF/HEIC on iOS/macOS<br/>- 8/10-bit, alpha support<br/>- EXIF metadata | 5 | P0 | — |
| IE-266 | As a developer, I want TIFF export (8/16/32-bit, LZW/ZIP compression) so that print and archival export is supported | - 8, 16, 32-bit float<br/>- LZW, ZIP, JPEG compression<br/>- Full EXIF/IPTC/XMP | 5 | P0 | — |
| IE-267 | As a developer, I want PDF export (single/multi-page, vector elements preserved) so that print-ready PDFs can be generated | - libharu integration<br/>- RGB, CMYK, grayscale<br/>- Vector elements preserved where possible | 5 | P0 | — |
| IE-268 | As a user, I want social media export presets (Instagram post/story/reel, Facebook, TikTok, Twitter, YouTube thumbnail, LinkedIn, Pinterest, Snapchat, WhatsApp) so that I can export for each platform | - 19 platform presets with correct dimensions<br/>- Auto format and quality per platform<br/>- Safe zone for overlays | 5 | P0 | IE-261, IE-262 |
| IE-269 | As a user, I want batch export (same canvas → multiple formats/sizes simultaneously) so that I can export efficiently | - Multiple BatchItems per canvas<br/>- Progress callback (current, total, percent)<br/>- Parallel or sequential execution | 5 | P0 | IE-261 |
| IE-270 | As a developer, I want print export: CMYK conversion with ICC profile so that print-ready files are accurate | - CMYK conversion with ICC profile<br/>- Rendering intent support<br/>- Color-accurate output | 5 | P0 | IE-267 |
| IE-271 | As a developer, I want print export: crop marks and bleed so that print files have proper trim | - Crop marks, registration marks<br/>- Bleed area (typically 3mm)<br/>- Slug area option | 3 | P0 | IE-267 |
| IE-272 | As a developer, I want print export: preflight check (resolution, color space) so that print readiness is validated | - PreflightResult with warnings and errors<br/>- Low resolution, RGB in CMYK, out of gamut<br/>- Layer-level issue reporting | 5 | P0 | IE-270 |
| IE-273 | As a developer, I want PSD export (layered, blend modes, effects) so that Adobe interchange works | - Layered PSD format<br/>- 8/16-bit support<br/>- Blend modes and effects preserved | 5 | P0 | — |
| IE-274 | As a user, I want export quality preview (before/after with file size) so that I can compare before exporting | - Before/after preview<br/>- Estimated file size display<br/>- Quality slider feedback | 3 | P0 | IE-261 |
| IE-275 | As a user, I want export metadata control (strip/preserve EXIF/IPTC/XMP) so that I can control privacy | - include_exif, include_iptc, include_xmp options<br/>- strip_location for GPS removal<br/>- Metadata in ExportConfig | 3 | P0 | IE-261 |
| IE-276 | As a developer, I want export config model so that all format options are centralized | - ExportConfig struct with format-specific options<br/>- Color management, metadata, watermark<br/>- Validation before export | 3 | P0 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
