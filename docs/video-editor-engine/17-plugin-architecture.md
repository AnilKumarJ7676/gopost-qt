## VE-17. Plugin and Extension Architecture

### 17.1 Plugin SDK

```cpp
namespace gp::plugin {

// Plugin types
enum class PluginType {
    Effect,         // Video/image effect (GPU shader + parameters)
    Transition,     // Custom transition
    Generator,      // Procedural content generator
    AudioEffect,    // Audio processing effect
    ImportFormat,   // Custom media format importer
    ExportFormat,   // Custom media format exporter
    AIModel,        // Custom AI model for processing
};

struct PluginManifest {
    std::string id;              // "com.developer.plugin_name"
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    PluginType type;
    std::string min_engine_version;
    std::vector<std::string> platform_support; // "ios", "macos", "android", "windows"
};

// Plugin interface that all plugins implement
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual const PluginManifest& manifest() const = 0;
    virtual bool initialize(const PluginHostAPI& host) = 0;
    virtual void shutdown() = 0;
};

// Effect plugin interface
class IEffectPlugin : public IPlugin {
public:
    virtual EffectDefinition definition() const = 0;
    virtual void process(IGpuContext& gpu,
                        GpuTexture input,
                        GpuTexture output,
                        const ParameterMap& params,
                        Rational time,
                        const RenderContext& ctx) = 0;
};

// Host API exposed to plugins (sandboxed)
struct PluginHostAPI {
    void (*log)(LogLevel level, const char* message);
    void* (*allocate)(size_t size);
    void (*deallocate)(void* ptr);
    GpuTexture (*create_temp_texture)(int width, int height, PixelFormat format);
    void (*release_temp_texture)(GpuTexture tex);
    bool (*compile_shader)(const char* source, ShaderStage stage, ShaderModule* out);
};

class PluginManager {
public:
    void scan_plugins(const std::string& plugin_directory);
    bool load_plugin(const std::string& plugin_id);
    void unload_plugin(const std::string& plugin_id);
    std::vector<PluginManifest> available_plugins() const;
    IPlugin* get_plugin(const std::string& plugin_id) const;
};

} // namespace gp::plugin
```

### 17.2 Custom Shader Effects (User-Created)

Users and developers can create custom GPU effects using a simplified GLSL dialect:

```glsl
// Example: Custom RGB Split effect
#pragma gopost_effect
#pragma name "RGB Split"
#pragma category "Stylize"
#pragma param float split_amount 10.0 0.0 100.0 "Split Amount"
#pragma param float angle 0.0 -180.0 180.0 "Angle"

uniform sampler2D u_input;
uniform vec2 u_resolution;
uniform float split_amount;
uniform float angle;

vec4 effect(vec2 uv) {
    float rad = radians(angle);
    vec2 dir = vec2(cos(rad), sin(rad)) * split_amount / u_resolution;
    float r = texture(u_input, uv + dir).r;
    float g = texture(u_input, uv).g;
    float b = texture(u_input, uv - dir).b;
    float a = texture(u_input, uv).a;
    return vec4(r, g, b, a);
}
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 7: Plugin & Polish |
| **Sprint(s)** | VE-Sprint 19-20 (Weeks 37-40) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [16-project-serialization](16-project-serialization.md) |
| **Successor** | [18-platform-integration](18-platform-integration.md) |
| **Story Points Total** | 68 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-276 | As a C++ engine developer, I want PluginManifest data model so that plugins are self-describing | - id, name, version, author, description<br/>- PluginType enum, min_engine_version<br/>- platform_support array | 3 | P0 | — |
| VE-277 | As a C++ engine developer, I want IPlugin interface (manifest, initialize, shutdown) so that all plugins follow a common lifecycle | - manifest() returns PluginManifest<br/>- initialize(PluginHostAPI) returns bool<br/>- shutdown() cleans up resources | 3 | P0 | VE-276 |
| VE-278 | As a C++ engine developer, I want IEffectPlugin interface (definition, process) so that video effects can be implemented as plugins | - definition() returns EffectDefinition<br/>- process(gpu, input, output, params, time, ctx)<br/>- Integrates with effect graph | 5 | P0 | VE-277 |
| VE-279 | As a C++ engine developer, I want PluginHostAPI (log, allocate, deallocate, create_temp_texture, compile_shader) so that plugins have safe host services | - log, allocate, deallocate functions<br/>- create_temp_texture, release_temp_texture<br/>- compile_shader for custom shaders | 5 | P0 | VE-277 |
| VE-280 | As a C++ engine developer, I want PluginManager (scan, load, unload, list) so that plugins can be discovered and managed | - scan_plugins(directory)<br/>- load_plugin(id), unload_plugin(id)<br/>- available_plugins(), get_plugin(id) | 5 | P0 | VE-277 |
| VE-281 | As a C++ engine developer, I want plugin sandbox isolation so that plugins cannot crash the host | - Memory limits for plugin allocations<br/>- No direct system calls<br/>- Timeout for long-running process() | 5 | P0 | VE-279 |
| VE-282 | As a C++ engine developer, I want custom GLSL shader effect parsing (#pragma directives) so that users can create effects | - Parse #pragma gopost_effect, name, category<br/>- #pragma param for parameters (type, default, min, max, label)<br/>- Valid GLSL output | 5 | P0 | VE-278 |
| VE-283 | As a C++ engine developer, I want shader parameter extraction from pragmas so that effect UI is auto-generated | - Extract param name, type, default, range<br/>- Generate ParameterMap from pragmas<br/>- Validation of param values | 3 | P1 | VE-282 |
| VE-284 | As a C++ engine developer, I want custom shader compilation and caching so that user shaders perform well | - Compile GLSL to platform shader (MSL/SPIR-V)<br/>- Cache compiled shaders by hash<br/>- Recompile on source change | 5 | P0 | VE-282 |
| VE-285 | As a C++ engine developer, I want transition plugin interface so that custom transitions can be added | - ITransitionPlugin extends IPlugin<br/>- transition(progress, frame_a, frame_b, output, params)<br/>- Duration and alignment support | 5 | P1 | VE-278 |
| VE-286 | As a C++ engine developer, I want generator plugin interface so that procedural content can be created | - IGeneratorPlugin extends IPlugin<br/>- generate(time, output, params)<br/>- Resolution and format support | 3 | P2 | VE-278 |
| VE-287 | As a C++ engine developer, I want audio effect plugin interface so that custom audio processing is possible | - IAudioEffectPlugin extends IPlugin<br/>- process(audio_buffer, params, sample_rate)<br/>- Real-time latency constraints | 5 | P2 | VE-277 |
| VE-288 | As a C++ engine developer, I want plugin hot-reload for development so that iteration is fast | - Unload and reload plugin without restart<br/>- Replace effect instances with new version<br/>- Development-mode only | 5 | P2 | VE-280 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed

---
