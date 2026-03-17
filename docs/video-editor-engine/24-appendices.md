## VE-Appendix A: Third-Party Dependency Matrix

| Library | Version | License | Purpose | Platforms |
|---|---|---|---|---|
| FFmpeg | 7.0+ | LGPL 2.1 | Codec decode/encode, format I/O | All |
| FreeType | 2.13+ | FTL (BSD-like) | Font rasterization | All |
| HarfBuzz | 8.0+ | MIT | Text shaping and layout | All |
| Lua | 5.4 | MIT | Expression engine | All |
| msgpack-c | 6.0+ | BSL 1.0 | Project serialization | All |
| SPIRV-Cross | Latest | Apache 2.0 | Shader cross-compilation | Build-time only |
| glslc (shaderc) | Latest | Apache 2.0 | GLSL → SPIR-V compilation | Build-time only |
| Google Test | 1.14+ | BSD 3 | Unit testing | Build-time only |
| Google Benchmark | 1.8+ | Apache 2.0 | Performance benchmarking | Build-time only |
| stb_image | Latest | Public domain | Lightweight image decode | All |
| Vulkan SDK | 1.3+ | Apache 2.0 | Vulkan headers and validation | Android, Windows |
| Core ML | System | Apple | ML inference (iOS/macOS) | iOS, macOS |
| TensorFlow Lite | 2.16+ | Apache 2.0 | ML inference (Android) | Android |
| ONNX Runtime | 1.18+ | MIT | ML inference (Windows) | Windows |

## VE-Appendix B: Shader Source Organization

```
shaders/
├── compositing/
│   ├── alpha_blend.comp.glsl
│   ├── blend_modes.comp.glsl           # All 30+ modes in one compute shader
│   ├── premultiply.comp.glsl
│   └── track_matte.comp.glsl
├── color/
│   ├── brightness_contrast.comp.glsl
│   ├── hue_saturation.comp.glsl
│   ├── levels.comp.glsl
│   ├── curves.comp.glsl
│   ├── channel_mixer.comp.glsl
│   ├── color_balance.comp.glsl
│   ├── color_wheels.comp.glsl
│   ├── lut3d.comp.glsl
│   ├── exposure.comp.glsl
│   ├── white_balance.comp.glsl
│   ├── vibrance.comp.glsl
│   ├── selective_color.comp.glsl
│   ├── posterize.comp.glsl
│   ├── threshold.comp.glsl
│   ├── invert.comp.glsl
│   ├── tint.comp.glsl
│   └── tone_mapping.comp.glsl
├── blur/
│   ├── gaussian_h.comp.glsl
│   ├── gaussian_v.comp.glsl
│   ├── box_blur.comp.glsl
│   ├── directional_blur.comp.glsl
│   ├── radial_blur.comp.glsl
│   ├── zoom_blur.comp.glsl
│   ├── lens_blur.comp.glsl
│   └── bilateral_blur.comp.glsl
├── distort/
│   ├── transform.vert.glsl
│   ├── corner_pin.vert.glsl
│   ├── mesh_warp.vert.glsl
│   ├── bulge.comp.glsl
│   ├── twirl.comp.glsl
│   ├── ripple.comp.glsl
│   ├── wave_warp.comp.glsl
│   ├── displacement.comp.glsl
│   ├── turbulent_displace.comp.glsl
│   └── lens_correction.comp.glsl
├── keying/
│   ├── chroma_key.comp.glsl
│   ├── luma_key.comp.glsl
│   ├── difference_key.comp.glsl
│   ├── spill_suppress.comp.glsl
│   ├── matte_choker.comp.glsl
│   └── refine_edge.comp.glsl
├── generate/
│   ├── solid.comp.glsl
│   ├── gradient.comp.glsl
│   ├── noise.comp.glsl
│   ├── fractal_noise.comp.glsl
│   ├── grid.comp.glsl
│   ├── checkerboard.comp.glsl
│   ├── film_grain.comp.glsl
│   └── vignette.comp.glsl
├── stylize/
│   ├── glow.comp.glsl
│   ├── drop_shadow.comp.glsl
│   ├── emboss.comp.glsl
│   ├── find_edges.comp.glsl
│   ├── mosaic.comp.glsl
│   └── sharpen.comp.glsl
├── transitions/
│   ├── cross_dissolve.comp.glsl
│   ├── linear_wipe.comp.glsl
│   ├── radial_wipe.comp.glsl
│   ├── iris_wipe.comp.glsl
│   ├── push.comp.glsl
│   ├── slide.comp.glsl
│   ├── zoom.comp.glsl
│   ├── glitch.comp.glsl
│   ├── page_curl.comp.glsl
│   └── luma_fade.comp.glsl
├── utility/
│   ├── format_convert.comp.glsl        # NV12→RGBA, YUV→RGB
│   ├── resize.comp.glsl               # Bilinear / Lanczos
│   ├── gamma.comp.glsl
│   └── motion_blur.comp.glsl
└── text/
    ├── text_sdf.vert.glsl             # SDF text rendering
    ├── text_sdf.frag.glsl
    ├── text_outline.frag.glsl
    └── text_shadow.frag.glsl
```

---

*This video editor engine plan is the canonical reference for all Gopost VE engineering decisions. It extends and integrates with the main Gopost Application Architecture (Sections 1–14 above). All engine development must follow this plan; deviations require Tech Lead approval.*

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Reference Document |
| **Sprint(s)** | Reference |
| **Team** | Tech Lead |
| **Predecessor** | [23-development-roadmap](23-development-roadmap.md) |
| **Successor** | — |
| **Story Points Total** | 18 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-371 | As a Tech Lead, I want third-party dependency license audit so that licensing is compliant | - Audit all dependencies (FFmpeg, FreeType, Lua, etc.)<br/>- License compatibility matrix<br/>- Document in repo | 5 | P0 | — |
| VE-372 | As a Tech Lead, I want FFmpeg LGPL compliance documentation so that LGPL obligations are met | - Document FFmpeg LGPL 2.1 obligations<br/>- Dynamic linking strategy<br/>- Source availability, attribution | 5 | P0 | VE-371 |
| VE-373 | As a C++ engine developer, I want shader source organization and naming convention so that shaders are maintainable | - Directory structure (compositing/, color/, blur/, etc.)<br/>- Naming: effect_name.stage.glsl<br/>- Document in appendices | 3 | P1 | — |
| VE-374 | As a Tech Lead, I want dependency version pinning and update policy so that builds are reproducible | - Pin versions in CMake or package manifest<br/>- Update policy (quarterly, security patches)<br/>- Changelog for upgrades | 3 | P1 | — |
| VE-375 | As a C++ engine developer, I want build artifact naming convention so that outputs are consistent | - libgopost_ve.so / gopost_ve.framework / gopost_ve.dll<br/>- Version in filename or metadata<br/>- Document in appendices | 2 | P2 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
