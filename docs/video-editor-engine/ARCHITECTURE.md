# Video Editor Engine — Architecture Diagrams

> Architecture diagrams for `libgopost_ve`, the shared C/C++ video editor engine.
> Maps to implementation documents in `docs/video-editor-engine/`.

---

## 1. Module Layout & Dependency Graph

> Ref: [02-engine-architecture.md](02-engine-architecture.md)

```mermaid
graph TD
    subgraph foundation ["Foundation Layer"]
        CORE["core/<br/>Memory · Threading · Math · Logging"]
        PLATFORM["platform/<br/>OS · GPU · HW Abstraction"]
        CACHE["cache/<br/>Frame · Thumbnail · Waveform"]
    end

    subgraph media_io ["Media I/O Layer"]
        MEDIA["media/<br/>Decode · Encode · Format I/O"]
    end

    subgraph processing ["Processing Layer"]
        EFFECTS["effects/<br/>Effect Graph · 80+ Built-ins"]
        COLOR["color/<br/>Grading · HDR · LUT"]
        AUDIO["audio/<br/>Audio Graph · Effects · Analysis"]
        TEXT["text/<br/>FreeType · HarfBuzz · Animation"]
        MOTION["motion/<br/>Shape Layers · Motion Graphics"]
        ANIM["animation/<br/>Keyframes · Curves · Expressions"]
        TRACK["tracking/<br/>Point · Planar · Stabilization"]
        MASK["masking/<br/>Roto · Bezier Masks"]
        AI["ai/<br/>ML Inference · Segmentation"]
        TRANS["transitions/<br/>40+ GPU Transitions"]
    end

    subgraph composition_layer ["Composition Layer"]
        COMP["composition/<br/>Layer Compositor · Blend Modes"]
        TIMELINE["timeline/<br/>NLE · Tracks · Clips · Edit Ops"]
        RENDER["rendering/<br/>Render Graph · GPU Pipeline"]
    end

    subgraph orchestration ["Orchestration Layer"]
        VTMPL["vtemplates/<br/>Sections · Placeholders · Smart Duration<br/>Music Engine · Brand Kit · AI Gen<br/>Auto-Edit · Batch · Smart Resize<br/>Marketplace · Collaboration"]
        PROJECT["project/<br/>Serialization · Undo/Redo · Auto-save"]
        PLUGIN["plugin/<br/>Plugin Host · SDK"]
        API["api/<br/>Public C API (FFI)"]
    end

    CORE --> PLATFORM --> CACHE
    PLATFORM --> MEDIA

    CORE --> EFFECTS & COLOR & AUDIO & TEXT & MOTION & ANIM & TRACK & MASK & AI & TRANS
    MEDIA --> EFFECTS & AUDIO

    EFFECTS & COLOR & TEXT & MOTION & ANIM & TRACK & MASK & AI & TRANS --> COMP
    AUDIO & ANIM --> TIMELINE
    COMP & TIMELINE --> RENDER --> PLATFORM

    TIMELINE & COMP --> VTMPL & PROJECT
    VTMPL --> API
    EFFECTS --> PLUGIN
    PROJECT & TIMELINE & COMP & RENDER --> API

    style CORE fill:#e1f5fe
    style API fill:#fff3e0
    style RENDER fill:#f3e5f5
```

---

## 2. Threading Architecture

> Ref: [02-engine-architecture.md](02-engine-architecture.md), [03-core-foundation.md](03-core-foundation.md)

```mermaid
graph TB
    subgraph flutter ["Flutter / Dart"]
        UI_THREAD["Main/UI Thread<br/>FFI Entry Point"]
    end

    CMD["Command Queue<br/>(Lock-free SPSC)"]
    RES["Result Queue<br/>(Lock-free SPMC)"]

    subgraph engine_threads ["Engine Threads"]
        ENG["Engine Thread<br/>⬆ Above Normal<br/>Timeline eval · Composition graph"]
        REND["Render Thread<br/>⬆⬆ High<br/>GPU commands · Submission"]
        DEC["Decode Pool (N=cores/2)<br/>Normal<br/>Video/Image decode"]
        AUD["Audio Thread<br/>⬆⬆⬆ Real-time<br/>Mixing · Playback"]
        EXP["Export Thread<br/>⬇ Below Normal<br/>Encode pipeline"]
        AI_T["AI Pool<br/>⬇⬇ Low<br/>ML inference"]
        CACHE_T["Cache Thread<br/>⬇⬇ Low<br/>Async I/O · Thumbnails"]
    end

    UI_THREAD -->|commands| CMD --> ENG
    ENG --> REND & DEC & AUD & EXP & AI_T & CACHE_T
    REND & DEC & AUD -->|frames| RES --> UI_THREAD
```

---

## 3. Timeline Data Model

> Ref: [04-timeline-engine.md](04-timeline-engine.md)

```mermaid
classDiagram
    class Timeline {
        +Rational frame_rate
        +uint32 width, height
        +ColorSpace color_space
        +uint32 sample_rate
        +vector~Track~ video_tracks
        +vector~Track~ audio_tracks
        +vector~Marker~ markers
        +Rational duration()
    }

    class Track {
        +TrackId id
        +TrackType type
        +vector~Clip~ clips
        +float volume, opacity
        +BlendMode blend_mode
        +bool locked, visible, solo, muted
    }

    class Clip {
        +ClipId id
        +ClipType type
        +TimeRange timeline_range
        +Rational source_in, source_out
        +float speed, opacity
        +BlendMode blend_mode
        +Transform2D transform
        +vector~EffectInstance~ effects
        +vector~TransitionRef~ transitions
        +vector~KeyframeTrack~ keyframe_tracks
    }

    class TrackType {
        <<enumeration>>
        Video
        Audio
        Title
        Effect
        Subtitle
    }

    class ClipType {
        <<enumeration>>
        MediaClip
        StillImage
        SolidColor
        Composition
        Generator
        Title
        AdjustmentLayer
    }

    Timeline "1" --> "*" Track
    Track "1" --> "*" Clip
    Track --> TrackType
    Clip --> ClipType
```

---

## 4. Timeline Evaluation Pipeline

> Ref: [04-timeline-engine.md](04-timeline-engine.md)

```mermaid
flowchart LR
    subgraph gather ["1. Gather"]
        PH["Playhead Position"]
        AC["Active Clips"]
        FR["Frame Requests"]
    end

    subgraph decode ["2. Decode"]
        CC["Cache Check"]
        HW["HW Decode<br/>VideoToolbox · MediaCodec · NVDEC"]
        SW["SW Decode (FFmpeg)"]
    end

    subgraph process ["3. Process"]
        CE["Clip Effects"]
        TC["Track Compositing<br/>(bottom → top)"]
        TE["Track Effects"]
        ME2["Master Effects"]
    end

    subgraph output ["4. Output"]
        PV["Preview Texture<br/>→ Flutter"]
        ENC["Encode Pipeline<br/>→ File Export"]
    end

    PH --> AC --> FR
    FR --> CC
    CC -->|miss| HW & SW
    CC -->|hit| CE
    HW & SW --> CE --> TC --> TE --> ME2
    ME2 --> PV & ENC
```

---

## 5. Composition Engine

> Ref: [05-composition-engine.md](05-composition-engine.md)

```mermaid
flowchart TB
    subgraph eval ["Evaluation"]
        SL["Sort Layers"]
        RP["Resolve Parenting"]
        EK["Evaluate Keyframes"]
        EX["Evaluate Expressions"]
    end

    subgraph per_layer ["Per-Layer Processing"]
        DC["Decode Content"]
        MK["Apply Masks"]
        FX["Apply Effects"]
        XF["Apply Transform"]
        MB["Motion Blur"]
    end

    subgraph composite ["Compositing"]
        BL["Blend onto Canvas<br/>(30+ blend modes)"]
        TM["Track Matte<br/>Alpha · Luma · Inverted"]
        AL["Adjustment Layer"]
        CL["Collapse Groups"]
    end

    subgraph out ["Output"]
        MFX["Master Effects"]
        OF["Output Frame"]
    end

    SL --> RP --> EK --> EX
    EX --> DC --> MK --> FX --> XF --> MB
    MB --> BL --> TM --> AL --> CL
    CL --> MFX --> OF
```

---

## 6. GPU Rendering Pipeline

> Ref: [06-gpu-rendering-pipeline.md](06-gpu-rendering-pipeline.md)

```mermaid
graph TB
    subgraph render_graph ["Render Graph (DAG)"]
        TP1["Pass: Decode Frame"]
        TP2["Pass: Effect Chain"]
        TP3["Pass: Composite"]
        TP4["Pass: Color Grade"]
        TP5["Pass: Output"]
        TP1 --> TP2 --> TP3 --> TP4 --> TP5
    end

    subgraph gpu_abstraction ["GPU Abstraction (IGpuContext)"]
        MTL["Metal Backend<br/>iOS · macOS"]
        VLK["Vulkan Backend<br/>Android · Windows"]
        GL["OpenGL ES Backend<br/>Fallback"]
        WGL["WebGL Backend<br/>Web (WASM)"]
    end

    subgraph shader_pipeline ["Shader Pipeline"]
        GLSL["GLSL Source"]
        SPIRV["SPIR-V (glslc)"]
        CROSS["SPIRV-Cross"]
        MSL["Metal Shading Language"]
        GLSL_ES["GLSL ES"]
    end

    TP5 -->|execute| MTL & VLK & GL & WGL
    GLSL --> SPIRV --> CROSS --> MSL & GLSL_ES
    SPIRV --> VLK
    MSL --> MTL
    GLSL_ES --> GL & WGL
```

---

## 7. Effects & Filter System

> Ref: [07-effects-filter-system.md](07-effects-filter-system.md)

```mermaid
graph LR
    subgraph registry ["EffectRegistry"]
        REG["register_effect()"]
        FIND["find() · list_by_category()"]
        PROC["get_processor()"]
    end

    subgraph categories ["Effect Categories"]
        CT["Color & Tone<br/>Brightness · Curves · LUT"]
        BS["Blur & Sharpen<br/>Gaussian · Lens · Radial"]
        DT["Distort<br/>Warp · Bulge · Ripple"]
        KM["Keying & Matte<br/>Keylight · Luma Key"]
        GN["Generate<br/>Noise · Gradient · Grid"]
        ST["Stylize<br/>Glow · Shadow · Mosaic"]
    end

    subgraph pipeline ["Processing"]
        DEF["EffectDefinition<br/>params · shaders"]
        INST["EffectInstance<br/>enabled · mix · animated"]
        GPU_P["EffectProcessor<br/>prepare → process → release"]
    end

    REG --> CT & BS & DT & KM & GN & ST
    FIND --> DEF
    DEF --> INST --> GPU_P
    PROC --> GPU_P
```

---

## 8. Audio Engine

> Ref: [11-audio-engine.md](11-audio-engine.md)

```mermaid
graph TB
    subgraph audio_graph ["Audio Graph"]
        SRC["Source Nodes<br/>Decoded audio buffers"]
        GAIN["Gain"]
        PAN["Pan"]
        EQ["Parametric EQ<br/>(8-band)"]
        COMP2["Compressor"]
        REV["Reverb / Conv. Reverb"]
        DEL["Delay"]
        LIM["Limiter"]
        MIX["Mixer Node"]
        OUT["Output Node"]
    end

    subgraph analysis ["Analysis"]
        WAVE["Waveform Generator"]
        SPEC["Spectrum (FFT)"]
        BEAT["Beat Detector<br/>BPM · Snap Points"]
        METER["Level Meter<br/>Peak · RMS · LUFS"]
    end

    SRC --> GAIN --> PAN --> EQ --> COMP2 --> REV --> DEL --> LIM --> MIX --> OUT
    SRC --> WAVE & SPEC & BEAT & METER
```

---

## 9. Media I/O & Codec Pipeline

> Ref: [15-media-io-codec.md](15-media-io-codec.md)

```mermaid
flowchart LR
    subgraph input ["Input"]
        FILE["Source File"]
        CAM["Camera Feed"]
    end

    subgraph decode ["Decode"]
        DEMUX["Demux"]
        HW_DEC["HW Decode<br/>VideoToolbox · MediaCodec · NVDEC"]
        SW_DEC["SW Decode (FFmpeg)"]
        AUD_DEC["Audio Decode"]
    end

    subgraph process2 ["Process"]
        FG["Filter Graph"]
        COMP3["Compositor"]
        TRANS2["Transitions"]
        KF["Keyframe Interpolation"]
    end

    subgraph encode ["Encode / Export"]
        HW_ENC["HW Encode<br/>VideoToolbox · MediaCodec · NVENC"]
        SW_ENC["SW Encode (x264 · x265)"]
        AUD_ENC["Audio Encode (AAC · Opus)"]
        MUX["Muxer<br/>MP4 · MOV · WebM"]
    end

    subgraph presets ["Export Presets (15+)"]
        P1["Instagram Reel"]
        P2["TikTok"]
        P3["YouTube 4K"]
        P4["ProRes HQ"]
        P5["Web H.264"]
    end

    FILE & CAM --> DEMUX --> HW_DEC & SW_DEC & AUD_DEC
    HW_DEC & SW_DEC --> FG --> COMP3 --> TRANS2 --> KF
    KF --> HW_ENC & SW_ENC
    AUD_DEC --> AUD_ENC
    HW_ENC & SW_ENC & AUD_ENC --> MUX
    MUX --> presets
```

---

## 10. Video Template System

> Ref: [25-template-system.md](25-template-system.md)

```mermaid
graph TB
    subgraph core_tmpl ["Core Template"]
        VT["VideoTemplate<br/>Timeline · Sections · Placeholders"]
        SEC["Template Sections<br/>Intro · Content · CTA · Outro"]
        PH["Placeholder System<br/>Media · Text · Audio · Logo · Grade"]
    end

    subgraph style ["Style & Audio"]
        SC["Style Controls<br/>Grade · Font · Transitions · Pacing"]
        MUS["Music Engine<br/>Beat Sync · Auto-Duck · Library"]
        PAL["Palette + Grade<br/>80+ LUT Presets · Custom Grading"]
    end

    subgraph smart ["Smart Engines"]
        DUR["Smart Duration<br/>Auto-Adapt · Beat-Aligned Cuts"]
        RSZ["Smart Resize<br/>Multi-Platform · Auto-Reframe"]
        AE["Auto-Edit Engine<br/>Scene Detect · Highlight · Beat Match"]
    end

    subgraph ai_tmpl ["AI Features"]
        AIGEN["AI Generator<br/>Text-to-Video-Template"]
        AISC["AI Scene Analysis<br/>Moment Detection · Ranking"]
        AITX["AI Text/Music<br/>Captions · Music Match"]
    end

    subgraph authoring ["Authoring & Distribution"]
        STUDIO["Creator Studio<br/>Author · Preview · Validate · Package"]
        BRAND["Video Brand Kit<br/>Intro · Outro · Lower Thirds · Grade"]
        ANIM["Animation Library<br/>80+ Text Reveals · 30+ Kinetic"]
        BATCH["Data-Driven Batch<br/>CSV/JSON → Personalized Videos"]
        MKTPL["Marketplace<br/>Publish · Search · Rate · Collections"]
        COLLAB["Collaboration<br/>Version · Share · Comment · Library"]
    end

    VT --> SEC & PH
    VT --> SC & MUS & PAL
    VT --> DUR & RSZ & AE
    VT --> AIGEN & STUDIO & BRAND & ANIM & BATCH & MKTPL & COLLAB
    AISC --> AE
    AITX --> AIGEN
    MUS --> DUR
    STUDIO --> MKTPL
    COLLAB --> MKTPL

    style VT fill:#e8f5e9
    style AIGEN fill:#e1f5fe
    style MKTPL fill:#fff3e0
    style AE fill:#f3e5f5
```

---

## 11. Public C API (FFI Boundary)

> Ref: [21-public-c-api.md](21-public-c-api.md)

```mermaid
graph LR
    subgraph dart_side ["Dart / Flutter"]
        FFI_B["ffigen Bindings"]
        ENGINE_API["GopostEngine<br/>Abstract Interface"]
    end

    subgraph c_api ["Public C API (extern C)"]
        GP_ENG["gp_engine_init / destroy"]
        GP_TMPL["gp_template_load / unload"]
        GP_TL["gp_timeline_create / destroy<br/>add_clip · render_frame · export"]
        GP_FRM["gp_frame_release<br/>gp_frame_get_texture_handle"]
    end

    subgraph handles ["Opaque Handles"]
        H1["GpEngine*"]
        H2["GpTemplate*"]
        H3["GpTimeline*"]
        H4["GpFrame*"]
    end

    FFI_B --> GP_ENG & GP_TMPL & GP_TL & GP_FRM
    ENGINE_API --> FFI_B
    GP_ENG --> H1
    GP_TMPL --> H2
    GP_TL --> H3
    GP_FRM --> H4
```

---

## 12. Development Roadmap (48 Weeks)

> Ref: [23-development-roadmap.md](23-development-roadmap.md)

```mermaid
gantt
    title Video Editor Engine — 48-Week Roadmap
    dateFormat  YYYY-MM-DD
    axisFormat  Wk %W

    section Phase 1: Core Foundation
    Memory, Math, GPU, Codecs, FFI, Build  :p1, 2026-03-02, 6w

    section Phase 2: Timeline Engine
    Timeline, Composition, Project, Undo    :p2, after p1, 6w

    section Phase 3: Effects & Color
    Effects, Keyframes, Grading, LUTs, HDR  :p3, after p2, 6w

    section Phase 4: Transitions/Text/Audio
    Transitions, Text Engine, Audio Graph   :p4, after p3, 6w

    section Phase 5: Motion Graphics
    Shape Layers, Masking, Tracking, Exprs  :p5, after p4, 6w

    section Phase 6: AI Features
    ML Inference, BG Removal, Captions      :p6, after p5, 6w

    section Phase 7: Plugin & Polish
    Plugins, Performance, Memory, Testing   :p7, after p6, 6w

    section Phase 8: Hardening & Launch
    Platform Hardening, Validation, RC      :p8, after p7, 6w
```

---

## Module Cross-Reference

```mermaid
graph TD
    VS["01 Vision & Scope"] --> EA["02 Engine Architecture"]
    EA --> CF["03 Core Foundation"]
    CF --> TL["04 Timeline"] & CO["05 Composition"] & GPU_R["06 GPU Rendering"]
    TL --> EFF["07 Effects"] & KF["08 Keyframes"] & AUD2["11 Audio"]
    CO --> MG["09 Motion Graphics"] & TXT["10 Text"]
    EFF --> AI2["13 AI Features"] & CLR["14 Color Science"]
    AUD2 --> TR2["12 Transitions"]
    GPU_R --> MIO["15 Media I/O"]
    TL & CO --> VTS["25 Video Template System"]
    TL & CO --> PS["16 Project/Serial."]
    VTS --> CAPI
    EFF --> PLG["17 Plugin"]
    GPU_R --> PLAT["18 Platform"]
    CF --> MEM["19 Memory/Perf"]
    PLAT --> BLD["20 Build System"]
    BLD --> CAPI["21 Public C API"]
    CAPI --> TST["22 Testing"]
    TST --> ROAD["23 Roadmap"]

    style VS fill:#e8f5e9
    style EA fill:#e1f5fe
    style CAPI fill:#fff3e0
```
