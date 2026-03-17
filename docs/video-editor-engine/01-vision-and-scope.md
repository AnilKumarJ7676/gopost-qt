
## VE-1. Vision and Scope

### 1.1 Mission

Build a single shared C/C++ video editor engine (`libgopost_ve`) that provides After Effects-level compositing, Premiere Pro-level non-linear editing, and CapCut-level ease-of-use and AI features — deployed as a shared library across iOS, macOS, Android, and Windows via Flutter FFI.

### 1.2 Feature Parity Matrix

| Feature Domain | After Effects | Premiere Pro | CapCut | DaVinci Resolve | Gopost VE Target |
|---|---|---|---|---|---|
| **Multi-track NLE timeline** | — | Yes | Yes | Yes | Yes |
| **Layer-based compositing** | Yes | — | Partial | Yes (Fusion) | Yes |
| **Keyframe animation (bezier)** | Yes | Yes | Yes | Yes | Yes |
| **Expression / scripting engine** | Yes (JS) | — | — | — | Yes (Lua) |
| **Motion graphics / shape layers** | Yes | — | Partial | Yes | Yes |
| **Advanced text animation** | Yes | Partial | Yes | Partial | Yes |
| **3D camera and lighting** | Yes | — | — | Yes | Phase 2 |
| **Particle systems** | Yes (plugins) | — | Partial | Yes | Phase 2 |
| **Professional color grading** | Partial | Yes (Lumetri) | Partial | Yes (full) | Yes |
| **LUT support (3D LUT)** | Yes | Yes | Yes | Yes | Yes |
| **HDR pipeline** | Partial | Yes | — | Yes | Yes |
| **Audio multi-track mixing** | Basic | Yes | Yes | Yes (Fairlight) | Yes |
| **Audio effects (EQ, comp, reverb)** | — | Yes | Partial | Yes | Yes |
| **Speed ramping / time remapping** | Yes | Yes | Yes | Yes | Yes |
| **Masking (roto, feather)** | Yes | Yes | Partial | Yes | Yes |
| **Tracking (point, planar)** | Yes (Mocha) | Partial | — | Yes | Yes |
| **Chroma key (green screen)** | Yes | Yes | Yes | Yes | Yes |
| **AI background removal** | — | — | Yes | Yes (Magic Mask) | Yes |
| **AI auto-captions** | — | Yes | Yes | Yes | Yes |
| **AI scene detection** | — | Yes | Yes | Yes | Yes |
| **AI music generation** | — | — | Yes | — | Phase 2 |
| **Stabilization** | Yes (Warp) | Yes | Yes | Yes | Yes |
| **Lens correction** | Yes | Yes | — | Yes | Yes |
| **Noise reduction** | Partial | Yes | — | Yes (Temporal/Spatial) | Yes |
| **Multi-cam editing** | — | Yes | — | Yes | Phase 2 |
| **Proxy workflow** | — | Yes | — | Yes | Yes |
| **Hardware-accelerated export** | Yes | Yes | Yes | Yes | Yes |
| **GPU compute rendering** | Yes | Yes | Partial | Yes | Yes |

### 1.3 Platform Targets

| Platform | GPU API (Primary) | GPU API (Fallback) | HW Decode | HW Encode | Min OS |
|---|---|---|---|---|---|
| iOS | Metal 3 | Metal 2 | VideoToolbox | VideoToolbox | iOS 15.0+ |
| macOS | Metal 3 | Metal 2 | VideoToolbox | VideoToolbox | macOS 12.0+ |
| Android | Vulkan 1.1 | OpenGL ES 3.2 | MediaCodec | MediaCodec | API 26+ |
| Windows | Vulkan 1.2 | DirectX 12 / OpenGL 4.6 | NVDEC / D3D11VA / VAAPI | NVENC / AMF / QSV | Windows 10+ |

### 1.4 Non-Functional Requirements

| Requirement | Target |
|---|---|
| Preview playback | 60 fps at 1080p, 30 fps at 4K (desktop) |
| Timeline scrub latency | < 50 ms from seek to frame display |
| Effect preview | Real-time for ≤ 5 stacked effects on mid-range device |
| Export speed (1080p 60s, H.265) | ≤ 45 seconds on flagship mobile, ≤ 20 seconds on desktop |
| Cold engine init | < 300 ms |
| Memory ceiling (mobile) | 512 MB engine total |
| Memory ceiling (desktop) | 2 GB engine total |
| Crash rate | < 0.1% of editing sessions |
| Undo/redo depth | 200 operations minimum |
| Auto-save interval | Every 30 seconds during active editing |
| Max timeline duration | 4 hours |
| Max concurrent tracks | 32 video + 32 audio |
| Max layers per composition | 100 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Pre-Sprint (Reference) |
| **Sprint(s)** | Pre-Sprint |
| **Team** | Tech Lead, Product Owner, QA Lead |
| **Predecessor** | — |
| **Successor** | [02-engine-architecture](02-engine-architecture.md) |
| **Story Points Total** | 21 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-001 | As a Tech Lead, I want the feature parity matrix sign-off so that we have stakeholder alignment on scope before development | - Matrix reviewed and approved by product owner<br/>- All target features documented with phase assignment<br/>- Sign-off recorded in project documentation | 5 | P0 | — |
| VE-002 | As a C++ engine developer, I want platform target validation so that we know the minimum OS and GPU requirements for each platform | - iOS 15+, macOS 12+, Android API 26+, Windows 10+ validated<br/>- GPU API (Metal/Vulkan/OpenGL) requirements documented<br/>- HW decode/encode capabilities per platform documented | 5 | P0 | — |
| VE-003 | As a QA engineer, I want non-functional requirements sign-off so that we have measurable acceptance criteria for performance and quality | - Preview playback, scrub latency, export speed targets documented<br/>- Memory ceiling (512MB mobile, 2GB desktop) agreed<br/>- Crash rate, undo depth, auto-save interval specified | 5 | P0 | — |
| VE-004 | As a C++ engine developer, I want GPU API selection per platform finalized so that we can implement the correct backend for each target | - Metal 3 primary for iOS/macOS confirmed<br/>- Vulkan 1.1/1.2 for Android/Windows confirmed<br/>- OpenGL ES 3.2 fallback strategy documented | 3 | P0 | — |
| VE-005 | As a QA engineer, I want performance target baselines established so that we can measure regression during development | - Benchmark suite defined for cold init, scrub, export<br/>- Baseline metrics captured on reference devices<br/>- CI integration plan for performance gates documented | 3 | P1 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
