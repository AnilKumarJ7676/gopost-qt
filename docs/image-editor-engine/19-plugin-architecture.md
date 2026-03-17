
## IE-19. Plugin Architecture

### 19.1 Plugin System Overview

The image editor plugin system extends the shared video editor plugin architecture (VE-17) with image-specific extension points.

```cpp
namespace gp::plugin_ie {

// Plugin types supported
enum class PluginType {
    Filter,             // Custom image filter (GPU shader or CPU)
    Adjustment,         // Custom adjustment type
    BrushTip,           // Custom brush tip shape
    BrushDynamic,       // Custom brush dynamic behavior
    ExportFormat,       // Custom export format encoder
    ImportFormat,       // Custom import format decoder
    TextEffect,         // Custom text effect
    ShapePreset,        // Custom shape preset
    TemplateElement,    // Custom template element type
    AIModel,            // Custom AI model integration
    ColorEffect,        // Custom color grading effect
};

struct PluginManifest {
    std::string id;                // "com.example.my-filter"
    std::string name;              // "My Amazing Filter"
    std::string version;           // "1.2.0"
    std::string author;
    std::string description;
    PluginType type;
    std::string min_engine_version;
    std::vector<std::string> capabilities;   // Required GPU features
    std::string entry_point;                 // DLL/dylib symbol or shader path
};

// Plugin SDK interface
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual const PluginManifest& manifest() const = 0;
    virtual bool initialize(IGpuContext& gpu) = 0;
    virtual void shutdown() = 0;
};

// Filter plugin interface
class IFilterPlugin : public IPlugin {
public:
    virtual std::vector<ParameterDef> parameters() const = 0;
    virtual GpuTexture process(IGpuContext& gpu,
                                GpuTexture input,
                                const std::unordered_map<std::string, ParameterValue>& params,
                                const RenderContext& ctx) = 0;
    virtual GpuTexture preview(IGpuContext& gpu,
                                GpuTexture input,
                                const std::unordered_map<std::string, ParameterValue>& params) = 0;
};

// Brush plugin interface
class IBrushPlugin : public IPlugin {
public:
    virtual brush::BrushTip get_tip() const = 0;
    virtual brush::BrushDynamics get_dynamics() const = 0;
    virtual void stamp(IGpuContext& gpu, GpuTexture target,
                        Vec2 position, float pressure, float tilt,
                        const brush::BrushPreset& preset) = 0;
};

} // namespace gp::plugin_ie
```

### 19.2 Shader-Based Filter Plugins

```cpp
namespace gp::plugin_ie {

// Simplest plugin type: a single GPU shader with parameters
struct ShaderFilterPlugin {
    std::string id;
    std::string name;
    std::string category;
    std::vector<ParameterDef> parameters;

    // Shader source (GLSL compute)
    std::string shader_source;
    // OR pre-compiled SPIR-V
    std::vector<uint8_t> spirv_bytecode;

    // Shader expects uniform buffer layout:
    // layout(set=0, binding=0) uniform sampler2D input_image;
    // layout(set=0, binding=1, rgba8) writeonly uniform image2D output_image;
    // layout(set=0, binding=2) uniform Params { ... };
};

// Plugin loader
class PluginManager {
public:
    static PluginManager& instance();

    // Scan and load plugins from directory
    void scan_directory(const std::string& plugin_dir);

    // Register plugin manually
    void register_plugin(std::unique_ptr<IPlugin> plugin);

    // Load a shader filter from .glsl file
    void load_shader_filter(const std::string& path);

    // Query plugins
    std::vector<const PluginManifest*> list_plugins(PluginType type = {}) const;
    IPlugin* get_plugin(const std::string& id) const;

    // Enable/disable
    void enable_plugin(const std::string& id);
    void disable_plugin(const std::string& id);

    // Plugin sandboxing
    void set_memory_limit(const std::string& id, size_t bytes);
    void set_time_limit(const std::string& id, int milliseconds);
};

} // namespace gp::plugin_ie
```

### 19.3 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 23 | Wk 43-44 | IE-241 to IE-248 | Performance, memory, plugins |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-241 | As a developer, I want plugin manifest model so that plugins declare metadata and capabilities | - PluginManifest: id, name, version, author, type<br/>- min_engine_version, capabilities, entry_point<br/>- Validation on load | 3 | Sprint 23 | — |
| IE-242 | As a developer, I want IPlugin base interface so that all plugins share a common contract | - initialize(IGpuContext), shutdown()<br/>- manifest() const<br/>- Virtual destructor | 2 | Sprint 23 | IE-241 |
| IE-243 | As a developer, I want IFilterPlugin interface so that filter plugins can process images | - parameters() returns ParameterDef vector<br/>- process() and preview() with params<br/>- GpuTexture input/output | 3 | Sprint 23 | IE-242 |
| IE-244 | As a developer, I want IBrushPlugin interface so that brush plugins can provide custom tips | - get_tip(), get_dynamics()<br/>- stamp() with position, pressure, tilt<br/>- Integrates with BrushEngine | 3 | Sprint 23 | IE-242 |
| IE-245 | As a developer, I want shader-based filter loading (.glsl) so that simple filters need no native code | - Load .glsl file as filter plugin<br/>- ParameterDef from shader uniforms<br/>- SPIR-V compilation path | 5 | Sprint 23 | IE-243 |
| IE-246 | As a developer, I want plugin manager (scan, load, enable/disable) so that plugins are discoverable | - scan_directory(), load_shader_filter()<br/>- register_plugin(), list_plugins(), get_plugin()<br/>- enable_plugin(), disable_plugin() | 5 | Sprint 23 | IE-242 |
| IE-247 | As a developer, I want plugin sandboxing (memory + time limits) so that plugins cannot crash the app | - set_memory_limit() per plugin<br/>- set_time_limit() per plugin<br/>- Enforcement and graceful failure | 5 | Sprint 23 | IE-246 |
| IE-248 | As a developer, I want plugin SDK documentation so that third parties can build plugins | - API reference for IPlugin, IFilterPlugin, IBrushPlugin<br/>- Shader filter authoring guide<br/>- Manifest schema and examples | 3 | Sprint 23 | IE-246 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 8: Export, Polish & Launch |
| **Sprint(s)** | IE-Sprint 23-24 (Weeks 43-46) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [17-export-pipeline](17-export-pipeline.md) |
| **Successor** | [24-testing-validation](24-testing-validation.md) |
| **Story Points Total** | 50 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-291 | As a developer, I want shared plugin SDK from VE engine so that IE and VE use the same plugin contract | - IPlugin base interface<br/>- initialize(), shutdown(), manifest()<br/>- Shared PluginManifest schema | 3 | P0 | — |
| IE-292 | As a developer, I want IFilterPlugin interface (input→output texture processing) so that filter plugins can process images | - parameters() returns ParameterDef vector<br/>- process() and preview() with params<br/>- GpuTexture input/output | 5 | P0 | IE-291 |
| IE-293 | As a developer, I want IBrushPlugin interface (custom brush tip + dynamics) so that brush plugins can provide custom tips | - get_tip(), get_dynamics()<br/>- stamp() with position, pressure, tilt<br/>- Integrates with BrushEngine | 5 | P0 | IE-291 |
| IE-294 | As a developer, I want filter plugin GLSL shader support so that simple filters need no native code | - Load .glsl file as filter plugin<br/>- ParameterDef from shader uniforms<br/>- SPIR-V compilation path | 5 | P0 | IE-292 |
| IE-295 | As a developer, I want brush plugin texture support so that custom brush tips use image textures | - Custom texture in BrushTip<br/>- Load from plugin asset<br/>- Stamp with texture sampling | 3 | P0 | IE-293 |
| IE-296 | As a developer, I want plugin parameter UI description (sliders, colors, toggles) so that plugin params are configurable | - ParameterDef: type, min, max, default<br/>- Sliders, colors, toggles<br/>- UI generation from description | 5 | P0 | IE-292 |
| IE-297 | As a developer, I want plugin discovery and loading so that plugins are discoverable | - scan_directory(), load_shader_filter()<br/>- register_plugin(), list_plugins(), get_plugin()<br/>- enable_plugin(), disable_plugin() | 5 | P0 | IE-291 |
| IE-298 | As a developer, I want plugin settings persistence so that plugin state is saved | - Save/load plugin params with project<br/>- Per-plugin settings storage<br/>- Restore on load | 3 | P0 | IE-297 |
| IE-299 | As a developer, I want third-party filter marketplace integration (future) so that users can discover plugins | - Placeholder for marketplace API<br/>- Document integration points<br/>- Future roadmap item | 2 | P1 | IE-297 |
| IE-300 | As a developer, I want plugin sandboxing and validation so that plugins cannot crash the app | - set_memory_limit(), set_time_limit() per plugin<br/>- Enforcement and graceful failure<br/>- Validation on load | 5 | P0 | IE-297 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
