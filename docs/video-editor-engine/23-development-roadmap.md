## VE-23. Development Roadmap

### Phase 1: Core Engine Foundation (Weeks 1–6)

| Week | Deliverables |
|---|---|
| 1–2 | Memory system (pool, arena, slab allocators); Math library (SIMD Vec/Mat/Quat); Thread pool + task system; Platform abstraction (iOS, macOS, Android, Windows stubs); CMake build system for all 4 platforms |
| 3–4 | GPU abstraction layer (IGpuContext); Metal backend (iOS/macOS); Vulkan backend (Android/Windows); Shader cross-compilation pipeline (SPIR-V → Metal IR → GLSL); Basic render graph (texture creation, compute dispatch, compositing) |
| 5–6 | Media decoder (FFmpeg integration + HW accel per platform); Media encoder (FFmpeg + HW encode); Frame cache system; Thumbnail generator; FFI C API: engine lifecycle + media import; Flutter FFI bridge (Dart bindings via ffigen) |

### Phase 2: Timeline Engine (Weeks 7–12)

| Week | Deliverables |
|---|---|
| 7–8 | Timeline data model (tracks, clips, time ranges); Core edit operations (add, remove, move, trim, split); Playback engine (forward play with decoded frame pipeline); Seek (keyframe-accurate random access) |
| 9–10 | Multi-track video compositing (bottom-to-top, blend modes, opacity); Multi-track audio mixing; Clip speed control (constant speed, reverse); Timeline snap points; Undo/redo system (command pattern) |
| 11–12 | Ripple/insert/overwrite edits; Slip, slide, roll edit operations; Timeline rendering to Flutter Texture (zero-copy GPU); Audio waveform generation and visualization data; FFI API: all timeline operations; Project save/load (MessagePack) |

### Phase 3: Effects and Color (Weeks 13–18)

| Week | Deliverables |
|---|---|
| 13–14 | Effect graph architecture; Effect registry + processor system; Color effects: brightness, contrast, hue/sat, levels, curves; Blur effects: gaussian, directional, radial; Transform effects: scale, rotate, corner pin |
| 15–16 | Full 30+ blend mode library (GPU shaders); LUT engine (.cube, .3dl loading, 3D texture upload); Color wheels (lift/gamma/gain); HDR pipeline (RGBA16F working space, tone mapping); Keyframe animation engine (bezier interpolation, graph editor data) |
| 17–18 | Complete color grading pipeline (primary + secondary); Distortion effects (warp, bulge, twirl, displacement); Stylize effects (glow, shadow, emboss); Sharpen effects; Generate effects (noise, gradient, fractal); Effect masking (limit effect to mask region) |

### Phase 4: Transitions, Text, and Audio (Weeks 19–24)

| Week | Deliverables |
|---|---|
| 19–20 | Transition system: cross dissolve, wipe, slide, zoom; Transition renderer (A/B frame blending with GPU shaders); 40+ built-in transitions; Custom transition shader API; Transition UI data (parameters, duration, alignment) |
| 21–22 | Text engine: FreeType + HarfBuzz layout; Rich text styling (multi-run, colors, sizes); Text animation: per-character animators with range selectors; Font management (bundled + system fonts); Text-on-path |
| 23–24 | Audio engine: node-based graph (gain, pan, EQ, compressor, reverb, delay, limiter); Audio effects: full catalog; Beat detection and audio-sync editing; Audio level metering (RMS, peak, LUFS); Waveform + spectrum visualization |

### Phase 5: Motion Graphics and Advanced Features (Weeks 25–30)

| Week | Deliverables |
|---|---|
| 25–26 | Shape layer system (rectangle, ellipse, polystar, bezier paths); Shape operators (fill, stroke, trim, repeater, merge, round corners); Vector shape rendering (tessellation + GPU); Shape keyframe animation |
| 27–28 | Masking system (bezier masks, feathering, expansion); Track matte (alpha, luma, inverted); Chroma key (green screen) with spill suppression; Point tracking and motion stabilization; Parenting hierarchy and null layers |
| 29–30 | Expression engine (Lua): wiggle, loopOut, time-based, layer references; Proxy workflow (generate, switch, auto-detect); Time remapping (variable speed, freeze frames, speed ramp); Motion blur (temporal supersampling); Adjustment layers |

### Phase 6: AI Features (Weeks 31–36)

| Week | Deliverables |
|---|---|
| 31–32 | ML inference engine (CoreML/NNAPI/ONNX per platform); Background removal (real-time segmentation, temporal consistency); Portrait segmentation (hair, body, background); Face detection + face mesh; Model download + cache system |
| 33–34 | Auto-captions (Whisper ASR, word-level timestamps, subtitle track); Scene detection (auto-split clips at cuts); Auto-reframe (saliency-based crop for aspect ratio); Style transfer (artistic filters) |
| 35–36 | AI noise reduction (video + audio); Object tracking (bounding box persistence); Super resolution (upscale low-res footage); Voice isolation (separate voice from background); Smart trim (highlight detection) |

### Phase 7: Plugin System and Polish (Weeks 37–42)

| Week | Deliverables |
|---|---|
| 37–38 | Plugin SDK and host API; Custom shader effect loader; Plugin discovery and management; Third-party effect/transition support |
| 39–40 | Performance optimization pass (profiling all platforms); Memory optimization (leak hunting, budget tuning); GPU pipeline optimization (barrier reduction, resource aliasing); Thermal throttling system |
| 41–42 | Full test suite execution and bug fixing; Reference rendering validation; Fuzz testing (project parser, media decoder); Documentation: API reference, plugin SDK guide |

### Phase 8: Platform Hardening and Launch (Weeks 43–48)

| Week | Deliverables |
|---|---|
| 43–44 | iOS/macOS: Metal 3 optimization, VideoToolbox tuning, ANE inference; Android: Vulkan validation, MediaCodec edge cases, NNAPI tuning; Windows: DX12 backend (optional), NVENC/AMF/QSV validation |
| 45–46 | End-to-end integration testing (Flutter → FFI → Engine → GPU → Texture); Auto-save + crash recovery testing; Export quality validation (VMAF/SSIM against reference); Memory stress testing (long editing sessions, many tracks) |
| 47–48 | Final API freeze; Performance benchmarks published; Binary size optimization (LTO, dead code elimination, symbol stripping); Release candidate builds for all 4 platforms |

### Phase 2+ (Post-Launch)

| Feature | Target |
|---|---|
| 3D camera and lighting | Q3 post-launch |
| Particle systems | Q3 post-launch |
| Multi-cam editing | Q4 post-launch |
| AI music generation | Q4 post-launch |
| Collaborative editing (real-time) | 2027 |
| Cloud rendering | 2027 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Reference Document |
| **Sprint(s)** | Reference (roadmap traceability) |
| **Team** | Tech Lead |
| **Predecessor** | [22-testing-validation](22-testing-validation.md) |
| **Successor** | [24-appendices](24-appendices.md) |
| **Story Points Total** | 18 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-366 | As a Tech Lead, I want sprint-to-phase traceability matrix creation so that all stories map to phases | - Matrix: Sprint ID → Phase, Weeks<br/>- All VE-xxx stories mapped<br/>- Document in repo | 5 | P0 | — |
| VE-367 | As a Tech Lead, I want cross-team dependency mapping (VE↔App↔IE) so that integration is planned | - VE dependencies on App/IE<br/>- App/IE dependencies on VE<br/>- Critical path identified | 5 | P1 | — |
| VE-368 | As a Tech Lead, I want milestone definition (Alpha, Beta, RC, GA) so that release gates are clear | - Alpha: core editing works<br/>- Beta: feature complete, QA pass<br/>- RC: no P0 bugs, GA: shipped | 3 | P0 | — |
| VE-369 | As a Tech Lead, I want risk register review per phase so that risks are tracked | - Risk register document<br/>- Per-phase risk review<br/>- Mitigation plans | 3 | P1 | — |
| VE-370 | As a Tech Lead, I want post-launch roadmap prioritization so that Phase 2+ is planned | - Prioritize 3D, particles, multi-cam, AI music<br/>- Resource allocation<br/>- Timeline estimates | 2 | P2 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
