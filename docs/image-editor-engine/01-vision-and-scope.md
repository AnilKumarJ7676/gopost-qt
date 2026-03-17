
## IE-1. Vision and Scope

### 1.1 Mission

Build a single shared C/C++ image editor engine (`libgopost_ie`) that provides Photoshop-level compositing and retouching, Canva/CapCut-level template creation, PicsArt/Snapseed-level mobile editing, and After Effects-level layer compositing for still images — deployed as a shared library across iOS, macOS, Android, and Windows via Flutter FFI.

The image editor engine is purpose-built for **template creation**: pre-designed, customizable layouts with replaceable text, images, shapes, and effects that end users personalize and export for social media, print, and marketing.

### 1.2 Feature Parity Matrix

| Feature Domain | Photoshop | Canva | PicsArt | Snapseed | CapCut (Image) | After Effects (Still) | Gopost IE Target |
|---|---|---|---|---|---|---|---|
| **Layer-based compositing** | Yes | Partial | Yes | — | Partial | Yes | Yes |
| **30+ blend modes** | Yes | Partial | Partial | — | Partial | Yes | Yes |
| **Non-destructive adjustment layers** | Yes | — | — | Yes (stacks) | — | Yes | Yes |
| **Smart objects / placeholders** | Yes | Yes (frames) | — | — | — | Yes (pre-comps) | Yes |
| **Layer groups / folders** | Yes | Yes | — | — | — | Yes | Yes |
| **Clipping masks** | Yes | — | — | — | — | Yes | Yes |
| **Layer masks (brush-painted)** | Yes | — | Partial | Partial | — | Yes | Yes |
| **Vector shape layers** | Yes | Yes | Partial | — | Partial | Yes | Yes |
| **Pen tool (bezier paths)** | Yes | — | — | — | — | Yes | Yes |
| **Boolean operations (shapes)** | Yes | — | — | — | — | Yes | Yes |
| **Rich text with styles** | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| **Text on path** | Yes | Partial | — | — | — | Yes | Yes |
| **Text effects (shadow, outline, glow, 3D)** | Yes | Partial | Partial | — | Partial | Yes | Yes |
| **100+ photo filters/effects** | Yes (via plugins) | Yes | Yes | Yes | Yes | Yes (built-in) | Yes |
| **Color grading (curves, levels, HSL)** | Yes | Partial | Partial | Yes | Partial | Yes | Yes |
| **LUT support (3D LUT)** | Yes | — | — | — | Yes | Yes | Yes |
| **HDR imaging pipeline** | Yes | — | — | Yes (HDR Scape) | — | Partial | Yes |
| **RAW image processing** | Yes (ACR) | — | — | — | — | — | Yes |
| **Healing / clone / patch** | Yes | — | Partial | Yes | — | — | Yes |
| **Content-aware fill** | Yes | — | — | — | — | — | Yes (AI) |
| **Liquify / face reshape** | Yes | — | Yes | Partial | Yes | — | Yes |
| **Perspective / mesh warp** | Yes | — | — | Yes (perspective) | — | Yes | Yes |
| **Brush engine (painting)** | Yes | — | Yes | Partial | — | Yes (paint) | Yes |
| **AI background removal** | Yes | Yes | Yes | — | Yes | — | Yes |
| **AI object removal** | Yes (gen fill) | Yes | Yes | Yes (healing) | — | — | Yes |
| **AI style transfer** | — | — | Yes | — | Yes | — | Yes |
| **AI super-resolution (upscale)** | Yes (Super Res) | — | — | — | — | — | Yes |
| **AI face detection / beautify** | — | — | Yes | Yes | Yes | — | Yes |
| **Template system (presets)** | — | Yes | Yes | — | Yes | — | Yes |
| **Sticker / element library** | — | Yes | Yes | — | Yes | — | Yes |
| **Collage layouts** | — | Yes | Yes | — | — | — | Yes |
| **Social media format presets** | — | Yes | — | — | Yes | — | Yes |
| **Batch export (multi-format)** | Yes | Yes (brand kit) | — | — | — | Yes | Yes |
| **Print-ready output (CMYK, 300 DPI)** | Yes | Yes | — | — | — | — | Yes |
| **SVG import / export** | Yes | Yes | — | — | — | — | Yes |
| **PSD import (partial)** | Yes | — | — | — | — | — | Phase 2 |
| **GPU-accelerated rendering** | Yes | Partial | Partial | Partial | Yes | Yes | Yes |
| **Tile-based rendering (large images)** | Yes | — | — | — | — | — | Yes |
| **16-bit / 32-bit float pipeline** | Yes | — | — | — | — | Yes | Yes |

### 1.3 Platform Targets

| Platform | GPU API (Primary) | GPU API (Fallback) | Image I/O | Min OS |
|---|---|---|---|---|
| iOS | Metal 3 | Metal 2 | ImageIO + libraw + libpng + libjpeg-turbo | iOS 15.0+ |
| macOS | Metal 3 | Metal 2 | ImageIO + libraw + libpng + libjpeg-turbo | macOS 12.0+ |
| Android | Vulkan 1.1 | OpenGL ES 3.2 | BitmapFactory + libraw + libpng + libjpeg-turbo | API 26+ |
| Windows | Vulkan 1.2 | DirectX 12 / OpenGL 4.6 | WIC + libraw + libpng + libjpeg-turbo | Windows 10+ |

### 1.4 Non-Functional Requirements

| Requirement | Target |
|---|---|
| Canvas rendering (preview) | 60 fps for ≤ 50 layers on mid-range device |
| Canvas rendering (high-res) | < 3 seconds for 4K (3840x2160) full-quality composite |
| Filter application | Real-time preview for ≤ 5 stacked effects |
| Cold engine init | < 200 ms |
| Image load (12 MP JPEG) | < 150 ms to decoded + displayed |
| Image load (50 MP RAW) | < 800 ms to preview-quality decoded |
| Export (4K JPEG, quality 95) | < 500 ms |
| Export (4K PNG) | < 1.5 seconds |
| Export (8K print TIFF) | < 4 seconds |
| Memory ceiling (mobile) | 384 MB engine total |
| Memory ceiling (desktop) | 1.5 GB engine total |
| Maximum canvas size (mobile) | 8192 x 8192 px |
| Maximum canvas size (desktop) | 16384 x 16384 px (limited by GPU max texture) |
| Maximum layers | 200 |
| Maximum effects per layer | 20 |
| Undo/redo depth | 200 operations minimum |
| Auto-save interval | Every 20 seconds during active editing |
| Crash rate | < 0.1% of editing sessions |
| Template load (encrypted .gpit) | < 400 ms to interactive preview |

### 1.5 Shared Modules with Video Editor Engine

The image editor engine (`libgopost_ie`) shares the following modules with the video editor engine (`libgopost_ve`) to maximize code reuse:

| Shared Module | Namespace | Usage in IE |
|---|---|---|
| core/ | `gp::core` | Memory allocators, task system, event system, logging |
| rendering/ | `gp::rendering` | GPU abstraction (IGpuContext), render graph, shader management |
| effects/ | `gp::effects` | Effect graph, built-in effects (color, blur, distort, etc.) |
| animation/ | `gp::animation` | Keyframe engine (for animated templates) |
| text/ | `gp::text` | FreeType/HarfBuzz text engine, font management |
| color/ | `gp::color` | Color science, grading, LUT engine, HDR pipeline |
| masking/ | `gp::masking` | Bezier masks, feathering, mask operations |
| ai/ | `gp::ai` | ML inference, background removal, segmentation |
| platform/ | `gp::platform` | Platform abstraction, GPU detection, thermal monitoring |
| cache/ | `gp::cache` | Texture cache, thumbnail cache |

| IE-Only Module | Namespace | Description |
|---|---|---|
| canvas/ | `gp::canvas` | Canvas data model, layer system, smart objects |
| image/ | `gp::image` | RAW processing, retouching, healing, HDR merge |
| vector/ | `gp::vector` | Vector shapes, pen tool, boolean ops, SVG I/O |
| brush/ | `gp::brush` | Brush engine, painting, pressure simulation |
| selection/ | `gp::selection` | Selection tools, marching ants, refine edge |
| transform/ | `gp::transform` | Perspective, mesh warp, liquify, content-aware |
| templates/ | `gp::templates` | Template model, placeholders, element library |
| export/ | `gp::xport` | Multi-format export, batch processing, print profiles |
| project/ | `gp::project_ie` | .gpimg format, undo/redo, auto-save |
| plugin/ | `gp::plugin_ie` | Plugin SDK, custom filters, extensions |

### 1.6 Sprint Planning

#### Sprint Overview (24 Sprints, 48 Weeks)

| Phase | Sprints | Weeks | Focus |
|---|---|---|---|
| **Phase 1: Foundation & Core** | 1–3 | Wk 1–5 | Shared core integration, build system, GPU backends, canvas renderer, FFI |
| **Phase 2: Canvas & Layers** | 4–6 | Wk 6–10 | Image/solid/gradient layers, blend modes, compositor, masks, layer styles |
| **Phase 3: Effects Pipeline** | 7–9 | Wk 11–16 | Effect graph, color/blur effects, adjustment layers, filter presets, LUT |
| **Phase 4: Text & Shapes** | 10–12 | Wk 17–22 | Text engine, shapes, pen tool, boolean ops, SVG, font management |
| **Phase 5: Templates** | 13–15 | Wk 23–28 | Template model, placeholders, element library, collage, social presets |
| **Phase 6: Selection & Tools** | 16–18 | Wk 29–34 | Selection tools, brush engine, retouching, mask painting |
| **Phase 7: Transform & AI** | 19–21 | Wk 35–40 | Mesh warp, liquify, AI removal/upscale/style transfer, RAW, HDR |
| **Phase 8: Export & Polish** | 22–24 | Wk 41–48 | Export pipeline, batch, print, PSD, optimization, testing, RC |

#### Epic Definitions (One per Phase)

| Epic | Description |
|---|---|
| **Epic 1: Foundation** | Shared core, build system, GPU backends (Metal/Vulkan/GLES), tile grid, FFI scaffolding |
| **Epic 2: Canvas & Layers** | Canvas model, layer types, blend modes, compositor, masks, layer styles |
| **Epic 3: Effects** | Effect graph, color/blur effects, adjustment layers, filter presets, LUT engine |
| **Epic 4: Text & Shapes** | Text engine, vector shapes, pen tool, boolean ops, SVG I/O |
| **Epic 5: Templates** | Template model, placeholders, element library, collage layouts |
| **Epic 6: Tools** | Selection, brush engine, retouching tools |
| **Epic 7: AI & Transform** | Mesh warp, liquify, AI features, RAW, HDR |
| **Epic 8: Ship** | Export, optimization, plugin system, testing, release |

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 1 | Wk 1–2 | IE-001 to IE-005 | Project setup, platform validation, NFR benchmarking, shared module verification |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-001 | As a developer, I want a libgopost_ie project structure and build configuration so that I can compile the engine on all target platforms | - CMakeLists.txt creates libgopost_ie target<br/>- Build succeeds on iOS, macOS, Android, Windows<br/>- Shared core/rendering/effects modules are referenced<br/>- Output produces libgopost_ie.a/.so/.dll | 5 | Sprint 1 | None |
| IE-002 | As a developer, I want platform target validation so that GPU APIs and image I/O work on each platform | - Metal 3/2 detected on iOS 15+ and macOS 12+<br/>- Vulkan 1.1/GLES 3.2 detected on Android API 26+<br/>- Vulkan 1.2 detected on Windows 10+<br/>- ImageIO/libraw/libpng/libjpeg-turbo available per platform | 3 | Sprint 1 | IE-001 |
| IE-003 | As a developer, I want NFR benchmarking infrastructure so that we can validate performance targets | - Benchmarks for canvas render (60 fps target), cold init (<200 ms), image load (<150 ms JPEG)<br/>- Memory ceiling tests (384 MB mobile, 1.5 GB desktop)<br/>- CI runs benchmarks and reports regressions | 5 | Sprint 1 | IE-001 |
| IE-004 | As a developer, I want shared module verification so that core, rendering, and effects integrate correctly | - libgopost_ie links against shared core, rendering, effects<br/>- AllocatorSystem, TaskGraph, SPSCRingBuffer usable from IE<br/>- IGpuContext and RenderGraph accessible<br/>- Effect graph types compile and link | 3 | Sprint 1 | IE-001 |
| IE-005 | As a technical writer, I want engine documentation so that developers can onboard and extend the engine | - README with build instructions per platform<br/>- Architecture overview (module diagram)<br/>- API surface documented (FFI boundary)<br/>- Contributing guide for IE-specific modules | 2 | Sprint 1 | IE-001 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Pre-Sprint (Reference) |
| **Sprint(s)** | Pre-Sprint |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | — |
| **Successor** | [02-engine-architecture](02-engine-architecture.md) |
| **Story Points Total** | 18 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-001 | As a Tech Lead, I want feature parity matrix sign-off vs Photoshop/Canva/PicsArt/Snapseed so that we have a validated target scope | - Feature parity matrix documented and reviewed<br/>- Sign-off from stakeholders for each competitor column<br/>- Gopost IE target column defines acceptance for each feature domain | 5 | P0 | — |
| IE-002 | As a C++ engine developer, I want platform target validation (Metal/Vulkan/GLES per platform) so that GPU APIs are confirmed for each target | - Metal 3/2 validated on iOS 15+ and macOS 12+<br/>- Vulkan 1.1/GLES 3.2 validated on Android API 26+<br/>- Vulkan 1.2 validated on Windows 10+<br/>- Fallback path documented per platform | 5 | P0 | — |
| IE-003 | As a QA engineer, I want non-functional requirements sign-off (30fps preview, <100ms brush stroke, 512MB mobile) so that performance targets are agreed | - NFR document with 30fps preview, <100ms brush stroke, 512MB mobile ceiling<br/>- Stakeholder sign-off on targets<br/>- Benchmark methodology defined | 3 | P0 | — |
| IE-004 | As a Tech Lead, I want shared core integration plan with VE engine sign-off so that we maximize code reuse | - Integration plan for core/, rendering/, platform/ from libgopost_ve<br/>- Dependency graph and namespace mapping documented<br/>- VE engine team sign-off on shared module usage | 3 | P0 | — |
| IE-005 | As a C++ engine developer, I want performance baseline targets established so that we can track regression | - Baseline benchmarks defined for key operations<br/>- CI integration plan for benchmark runs<br/>- Regression threshold policy documented | 2 | P1 | IE-001 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
