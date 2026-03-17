
## IE-20. Platform Integration

### 20.1 Platform Abstraction (Shared with VE)

The image editor shares the complete platform abstraction layer from the video editor:
- **IPlatformContext** — OS-level abstraction (file I/O, threading, system info)
- **IGpuContext** — GPU backend selection (Metal, Vulkan, GLES, DX12)
- **Thermal monitoring** — Thermal throttling and quality reduction
- **Memory pressure handling** — OS memory warnings, cache eviction

### 20.2 Per-Platform Details

#### 20.2.1 iOS

| Component | Implementation |
|---|---|
| GPU Backend | Metal 3 (primary), Metal 2 (fallback) |
| Image I/O | ImageIO framework + libraw (RAW) + libjpeg-turbo + libpng |
| ML Inference | CoreML + Vision framework |
| Pressure Input | Apple Pencil (force + tilt + azimuth) via UITouch |
| Color Management | ColorSync framework, Display P3 native |
| Share Sheet | UIActivityViewController integration |
| Photo Library | PHPhotoLibrary for save + import |
| iCloud | NSFileProviderManager for document sync |
| Haptics | UIImpactFeedbackGenerator for tool interactions |
| Memory Limit | 384 MB engine budget (monitor with os_proc_available_memory) |

```cpp
namespace gp::platform::ios {

class IOSPlatform : public IPlatformContext {
public:
    void* create_metal_layer(void* ui_view) override;
    void handle_memory_warning() override;  // Evict caches, reduce quality

    // Apple Pencil input
    struct PencilInput {
        Vec2 position;
        float force;           // 0-1 (maps to brush pressure)
        float azimuth;         // Radians
        float altitude;        // Radians (tilt)
        bool is_pencil;        // true = Pencil, false = finger
        int touch_type;        // Direct, Pencil, IndirectPointer
    };
    PencilInput process_touch(void* ui_touch);

    // Photo library integration
    void save_to_photos(const std::string& image_path,
                         std::function<void(bool)> completion);
    void pick_from_photos(std::function<void(std::string)> completion);
};

} // namespace gp::platform::ios
```

#### 20.2.2 macOS

| Component | Implementation |
|---|---|
| GPU Backend | Metal 3 (Apple Silicon), Metal 2 (Intel) |
| Image I/O | ImageIO framework + libraw + libjpeg-turbo + libpng |
| ML Inference | CoreML + Vision framework, ANE (Apple Neural Engine) |
| Pressure Input | NSEvent pressure (Force Touch trackpad), Wacom tablet via NSTabletPoint |
| Color Management | ColorSync, ICC profile support |
| File System | NSOpenPanel/NSSavePanel, Sandbox-compliant |
| Memory Limit | 1.5 GB engine budget |
| Window Management | Multiple windows (document per window) |

#### 20.2.3 Android

| Component | Implementation |
|---|---|
| GPU Backend | Vulkan 1.1 (primary), OpenGL ES 3.2 (fallback) |
| Image I/O | BitmapFactory + libraw + libjpeg-turbo + libpng |
| ML Inference | NNAPI (Android NN), TensorFlow Lite |
| Pressure Input | MotionEvent (S-Pen on Samsung: pressure + tilt) |
| Color Management | ColorSpace API (Android 8.0+), ICC profiles |
| Share Intent | Intent.ACTION_SEND for sharing |
| Media Store | MediaStore API for gallery integration |
| Memory Limit | 384 MB engine budget (ActivityManager.getMemoryClass) |

```cpp
namespace gp::platform::android {

class AndroidPlatform : public IPlatformContext {
public:
    void* create_vulkan_surface(void* android_window) override;
    void handle_low_memory() override;

    // Stylus input (Samsung S-Pen, etc.)
    struct StylusInput {
        Vec2 position;
        float pressure;        // 0-1
        float tilt_x, tilt_y;  // -1 to 1
        int tool_type;         // Finger, Stylus, Eraser
        int button_state;      // Stylus button pressed
    };
    StylusInput process_motion_event(void* motion_event);

    // GPU capability detection
    bool has_vulkan_1_1() const;
    bool has_compute_shaders() const;
    int max_texture_size() const;

    // Thermal management
    float thermal_headroom() const;   // Android 11+ PowerManager
    void reduce_quality_on_thermal();
};

} // namespace gp::platform::android
```

#### 20.2.4 Windows

| Component | Implementation |
|---|---|
| GPU Backend | Vulkan 1.2 (primary), DirectX 12 (secondary), OpenGL 4.6 (fallback) |
| Image I/O | WIC (Windows Imaging Component) + libraw + libjpeg-turbo + libpng |
| ML Inference | ONNX Runtime (DirectML backend for GPU) |
| Pressure Input | Windows Ink API (pen pressure + tilt + rotation) |
| Color Management | WCS (Windows Color System), ICC profiles |
| File System | IFileDialog (Open/Save) |
| Memory Limit | 1.5 GB engine budget |
| Multi-Monitor | DPI-aware, per-monitor scaling |

```cpp
namespace gp::platform::windows {

class WindowsPlatform : public IPlatformContext {
public:
    void* create_vulkan_surface(void* hwnd) override;

    // Windows Ink / Pen input
    struct PenInput {
        Vec2 position;
        float pressure;
        float tilt_x, tilt_y;
        float rotation;        // Pen barrel rotation
        bool is_inverted;      // Eraser end
        int button_state;      // Pen buttons
    };
    PenInput process_pen_event(void* pointer_info);

    // GPU vendor detection
    GpuVendor detect_gpu_vendor() const;  // NVIDIA, AMD, Intel
    bool has_hardware_raytracing() const;

    // DPI handling
    float get_monitor_dpi(void* hwnd) const;
    float get_monitor_scale_factor(void* hwnd) const;
};

enum class GpuVendor { NVIDIA, AMD, Intel, Qualcomm, Unknown };

} // namespace gp::platform::windows
```

### 20.3 Flutter FFI Bridge Configuration

```dart
// rendering_bridge/image_engine_api.dart

abstract class GopostImageEngine {
  Future<void> initialize();
  Future<void> dispose();

  // Canvas operations
  Future<int> createCanvas(CanvasConfig config);
  Future<void> destroyCanvas(int canvasId);
  Future<void> resizeCanvas(int canvasId, int width, int height);

  // Layer operations
  Future<int> addLayer(int canvasId, LayerDescriptor layer);
  Future<void> removeLayer(int canvasId, int layerId);
  Future<void> reorderLayer(int canvasId, int layerId, int newIndex);
  Future<void> updateLayerTransform(int canvasId, int layerId, TransformData transform);
  Future<void> updateLayerProperty(int canvasId, int layerId, String property, dynamic value);

  // Effect operations
  Future<int> addEffect(int canvasId, int layerId, String effectId);
  Future<void> removeEffect(int canvasId, int layerId, int effectInstanceId);
  Future<void> setEffectParam(int canvasId, int layerId, int effectInstanceId,
                               String paramId, dynamic value);

  // Rendering
  Stream<RenderFrame> canvasPreviewStream(int canvasId);
  Future<void> invalidateCanvas(int canvasId);

  // Export
  Future<ExportResult> exportImage(int canvasId, ExportConfig config);

  // Template operations
  Future<int> loadTemplate(Uint8List encryptedBlob, Uint8List sessionKey);
  Future<void> setPlaceholder(int canvasId, String placeholderId, dynamic value);
  Future<List<PlaceholderInfo>> getPlaceholders(int canvasId);

  // Tools
  Future<void> beginBrushStroke(int canvasId, int layerId, BrushConfig brush, Offset position);
  Future<void> continueBrushStroke(Offset position, double pressure);
  Future<void> endBrushStroke();

  // Selection
  Future<void> magicWandSelect(int canvasId, Offset point, int tolerance);
  Future<void> modifySelection(SelectionOperation op);

  // Undo/Redo
  Future<void> undo(int canvasId);
  Future<void> redo(int canvasId);
  Future<bool> canUndo(int canvasId);
  Future<bool> canRedo(int canvasId);

  // GPU info
  Future<GpuCapabilities> queryGpuCapabilities();
}
```

### 20.4 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 1 | Wk 1-2 | IE-249 to IE-253 | iOS, macOS platform impl |
| Sprint 2 | Wk 3-4 | IE-254 to IE-256 | Android platform impl |
| Sprint 3 | Wk 5-6 | IE-257 to IE-259 | Windows platform impl |
| Sprint 24 | Wk 47-48 | IE-260 to IE-262 | Flutter FFI, texture registry, capability detection |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-249 | As a developer, I want iOS platform impl (Metal layer, memory warning) so that the editor runs on iPhone/iPad | - Metal layer creation from UIView<br/>- handle_memory_warning() evicts caches<br/>- 384 MB engine budget | 5 | Sprint 1 | — |
| IE-250 | As a user, I want iOS Apple Pencil input so that I can draw with pressure and tilt | - PencilInput: position, force, azimuth, altitude<br/>- process_touch() for UITouch<br/>- is_pencil detection | 5 | Sprint 1 | IE-249 |
| IE-251 | As a user, I want iOS Photo Library integration so that I can save and import photos | - save_to_photos() with completion<br/>- pick_from_photos() with callback<br/>- PHPhotoLibrary integration | 3 | Sprint 1 | IE-249 |
| IE-252 | As a developer, I want macOS platform impl (Wacom tablet) so that the editor runs on Mac with tablet support | - Metal backend (Apple Silicon + Intel)<br/>- NSTabletPoint for Wacom pressure/tilt<br/>- 1.5 GB engine budget | 5 | Sprint 1 | — |
| IE-253 | As a developer, I want macOS color management (ColorSync) so that colors are accurate on Mac | - ColorSync framework integration<br/>- ICC profile support<br/>- Display calibration awareness | 3 | Sprint 1 | IE-252 |
| IE-254 | As a developer, I want Android platform impl (Vulkan surface) so that the editor runs on Android | - Vulkan surface from Android window<br/>- handle_low_memory()<br/>- 384 MB engine budget | 5 | Sprint 2 | — |
| IE-255 | As a user, I want Android stylus input (S-Pen) so that I can draw with pressure and tilt | - StylusInput: position, pressure, tilt_x/y, tool_type<br/>- process_motion_event() for MotionEvent<br/>- Eraser and button state | 5 | Sprint 2 | IE-254 |
| IE-256 | As a developer, I want Android thermal management so that the app throttles under heat | - thermal_headroom() (PowerManager)<br/>- reduce_quality_on_thermal()<br/>- Graceful degradation | 3 | Sprint 2 | IE-254 |
| IE-257 | As a developer, I want Windows platform impl (Vulkan/DX12) so that the editor runs on Windows | - Vulkan 1.2 primary, DX12 secondary<br/>- create_vulkan_surface() from HWND<br/>- 1.5 GB engine budget | 5 | Sprint 3 | — |
| IE-258 | As a user, I want Windows Ink pen input so that I can draw with pressure and tilt | - PenInput: position, pressure, tilt, rotation, is_inverted<br/>- process_pen_event() for pointer info<br/>- Pen button state | 5 | Sprint 3 | IE-257 |
| IE-259 | As a developer, I want Windows DPI handling so that UI scales correctly on high-DPI displays | - get_monitor_dpi(), get_monitor_scale_factor()<br/>- Per-monitor scaling<br/>- DPI-aware rendering | 3 | Sprint 3 | IE-257 |
| IE-260 | As a developer, I want Flutter FFI bridge (Dart bindings via ffigen) so that Flutter can call the engine | - GopostImageEngine abstract class in Dart<br/>- FFI bindings via ffigen<br/>- All canvas, layer, export, tool APIs exposed | 8 | Sprint 24 | — |
| IE-261 | As a developer, I want Flutter texture registry (zero-copy) so that GPU textures display in Flutter | - Zero-copy texture sharing<br/>- Platform-specific texture registry (Metal, Vulkan, etc.)<br/>- canvasPreviewStream() integration | 5 | Sprint 24 | IE-260 |
| IE-262 | As a developer, I want platform capability detection so that features adapt to device | - queryGpuCapabilities() from Flutter<br/>- Vulkan/compute/texture size detection<br/>- Per-platform capability flags | 3 | Sprint 24 | IE-260 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1 + Phase 8: Core Foundation + Export, Polish & Launch |
| **Sprint(s)** | IE-Sprint 1 (Weeks 1-2) + IE-Sprint 24 (Weeks 47-48) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [22-build-system](22-build-system.md) |
| **Successor** | [18-project-serialization](18-project-serialization.md) / [24-testing-validation](24-testing-validation.md) |
| **Story Points Total** | 65 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-301 | As a developer, I want shared IPlatform from VE engine so that IE uses the same platform abstraction | - IPlatformContext, IGpuContext<br/>- File I/O, threading, system info<br/>- Thermal monitoring, memory pressure | 5 | P0 | — |
| IE-302 | As a developer, I want iOS: Apple Pencil pressure/tilt input handling so that drawing feels natural on iPad | - PencilInput: position, force, azimuth, altitude<br/>- process_touch() for UITouch<br/>- is_pencil detection | 5 | P0 | IE-301 |
| IE-303 | As a developer, I want iOS: Photo Library integration (PHPicker) so that users can save and import photos | - save_to_photos() with completion<br/>- pick_from_photos() with PHPicker<br/>- PHPhotoLibrary integration | 3 | P0 | IE-301 |
| IE-304 | As a developer, I want macOS: Wacom tablet pressure/tilt so that the editor works with tablets | - NSTabletPoint for Wacom pressure/tilt<br/>- Metal backend (Apple Silicon + Intel)<br/>- 1.5 GB engine budget | 5 | P0 | IE-301 |
| IE-305 | As a developer, I want macOS: color management (ColorSync profiles) so that colors are accurate | - ColorSync framework integration<br/>- ICC profile support<br/>- Display calibration awareness | 3 | P0 | IE-301 |
| IE-306 | As a developer, I want Android: S-Pen pressure input so that drawing works on Samsung devices | - StylusInput: position, pressure, tilt_x/y<br/>- process_motion_event() for MotionEvent<br/>- Eraser and button state | 5 | P0 | IE-301 |
| IE-307 | As a developer, I want Android: MediaStore integration (SAF) so that gallery access works | - MediaStore API for gallery<br/>- Storage Access Framework<br/>- Save and import flows | 3 | P0 | IE-301 |
| IE-308 | As a developer, I want Windows: Windows Ink API for stylus so that pen input works | - PenInput: position, pressure, tilt, rotation<br/>- process_pen_event() for pointer info<br/>- Pen button state, is_inverted | 5 | P0 | IE-301 |
| IE-309 | As a developer, I want Windows: DPI awareness (per-monitor scaling) so that UI scales correctly | - get_monitor_dpi(), get_monitor_scale_factor()<br/>- Per-monitor scaling<br/>- DPI-aware rendering | 3 | P0 | IE-301 |
| IE-310 | As a developer, I want Flutter FFI bridge for IE (Dart bindings via ffigen) so that Flutter can call the engine | - GopostImageEngine abstract class in Dart<br/>- FFI bindings via ffigen<br/>- All canvas, layer, export, tool APIs exposed | 8 | P0 | — |
| IE-311 | As a developer, I want texture registry for canvas preview so that GPU textures display in Flutter | - Zero-copy texture sharing<br/>- Platform-specific texture registry<br/>- canvasPreviewStream() integration | 5 | P0 | IE-310 |
| IE-312 | As a developer, I want platform hardening pass (per-platform optimization) so that each platform is optimized | - Per-platform profiling and tuning<br/>- Memory budget compliance<br/>- Performance targets met | 5 | P0 | IE-301, IE-310 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
