
## VE-12. Transition System

### 12.1 Transition Architecture

```cpp
namespace gp::transitions {

enum class TransitionCategory {
    Dissolve,
    Wipe,
    Slide,
    Zoom,
    Glitch,
    Blur,
    Light,
    Color,
    Geometric,
    Custom,
};

struct TransitionDefinition {
    std::string id;                // "gp.transition.cross_dissolve"
    std::string display_name;
    TransitionCategory category;
    std::string thumbnail;
    int32_t default_duration_frames;
    bool supports_direction;       // Left/right/up/down
    bool supports_color;           // Dip to color
    std::vector<ParameterDef> parameters;
    std::string shader;            // GPU shader for the transition
};

struct TransitionInstance {
    std::string def_id;
    Rational duration;
    TransitionAlignment alignment;  // Center, StartAtCut, EndAtCut
    std::unordered_map<std::string, ParameterValue> parameters;
};

class TransitionRenderer {
public:
    void render(IGpuContext& gpu,
                GpuTexture outgoing_frame,    // Frame A (source)
                GpuTexture incoming_frame,    // Frame B (destination)
                GpuTexture output,
                float progress,               // 0.0 to 1.0
                const TransitionInstance& instance);
};

} // namespace gp::transitions
```

### 12.2 Built-in Transitions

| Category | Transitions |
|---|---|
| **Dissolve** | Cross Dissolve, Additive Dissolve, Dip to Black, Dip to White, Dip to Color, Film Dissolve |
| **Wipe** | Linear Wipe, Radial Wipe, Iris Wipe (circle, diamond, star), Clock Wipe, Band Wipe, Barn Door, Venetian Blinds, Checkerboard, Inset, Split |
| **Slide** | Push (L/R/U/D), Slide (L/R/U/D), Cover, Reveal, Swap |
| **Zoom** | Zoom In, Zoom Out, Zoom Through, Cross Zoom |
| **Blur** | Blur Dissolve, Directional Blur Wipe, Radial Blur Transition |
| **Glitch** | RGB Split, Pixel Sort, Block Glitch, VHS Distortion, Digital Noise |
| **Light** | Light Leak, Lens Flare Wipe, Flash White, Burn Away |
| **Color** | Luma Fade, Color Fade, Gradient Wipe (custom gradient map) |
| **Geometric** | Page Curl, Fold, Cube Rotate, Flip, Morph (shape interpolation) |
| **Custom** | User-provided GLSL shader (plugin API) |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 4: Transitions/Text/Audio |
| **Sprint(s)** | VE-Sprint 10-11 (Weeks 19-22) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [05-composition-engine](05-composition-engine.md) |
| **Successor** | [07-effects-filter-system](07-effects-filter-system.md) |
| **Story Points Total** | 64 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-185 | As a C++ engine developer, I want TransitionDefinition data model so that we can describe transitions declaratively | - id, display_name, category, thumbnail<br/>- default_duration_frames, supports_direction, supports_color<br/>- parameters (ParameterDef), shader path | 5 | P0 | VE-089 |
| VE-186 | As a C++ engine developer, I want TransitionInstance (duration, alignment, parameters) so that we can apply transitions to cuts | - def_id, duration (Rational), alignment (Center, StartAtCut, EndAtCut)<br/>- parameters map<br/>- progress 0.0-1.0 for render | 3 | P0 | VE-185 |
| VE-187 | As a C++ engine developer, I want TransitionRenderer (A/B frame→output with progress) so that we can render transitions | - render(gpu, outgoing, incoming, output, progress, instance)<br/>- A = outgoing_frame, B = incoming_frame<br/>- progress drives transition | 5 | P0 | VE-076 |
| VE-188 | As a C++ engine developer, I want Cross dissolve + additive dissolve + dip to black/white/color so that we have basic dissolves | - Cross dissolve: lerp(A, B, progress)<br/>- Additive: A + B * progress<br/>- Dip to black/white/color: A → color → B | 5 | P0 | VE-187 |
| VE-189 | As a C++ engine developer, I want Film dissolve so that we have a film-style dissolve | - Film dissolve shader<br/>- Grain or texture during transition<br/>- Smooth blend | 3 | P1 | VE-188 |
| VE-190 | As a C++ engine developer, I want Linear/radial/iris/clock wipe so that we have directional wipes | - Linear wipe: direction, angle<br/>- Radial: center, angle<br/>- Iris: shape (circle, diamond, star)<br/>- Clock: center, clockwise | 5 | P0 | VE-187 |
| VE-191 | As a C++ engine developer, I want Band wipe + barn door + venetian blinds + checkerboard so that we have pattern wipes | - Band wipe: band count, angle<br/>- Barn door: horizontal/vertical<br/>- Venetian blinds: count, angle<br/>- Checkerboard: rows, columns | 5 | P0 | VE-190 |
| VE-192 | As a C++ engine developer, I want Push/slide/cover/reveal transitions so that we have motion-based transitions | - Push: A slides out, B slides in (L/R/U/D)<br/>- Slide: B slides over A<br/>- Cover: B expands over A<br/>- Reveal: A reveals B | 5 | P0 | VE-187 |
| VE-193 | As a C++ engine developer, I want Zoom in/out/through/cross zoom so that we have zoom transitions | - Zoom in: B scales from center<br/>- Zoom out: A scales down<br/>- Zoom through: A zooms out, B zooms in<br/>- Cross zoom: both scale | 5 | P0 | VE-187 |
| VE-194 | As a C++ engine developer, I want Blur dissolve + directional blur wipe so that we have blur-based transitions | - Blur dissolve: blur both, crossfade<br/>- Directional blur wipe: blur in direction of wipe<br/>- GPU shader | 5 | P1 | VE-099, VE-187 |
| VE-195 | As a C++ engine developer, I want RGB split + pixel sort + block glitch + VHS glitch so that we have glitch transitions | - RGB split: channel offset by progress<br/>- Pixel sort: sort pixels by value in region<br/>- Block glitch: block displacement<br/>- VHS: scanlines, tracking error | 8 | P1 | VE-187 |
| VE-196 | As a C++ engine developer, I want Light leak + lens flare wipe + flash white so that we have light-based transitions | - Light leak: overlay gradient/texture<br/>- Lens flare: flare sweep<br/>- Flash white: flash to white, cut to B | 5 | P1 | VE-187 |
| VE-197 | As a C++ engine developer, I want Luma fade + color fade + gradient wipe so that we have luma-based transitions | - Luma fade: fade based on luminance<br/>- Color fade: fade through color<br/>- Gradient wipe: custom gradient as mask | 5 | P0 | VE-187 |
| VE-198 | As a C++ engine developer, I want Page curl + fold + cube rotate + flip so that we have geometric 3D-style transitions | - Page curl: corner peel<br/>- Fold: fold along axis<br/>- Cube rotate: cube face rotation<br/>- Flip: card flip | 8 | P1 | VE-187 |
| VE-199 | As a C++ engine developer, I want Custom transition GLSL shader API so that plugins can add transitions | - TransitionDefinition with custom shader path<br/>- Shader receives A, B, progress, params<br/>- Plugin registration | 5 | P1 | VE-185 |
| VE-200 | As a C++ engine developer, I want Transition parameter animation support so that we can animate transition parameters | - TransitionInstance parameters keyframeable<br/>- evaluate_params(time) for animated params<br/>- Integrates with KeyframeTrack | 5 | P1 | VE-186, VE-114 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
