## VE-22. Testing and Validation Strategy

### 22.1 Test Categories

| Category | Framework | Scope | Target Coverage |
|---|---|---|---|
| **Core unit tests** | Google Test | Allocators, math, containers, utilities | 95% |
| **Timeline unit tests** | Google Test | Clip operations, trim, split, ripple, snap | 90% |
| **Compositor unit tests** | Google Test | Layer evaluation, blend modes, parenting | 90% |
| **Effect unit tests** | Google Test | Parameter evaluation, keyframe interpolation | 85% |
| **Animation unit tests** | Google Test | Bezier, spring, expression evaluation | 90% |
| **Audio unit tests** | Google Test | Mixing, effects, waveform generation | 85% |
| **Codec integration** | Google Test + fixtures | Decode/encode round-trip for all supported formats | 85% |
| **GPU render tests** | Custom framework | Pixel-accurate comparison against reference images | 80% |
| **FFI integration** | Flutter integration tests | Dart → FFI → C++ → GPU → Flutter Texture round-trip | 75% |
| **Performance benchmarks** | Google Benchmark | Frame render time, decode throughput, effect latency | N/A (regression tracking) |
| **Memory leak tests** | ASan + custom tracker | Full editing session with all features exercised | 100% (zero leaks) |
| **Fuzz testing** | libFuzzer / AFL++ | Project file parser, media decoder, effect parameters | N/A (continuous) |

### 22.2 Reference Rendering

Pixel-accurate GPU rendering tests compare output frames against known-good reference images:

```cpp
TEST(CompositorTest, BlendModeMultiply) {
    auto engine = create_test_engine();
    auto comp = create_test_composition(1920, 1080);

    // Layer 1: solid red
    add_solid_layer(comp, Color4f{1, 0, 0, 1});
    // Layer 2: 50% grey, blend mode multiply
    add_solid_layer(comp, Color4f{0.5f, 0.5f, 0.5f, 1}, BlendMode::Multiply);

    auto frame = render_frame(engine, comp, Rational{0, 1});
    EXPECT_IMAGE_MATCH(frame, "references/blend_multiply_red_grey.png", /*tolerance=*/1);
}
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 7-8: Plugin & Polish / Platform Hardening |
| **Sprint(s)** | VE-Sprint 21-23 (Weeks 41-46) |
| **Team** | C/C++ Engine Developer (2), Tech Lead, QA Engineer |
| **Predecessor** | [21-public-c-api](21-public-c-api.md) |
| **Successor** | [23-development-roadmap](23-development-roadmap.md) |
| **Story Points Total** | 78 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-351 | As a QA engineer, I want Google Test setup for core unit tests so that tests can be run | - enable_testing(), find_package(GTest)<br/>- add_subdirectory(tests)<br/>- CI integration | 3 | P0 | — |
| VE-352 | As a QA engineer, I want timeline unit test suite (clip ops, trim, split, ripple, snap) so that timeline behavior is validated | - AddClip, RemoveClip, MoveClip tests<br/>- TrimClip, SplitClip tests<br/>- Ripple, snap behavior tests | 8 | P0 | — |
| VE-353 | As a QA engineer, I want compositor unit test suite (blend modes, parenting, mattes) so that composition is correct | - Blend mode tests (multiply, screen, overlay, etc.)<br/>- Parenting hierarchy tests<br/>- Track matte tests | 8 | P0 | — |
| VE-354 | As a QA engineer, I want effect parameter evaluation unit tests so that effects behave correctly | - Parameter type validation<br/>- Keyframe-driven parameter tests<br/>- Edge cases (min, max, default) | 5 | P0 | — |
| VE-355 | As a QA engineer, I want keyframe interpolation unit tests so that animation is accurate | - Bezier interpolation correctness<br/>- Hold, linear, ease variants<br/>- Multi-keyframe sequences | 5 | P0 | — |
| VE-356 | As a QA engineer, I want audio mixing unit tests so that audio output is correct | - Multi-track mixing<br/>- Volume, pan application<br/>- Sample accuracy | 5 | P0 | — |
| VE-357 | As a QA engineer, I want codec integration test (decode/encode round-trip all formats) so that media I/O is reliable | - H.264, HEVC, VP9, ProRes round-trip<br/>- AAC, Opus audio round-trip<br/>- Pixel/audio comparison | 8 | P0 | — |
| VE-358 | As a QA engineer, I want GPU render test framework (pixel comparison against references) so that visual regressions are caught | - EXPECT_IMAGE_MATCH macro<br/>- Tolerance for platform variance<br/>- Reference image generation | 8 | P0 | — |
| VE-359 | As a QA engineer, I want FFI integration test (Dart→C++→GPU→Texture round-trip) so that Flutter integration works | - Dart test that creates engine, timeline<br/>- Renders frame, receives texture<br/>- Validates pixel data | 8 | P0 | — |
| VE-360 | As a QA engineer, I want Google Benchmark setup (frame render time, decode throughput, effect latency) so that performance is tracked | - Benchmark for frame render time<br/>- Decode throughput benchmark<br/>- Effect latency benchmark | 5 | P1 | VE-351 |
| VE-361 | As a QA engineer, I want ASan memory leak test (full editing session) so that leaks are detected | - Build with -fsanitize=address<br/>- Run full editing workflow<br/>- Zero leaks reported | 5 | P0 | — |
| VE-362 | As a QA engineer, I want libFuzzer: project file parser so that malformed projects don't crash | - Fuzz target for .gpproj parsing<br/>- No crashes, no hangs<br/>- Corpus from valid projects | 5 | P1 | — |
| VE-363 | As a QA engineer, I want libFuzzer: media decoder so that malformed media doesn't crash | - Fuzz target for media decode<br/>- Various container/codec combos<br/>- No crashes | 5 | P1 | — |
| VE-364 | As a QA engineer, I want libFuzzer: effect parameters so that invalid params don't crash | - Fuzz target for effect param parsing<br/>- All effect types<br/>- No crashes | 3 | P2 | — |
| VE-365 | As a Tech Lead, I want reference rendering image set generation so that GPU tests have known-good outputs | - Generate reference images for blend modes, effects<br/>- Version-controlled in repo<br/>- Platform-specific if needed | 5 | P1 | VE-358 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
