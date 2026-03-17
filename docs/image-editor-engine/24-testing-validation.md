
## IE-24. Testing and Validation

### 24.1 Test Categories

| Category | Scope | Tool | Target |
|---|---|---|---|
| **Unit Tests** | Individual functions, classes | Google Test | All modules, > 80% coverage |
| **Integration Tests** | Module interactions (canvas + compositor + GPU) | Google Test + custom harness | Cross-module data flow |
| **Reference Rendering** | Pixel-accurate output comparison | Custom (SSIM + perceptual hash) | All effects, blend modes, layer styles |
| **Performance Benchmarks** | Latency, throughput, memory | Google Benchmark + custom | Per-function and end-to-end |
| **Fuzz Testing** | Crash resistance (project parser, image decoder, API) | libFuzzer / AFL++ | Parser/decoder inputs |
| **Memory Tests** | Leak detection, budget compliance | ASan, LSan, custom tracker | All allocators |
| **GPU Validation** | Shader correctness, resource leaks | GPU validation layers (Metal/Vulkan) | All GPU operations |
| **Cross-Platform** | Consistent output across platforms | CI matrix (4 platforms) | Pixel diff < 1% |

### 24.2 Reference Rendering Tests

```cpp
namespace gp::test {

// Each reference test:
// 1. Loads a test project (.gpimg)
// 2. Renders the canvas
// 3. Compares output against a reference image (golden file)
// 4. Passes if SSIM > threshold AND max pixel error < limit

struct ReferenceTest {
    std::string name;
    std::string project_path;        // .gpimg test project
    std::string reference_path;      // Reference PNG
    float ssim_threshold;            // Default 0.999
    int max_pixel_error;             // Default 2 (out of 255)
    int tolerance_pixels;            // Pixels allowed to exceed max_pixel_error
};

class ReferenceTestRunner {
public:
    struct Result {
        bool passed;
        float ssim;
        int max_error;
        int pixels_exceeding;
        std::string diff_image_path;   // Visual diff output
    };

    Result run(const ReferenceTest& test, IGpuContext& gpu);
    void generate_reference(const ReferenceTest& test, IGpuContext& gpu);
};

// Reference test suite covers:
// - All 30+ blend modes (each mode in isolation)
// - All layer styles (drop shadow, glow, bevel, stroke, overlays)
// - All adjustment types (levels, curves, hue/sat, etc.)
// - All built-in effects/filters
// - Clipping masks and layer masks
// - Group compositing (pass-through vs isolated)
// - Text rendering (various fonts, sizes, styles)
// - Vector shapes (fills, strokes, boolean operations)
// - Transform operations (rotate, scale, perspective, warp)
// - Template rendering (placeholder replacement)
// - Export format accuracy (JPEG, PNG, WebP, TIFF)
// - Color space conversions
// - Tile boundary seams (no visible seams at tile edges)
// - Large canvas rendering (> 8K)

} // namespace gp::test
```

### 24.3 Performance Test Suite

| Test | Metric | Mobile Target | Desktop Target |
|---|---|---|---|
| Canvas render (20 layers, 1080p) | Frame time | < 16 ms | < 8 ms |
| Canvas render (50 layers, 4K) | Total time | < 3 s | < 1.5 s |
| Gaussian blur (radius 50, 4K) | Process time | < 50 ms | < 20 ms |
| Effect chain (5 effects, 1080p) | Process time | < 200 ms | < 80 ms |
| Layer add (image layer) | Latency | < 5 ms | < 2 ms |
| Brush stroke (100 stamps) | Total time | < 200 ms | < 50 ms |
| Magic wand (12 MP image) | Selection time | < 100 ms | < 50 ms |
| Template load (.gpit, 2 MB) | Load to interactive | < 400 ms | < 200 ms |
| Image import (12 MP JPEG) | Decode + display | < 150 ms | < 80 ms |
| RAW import (50 MP) | Decode + preview | < 800 ms | < 400 ms |
| Export (4K JPEG, q95) | Encode time | < 500 ms | < 200 ms |
| Export (4K PNG) | Encode time | < 1.5 s | < 600 ms |
| Undo/redo (any operation) | Latency | < 20 ms | < 10 ms |
| Auto-save (incremental) | Save time | < 200 ms | < 100 ms |
| AI background removal | Process time | < 500 ms | < 200 ms |

### 24.4 Fuzz Testing Targets

| Target | Input | Approach |
|---|---|---|
| .gpimg project parser | Mutated MessagePack blobs | libFuzzer with custom mutator |
| JPEG decoder (libjpeg-turbo) | Corrupted JPEG files | AFL++ on decode path |
| PNG decoder (libpng) | Corrupted PNG files | AFL++ on decode path |
| RAW decoder (LibRaw) | Corrupted RAW files | AFL++ on decode path |
| SVG importer | Malformed SVG XML | libFuzzer |
| Template parser (.gpit) | Mutated encrypted blobs | libFuzzer post-decryption |
| Effect parameter parser | Random JSON parameter values | libFuzzer on set_param |
| Brush input | Random stroke sequences | Custom fuzzer |
| FFI API surface | Random API call sequences | Custom fuzzer (stateful) |

### 24.5 Memory Testing

```
Memory test protocol:

1. Leak detection (ASan + LSan):
   - Run full test suite with AddressSanitizer
   - Zero leaks tolerance on all platforms

2. Budget compliance:
   - Open 50-layer project with large images
   - Monitor total engine memory
   - Must stay under 384 MB (mobile) / 1.5 GB (desktop)

3. Stress test:
   - Apply 20 effects, undo all, redo all, repeat 100 times
   - Monitor for memory growth (< 1% drift allowed)

4. Low memory simulation:
   - Simulate OS memory pressure warnings
   - Verify cache eviction, quality reduction, emergency save
   - No crashes under memory pressure

5. GPU memory:
   - Track all GPU texture allocations
   - Verify cleanup on canvas destroy
   - No GPU memory leaks on Metal/Vulkan validation
```

### 24.6 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 24 | Wk 45-48 | IE-304 to IE-318 | Testing, validation, platform hardening, RC |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-304 | As a developer, I want unit test framework setup (Google Test) so that I can write and run unit tests | - Google Test integrated via CMake<br/>- GP_BUILD_TESTS option enables test build<br/>- At least one sample test passes | 3 | Sprint 24 | — |
| IE-305 | As a developer, I want unit tests for the core module (allocators, math, task system) so that core behavior is validated | - Allocator tests: slab, arena, pool<br/>- Math tests: color, matrix, rect, blend<br/>- Task system tests: enqueue, completion, cancellation | 5 | Sprint 24 | IE-304 |
| IE-306 | As a developer, I want unit tests for the canvas module (layer CRUD, groups, reorder) so that canvas operations are validated | - Layer add/remove/duplicate/reorder tests<br/>- Group create/nest/ungroup tests<br/>- Merge/flatten tests | 5 | Sprint 24 | IE-304 |
| IE-307 | As a developer, I want unit tests for the effect module (all 100+ effects parameter validation) so that effects accept valid params and reject invalid | - Each effect has parameter validation test<br/>- Invalid params return GP_ERR_INVALID_ARG<br/>- Edge cases (min/max, NaN) covered | 8 | Sprint 24 | IE-304 |
| IE-308 | As a developer, I want an integration test: canvas → compositor → GPU → texture so that the full render pipeline is validated | - Test loads project, triggers render, receives texture<br/>- Validates data flow across modules<br/>- Passes on all 4 platforms | 5 | Sprint 24 | IE-304 |
| IE-309 | As a developer, I want reference rendering for blend modes (30+ golden images) so that blend mode output is pixel-accurate | - Golden images for all 30+ blend modes<br/>- SSIM > threshold, max pixel error < limit<br/>- Diff images generated on failure | 5 | Sprint 24 | IE-308 |
| IE-310 | As a developer, I want reference rendering for layer styles (all styles) so that layer style output is validated | - Golden images for drop shadow, glow, bevel, stroke, overlays<br/>- All style combinations covered<br/>- Pixel diff < 1% | 5 | Sprint 24 | IE-308 |
| IE-311 | As a developer, I want reference rendering for effects (all effects) so that effect output is validated | - Golden images for all built-in effects<br/>- Parameter variations where relevant<br/>- Cross-platform consistency | 8 | Sprint 24 | IE-308 |
| IE-312 | As a developer, I want reference rendering for text (fonts, styles, warp) so that text rendering is validated | - Golden images for various fonts, sizes, styles<br/>- Text warp (arc, wave, flag) covered<br/>- Complex scripts if applicable | 5 | Sprint 24 | IE-308 |
| IE-313 | As a developer, I want a performance benchmark suite (Google Benchmark) so that regressions are detectable | - Google Benchmark integrated<br/>- Benchmarks run in CI<br/>- Results logged or compared to baseline | 3 | Sprint 24 | IE-304 |
| IE-314 | As a developer, I want performance benchmarks for all targets from 21.1 so that we meet performance targets | - Benchmarks for canvas render, filter, effect chain, layer add, brush, selection, load, export, undo<br/>- Mobile and desktop targets from 21.1<br/>- Fail if targets exceeded | 5 | Sprint 24 | IE-313 |
| IE-315 | As a developer, I want fuzz testing for the .gpimg parser (libFuzzer) so that parser crashes are found | - libFuzzer target for .gpimg parser<br/>- Custom mutator for MessagePack blobs<br/>- No crashes on valid fuzz corpus | 5 | Sprint 24 | IE-304 |
| IE-316 | As a developer, I want fuzz testing for image decoders (JPEG/PNG/WebP/RAW) so that decoder crashes are found | - Fuzz targets for JPEG, PNG, WebP, RAW decode paths<br/>- AFL++ or libFuzzer<br/>- No crashes on corrupted inputs | 5 | Sprint 24 | IE-304 |
| IE-317 | As a developer, I want a memory stress test (50 layers, long sessions, undo pressure) so that memory leaks and growth are detected | - Test: 50-layer project, long editing session, 100+ undo/redo cycles<br/>- Memory growth < 1% allowed<br/>- ASan/LSan clean | 5 | Sprint 24 | IE-304 |
| IE-318 | As a developer, I want a cross-platform pixel diff test (< 1% variance) so that output is consistent across platforms | - Same project rendered on iOS, macOS, Android, Windows<br/>- Pixel diff between platforms < 1%<br/>- Documented acceptable variance sources | 5 | Sprint 24 | IE-308 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 8: Export, Polish & Launch |
| **Sprint(s)** | IE-Sprint 23-24 (Weeks 45-48) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [21-memory-performance](21-memory-performance.md) |
| **Successor** | [25-development-roadmap](25-development-roadmap.md) |
| **Story Points Total** | 80 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-356 | As a developer, I want Google Test setup for IE so that unit tests can run | - Google Test integrated via CMake<br/>- GP_BUILD_TESTS option<br/>- At least one sample test passes | 3 | P0 | — |
| IE-357 | As a developer, I want canvas/layer unit tests so that canvas operations are validated | - Layer add/remove/duplicate/reorder tests<br/>- Group create/nest/ungroup tests<br/>- Merge/flatten tests | 5 | P0 | IE-356 |
| IE-358 | As a developer, I want compositor unit tests (blend modes, masks, styles) so that compositing is validated | - All 30+ blend modes tested<br/>- Layer mask and clipping mask tests<br/>- Layer style tests | 5 | P0 | IE-356 |
| IE-359 | As a developer, I want effect chain unit tests so that effects work correctly | - Effect add/remove/set_param tests<br/>- Effect chain ordering tests<br/>- Parameter validation tests | 5 | P0 | IE-356 |
| IE-360 | As a developer, I want text layout unit tests so that text rendering is validated | - Font loading and layout tests<br/>- Multi-style runs, alignment<br/>- Text warp tests | 5 | P0 | IE-356 |
| IE-361 | As a developer, I want vector boolean operation tests so that Clipper2 integration works | - Unite, subtract, intersect, exclude tests<br/>- Complex path tests<br/>- Edge case handling | 3 | P0 | IE-356 |
| IE-362 | As a developer, I want selection tool unit tests so that selection operations work | - Magic wand, rect, ellipse tests<br/>- Feather, expand, contract tests<br/>- Invert, to mask tests | 5 | P0 | IE-356 |
| IE-363 | As a developer, I want brush stroke unit tests so that painting works correctly | - Stroke begin/continue/end tests<br/>- Pressure, tilt dynamics tests<br/>- Undo per stroke tests | 5 | P0 | IE-356 |
| IE-364 | As a developer, I want codec integration tests (all image formats round-trip) so that import/export is correct | - JPEG, PNG, WebP, AVIF, HEIF, TIFF round-trip<br/>- Metadata preservation tests<br/>- Quality/compression tests | 5 | P0 | IE-356 |
| IE-365 | As a developer, I want GPU render tests (pixel comparison vs references) so that rendering is accurate | - Golden image comparison<br/>- SSIM > threshold, max pixel error < limit<br/>- Diff images on failure | 5 | P0 | IE-356 |
| IE-366 | As a developer, I want FFI integration test (Dart→C++→GPU→Texture) so that the Flutter bridge works | - Dart calls C API, receives texture<br/>- Full pipeline validation<br/>- All 4 platforms | 5 | P0 | IE-356 |
| IE-367 | As a developer, I want performance benchmarks (layer count, effect count, brush latency) so that regressions are detected | - Google Benchmark integration<br/>- Benchmarks for layer count, effect count, brush<br/>- CI comparison to baseline | 5 | P0 | IE-356 |
| IE-368 | As a developer, I want ASan memory leak test (full editing session) so that leaks are found | - ASan/LSan in test runs<br/>- Full editing session scenario<br/>- Zero leaks tolerance | 5 | P0 | IE-356 |
| IE-369 | As a developer, I want libFuzzer: project parser + image decoders so that crash resistance is validated | - libFuzzer target for .gpimg parser<br/>- Fuzz targets for JPEG, PNG, WebP, RAW<br/>- No crashes on corpus | 5 | P0 | IE-356 |
| IE-370 | As a developer, I want reference rendering image set generation so that golden images exist | - Generate reference set for all effects, blend modes, styles<br/>- Documented generation process<br/>- Version controlled | 5 | P0 | IE-365 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
