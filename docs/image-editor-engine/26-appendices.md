
## IE-26. Appendices

### A. Third-Party Dependencies

| Library | Version | License | Purpose | Shared with VE |
|---|---|---|---|---|
| **libjpeg-turbo** | 3.0+ | BSD-3 / IJG | JPEG encode/decode (SIMD-accelerated) | No (IE-only) |
| **libpng** | 1.6+ | libpng License | PNG encode/decode | No (IE-only) |
| **libwebp** | 1.3+ | BSD-3 | WebP encode/decode | No (IE-only) |
| **LibRaw** | 0.21+ | LGPL-2.1 / CDDL | RAW image processing | No (IE-only) |
| **FreeType** | 2.13+ | FTL / GPL-2 | Font rasterization | Yes |
| **HarfBuzz** | 8.0+ | MIT | Text shaping (complex scripts) | Yes |
| **msgpack-cxx** | 6.0+ | BSL-1.0 | Binary serialization (.gpimg format) | Yes |
| **Lua** | 5.4+ | MIT | Expression engine (animated templates) | Yes |
| **Clipper2** | 1.3+ | BSL-1.0 | Polygon boolean operations | No (IE-only) |
| **nanosvg** | Latest | zlib | SVG parsing and import | No (IE-only) |
| **libharu** | 2.4+ | zlib | PDF generation | No (IE-only) |
| **lcms2** | 2.15+ | MIT | ICC color management | No (IE-only) |
| **lz4** | 1.9+ | BSD-2 | Fast compression (undo history) | No (IE-only) |
| **stb_image_resize2** | Latest | Public Domain | Image resampling | No (IE-only) |
| **SPIRV-Cross** | Latest | Apache-2.0 | Shader cross-compilation | Yes |
| **Google Test** | 1.14+ | BSD-3 | Unit testing | Yes |
| **Google Benchmark** | 1.8+ | Apache-2.0 | Performance benchmarks | Yes |

### B. ML Models

| Model | Purpose | Size | Format | Platforms |
|---|---|---|---|---|
| **U2-Net** (or SAM) | Background removal (fast) | ~5 MB | CoreML / TFLite / ONNX | All |
| **IS-Net** / **SAM-HQ** | Background removal (high quality) | ~45 MB | CoreML / TFLite / ONNX | All |
| **LaMa** | Object removal (inpainting) | ~60 MB | CoreML / TFLite / ONNX | All |
| **RealESRGAN** | Super resolution (upscale) | ~20 MB | CoreML / TFLite / ONNX | All |
| **GFPGAN** | Face enhancement | ~30 MB | CoreML / TFLite / ONNX | All |
| **BlazeFace** | Face detection | ~0.5 MB | CoreML / TFLite / ONNX | All |
| **Face Mesh (MediaPipe)** | 468-point face landmarks | ~2 MB | CoreML / TFLite / ONNX | All |
| **Style Transfer (Fast)** | 20 built-in styles | ~5 MB each | CoreML / TFLite / ONNX | All |
| **MIRNet / NAFNet** | Auto-enhance | ~15 MB | CoreML / TFLite / ONNX | All |

### C. Shader Source Organization

```
shaders/
├── compositing/
│   ├── blend_modes.comp              # All 30+ blend modes
│   ├── alpha_composite.comp          # Premultiply/unpremultiply + composite
│   ├── layer_mask_apply.comp         # Apply grayscale mask to layer
│   ├── clipping_mask.comp            # Clipping mask composite
│   └── group_composite.comp          # Group blending (pass-through/isolated)
├── layer_style/
│   ├── drop_shadow.comp
│   ├── inner_shadow.comp
│   ├── outer_glow.comp
│   ├── inner_glow.comp
│   ├── bevel_emboss.comp
│   ├── satin.comp
│   ├── color_overlay.comp
│   ├── gradient_overlay.comp
│   ├── pattern_overlay.comp
│   └── stroke.comp
├── color/                              # (SHARED with VE)
│   ├── brightness_contrast.comp
│   ├── levels.comp
│   ├── curves.comp
│   ├── hue_saturation.comp
│   ├── color_balance.comp
│   ├── selective_color.comp
│   ├── vibrance.comp
│   ├── exposure.comp
│   ├── white_balance.comp
│   ├── lut3d.comp
│   ├── color_wheels.comp
│   └── tone_mapping.comp
├── blur/                               # (SHARED with VE)
│   ├── gaussian_blur.comp
│   ├── box_blur.comp
│   ├── directional_blur.comp
│   ├── radial_blur.comp
│   ├── lens_blur.comp
│   └── bilateral_blur.comp
├── distort/                            # (SHARED with VE)
│   ├── transform.comp
│   ├── corner_pin.comp
│   ├── mesh_warp.comp
│   ├── bulge.comp
│   ├── twirl.comp
│   ├── ripple.comp
│   └── displacement_map.comp
├── image_processing/
│   ├── healing_poisson.comp            # Poisson image editing
│   ├── clone_stamp.comp               # Clone stamp compositing
│   ├── content_aware_fill.comp         # PatchMatch-based inpainting
│   ├── seam_carving.comp              # Energy map for content-aware scale
│   ├── histogram.comp                 # GPU histogram computation
│   ├── auto_levels.comp               # Auto-levels correction
│   └── raw_develop.comp               # RAW processing stages
├── brush/
│   ├── brush_stamp.comp               # Stamp a brush tip
│   ├── brush_accumulate.comp          # Accumulate strokes
│   ├── brush_dynamics.comp            # Pressure/tilt/velocity processing
│   └── smudge.comp                    # Smudge/blend brush
├── selection/
│   ├── marching_ants.frag             # Selection outline rendering
│   ├── selection_feather.comp         # Feather selection edges
│   ├── flood_fill.comp               # Magic wand flood fill
│   └── refine_edge.comp              # Edge refinement
├── vector/
│   ├── path_fill_sdf.comp            # Signed distance field path fill
│   ├── path_stroke.comp              # Path stroke rendering
│   ├── shape_rasterize.comp          # Shape to texture rasterization
│   └── gradient_fill.comp            # Gradient generation
├── transform/
│   ├── perspective_warp.comp          # Homography transform
│   ├── mesh_warp_bicubic.comp         # Bicubic mesh warp
│   ├── liquify.comp                   # Liquify displacement field
│   ├── content_aware_scale.comp       # Seam carving resize
│   └── lens_correction.comp           # Lens distortion correction
├── template/
│   ├── placeholder_frame.comp         # Placeholder shape rendering
│   ├── placeholder_mask.comp          # Frame shape masking
│   └── pattern_tile.comp             # Pattern tiling
├── utility/
│   ├── checkerboard.frag             # Transparency indicator
│   ├── grid_overlay.frag             # Grid rendering
│   ├── canvas_border.frag            # Canvas edge shadow
│   ├── soft_proof.comp               # CMYK soft proofing
│   ├── gamut_warning.comp            # Out-of-gamut highlight
│   ├── pixel_grid.frag               # Pixel grid at high zoom
│   └── color_convert.comp            # Color space conversion
└── generate/                           # (SHARED with VE)
    ├── solid_color.comp
    ├── gradient_ramp.comp
    ├── fractal_noise.comp
    ├── checkerboard_gen.comp
    └── grain.comp
```

### D. Canvas Size Presets Reference

| Category | Preset | Width | Height | DPI | Aspect |
|---|---|---|---|---|---|
| **Social Media** | Instagram Post | 1080 | 1080 | 72 | 1:1 |
| | Instagram Story | 1080 | 1920 | 72 | 9:16 |
| | Instagram Carousel | 1080 | 1350 | 72 | 4:5 |
| | Facebook Post | 1200 | 630 | 72 | ~1.91:1 |
| | Facebook Story | 1080 | 1920 | 72 | 9:16 |
| | Facebook Cover | 820 | 312 | 72 | ~2.63:1 |
| | Twitter/X Post | 1600 | 900 | 72 | 16:9 |
| | Twitter/X Header | 1500 | 500 | 72 | 3:1 |
| | LinkedIn Post | 1200 | 1200 | 72 | 1:1 |
| | LinkedIn Cover | 1584 | 396 | 72 | 4:1 |
| | Pinterest Pin | 1000 | 1500 | 72 | 2:3 |
| | TikTok Cover | 1080 | 1920 | 72 | 9:16 |
| | YouTube Thumbnail | 1280 | 720 | 72 | 16:9 |
| | YouTube Banner | 2560 | 1440 | 72 | 16:9 |
| | Snapchat Story | 1080 | 1920 | 72 | 9:16 |
| | WhatsApp Status | 1080 | 1920 | 72 | 9:16 |
| | Telegram Sticker | 512 | 512 | 72 | 1:1 |
| **Print** | A4 | 2480 | 3508 | 300 | ~1:1.41 |
| | A5 | 1748 | 2480 | 300 | ~1:1.41 |
| | A3 | 3508 | 4961 | 300 | ~1:1.41 |
| | US Letter | 2550 | 3300 | 300 | ~1:1.29 |
| | US Legal | 2550 | 4200 | 300 | ~1:1.65 |
| | Business Card | 1050 | 600 | 300 | ~1.75:1 |
| | Poster 18x24 | 5400 | 7200 | 300 | 3:4 |
| | Poster 24x36 | 7200 | 10800 | 300 | 2:3 |
| | Flyer (A5) | 1748 | 2480 | 300 | ~1:1.41 |
| | Invitation (5x7) | 1500 | 2100 | 300 | 5:7 |
| **Marketing** | Email Header | 600 | 200 | 72 | 3:1 |
| | Web Banner | 1200 | 300 | 72 | 4:1 |
| | Blog Header | 1200 | 628 | 72 | ~1.91:1 |
| | Presentation 16:9 | 1920 | 1080 | 72 | 16:9 |
| | Presentation 4:3 | 1024 | 768 | 72 | 4:3 |
| | Logo (square) | 1000 | 1000 | 300 | 1:1 |
| | Favicon | 512 | 512 | 72 | 1:1 |

### E. Keyboard Shortcut Mapping (for Desktop)

| Action | macOS | Windows | Category |
|---|---|---|---|
| New Canvas | Cmd+N | Ctrl+N | File |
| Open | Cmd+O | Ctrl+O | File |
| Save | Cmd+S | Ctrl+S | File |
| Export | Cmd+Shift+E | Ctrl+Shift+E | File |
| Undo | Cmd+Z | Ctrl+Z | Edit |
| Redo | Cmd+Shift+Z | Ctrl+Shift+Z | Edit |
| Cut | Cmd+X | Ctrl+X | Edit |
| Copy | Cmd+C | Ctrl+C | Edit |
| Paste | Cmd+V | Ctrl+V | Edit |
| Select All | Cmd+A | Ctrl+A | Selection |
| Deselect | Cmd+D | Ctrl+D | Selection |
| Invert Selection | Cmd+Shift+I | Ctrl+Shift+I | Selection |
| Free Transform | Cmd+T | Ctrl+T | Transform |
| Zoom In | Cmd+= | Ctrl+= | View |
| Zoom Out | Cmd+- | Ctrl+- | View |
| Fit to Screen | Cmd+0 | Ctrl+0 | View |
| Actual Pixels | Cmd+1 | Ctrl+1 | View |
| New Layer | Cmd+Shift+N | Ctrl+Shift+N | Layer |
| Duplicate Layer | Cmd+J | Ctrl+J | Layer |
| Merge Down | Cmd+E | Ctrl+E | Layer |
| Flatten | Cmd+Shift+E | Ctrl+Shift+E | Layer |
| Group | Cmd+G | Ctrl+G | Layer |
| Ungroup | Cmd+Shift+G | Ctrl+Shift+G | Layer |

### F. Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 1 | Wk 1-2 | IE-327 to IE-330 | Third-party audit, dependencies, shaders, ML pipeline |
| Ongoing | — | IE-331 to IE-334 | Presets, shortcuts, accessibility |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-327 | As a developer, I want a third-party license audit so that all dependencies are compliant | - Audit of all dependencies (Appendix A)<br/>- License compatibility documented<br/>- Attribution and notices prepared | 3 | Sprint 1 | — |
| IE-328 | As a developer, I want dependency version pinning so that builds are reproducible | - All dependencies have pinned versions in CMake/vcpkg/conan<br/>- Version upgrade process documented<br/>- Security update process defined | 3 | Sprint 1 | — |
| IE-329 | As a developer, I want shader source organization setup so that shaders are maintainable | - Directory structure per Appendix C (compositing, layer_style, color, blur, etc.)<br/>- Naming convention documented<br/>- Build integration for shader compilation | 3 | Sprint 1 | IE-277 |
| IE-330 | As a developer, I want an ML model format conversion pipeline (CoreML/TFLite/ONNX) so that models run on all platforms | - Pipeline to convert source models to CoreML, TFLite, ONNX<br/>- Models from Appendix B supported<br/>- Conversion documented and repeatable | 5 | Sprint 1 | — |
| IE-331 | As a developer, I want ML model download CDN setup so that models are fetched efficiently | - CDN or asset delivery for ML models<br/>- Lazy download with caching<br/>- Fallback for offline | 3 | Ongoing | IE-330 |
| IE-332 | As a developer, I want canvas preset validation (all 50+ presets render correctly) so that presets are reliable | - All presets from Appendix D validated<br/>- Each preset creates correct canvas size/DPI<br/>- Render test for each preset | 5 | Ongoing | — |
| IE-333 | As a developer, I want keyboard shortcut implementation (desktop) so that power users can work efficiently | - All shortcuts from Appendix E implemented<br/>- macOS (Cmd) and Windows (Ctrl) variants<br/>- Shortcuts configurable or documented | 5 | Ongoing | — |
| IE-334 | As a developer, I want an accessibility audit (screen reader, high contrast) so that the engine supports accessibility | - Screen reader compatibility for UI elements<br/>- High contrast mode support<br/>- Audit report and remediation plan | 5 | Ongoing | — |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Reference |
| **Sprint(s)** | Reference |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [25-development-roadmap](25-development-roadmap.md) |
| **Successor** | — |
| **Story Points Total** | 25 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-376 | As a developer, I want third-party dependency license audit so that all dependencies are compliant | - Audit of all dependencies (Appendix A)<br/>- License compatibility documented<br/>- Attribution and notices prepared | 5 | P0 | — |
| IE-377 | As a developer, I want LibRaw/Clipper2/nanosvg license compliance so that IE-specific deps are compliant | - LibRaw (LGPL/CDDL), Clipper2 (BSL), nanosvg (zlib)<br/>- Compliance verified<br/>- Notices in distribution | 5 | P0 | IE-376 |
| IE-378 | As a developer, I want shader source organization so that shaders are maintainable | - Directory structure per Appendix C<br/>- Naming convention documented<br/>- Build integration for compilation | 5 | P0 | — |
| IE-379 | As a developer, I want ML model inventory and size tracking so that model footprint is known | - Inventory of all models (Appendix B)<br/>- Size per model, total footprint<br/>- Platform-specific sizes | 5 | P0 | — |
| IE-380 | As a developer, I want filter preset catalog documentation so that presets are documented | - Catalog of all built-in filter presets<br/>- Parameter ranges and defaults<br/>- Category organization | 5 | P0 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
