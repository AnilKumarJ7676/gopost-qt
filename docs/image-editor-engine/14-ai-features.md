
## IE-14. AI Features

### 14.1 Shared AI Infrastructure

The image editor shares the ML inference engine from the video editor:
- **Platform-specific inference** — CoreML (iOS/macOS), NNAPI/TFLite (Android), ONNX Runtime (Windows)
- **Model management** — Download, cache, versioning
- **Background removal** — Segmentation model (shared)

### 14.2 AI Background Removal (Enhanced for Images)

```cpp
namespace gp::ai {

class BackgroundRemoval {
public:
    struct Params {
        ModelQuality quality;         // Fast, Balanced, HighQuality
        bool refine_edges;            // Post-process edge refinement
        float edge_feather;           // Feather amount for edges
        bool hair_refinement;         // Special handling for hair/fur
        bool preserve_shadows;        // Keep shadows semi-transparent
    };

    enum class ModelQuality {
        Fast,            // ~50ms, good for previews
        Balanced,        // ~200ms, good quality
        HighQuality,     // ~500ms, best quality (Segment Anything-class)
    };

    // Full image background removal
    GpuTexture remove_background(IGpuContext& gpu, GpuTexture image, const Params& params);

    // Get alpha matte only
    GpuTexture get_matte(IGpuContext& gpu, GpuTexture image, const Params& params);

    // Interactive: user taps foreground/background points
    GpuTexture interactive_segment(IGpuContext& gpu, GpuTexture image,
                                    const std::vector<Vec2>& foreground_points,
                                    const std::vector<Vec2>& background_points);

    // Replace background
    GpuTexture replace_background(IGpuContext& gpu, GpuTexture image,
                                   GpuTexture new_background, const Params& params);
};

} // namespace gp::ai
```

### 14.3 AI Object Removal (Content-Aware Fill)

```cpp
namespace gp::ai {

class ObjectRemoval {
public:
    struct Params {
        RemovalMethod method;
        int iterations;               // Higher = better quality, slower
        bool color_adaptation;         // Adapt colors to surrounding area
    };

    enum class RemovalMethod {
        PatchMatch,          // Traditional PatchMatch inpainting
        LaMa,               // Large Mask Inpainting (deep learning)
        StableDiffusion,     // Generative inpainting (highest quality, slowest)
    };

    // Remove objects within mask region
    GpuTexture remove(IGpuContext& gpu, GpuTexture image,
                       GpuTexture mask, const Params& params);

    // Quick remove: user draws over object
    GpuTexture quick_remove(IGpuContext& gpu, GpuTexture image,
                             const std::vector<Vec2>& brush_path,
                             float brush_radius);

    // Smart object detection: auto-detect removable objects
    struct DetectedObject {
        Rect bounding_box;
        std::string label;             // "person", "car", "text", etc.
        float confidence;
        GpuTexture mask;               // Per-pixel mask
    };
    std::vector<DetectedObject> detect_objects(IGpuContext& gpu, GpuTexture image);
};

} // namespace gp::ai
```

### 14.4 AI Super Resolution (Upscale)

```cpp
namespace gp::ai {

class SuperResolution {
public:
    enum class ScaleFactor { X2, X4 };
    enum class Model {
        RealESRGAN,           // General purpose (photos)
        RealESRGAN_Anime,     // Anime/illustration optimized
        SwinIR,               // Best quality, slower
    };

    // Upscale entire image
    GpuTexture upscale(IGpuContext& gpu, GpuTexture image,
                        ScaleFactor scale, Model model = Model::RealESRGAN);

    // Upscale with progress callback (for large images, tiled processing)
    TaskHandle upscale_async(IGpuContext& gpu, GpuTexture image,
                              ScaleFactor scale, Model model,
                              std::function<void(float)> progress);

    // Enhance faces specifically (GFPGAN integration)
    GpuTexture enhance_faces(IGpuContext& gpu, GpuTexture image,
                              float fidelity_weight = 0.5f);
};

} // namespace gp::ai
```

### 14.5 AI Style Transfer

```cpp
namespace gp::ai {

class StyleTransfer {
public:
    enum class BuiltInStyle {
        Mosaic, Starry_Night, The_Scream, Udnie, Candy,
        Rain_Princess, Feathers, La_Muse, Wave, Composition_VII,
        Cubism, Impressionism, Watercolor, Sketch, OilPainting,
        PopArt, Anime, Pixel, Cyberpunk, Vintage,
    };

    // Apply built-in style
    GpuTexture apply_style(IGpuContext& gpu, GpuTexture content,
                            BuiltInStyle style, float strength = 1.0f);

    // Apply custom style from reference image
    GpuTexture transfer_style(IGpuContext& gpu, GpuTexture content,
                               GpuTexture style_reference,
                               float strength = 1.0f,
                               bool preserve_color = false);

    // Apply style to specific region only
    GpuTexture apply_style_masked(IGpuContext& gpu, GpuTexture content,
                                   GpuTexture mask,
                                   BuiltInStyle style, float strength = 1.0f);
};

} // namespace gp::ai
```

### 14.6 AI Face Detection and Beautification

```cpp
namespace gp::ai {

class FaceProcessor {
public:
    struct FaceDetection {
        Rect bounding_box;
        float confidence;
        float age_estimate;
        std::string gender_estimate;

        // 68-point facial landmarks
        struct Landmarks {
            std::vector<Vec2> jaw;          // 17 points
            std::vector<Vec2> left_eyebrow; // 5 points
            std::vector<Vec2> right_eyebrow;// 5 points
            std::vector<Vec2> nose_bridge;  // 4 points
            std::vector<Vec2> nose_tip;     // 5 points
            std::vector<Vec2> left_eye;     // 6 points
            std::vector<Vec2> right_eye;    // 6 points
            std::vector<Vec2> outer_lip;    // 12 points
            std::vector<Vec2> inner_lip;    // 8 points
        } landmarks;
    };

    // Detect faces in image
    std::vector<FaceDetection> detect_faces(IGpuContext& gpu, GpuTexture image);

    // Face beautification
    struct BeautyParams {
        float skin_smooth;           // 0-100
        float blemish_removal;       // 0-100
        float eye_brighten;          // 0-100
        float teeth_whiten;          // 0-100
        float face_slim;             // 0-100
        float eye_enlarge;           // 0-100
        float nose_slim;             // 0-100
        float jaw_reshape;           // 0-100
        float lip_color;             // 0-100
        Color4f lip_tint;
        float skin_tone_even;        // 0-100
        float under_eye_fix;         // 0-100
    };

    GpuTexture beautify(IGpuContext& gpu, GpuTexture image,
                          const FaceDetection& face, const BeautyParams& params);

    // Face swap (between two faces in same or different images)
    GpuTexture face_swap(IGpuContext& gpu, GpuTexture target,
                          const FaceDetection& target_face,
                          GpuTexture source,
                          const FaceDetection& source_face);
};

} // namespace gp::ai
```

### 14.7 AI Text Recognition (OCR)

```cpp
namespace gp::ai {

class TextRecognition {
public:
    struct TextBlock {
        std::string text;
        Rect bounding_box;
        float confidence;
        std::string language;          // ISO 639-1 code
        float rotation_angle;
        std::vector<Vec2> quad;        // Four corner points
    };

    // Detect and recognize text in image
    std::vector<TextBlock> recognize(IGpuContext& gpu, GpuTexture image,
                                      const std::string& language_hint = "en");

    // Extract text for template: auto-create text layers from detected text
    std::vector<canvas::Layer> extract_text_layers(IGpuContext& gpu, GpuTexture image);
};

} // namespace gp::ai
```

### 14.8 AI Auto-Enhance

```cpp
namespace gp::ai {

class AutoEnhance {
public:
    struct EnhanceResult {
        image::ImageAdjustments adjustments;  // Computed optimal adjustments
        float quality_before;                  // 0-1 quality score
        float quality_after;
    };

    // Auto-enhance: compute optimal adjustments for an image
    EnhanceResult enhance(IGpuContext& gpu, GpuTexture image);

    // Scene-specific auto-enhance
    EnhanceResult enhance_portrait(IGpuContext& gpu, GpuTexture image);
    EnhanceResult enhance_landscape(IGpuContext& gpu, GpuTexture image);
    EnhanceResult enhance_food(IGpuContext& gpu, GpuTexture image);
    EnhanceResult enhance_night(IGpuContext& gpu, GpuTexture image);

    // Auto-crop: suggest optimal crop based on composition rules
    struct CropSuggestion {
        Rect crop_rect;
        float score;
        std::string rule;             // "rule_of_thirds", "golden_ratio", "center"
    };
    std::vector<CropSuggestion> suggest_crop(IGpuContext& gpu, GpuTexture image,
                                              float target_aspect = 0.0f);
};

} // namespace gp::ai
```

### 14.9 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 20 | Wk 37-38 | IE-166 to IE-176 | AI background removal, object removal, super resolution, style transfer |
| Sprint 21 | Wk 39-40 | IE-177 to IE-182 | AI face detection, beautification, auto-enhance, auto-crop |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-166 | As a developer, I want ML inference engine integration (shared) so that AI features use the same runtime as the video editor | - Platform-specific: CoreML (iOS/macOS), NNAPI/TFLite (Android), ONNX Runtime (Windows)<br/>- Shared model loading and inference<br/>- GPU acceleration where available | 5 | Sprint 20 | — |
| IE-167 | As a developer, I want a model download + cache system so that AI models are fetched and cached efficiently | - Download models on demand<br/>- Versioning and cache invalidation<br/>- Progress callback for large models | 5 | Sprint 20 | IE-166 |
| IE-168 | As a user, I want AI background removal (fast quality) so that I can quickly remove backgrounds | - remove_background() with ModelQuality::Fast (~50ms)<br/>- get_matte() for alpha only<br/>- refine_edges, edge_feather params | 5 | Sprint 20 | IE-166 |
| IE-169 | As a user, I want AI background removal (high quality / SAM) so that I can get best-quality masks | - ModelQuality::HighQuality (~500ms, Segment Anything-class)<br/>- hair_refinement, preserve_shadows<br/>- Best edge quality for hair/fur | 5 | Sprint 20 | IE-168 |
| IE-170 | As a user, I want AI interactive segmentation (point-based) so that I can refine selections with foreground/background points | - interactive_segment(foreground_points, background_points)<br/>- User taps to add positive/negative samples<br/>- Real-time mask update | 5 | Sprint 20 | IE-168 |
| IE-171 | As a user, I want AI replace background so that I can swap backgrounds seamlessly | - replace_background(image, new_background, params)<br/>- Composites new background with matte<br/>- Edge blending for natural result | 3 | Sprint 20 | IE-168 |
| IE-172 | As a user, I want AI object removal (PatchMatch) so that I can remove objects with traditional inpainting | - RemovalMethod::PatchMatch<br/>- remove(image, mask, params)<br/>- quick_remove() for brush-based removal<br/>- iterations, color_adaptation params | 5 | Sprint 20 | IE-166 |
| IE-173 | As a user, I want AI object removal (LaMa deep learning) so that I can remove objects with high quality | - RemovalMethod::LaMa (Large Mask Inpainting)<br/>- Better quality for large masks<br/>- Slower than PatchMatch | 5 | Sprint 20 | IE-172 |
| IE-174 | As a user, I want AI object detection (auto-detect removable) so that I can quickly find objects to remove | - detect_objects(image) → DetectedObject vector<br/>- bounding_box, label, confidence, mask<br/>- Labels: person, car, text, etc. | 5 | Sprint 20 | IE-172 |
| IE-175 | As a user, I want AI super resolution (RealESRGAN 2x/4x) so that I can upscale images | - SuperResolution::upscale() with ScaleFactor X2, X4<br/>- Model: RealESRGAN, RealESRGAN_Anime, SwinIR<br/>- upscale_async() with progress for large images | 5 | Sprint 20 | IE-166 |
| IE-176 | As a user, I want AI face enhancement (GFPGAN) so that I can improve face quality in upscaled images | - enhance_faces(image, fidelity_weight)<br/>- GFPGAN integration<br/>- Restores facial details in upscales | 5 | Sprint 20 | IE-175 |
| IE-177 | As a user, I want AI style transfer (20 built-in) so that I can apply artistic styles to images | - apply_style() with BuiltInStyle enum (20 styles)<br/>- Mosaic, Starry_Night, Udnie, Candy, etc.<br/>- strength parameter (0-1) | 5 | Sprint 20 | IE-166 |
| IE-178 | As a user, I want AI style transfer (custom reference) so that I can use my own style images | - transfer_style(content, style_reference, strength, preserve_color)<br/>- Neural style transfer from reference<br/>- apply_style_masked() for region-only | 5 | Sprint 20 | IE-177 |
| IE-179 | As a developer, I want AI face detection + landmarks so that face features can be located | - FaceProcessor::detect_faces() → FaceDetection vector<br/>- 68-point landmarks: jaw, eyebrows, nose, eyes, lips<br/>- bounding_box, confidence, age_estimate, gender_estimate | 5 | Sprint 21 | IE-166 |
| IE-180 | As a user, I want AI face beautification (skin/eyes/teeth/reshape) so that I can enhance portraits | - beautify(image, face, BeautyParams)<br/>- skin_smooth, blemish_removal, eye_brighten, teeth_whiten<br/>- face_slim, eye_enlarge, nose_slim, jaw_reshape, lip_color, etc. | 5 | Sprint 21 | IE-179 |
| IE-181 | As a user, I want AI auto-enhance (optimal adjustments) so that I can improve images with one click | - AutoEnhance::enhance(image) → EnhanceResult<br/>- Computes optimal ImageAdjustments<br/>- enhance_portrait(), enhance_landscape(), enhance_food(), enhance_night() | 5 | Sprint 21 | IE-166 |
| IE-182 | As a user, I want AI auto-crop (composition suggestions) so that I can get optimal crop suggestions | - suggest_crop(image, target_aspect) → CropSuggestion vector<br/>- Rules: rule_of_thirds, golden_ratio, center<br/>- score per suggestion | 5 | Sprint 21 | IE-166 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 7: Transform, AI & Advanced |
| **Sprint(s)** | IE-Sprint 19-21 (Weeks 37-42) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [15-transform-warp](15-transform-warp.md) |
| **Successor** | [17-export-pipeline](17-export-pipeline.md) |
| **Story Points Total** | 85 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-205 | As a developer, I want shared ML inference engine integration from VE so that AI features use the same runtime as the video editor | - Platform-specific: CoreML (iOS/macOS), NNAPI/TFLite (Android), ONNX Runtime (Windows)<br/>- Shared model loading and inference<br/>- GPU acceleration where available | 5 | P0 | — |
| IE-206 | As a user, I want AI background removal (3 quality tiers: fast/balanced/high) so that I can choose speed vs quality | - ModelQuality::Fast (~50ms), Balanced (~200ms), HighQuality (~500ms)<br/>- remove_background() and get_matte() APIs<br/>- Quality affects mask fidelity | 5 | P0 | IE-205 |
| IE-207 | As a user, I want AI background removal with edge refinement so that hair and fine edges are preserved | - refine_edges, edge_feather params<br/>- hair_refinement for hair/fur<br/>- preserve_shadows option | 5 | P0 | IE-206 |
| IE-208 | As a user, I want AI object removal (PatchMatch + LaMa inpainting) so that I can remove unwanted objects | - RemovalMethod::PatchMatch and LaMa<br/>- remove(image, mask, params) API<br/>- quick_remove() for brush-based removal | 5 | P0 | IE-205 |
| IE-209 | As a user, I want AI object removal with content-aware fill so that removed areas blend naturally | - Color adaptation to surrounding area<br/>- iterations param for quality<br/>- Seamless inpainting result | 5 | P0 | IE-208 |
| IE-210 | As a user, I want AI super resolution (RealESRGAN 2x upscale) so that I can enhance image resolution | - ScaleFactor::X2 with RealESRGAN model<br/>- upscale() and upscale_async() APIs<br/>- Progress callback for large images | 5 | P0 | IE-205 |
| IE-211 | As a user, I want AI super resolution (4x upscale) so that I can upscale images 4× | - ScaleFactor::X4 support<br/>- RealESRGAN, RealESRGAN_Anime, SwinIR models<br/>- Tiled processing for memory | 5 | P0 | IE-210 |
| IE-212 | As a user, I want AI style transfer (20 built-in styles) so that I can apply artistic styles | - apply_style() with BuiltInStyle enum (20 styles)<br/>- Mosaic, Starry_Night, Udnie, Candy, etc.<br/>- strength parameter (0-1) | 5 | P0 | IE-205 |
| IE-213 | As a user, I want AI style transfer with custom reference image so that I can use my own style | - transfer_style(content, style_reference, strength, preserve_color)<br/>- Neural style transfer from reference<br/>- apply_style_masked() for region-only | 5 | P0 | IE-212 |
| IE-214 | As a user, I want AI face detection (automatic) so that faces are located for processing | - FaceProcessor::detect_faces() → FaceDetection vector<br/>- 68-point landmarks, bounding_box, confidence<br/>- age_estimate, gender_estimate | 5 | P0 | IE-205 |
| IE-215 | As a user, I want AI face beautification: skin smoothing so that portraits look polished | - skin_smooth, blemish_removal in BeautyParams<br/>- 0-100 slider range<br/>- Natural-looking result | 3 | P0 | IE-214 |
| IE-216 | As a user, I want AI face beautification: eye brighten so that eyes stand out | - eye_brighten in BeautyParams<br/>- 0-100 slider range<br/>- Subtle enhancement | 3 | P0 | IE-214 |
| IE-217 | As a user, I want AI face beautification: teeth whiten so that smiles look better | - teeth_whiten in BeautyParams<br/>- 0-100 slider range<br/>- Natural tooth color | 3 | P0 | IE-214 |
| IE-218 | As a user, I want AI face beautification: face reshape (slider-based) so that I can adjust facial proportions | - face_slim, eye_enlarge, nose_slim, jaw_reshape sliders<br/>- -100 to 100 range per slider<br/>- Face-aware liquify integration | 5 | P0 | IE-214 |
| IE-219 | As a user, I want AI auto-enhance (optimal brightness/contrast/saturation) so that I can improve images with one click | - AutoEnhance::enhance(image) → EnhanceResult<br/>- Computes optimal ImageAdjustments<br/>- Scene-specific variants (portrait, landscape, etc.) | 5 | P0 | IE-205 |
| IE-220 | As a user, I want AI auto-crop (composition-aware rule-of-thirds) so that I can get optimal crop suggestions | - suggest_crop(image, target_aspect) → CropSuggestion vector<br/>- Rules: rule_of_thirds, golden_ratio, center<br/>- score per suggestion | 5 | P0 | IE-205 |
| IE-221 | As a user, I want AI auto-white-balance so that color casts are corrected automatically | - Auto white balance computation<br/>- Temperature and tint adjustment<br/>- One-click correction | 3 | P0 | IE-205 |
| IE-222 | As a user, I want AI sky replacement (segment sky + composite) so that I can change skies | - Sky segmentation model<br/>- Composite new sky with edge blending<br/>- Horizon-aware blending | 5 | P0 | IE-206 |
| IE-223 | As a user, I want AI colorize (grayscale → color) so that I can add color to B&W images | - Grayscale to color neural model<br/>- Natural colorization result<br/>- Optional manual color hints | 5 | P0 | IE-205 |
| IE-224 | As a developer, I want model download manager with progress so that AI models are fetched on demand | - Download models on demand<br/>- Progress callback for large models<br/>- Versioning and cache invalidation | 5 | P0 | IE-205 |
| IE-225 | As a developer, I want model cache management (LRU, max 200MB mobile) so that model storage is bounded | - LRU eviction policy<br/>- 200MB mobile, configurable desktop limit<br/>- Cache hit/miss metrics | 3 | P0 | IE-224 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
