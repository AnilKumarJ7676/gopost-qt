
## VE-14. Color Science and Grading Pipeline

### 14.1 Color Pipeline Architecture

```
Input Media
    │
    ▼
┌─────────────────────────┐
│ Decode to Linear Light  │  (Inverse OETF: sRGB, Rec.709, PQ, HLG)
│ (Scene-referred)        │
└────────┬────────────────┘
         │
         ▼
┌─────────────────────────┐
│ Input Color Space        │  (gamut conversion to working space)
│ Transform (via OCIO)     │  Source → ACEScg or Rec.2020 linear
└────────┬────────────────┘
         │
         ▼
┌─────────────────────────┐
│ Working Color Space      │  All processing here (linear light, wide gamut)
│ (ACEScg or Rec.2020)    │
│                          │
│  ┌─ Primary Grading ──┐ │  Lift/Gamma/Gain, Offset, CDL
│  ├─ Log Grading ──────┤ │  Log wheels (shadows/mids/highlights)
│  ├─ Curves ───────────┤ │  Per-channel bezier curves
│  ├─ Hue vs Hue ───────┤ │  Selective hue rotation
│  ├─ Hue vs Sat ───────┤ │  Selective saturation
│  ├─ Hue vs Lum ───────┤ │  Selective luminance
│  ├─ Sat vs Sat ───────┤ │
│  ├─ Lum vs Sat ───────┤ │
│  ├─ 3D LUT ──────────┤ │  .cube / .3dl / .mga
│  ├─ Color Match ──────┤ │  Match to reference frame
│  └─ Film Emulation ──┘ │  ACES Output Transforms / custom
│                          │
└────────┬────────────────┘
         │
         ▼
┌─────────────────────────┐
│ Output Color Space       │  Working → Output (Rec.709, sRGB, P3, etc.)
│ Transform               │
└────────┬────────────────┘
         │
         ▼
┌─────────────────────────┐
│ Display Transform        │  Apply OETF (gamma, PQ, HLG)
│ (OETF + tone mapping)   │  Tone map if HDR→SDR
└────────┬────────────────┘
         │
         ▼
    Output Frame
```

### 14.2 HDR Support

| Aspect | Implementation |
|---|---|
| **Working precision** | 16-bit float (RGBA16F) throughout GPU pipeline |
| **HDR input** | Decode HLG and PQ (HDR10, Dolby Vision Profile 5) via FFmpeg |
| **HDR preview** | EDR on Apple (CAMetalLayer.wantsExtendedDynamicRangeContent), HDR10 on Windows/Android |
| **Tone mapping** | ACES, Reinhard, Filmic, Hable, AgX |
| **HDR export** | H.265 Main 10 profile with HDR10 / HLG metadata |
| **SDR fallback** | Automatic tone mapping for SDR displays |
| **Color volume mapping** | BT.2446 for gamut/tone mapping between HDR standards |

### 14.3 LUT Engine

```cpp
namespace gp::color {

class LUT3D {
public:
    static LUT3D load_cube(const std::string& path);       // Adobe .cube
    static LUT3D load_3dl(const std::string& path);        // Lustre .3dl
    static LUT3D load_mga(const std::string& path);        // Pandora .mga
    static LUT3D from_identity(int size = 33);              // Generate identity LUT

    int size() const;   // Typically 33 or 65
    Color4f lookup(Color4f input) const;   // Trilinear interpolation

    // Upload to GPU as 3D texture for shader-based application
    GpuTexture upload(IGpuContext& gpu) const;

    // Combine two LUTs
    static LUT3D combine(const LUT3D& first, const LUT3D& second);

private:
    int size_;
    std::vector<Color4f> data_;   // size^3 entries
};

} // namespace gp::color
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 3: Effects & Color |
| **Sprint(s)** | VE-Sprint 8-9 (Weeks 15-18) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [13-ai-features](13-ai-features.md) |
| **Successor** | [15-media-io-codec](15-media-io-codec.md) |
| **Story Points Total** | 68 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-226 | As a C++ engine developer, I want linear light decode (inverse OETF: sRGB, Rec.709, PQ, HLG) so that scene-referred values are correct | - Inverse OETF for sRGB, Rec.709, PQ, HLG<br/>- Decode to linear light before grading<br/>- Unit tests for each transfer function | 5 | P0 | — |
| VE-227 | As a C++ engine developer, I want input color space transform (source→working via OCIO-style) so that different sources are normalized | - OCIO-style config support<br/>- Source gamut → working space conversion<br/>- ACEScg or Rec.2020 linear output | 5 | P0 | VE-226 |
| VE-228 | As a C++ engine developer, I want working color space (ACEScg or Rec.2020 linear) so that all grading happens in a consistent space | - ACEScg and Rec.2020 linear options<br/>- 16-bit float precision<br/>- Configurable per project | 3 | P0 | VE-227 |
| VE-229 | As a C++ engine developer, I want primary grading (Lift/Gamma/Gain, Offset, CDL) so that basic color correction is available | - Lift/Gamma/Gain controls<br/>- Offset and CDL support<br/>- GPU shader implementation | 5 | P0 | VE-228 |
| VE-230 | As a C++ engine developer, I want log grading (shadows/mids/highlights wheels) so that cinematic grading is possible | - Log wheels for shadows, mids, highlights<br/>- Per-channel and master controls<br/>- Smooth transitions | 5 | P1 | VE-229 |
| VE-231 | As a C++ engine developer, I want curves (per-channel bezier) so that precise tonal control is available | - Per-channel RGB curves<br/>- Bezier interpolation<br/>- Luma curve option | 5 | P0 | VE-228 |
| VE-232 | As a C++ engine developer, I want Hue-vs-Hue selective rotation so that color shifts can target specific hues | - Hue qualifier for selection<br/>- Hue rotation in selected range<br/>- Smooth falloff | 3 | P1 | VE-229 |
| VE-233 | As a C++ engine developer, I want Hue-vs-Sat + Hue-vs-Lum qualifiers so that secondary color correction works | - Hue vs Saturation curve<br/>- Hue vs Luminance curve<br/>- Qualifier-based selection | 5 | P1 | VE-232 |
| VE-234 | As a C++ engine developer, I want Sat-vs-Sat + Lum-vs-Sat qualifiers so that saturation and luminance can be adjusted selectively | - Sat vs Sat curve<br/>- Lum vs Sat curve<br/>- Combined with hue qualifiers | 3 | P1 | VE-233 |
| VE-235 | As a C++ engine developer, I want a 3D LUT engine (.cube/.3dl/.mga loader, trilinear GPU lookup) so that creative LUTs can be applied | - Load .cube, .3dl, .mga formats<br/>- Trilinear interpolation on GPU<br/>- 3D texture upload | 5 | P0 | VE-228 |
| VE-236 | As a C++ engine developer, I want LUT combination (chain two LUTs) so that multiple LUTs can be stacked | - combine() merges two LUTs<br/>- Order preserved (first then second)<br/>- No precision loss | 3 | P2 | VE-235 |
| VE-237 | As a C++ engine developer, I want output color space transform so that working space is converted for display/export | - Working → output gamut conversion<br/>- Rec.709, sRGB, P3 support<br/>- Configurable output | 3 | P0 | VE-227 |
| VE-238 | As a C++ engine developer, I want display transform (OETF + tone mapping) so that HDR content displays correctly on SDR | - OETF application (gamma, PQ, HLG)<br/>- Tone mapping for HDR→SDR<br/>- EDR preview on Apple | 5 | P0 | VE-226 |
| VE-239 | As a C++ engine developer, I want HDR pipeline (RGBA16F, EDR preview, HDR10 export) so that HDR workflows are supported | - RGBA16F throughout pipeline<br/>- EDR on Apple, HDR10 on Windows<br/>- HDR metadata passthrough | 8 | P1 | VE-238 |
| VE-240 | As a C++ engine developer, I want tone mapping algorithms (ACES, Reinhard, Filmic, Hable, AgX) so that HDR→SDR looks correct | - ACES, Reinhard, Filmic, Hable, AgX<br/>- User-selectable algorithm<br/>- Consistent results across platforms | 5 | P1 | VE-238 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
