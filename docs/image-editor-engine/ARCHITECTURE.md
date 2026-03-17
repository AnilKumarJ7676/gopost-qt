# Image Editor Engine — Architecture Diagrams

> Architecture diagrams for `libgopost_ie`, the shared C/C++ image editor engine.
> Maps to implementation documents in `docs/image-editor-engine/`.

---

## 1. Module Layout & Dependency Graph

> Ref: [02-engine-architecture.md](02-engine-architecture.md)

```mermaid
graph TD
    subgraph foundation ["Foundation Layer (Shared with VE)"]
        CORE["core/<br/>Memory · Threading · Math · Logging"]
        PLATFORM["platform/<br/>OS · GPU · HW Abstraction"]
        CACHE["cache/<br/>Texture · Thumbnail Cache"]
    end

    subgraph io_layer ["I/O Layer"]
        IMAGE["image/<br/>RAW · Retouching · Healing · HDR"]
        VECTOR["vector/<br/>Shapes · Pen · Boolean · SVG"]
    end

    subgraph processing ["Processing Layer"]
        EFFECTS["effects/<br/>100+ Filters · Presets"]
        COLOR["color/<br/>ICC · Grading · Soft Proofing"]
        TEXT["text/<br/>FreeType · HarfBuzz · Effects"]
        BRUSH["brush/<br/>Engine · Dynamics · 50+ Presets"]
        SELECT["selection/<br/>Magic Wand · Lasso · Refine Edge"]
        MASK["masking/<br/>Layer Masks · Vector Masks"]
        XFORM["transform/<br/>Warp · Liquify · Perspective"]
        ANIM["animation/<br/>Keyframes (Shared)"]
        AI["ai/<br/>ML Inference (Shared)"]
    end

    subgraph composition_layer ["Composition Layer"]
        CANVAS["canvas/<br/>Canvas Model · Layers · Smart Objects"]
        COMP["composition/<br/>Layer Compositor · Blend Modes"]
        RENDER["rendering/<br/>Tile Renderer · GPU Pipeline"]
    end

    subgraph orchestration ["Orchestration Layer"]
        TMPL["templates/<br/>Placeholders · Palettes · Collage<br/>Creator Studio · Smart Layout · Brand Kit<br/>AI Gen · Theming · Batch · Animation<br/>Smart Resize · Marketplace · Accessibility"]
        PROJECT["project/<br/>.gpimg · Undo/Redo · Auto-save"]
        EXPORT["export/<br/>Multi-format · Batch · Print"]
        PLUGIN["plugin/<br/>Plugin Host · SDK"]
        API["api/<br/>Public C API (FFI)"]
    end

    CORE --> PLATFORM --> CACHE
    PLATFORM --> IMAGE & VECTOR

    CORE --> EFFECTS & COLOR & TEXT & BRUSH & SELECT & MASK & XFORM & ANIM & AI
    IMAGE --> EFFECTS & COLOR

    EFFECTS & COLOR & TEXT & BRUSH & SELECT & MASK & XFORM & ANIM & AI --> CANVAS
    CANVAS --> COMP --> RENDER --> PLATFORM

    CANVAS --> TMPL & PROJECT
    TMPL --> EXPORT
    EFFECTS & BRUSH --> PLUGIN
    TMPL & PROJECT & CANVAS & RENDER & EXPORT --> API

    style CORE fill:#e1f5fe
    style API fill:#fff3e0
    style RENDER fill:#f3e5f5
    style CANVAS fill:#e8f5e9
```

---

## 2. Shared vs IE-Only Modules

> Ref: [01-vision-and-scope.md](01-vision-and-scope.md), [02-engine-architecture.md](02-engine-architecture.md)

```mermaid
graph LR
    subgraph shared ["Shared Modules (gp::)"]
        S_CORE["core/"]
        S_RENDER["rendering/"]
        S_EFFECTS["effects/"]
        S_ANIM["animation/"]
        S_TEXT["text/"]
        S_COLOR["color/"]
        S_MASK["masking/"]
        S_AI["ai/"]
        S_PLATFORM["platform/"]
        S_CACHE["cache/"]
    end

    subgraph ie_only ["IE-Only Modules (gpie::)"]
        IE_CANVAS["canvas/"]
        IE_IMAGE["image/"]
        IE_VECTOR["vector/"]
        IE_BRUSH["brush/"]
        IE_SELECT["selection/"]
        IE_XFORM["transform/"]
        IE_TMPL["templates/"]
        IE_EXPORT["export/"]
        IE_PROJECT["project/"]
        IE_PLUGIN["plugin/"]
    end

    S_CORE --- IE_CANVAS & IE_IMAGE & IE_VECTOR & IE_BRUSH & IE_SELECT
    S_RENDER --- IE_CANVAS
    S_EFFECTS --- IE_CANVAS & IE_BRUSH
    S_TEXT --- IE_CANVAS
    S_COLOR --- IE_IMAGE
    S_MASK --- IE_CANVAS
    S_AI --- IE_IMAGE

    style shared fill:#e1f5fe
    style ie_only fill:#fff3e0
```

---

## 3. Threading Architecture

> Ref: [02-engine-architecture.md](02-engine-architecture.md)

```mermaid
graph TB
    subgraph flutter ["Flutter / Dart"]
        UI["Main/UI Thread<br/>FFI Entry Point"]
    end

    CMD["Command Queue<br/>(Lock-free SPSC)"]
    RES["Result Queue<br/>(Lock-free SPMC)"]

    subgraph threads ["Engine Threads"]
        ENG["Engine Thread<br/>⬆ Above Normal<br/>Canvas eval · Composition"]
        REND["Render Thread<br/>⬆⬆ High<br/>GPU commands · Tile submit"]
        TILE["Tile Worker Pool (N)<br/>Normal<br/>Parallel tile rendering"]
        DEC["Decode Thread<br/>Normal<br/>Image loading"]
        AI_T["AI Pool<br/>⬇ Low<br/>ML inference"]
        EXP["Export Thread<br/>⬇ Below Normal<br/>Encode pipeline"]
        CACHE_T["Cache Thread<br/>⬇ Low<br/>Async disk I/O"]
    end

    UI -->|commands| CMD --> ENG
    ENG --> REND & TILE & DEC & AI_T & EXP & CACHE_T
    REND & TILE -->|tiles| RES --> UI
```

---

## 4. Canvas Data Model

> Ref: [04-canvas-engine.md](04-canvas-engine.md)

```mermaid
classDiagram
    class Canvas {
        +CanvasId id
        +CanvasSettings settings
        +string name
        +vector~Layer~ layers
        +Guides guides
        +Grid grid
        +ViewportState viewport
        +add_layer()
        +remove_layer()
        +reorder_layer()
    }

    class CanvasSettings {
        +uint32 width, height
        +float dpi
        +ColorSpace color_space
        +BitDepth bit_depth
        +Color4f background_color
        +bool transparent_background
    }

    class Layer {
        +LayerId id
        +LayerType type
        +string name
        +Transform2D transform
        +float opacity
        +BlendMode blend_mode
        +bool visible, locked
        +LayerMask layer_mask
        +VectorMask vector_mask
        +bool clipping_mask
        +vector~EffectInstance~ effects
        +LayerStyle layer_style
    }

    class LayerType {
        <<enumeration>>
        Image
        SolidColor
        Gradient
        Text
        Shape
        Group
        Adjustment
        SmartObject
        Sticker
        Frame
        Pattern
        Brush
    }

    class LayerStyle {
        +DropShadow drop_shadow
        +InnerShadow inner_shadow
        +OuterGlow outer_glow
        +InnerGlow inner_glow
        +BevelEmboss bevel
        +Stroke stroke
        +ColorOverlay color_overlay
        +GradientOverlay gradient_overlay
        +PatternOverlay pattern_overlay
    }

    class MediaPool {
        +import_image()
        +import_svg()
        +import_pattern()
        +get_texture()
    }

    Canvas "1" --> "1" CanvasSettings
    Canvas "1" --> "*" Layer
    Layer --> LayerType
    Layer "1" --> "0..1" LayerStyle
    Canvas "1" --> "1" MediaPool
```

---

## 5. Tile-Based Rendering

> Ref: [02-engine-architecture.md](02-engine-architecture.md), [06-gpu-rendering-pipeline.md](06-gpu-rendering-pipeline.md)

```mermaid
flowchart TB
    subgraph canvas_space ["Canvas Space"]
        FULL["Full Canvas<br/>Up to 16384×16384"]
        TILES["Tile Grid<br/>2048×2048 tiles · 2px overlap"]
    end

    subgraph dirty ["Dirty Region Tracking"]
        INV["invalidate_region(rect)"]
        VIS["visible_tiles(viewport)"]
        DRT["dirty_tiles()"]
    end

    subgraph render_modes ["Render Modes"]
        INT["Interactive Preview<br/>Viewport resolution"]
        CACHED["Cached Preview<br/>1/4 canvas"]
        FULL_Q["Full Quality<br/>Native resolution"]
        PROXY["Proxy<br/>Max 2048px"]
    end

    subgraph tile_render ["Tile Rendering"]
        WORKER["Tile Worker Pool"]
        GPU_TILE["GPU Execute per Tile"]
        ASSEMBLE["Tile Assembly"]
    end

    FULL --> TILES
    TILES --> INV & VIS & DRT
    DRT --> WORKER --> GPU_TILE --> ASSEMBLE
    ASSEMBLE --> INT & CACHED & FULL_Q & PROXY
```

---

## 6. Composition Pipeline

> Ref: [05-composition-engine.md](05-composition-engine.md)

```mermaid
flowchart TB
    subgraph eval ["1. Evaluation"]
        FT["Flatten Layer Tree"]
        RC["Resolve Clipping Groups"]
        EK["Evaluate Keyframes"]
    end

    subgraph per_layer ["2. Per-Layer"]
        DR["Decode / Rasterize"]
        LM["Layer Mask"]
        VM["Vector Mask"]
        FX["Effect Chain"]
        LS["Layer Style<br/>Shadow · Glow · Bevel · Stroke"]
        XF["Transform"]
    end

    subgraph compositing ["3. Compositing"]
        BL["Blend (30+ modes)"]
        CM["Clipping Mask"]
        GC["Group Composite<br/>Pass-Through / Isolated"]
        AL["Adjustment Layers"]
    end

    subgraph output ["4. Output"]
        MFX["Master Effects"]
        TA["Tile Assembly"]
        OUT["Output Texture"]
    end

    FT --> RC --> EK
    EK --> DR --> LM --> VM --> FX --> LS --> XF
    XF --> BL --> CM --> GC --> AL
    AL --> MFX --> TA --> OUT
```

---

## 7. Effects & Filter System (100+ Filters)

> Ref: [07-effects-filter-system.md](07-effects-filter-system.md)

```mermaid
graph TB
    subgraph filter_library ["Filter Preset Library"]
        NAT["Natural (8)<br/>Daylight · Golden Hour · Ocean"]
        PORT["Portrait (8)<br/>Soft Skin · Studio · Dramatic"]
        VINT["Vintage (12)<br/>Polaroid · Kodachrome · Cross Process"]
        CINE["Cinematic (8)<br/>Teal & Orange · Noir · Matrix"]
        BW["B&W (8)<br/>Classic · Selenium · Infrared"]
        MOOD["Mood (8)<br/>Dreamy · Cyberpunk · Vaporwave"]
    end

    subgraph ie_effects ["Image-Specific Effects"]
        CLR["Clarity · Dehaze · Texture"]
        REC["Shadow/Highlight Recovery"]
        SPL["Split Toning · Duotone"]
        ART["Oil Paint · Watercolor · Sketch"]
        OVL["Bokeh · Light Leak · Lens Flare"]
        GLT["Glitch · Pixelate · Halftone"]
    end

    subgraph apply_modes ["Application Modes"]
        LE["LayerEffect<br/>(Non-destructive)"]
        AJ["AdjustmentLayer<br/>(Affects below)"]
        DA["DestructiveApply<br/>(Rasterized)"]
        BA["BrushApplied<br/>(Selective)"]
        PV["Preview<br/>(Temporary)"]
    end

    filter_library --> apply_modes
    ie_effects --> apply_modes
```

---

## 8. Template System

> Ref: [11-template-system.md](11-template-system.md)

```mermaid
graph TB
    subgraph template ["ImageTemplate"]
        META["Metadata<br/>name · category · tags · author"]
        SETTINGS["CanvasSettings<br/>dimensions · DPI · color space"]
        LAYERS["Layer Definitions<br/>Full canvas layer tree"]
    end

    subgraph placeholders ["Placeholder System"]
        PH_TEXT["Text Placeholder<br/>hint · max chars · font control"]
        PH_IMG["Image Placeholder<br/>aspect ratio · fit mode · frame"]
        PH_LOGO["Logo Placeholder<br/>placement · preserve aspect"]
        PH_CLR["Color Placeholder<br/>affected layers · mapping"]
    end

    subgraph controls ["Style Controls"]
        SC_CLR["Color Control<br/>palette swaps"]
        SC_FONT["Font Control<br/>font alternatives"]
        SC_LAY["Layout Variants<br/>transform overrides"]
        SC_SLIDE["Slider Control<br/>property mapping"]
    end

    subgraph palette ["Palette System"]
        BUILTIN["200+ Built-in Palettes"]
        AUTO["Auto-generate from Image"]
        APPLY["apply_to_template()"]
    end

    subgraph collage ["Collage Engine"]
        GRID["Grid Layout"]
        MOSAIC["Mosaic Layout"]
        FREE["Freeform Layout"]
        MAG["Magazine Layout"]
    end

    template --> placeholders & controls & palette & collage

    subgraph format [".gpit Encrypted Format"]
        HDR["Header: GPIT · version"]
        PAY["AES-256-GCM Payload"]
        SIG["Ed25519 Signature"]
    end

    template --> format
```

### 8b. Extended Template Subsystems

> Ref: [11-template-system.md](11-template-system.md) — Sections 11.8–11.19

```mermaid
graph TB
    TMPL["ImageTemplate<br/>Core Data Model"]

    subgraph authoring ["Creator Studio (11.8)"]
        STUDIO["TemplateCreatorStudio<br/>Author · Promote · Constrain · Preview · Package"]
    end

    subgraph layout_engine ["Smart Layout (11.9)"]
        LAYOUT["SmartLayoutEngine<br/>Constraints · Auto-Layout Groups<br/>Reflow · Snap · Align"]
    end

    subgraph brand ["Brand Kit (11.10)"]
        BKIT["BrandKitManager<br/>Logos · Colors · Fonts · Voice<br/>Apply · Extract · Audit"]
    end

    subgraph ai_gen ["AI Generation (11.11)"]
        AIGEN["AITemplateGenerator<br/>Text-to-Template · Layout AI<br/>Color AI · Font AI · Copywriting<br/>Background Gen · Design Critique"]
    end

    subgraph smart_content ["Smart Content (11.12)"]
        SCONT["SmartContentReplacer<br/>Face-Aware Crop · Subject Detection<br/>Color Adaptation · Contrast Fix<br/>Auto Text Fit"]
    end

    subgraph theming ["Theming Engine (11.13)"]
        THEME["TemplateThemeEngine<br/>100+ Themes · Light/Dark<br/>Seasonal · Custom · Preview"]
    end

    subgraph batch ["Data-Driven / Batch (11.14)"]
        DDT["DataDrivenTemplateEngine<br/>CSV/JSON/Sheets/API<br/>Field Mapping · Batch Merge<br/>Preview · Validate"]
    end

    subgraph animation ["Animated Templates (11.15)"]
        ANIM["AnimatedTemplateEngine<br/>Entrance/Exit/Emphasis Anims<br/>Page Transitions · Timeline<br/>Export GIF/MP4/WebM/APNG/Lottie"]
    end

    subgraph resize ["Smart Resize (11.16)"]
        SRSZ["SmartResizeEngine<br/>Multi-Platform Adapt<br/>Content-Aware Reposition<br/>Face-Preserving Re-Crop"]
    end

    subgraph marketplace ["Marketplace (11.17)"]
        MKTPL["TemplatePublisher + TemplateStore<br/>Submit · Review · Publish · Version<br/>Search · Trending · Collections"]
    end

    subgraph a11y ["Accessibility (11.18)"]
        ACCS["TemplateAccessibilityEngine<br/>Contrast Audit · Colorblind Sim<br/>Alt Text · Reading Order<br/>Typography Check"]
    end

    subgraph collab ["Versioning & Collab (11.19)"]
        VCOL["VersionControl + Collaboration<br/>Save/Restore · Diff · Auto-Save<br/>Share · Comments · Layer Lock"]
    end

    TMPL --> STUDIO & LAYOUT & BKIT
    TMPL --> AIGEN & SCONT & THEME
    TMPL --> DDT & ANIM & SRSZ
    TMPL --> MKTPL & ACCS & VCOL

    STUDIO --> LAYOUT
    LAYOUT --> SRSZ
    BKIT --> THEME
    AIGEN --> SCONT
    SCONT --> ACCS
    STUDIO --> MKTPL
    VCOL --> MKTPL

    style TMPL fill:#e8f5e9
    style AIGEN fill:#e1f5fe
    style MKTPL fill:#fff3e0
    style ACCS fill:#f3e5f5
```

---

## 9. Brush Engine

> Ref: [16-brush-engine.md](16-brush-engine.md)

```mermaid
graph TB
    subgraph brush_system ["Brush System"]
        TIP["BrushTip<br/>shape · size · hardness · spacing"]
        DYN["BrushDynamics<br/>pressure · tilt · velocity · random"]
        PRESET["BrushPreset<br/>50+ built-in presets"]
    end

    subgraph tools ["Brush Tools (14)"]
        PAINT["Brush · Pencil · Eraser"]
        MIXER["Mixer Brush · Smudge"]
        CLONE["Clone Stamp · Healing Brush"]
        RETOUCH["Dodge · Burn · Sponge"]
        ADJUST["Blur · Sharpen"]
        PATTERN["Pattern Stamp · History Brush"]
    end

    subgraph pipeline ["Stroke Pipeline"]
        INPUT["Input Point<br/>x · y · pressure · tilt"]
        INTERP["Catmull-Rom Interpolation"]
        STAMP["Stamp Compositing"]
        ACCUM["Accumulation Buffer"]
        MERGE["Merge to Layer"]
    end

    subgraph pressure_sim ["Pressure Simulation"]
        CONST["Constant"]
        VEL["Velocity-Based"]
        DIST["Distance-Based"]
        TIME["Time-Based"]
    end

    brush_system --> tools
    tools --> pipeline
    INPUT --> INTERP --> STAMP --> ACCUM --> MERGE
    pressure_sim -.-> INPUT
```

---

## 10. Public C API (FFI Boundary)

> Ref: [23-public-c-api.md](23-public-c-api.md)

```mermaid
graph LR
    subgraph dart_side ["Dart / Flutter"]
        FFI_B["ffigen Bindings"]
        IE_API["ImageEditorEngine<br/>Abstract Interface"]
    end

    subgraph c_api ["Public C API (extern C)"]
        GP_ENG["gpie_engine_init / destroy"]
        GP_TMPL["gpie_template_load / unload"]
        GP_CV["gpie_canvas_create / destroy<br/>add_layer · render"]
        GP_BRUSH["gpie_brush_begin / continue / end"]
        GP_EXP["gpie_export_start / cancel"]
    end

    subgraph handles ["Opaque Handles"]
        H1["GpIEEngine*"]
        H2["GpIECanvas*"]
        H3["GpIELayer*"]
        H4["GpIEBrush*"]
    end

    FFI_B --> GP_ENG & GP_TMPL & GP_CV & GP_BRUSH & GP_EXP
    IE_API --> FFI_B
    GP_ENG --> H1
    GP_CV --> H2
    GP_CV --> H3
    GP_BRUSH --> H4
```

---

## 11. Development Roadmap (48 Weeks)

> Ref: [25-development-roadmap.md](25-development-roadmap.md)

```mermaid
gantt
    title Image Editor Engine — 48-Week Roadmap
    dateFormat  YYYY-MM-DD
    axisFormat  Wk %W

    section Phase 1: Core Foundation
    Shared core, Canvas, Media Pool, Tiles, FFI  :p1, 2026-03-02, 5w

    section Phase 2: Layer & Compositing
    Layers, Blend modes, Masks, Styles, Undo     :p2, after p1, 5w

    section Phase 3: Effects & Filters
    Adjustments, 100+ Filters, LUTs, Color       :p3, after p2, 6w

    section Phase 4: Text & Vector
    Text Engine, Shapes, Pen Tool, SVG, Booleans  :p4, after p3, 6w

    section Phase 5: Template System
    Templates, Placeholders, Palettes, Collage    :p5, after p4, 6w

    section Phase 6: Selection & Brush
    Selection Tools, Brush Engine, Retouching     :p6, after p5, 6w

    section Phase 7: Transform & AI
    Warp, Liquify, AI Removal/Upscale, RAW        :p7, after p6, 6w

    section Phase 8: Export & Launch
    Export Pipeline, Plugins, Perf, Testing        :p8, after p7, 8w
```

---

## Module Cross-Reference

```mermaid
graph TD
    VS["01 Vision & Scope"] --> EA["02 Engine Architecture"]
    EA --> CF["03 Core Foundation"]
    CF --> CV["04 Canvas Engine"]
    CV --> CO["05 Composition"] & GPU_R["06 GPU Rendering"]
    CO --> EFF["07 Effects/Filters"] & IMG["08 Image Processing"]
    CV --> TXT["09 Text"] & VEC["10 Vector Graphics"]
    CV --> TS["11 Template System<br/>(+12 extended subsystems)"]
    CO --> MSK["12 Masking/Selection"]
    EFF --> CLR["13 Color Science"]
    CLR --> AI2["14 AI Features"]
    CV --> XF["15 Transform/Warp"]
    CV --> BR["16 Brush Engine"]
    TS --> EXP["17 Export Pipeline"]
    CV --> PS["18 Project/Serial."]
    EFF & BR --> PLG["19 Plugin"]
    GPU_R --> PLAT["20 Platform"]
    CF --> MEM["21 Memory/Perf"]
    PLAT --> BLD["22 Build System"]
    BLD --> CAPI["23 Public C API"]
    CAPI --> TST["24 Testing"]
    TST --> ROAD["25 Roadmap"]

    style VS fill:#e8f5e9
    style EA fill:#e1f5fe
    style CAPI fill:#fff3e0
    style CV fill:#fff8e1
```
