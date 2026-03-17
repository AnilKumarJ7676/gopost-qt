## VE-18. Platform Integration Layer

### 18.1 Platform Abstraction

```cpp
namespace gp::platform {

// Abstract interface implemented per platform
class IPlatform {
public:
    virtual ~IPlatform() = default;

    // GPU
    virtual std::unique_ptr<IGpuContext> create_gpu_context() = 0;

    // Hardware decode/encode
    virtual std::unique_ptr<IHardwareDecoder> create_hw_decoder(const std::string& codec) = 0;
    virtual std::unique_ptr<IHardwareEncoder> create_hw_encoder(const std::string& codec) = 0;
    virtual std::vector<std::string> supported_hw_decoders() const = 0;
    virtual std::vector<std::string> supported_hw_encoders() const = 0;

    // File system
    virtual std::string temp_directory() const = 0;
    virtual std::string cache_directory() const = 0;
    virtual std::string documents_directory() const = 0;
    virtual int64_t available_disk_space() const = 0;

    // Device info
    virtual std::string device_model() const = 0;
    virtual std::string os_version() const = 0;
    virtual int cpu_core_count() const = 0;
    virtual int64_t total_memory() const = 0;
    virtual int64_t available_memory() const = 0;
    virtual bool is_low_power_mode() const = 0;
    virtual ThermalState thermal_state() const = 0;

    // ML inference
    virtual std::unique_ptr<InferenceEngine> create_inference_engine() = 0;

    // Haptic feedback
    virtual void haptic_light() = 0;
    virtual void haptic_medium() = 0;
    virtual void haptic_heavy() = 0;

    static std::unique_ptr<IPlatform> create();
};

} // namespace gp::platform
```

### 18.2 Platform-Specific Implementations

| Component | iOS / macOS | Android | Windows |
|---|---|---|---|
| **GPU Context** | `MetalContext` — `MTLDevice`, `MTLCommandQueue`, `CAMetalLayer` | `VulkanContext` — `VkDevice`, `VkQueue`, `VkSwapchain` | `VulkanContext` or `DX12Context` |
| **Shader Compile** | Metal Shading Language (pre-compiled `.metallib`) | SPIR-V (pre-compiled) | SPIR-V or DXIL |
| **HW Decode** | `VTDecompressionSession` (VideoToolbox) | `AMediaCodec` (NDK) | NVDEC via FFmpeg / D3D11VA |
| **HW Encode** | `VTCompressionSession` (VideoToolbox) | `AMediaCodec` (NDK) | NVENC / AMF / QSV via FFmpeg |
| **ML Inference** | Core ML (`.mlmodelc`) → ANE/GPU/CPU | TFLite + NNAPI delegate | ONNX Runtime + DirectML |
| **Audio Output** | `AVAudioEngine` / Core Audio | `AAudio` / `Oboe` | WASAPI |
| **File Picker** | `UIDocumentPickerViewController` / `NSOpenPanel` | Storage Access Framework | `IFileOpenDialog` (COM) |
| **Camera Capture** | `AVCaptureSession` | `Camera2` API | `MediaFoundation` |
| **Texture Interop** | `MTLTexture` → Flutter `IOSurface` | `VkImage` → `AHardwareBuffer` → Flutter | `ID3D12Resource` / `VkImage` → Flutter |

### 18.3 Thermal Throttling

```cpp
namespace gp::platform {

// Adapt processing quality based on device thermal state
class ThermalThrottler {
public:
    void update(ThermalState state);

    // Query current limits
    int max_decode_threads() const;
    int max_effect_stack_depth() const;
    ResolutionScale preview_scale() const;   // 1.0, 0.75, 0.5, 0.25
    bool gpu_compute_allowed() const;
    int target_preview_fps() const;

private:
    ThermalState current_state_;

    // Throttling tiers
    // Nominal: full quality
    // Fair: reduce preview to 3/4, limit decode threads
    // Serious: reduce preview to 1/2, disable heavy effects, limit to 30fps
    // Critical: reduce preview to 1/4, minimal effects, 15fps, warn user
};

} // namespace gp::platform
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1 + Phase 8: Core Foundation & Platform Hardening |
| **Sprint(s)** | VE-Sprint 1 (Weeks 1-2) + VE-Sprint 22-24 (Weeks 43-48) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [17-plugin-architecture](17-plugin-architecture.md) |
| **Successor** | [19-memory-performance](19-memory-performance.md) |
| **Story Points Total** | 75 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-289 | As a C++ engine developer, I want IPlatform abstract interface so that platform-specific code is isolated | - create_gpu_context(), create_hw_decoder/encoder<br/>- temp/cache/documents_directory()<br/>- device_model(), os_version(), memory/cpu queries | 5 | P0 | — |
| VE-290 | As a C++ engine developer, I want Apple platform impl (GPU, HW decode/encode, CoreML, file system, device info) so that iOS/macOS work correctly | - Metal GPU context, VideoToolbox decode/encode<br/>- CoreML inference engine<br/>- NSTemporaryDirectory, caches, documents | 8 | P0 | VE-289 |
| VE-291 | As a C++ engine developer, I want Android platform impl (Vulkan, MediaCodec, NNAPI, file system) so that Android works correctly | - Vulkan context, MediaCodec decode/encode<br/>- NNAPI inference engine<br/>- getCacheDir(), getFilesDir() | 8 | P0 | VE-289 |
| VE-292 | As a C++ engine developer, I want Windows platform impl (Vulkan/DX12, NVDEC/NVENC, ONNX, file system) so that Windows works correctly | - Vulkan or DX12 context<br/>- NVDEC/NVENC, ONNX inference<br/>- GetTempPath, local app data | 8 | P0 | VE-289 |
| VE-293 | As a C++ engine developer, I want Platform factory (IPlatform::create) so that the correct platform is selected at runtime | - create() returns platform-specific instance<br/>- Compile-time or runtime detection<br/>- Single entry point for platform access | 2 | P0 | VE-289 |
| VE-294 | As a C++ engine developer, I want temp/cache/documents directory per platform so that file I/O uses correct paths | - temp_directory() for scratch files<br/>- cache_directory() for persistent cache<br/>- documents_directory() for user projects | 3 | P0 | VE-290, VE-291, VE-292 |
| VE-295 | As a C++ engine developer, I want CPU/memory/thermal state queries so that the engine can adapt to device conditions | - cpu_core_count(), total/available_memory()<br/>- thermal_state() returns ThermalState enum<br/>- is_low_power_mode() | 3 | P0 | VE-289 |
| VE-296 | As a C++ engine developer, I want ThermalThrottler (adapt quality based on thermal state) so that devices don't overheat | - update(ThermalState) drives throttling<br/>- max_decode_threads(), preview_scale()<br/>- target_preview_fps(), gpu_compute_allowed() | 5 | P1 | VE-295 |
| VE-297 | As a C++ engine developer, I want thermal tier: Nominal (full quality) so that normal operation has no limits | - Nominal: full decode threads, full preview<br/>- No quality reduction | 1 | P1 | VE-296 |
| VE-298 | As a C++ engine developer, I want thermal tier: Fair (3/4 preview, limit threads) so that moderate throttling helps | - Fair: 3/4 resolution preview<br/>- Reduced decode threads<br/>- 60fps target | 2 | P1 | VE-296 |
| VE-299 | As a C++ engine developer, I want thermal tier: Serious (1/2 preview, disable heavy effects) so that heavy throttling prevents overheating | - Serious: 1/2 resolution, 30fps<br/>- Disable heavy effects (AI, complex blur)<br/>- Limit effect stack depth | 3 | P1 | VE-296 |
| VE-300 | As a C++ engine developer, I want thermal tier: Critical (1/4 preview, warn user) so that critical state is handled | - Critical: 1/4 resolution, 15fps<br/>- Minimal effects only<br/>- Warn user via callback | 3 | P1 | VE-296 |
| VE-301 | As a C++ engine developer, I want low-power mode detection and adaptation so that battery life is preserved | - is_low_power_mode() from platform<br/>- Reduce preview quality, limit background tasks<br/>- Optional aggressive throttling | 3 | P2 | VE-295 |
| VE-302 | As a C++ engine developer, I want haptic feedback integration so that UI interactions feel responsive | - haptic_light(), haptic_medium(), haptic_heavy()<br/>- Platform-specific implementation (UIImpactFeedbackGenerator, etc.)<br/>- No-op on platforms without haptics | 2 | P2 | VE-289 |
| VE-303 | As a Tech Lead, I want platform hardening pass (Metal 3, Vulkan validation, DX12) so that launch quality is high | - Metal 3 optimizations on Apple<br/>- Vulkan validation layer fixes<br/>- DX12 backend validation on Windows | 8 | P0 | VE-290, VE-291, VE-292 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
