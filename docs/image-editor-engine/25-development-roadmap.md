
## IE-25. Development Roadmap

### Phase 1: Core Engine Foundation (Weeks 1–5)

| Week | Deliverables |
|---|---|
| 1–2 | Shared core integration (link/include core/, rendering/, platform/ from libgopost_ve); IE-specific memory budget configuration; CMake build system for all 4 platforms; GPU backend verification (Metal/Vulkan/GLES) on all platforms |
| 3–4 | Canvas data model (CanvasSettings, Layer tree, ViewportState); Media pool (image import: JPEG, PNG, WebP via libjpeg-turbo, libpng, libwebp); Image decode pipeline (async decode thread, proxy generation); Tile grid system (TileGrid, dirty region tracking) |
| 5 | Interactive canvas renderer (viewport rendering, zoom/pan); Layer cache system; FFI C API: engine lifecycle + canvas creation + media import; Flutter FFI bridge (Dart bindings via ffigen) |

### Phase 2: Layer System and Compositing (Weeks 6–10)

| Week | Deliverables |
|---|---|
| 6–7 | Image layers (place, crop, resize); Solid color layers; Gradient layers (linear, radial, angular, diamond); Layer transform (position, scale, rotation, anchor point); Layer properties (opacity, visibility, lock, name) |
| 8–9 | All 30+ blend modes (GPU compute shaders — shared from VE); Layer compositor pipeline (bottom-to-top evaluation); Layer groups (folders, pass-through vs isolated blending); Clipping masks; Layer reordering; Duplicate / merge / flatten |
| 10 | Layer masks (grayscale, brush-painted); Vector masks (bezier path clipping); Layer style system: drop shadow, inner shadow, outer glow, stroke; Undo/redo system (command pattern, 200-step history) |

### Phase 3: Effects and Filters (Weeks 11–16)

| Week | Deliverables |
|---|---|
| 11–12 | Shared effect graph integration (EffectRegistry, EffectProcessor from VE); Color effects: brightness/contrast, levels, curves, hue/saturation, color balance; Blur effects: gaussian, directional, radial, lens blur (bokeh); Non-destructive effect chain per layer |
| 13–14 | Adjustment layers (brightness, levels, curves, hue/sat, black & white, exposure, vibrance); Full layer style system: bevel/emboss, satin, color/gradient/pattern overlay; Filter preset library (100+ one-tap photo filters); LUT engine (.cube, .3dl support) |
| 15–16 | Image-specific effects: clarity, dehaze, texture, shadow/highlight recovery, split toning; Distortion effects: warp, bulge, twirl, displacement; Stylize effects: glow, drop shadow, emboss, mosaic, halftone; Creative effects: duotone, color splash, glitch, oil paint, watercolor |

### Phase 4: Text and Vector Graphics (Weeks 17–22)

| Week | Deliverables |
|---|---|
| 17–18 | Shared text engine integration (FreeType + HarfBuzz from VE); Rich text layer: multi-style runs, paragraph alignment, box text; Text effects: gradient fill, image fill, outlines, shadows; Text warp (arc, wave, flag, etc.); Text-on-path (circle, custom bezier); Text style presets (100+) |
| 19–20 | Vector shape system: rectangle, rounded rect, ellipse, polygon, star, line, arrow; Shape fill (solid, gradient, pattern, image); Shape stroke (color, width, dash, variable width); Pen tool (bezier path creation + editing); Curvature pen tool |
| 21–22 | Boolean operations (unite, subtract, intersect, exclude — via Clipper2); SVG import (nanosvg → canvas layers); SVG export; Shape preset library (200+ shapes + icons); Font management (system fonts, bundled fonts, Google Fonts download) |

### Phase 5: Template System (Weeks 23–28)

| Week | Deliverables |
|---|---|
| 23–24 | Template data model (placeholders, style controls, palette); Template loader (encrypted .gpit decryption, asset extraction); Image placeholder (smart replace with fit modes); Text placeholder (editable with constraints, auto-shrink) |
| 25–26 | Color palette system (apply palette to template, auto-generate from image); Style controls (color, font, layout variants, slider, toggle); Logo placeholder; QR code placeholder (auto-generation); Collage system (grid, mosaic, magazine layouts) |
| 27–28 | Element library integration (stickers, icons, illustrations, frames); Template serialization (.gpit encryption); Multi-page templates (carousel support); Social media canvas presets (Instagram, Facebook, TikTok, etc.); Template preview generation |

### Phase 6: Selection, Masking, and Brush (Weeks 29–34)

| Week | Deliverables |
|---|---|
| 29–30 | Selection tools: rectangular/elliptical marquee, lasso, polygonal lasso; Magic wand (tolerance-based flood fill); Quick selection (edge-aware brush selection); Selection operations (feather, expand, contract, smooth, invert); Selection rendering (marching ants overlay) |
| 31–32 | Brush engine: brush tip (round, square, custom texture); Brush dynamics (pressure, tilt, velocity, scatter, jitter); Brush tools: paint, pencil, eraser, smudge, blur, sharpen; Dodge, burn, sponge tools; Brush preset library (50+ built-in brushes) |
| 33–34 | Retouching tools: healing brush (Poisson blending), clone stamp, patch tool, spot healing; Red eye removal; Layer mask painting (brush-on-mask mode); Refine edge / select and mask (edge refinement UI); Color range selection |

### Phase 7: Transform, AI, and Advanced Features (Weeks 35–40)

| Week | Deliverables |
|---|---|
| 35–36 | Mesh warp (Photoshop-style 4x4 grid + preset shapes); Perspective correction (4-point + auto-detect); Content-aware scale (seam carving); Liquify tool (forward warp, twirl, pucker, bloat, freeze mask); Face-aware liquify (slider-based face reshaping) |
| 37–38 | AI background removal (3 quality levels: fast, balanced, high quality); AI object removal (PatchMatch + LaMa inpainting); AI super resolution (RealESRGAN 2x/4x upscale); AI style transfer (20 built-in styles + custom reference) |
| 39–40 | AI face detection + beautification (skin smooth, eye brighten, teeth whiten, face reshape); AI auto-enhance (optimal adjustments computation); AI auto-crop (composition-aware suggestions); RAW image processing (LibRaw integration, full RAW develop pipeline); HDR merge (multi-exposure bracket merging + tone mapping) |

### Phase 8: Export, Polish, and Launch (Weeks 41–48)

| Week | Deliverables |
|---|---|
| 41–42 | Export pipeline: JPEG, PNG, WebP, AVIF, HEIF, TIFF, PDF; Social media export presets (19 platform presets); Batch export (same canvas, multiple formats/sizes); Print export pipeline (CMYK, ICC profiles, crop marks, preflight); PSD export (layered) |
| 43–44 | Performance optimization pass (profiling all 4 platforms); Memory optimization (leak hunting, budget tuning, cache efficiency); GPU pipeline optimization (reduce draw calls, texture aliasing); Plugin system (filter plugins, brush plugins, shader-based filters) |
| 45–46 | Full test suite execution; Reference rendering validation (all effects, blend modes, styles); Fuzz testing (project parser, image decoders, API surface); Memory stress testing (long editing sessions, many layers, undo pressure) |
| 47–48 | Platform hardening: iOS (Metal 3, Apple Pencil, Photo Library); Android (Vulkan tuning, S-Pen, MediaStore); macOS (Wacom tablet, color management); Windows (Windows Ink, DPI awareness); Final API freeze; Binary size optimization; Release candidate builds |

### Post-Launch Roadmap

| Feature | Target |
|---|---|
| PSD import (partial layer support) | Q1 post-launch |
| Collaborative templates (real-time multi-user) | Q2 post-launch |
| Video-to-still integration (extract frame from VE) | Q2 post-launch |
| 3D text extrusion (real-time) | Q3 post-launch |
| Generative AI fill (Stable Diffusion integration) | Q3 post-launch |
| Animated template export (GIF, APNG, short video) | Q4 post-launch |
| Web engine (WebAssembly + WebGPU port) | 2027 |
| Cloud-based template rendering | 2027 |

### Sprint-to-Epic-to-Story Traceability Matrix

| Sprint | Weeks | Phase / Epic | Key Stories | Doc Reference |
|---|---|---|---|---|
| Sprint 1 | Wk 1-2 | Build system, initial setup | IE-277 to IE-288 | IE-22 |
| Sprint 2 | Wk 3-4 | Core engine foundation | Canvas, media pool, tile grid | IE-25 Phase 1 |
| Sprint 3 | Wk 5-6 | Core engine foundation, FFI | IE-289 to IE-291, engine/canvas/media API | IE-23, IE-25 |
| Sprint 4 | Wk 7-8 | Layer system | Image, solid, gradient layers | IE-25 Phase 2 |
| Sprint 5 | Wk 9-10 | Layer system | Blend modes, compositor, groups | IE-25 Phase 2 |
| Sprint 6 | Wk 11-12 | Layer system, FFI | IE-292 to IE-294, layer API | IE-23, IE-25 |
| Sprint 7 | Wk 13-14 | Effects | Effect graph, color/blur effects | IE-25 Phase 3 |
| Sprint 8 | Wk 15-16 | Effects | Adjustment layers, filter presets | IE-25 Phase 3 |
| Sprint 9 | Wk 17-18 | Effects | Image-specific, distortion, stylize | IE-25 Phase 3 |
| Sprint 10 | Wk 19-20 | Text and vector, FFI | IE-295 to IE-296, effect/text API | IE-23, IE-25 Phase 4 |
| Sprint 11 | Wk 21-22 | Text and vector | Shape system, pen tool | IE-25 Phase 4 |
| Sprint 12 | Wk 23-24 | Text and vector | Boolean ops, SVG, font management | IE-25 Phase 4 |
| Sprint 13 | Wk 25-26 | Template system, FFI | IE-297 to IE-298, selection/brush API | IE-23, IE-25 Phase 5 |
| Sprint 14 | Wk 27-28 | Template system | Placeholders, palette, collage | IE-25 Phase 5 |
| Sprint 15 | Wk 29-30 | Selection and masking | Selection tools, magic wand | IE-25 Phase 6 |
| Sprint 16 | Wk 31-32 | Brush and retouching | Brush engine, healing, refine edge | IE-25 Phase 6 |
| Sprint 17 | Wk 33-34 | Transform, FFI | IE-299 to IE-300, template/export API | IE-23, IE-25 Phase 7 |
| Sprint 18 | Wk 35-36 | Transform and AI | Mesh warp, liquify, AI removal/upscale | IE-25 Phase 7 |
| Sprint 19 | Wk 37-38 | AI features | AI style transfer, face detection, auto-enhance | IE-25 Phase 7 |
| Sprint 20 | Wk 39-40 | AI and RAW | RAW pipeline, HDR merge | IE-25 Phase 7 |
| Sprint 21 | Wk 41-42 | Export and polish | Export pipeline, batch, print, PSD | IE-25 Phase 8 |
| Sprint 22 | Wk 43-44 | Performance, FFI | IE-301 to IE-303, undo/callback/render API | IE-23, IE-25 Phase 8 |
| Sprint 23 | Wk 43-44 | Performance, memory optimization | IE-263 to IE-276 | IE-21 |
| Sprint 24 | Wk 45-48 | Testing, validation, platform hardening, RC | IE-304 to IE-318 | IE-24, IE-25 Phase 8 |

### Cross-Cutting Concerns (IE-319 to IE-326)

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-319 | As a team, I want a sprint retrospective cadence so that we continuously improve process | - Retrospective at end of each 2-week sprint<br/>- Action items tracked and assigned<br/>- Format documented | 2 | Ongoing | — |
| IE-320 | As a team, I want a demo/review schedule so that stakeholders see progress | - Demo at end of each sprint or phase<br/>- Review criteria defined<br/>- Feedback incorporated | 2 | Ongoing | — |
| IE-321 | As a team, I want a definition of done so that stories are consistently completed | - DoD includes: code complete, tests pass, reviewed, documented<br/>- DoD applied to all stories<br/>- Exceptions documented | 2 | Sprint 1 | — |
| IE-322 | As a team, I want a code review policy so that quality is maintained | - All changes require review before merge<br/>- Review checklist (style, tests, security)<br/>- Turnaround target defined | 2 | Sprint 1 | — |
| IE-323 | As a team, I want a branch strategy (feature branches → develop → main) so that integration is controlled | - Feature branches from develop<br/>- develop is integration branch<br/>- main for release tags only | 2 | Sprint 1 | — |
| IE-324 | As a team, I want a release tagging convention so that releases are traceable | - Semantic versioning (e.g., v1.0.0)<br/>- Tags on main for each release<br/>- Changelog or release notes process | 2 | Sprint 1 | — |
| IE-325 | As a team, I want a documentation update process so that docs stay current | - Docs updated with code changes<br/>- API docs generated from source<br/>- Review includes doc updates | 2 | Ongoing | — |
| IE-326 | As a team, I want technical debt tracking so that debt is visible and prioritized | - Debt items logged (e.g., backlog, ADR)<br/>- Debt considered in sprint planning<br/>- Paydown stories when critical | 2 | Ongoing | — |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Reference (roadmap coordination) |
| **Sprint(s)** | Reference |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [24-testing-validation](24-testing-validation.md) |
| **Successor** | [26-appendices](26-appendices.md) |
| **Story Points Total** | 25 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-371 | As a team lead, I want sprint-to-phase traceability matrix review so that all stories map to phases | - Matrix covers all 24 sprints<br/>- Each story maps to phase and sprint<br/>- Gaps identified and addressed | 5 | P0 | — |
| IE-372 | As a team lead, I want cross-team dependency mapping (IE↔VE↔App) so that blockers are visible | - IE dependencies on VE documented<br/>- App dependencies on IE documented<br/>- Shared components identified | 5 | P0 | — |
| IE-373 | As a team lead, I want milestone definition (Alpha, Beta, RC, GA) so that release gates are clear | - Alpha, Beta, RC, GA criteria defined<br/>- Per-milestone deliverables<br/>- Sign-off process | 5 | P0 | — |
| IE-374 | As a team lead, I want risk register review per phase so that risks are tracked | - Risk register per phase<br/>- Mitigation plans<br/>- Escalation path | 5 | P0 | — |
| IE-375 | As a team lead, I want post-launch roadmap prioritization so that future work is planned | - Post-launch features prioritized<br/>- Q1–Q4 targets<br/>- Resource allocation | 5 | P0 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
