
## IE-22. Build System and Cross-Compilation

### 22.1 CMake Project Structure

```cmake
cmake_minimum_required(VERSION 3.22)
project(gopost_ie VERSION 1.0.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Platform Detection ─────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(GP_PLATFORM "ios")
    set(GP_GPU_BACKEND "metal")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(GP_PLATFORM "macos")
    set(GP_GPU_BACKEND "metal")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(GP_PLATFORM "android")
    set(GP_GPU_BACKEND "vulkan")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(GP_PLATFORM "windows")
    set(GP_GPU_BACKEND "vulkan")
endif()

# ─── Options ────────────────────────────────────
option(GP_ENABLE_METAL "Enable Metal GPU backend" OFF)
option(GP_ENABLE_VULKAN "Enable Vulkan GPU backend" OFF)
option(GP_ENABLE_GLES "Enable OpenGL ES fallback" OFF)
option(GP_ENABLE_DX12 "Enable DirectX 12 backend" OFF)
option(GP_ENABLE_AI "Enable AI/ML features" ON)
option(GP_ENABLE_RAW "Enable RAW image processing (LibRaw)" ON)
option(GP_ENABLE_PLUGINS "Enable plugin system" ON)
option(GP_ENABLE_SVG "Enable SVG import/export" ON)
option(GP_ENABLE_PDF "Enable PDF export" ON)
option(GP_ENABLE_PSD "Enable PSD import/export" ON)
option(GP_BUILD_TESTS "Build unit tests" OFF)
option(GP_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(GP_ASAN "Enable AddressSanitizer" OFF)

# ─── Shared Core (from libgopost_ve) ────────────
# Link against the shared core library or include source directly
option(GP_SHARED_CORE_DIR "Path to shared core source" "../libgopost_ve")

set(SHARED_MODULES
    ${GP_SHARED_CORE_DIR}/src/core
    ${GP_SHARED_CORE_DIR}/src/rendering
    ${GP_SHARED_CORE_DIR}/src/effects
    ${GP_SHARED_CORE_DIR}/src/animation
    ${GP_SHARED_CORE_DIR}/src/text
    ${GP_SHARED_CORE_DIR}/src/color
    ${GP_SHARED_CORE_DIR}/src/masking
    ${GP_SHARED_CORE_DIR}/src/ai
    ${GP_SHARED_CORE_DIR}/src/platform
    ${GP_SHARED_CORE_DIR}/src/cache
)

# Collect shared source files
foreach(MODULE ${SHARED_MODULES})
    file(GLOB_RECURSE MODULE_SOURCES "${MODULE}/*.cpp")
    list(APPEND SHARED_SOURCES ${MODULE_SOURCES})
endforeach()

# ─── IE-Specific Source Files ───────────────────
file(GLOB_RECURSE CANVAS_SOURCES    "src/canvas/*.cpp")
file(GLOB_RECURSE COMP_SOURCES      "src/composition/*.cpp")
file(GLOB_RECURSE IMAGE_SOURCES     "src/image/*.cpp")
file(GLOB_RECURSE VECTOR_SOURCES    "src/vector/*.cpp")
file(GLOB_RECURSE BRUSH_SOURCES     "src/brush/*.cpp")
file(GLOB_RECURSE SELECT_SOURCES    "src/selection/*.cpp")
file(GLOB_RECURSE TRANSFORM_SOURCES "src/transform/*.cpp")
file(GLOB_RECURSE TEMPLATE_SOURCES  "src/templates/*.cpp")
file(GLOB_RECURSE EXPORT_SOURCES    "src/export/*.cpp")
file(GLOB_RECURSE PROJECT_SOURCES   "src/project/*.cpp")
file(GLOB_RECURSE API_SOURCES       "src/api/*.cpp")

set(IE_SOURCES
    ${CANVAS_SOURCES} ${COMP_SOURCES} ${IMAGE_SOURCES} ${VECTOR_SOURCES}
    ${BRUSH_SOURCES} ${SELECT_SOURCES} ${TRANSFORM_SOURCES} ${TEMPLATE_SOURCES}
    ${EXPORT_SOURCES} ${PROJECT_SOURCES} ${API_SOURCES}
)

# ─── Platform-Specific Sources ──────────────────
if(GP_PLATFORM STREQUAL "ios" OR GP_PLATFORM STREQUAL "macos")
    file(GLOB_RECURSE METAL_SOURCES "src/rendering/metal/*.mm")
    file(GLOB_RECURSE APPLE_PLATFORM "src/platform/apple/*.mm")
    list(APPEND PLATFORM_SOURCES ${METAL_SOURCES} ${APPLE_PLATFORM})
    set(GP_ENABLE_METAL ON)
elseif(GP_PLATFORM STREQUAL "android")
    file(GLOB_RECURSE VULKAN_SOURCES "src/rendering/vulkan/*.cpp")
    file(GLOB_RECURSE ANDROID_PLATFORM "src/platform/android/*.cpp")
    list(APPEND PLATFORM_SOURCES ${VULKAN_SOURCES} ${ANDROID_PLATFORM})
    set(GP_ENABLE_VULKAN ON)
    set(GP_ENABLE_GLES ON)
elseif(GP_PLATFORM STREQUAL "windows")
    file(GLOB_RECURSE VULKAN_SOURCES "src/rendering/vulkan/*.cpp")
    file(GLOB_RECURSE WIN_PLATFORM "src/platform/windows/*.cpp")
    list(APPEND PLATFORM_SOURCES ${VULKAN_SOURCES} ${WIN_PLATFORM})
    set(GP_ENABLE_VULKAN ON)
endif()

# ─── Plugin Module ──────────────────────────────
if(GP_ENABLE_PLUGINS)
    file(GLOB_RECURSE PLUGIN_SOURCES "src/plugin/*.cpp")
    list(APPEND IE_SOURCES ${PLUGIN_SOURCES})
endif()

# ─── Library Target ─────────────────────────────
add_library(gopost_ie SHARED
    ${SHARED_SOURCES}
    ${IE_SOURCES}
    ${PLATFORM_SOURCES}
)

target_include_directories(gopost_ie PUBLIC include)
target_include_directories(gopost_ie PRIVATE src)
target_include_directories(gopost_ie PRIVATE ${GP_SHARED_CORE_DIR}/include)
target_include_directories(gopost_ie PRIVATE ${GP_SHARED_CORE_DIR}/src)

target_compile_definitions(gopost_ie PRIVATE
    GP_PLATFORM_${GP_PLATFORM}
    GP_IMAGE_EDITOR
    $<$<BOOL:${GP_ENABLE_METAL}>:GP_GPU_METAL>
    $<$<BOOL:${GP_ENABLE_VULKAN}>:GP_GPU_VULKAN>
    $<$<BOOL:${GP_ENABLE_GLES}>:GP_GPU_GLES>
    $<$<BOOL:${GP_ENABLE_DX12}>:GP_GPU_DX12>
    $<$<BOOL:${GP_ENABLE_AI}>:GP_ENABLE_AI>
    $<$<BOOL:${GP_ENABLE_RAW}>:GP_ENABLE_RAW>
    $<$<BOOL:${GP_ENABLE_SVG}>:GP_ENABLE_SVG>
    $<$<BOOL:${GP_ENABLE_PDF}>:GP_ENABLE_PDF>
    $<$<BOOL:${GP_ENABLE_PSD}>:GP_ENABLE_PSD>
)

# ─── Third-Party Dependencies ───────────────────
# Image codecs
find_package(libjpeg-turbo REQUIRED)
find_package(PNG REQUIRED)
find_package(WebP REQUIRED)
target_link_libraries(gopost_ie PRIVATE libjpeg-turbo::jpeg PNG::PNG WebP::webp)

# RAW processing
if(GP_ENABLE_RAW)
    find_package(LibRaw REQUIRED)
    target_link_libraries(gopost_ie PRIVATE LibRaw::LibRaw)
endif()

# FreeType + HarfBuzz (text rendering)
find_package(Freetype REQUIRED)
find_package(HarfBuzz REQUIRED)
target_link_libraries(gopost_ie PRIVATE Freetype::Freetype HarfBuzz::HarfBuzz)

# MessagePack (project serialization)
find_package(msgpack-cxx REQUIRED)
target_link_libraries(gopost_ie PRIVATE msgpack-cxx)

# SVG support
if(GP_ENABLE_SVG)
    find_package(nanosvg REQUIRED)
    target_link_libraries(gopost_ie PRIVATE nanosvg::nanosvg)
endif()

# PDF export
if(GP_ENABLE_PDF)
    find_package(libharu REQUIRED)
    target_link_libraries(gopost_ie PRIVATE libharu::libharu)
endif()

# Boolean operations (vector)
find_package(Clipper2 REQUIRED)
target_link_libraries(gopost_ie PRIVATE Clipper2::Clipper2)

# LZ4 compression (undo history)
find_package(lz4 REQUIRED)
target_link_libraries(gopost_ie PRIVATE lz4::lz4)

# ICC color management
find_package(lcms2 REQUIRED)
target_link_libraries(gopost_ie PRIVATE lcms2::lcms2)

# Platform GPU frameworks
if(GP_ENABLE_METAL)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(gopost_ie PRIVATE
        ${METAL_FRAMEWORK} ${METALKIT_FRAMEWORK} ${QUARTZCORE_FRAMEWORK})
endif()

if(GP_ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(gopost_ie PRIVATE Vulkan::Vulkan)
endif()

# Platform ML frameworks
if(GP_ENABLE_AI)
    if(GP_PLATFORM STREQUAL "ios" OR GP_PLATFORM STREQUAL "macos")
        find_library(COREML_FRAMEWORK CoreML REQUIRED)
        find_library(VISION_FRAMEWORK Vision REQUIRED)
        target_link_libraries(gopost_ie PRIVATE ${COREML_FRAMEWORK} ${VISION_FRAMEWORK})
    elseif(GP_PLATFORM STREQUAL "android")
        # TFLite linked via pre-built AAR
    elseif(GP_PLATFORM STREQUAL "windows")
        find_package(onnxruntime REQUIRED)
        target_link_libraries(gopost_ie PRIVATE onnxruntime::onnxruntime)
    endif()
endif()

# ─── Tests ──────────────────────────────────────
if(GP_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()
```

### 22.2 Build Matrix

| Platform | Toolchain | Architecture | Build Command |
|---|---|---|---|
| iOS | Xcode + Apple Clang | arm64 | `cmake -GXcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64` |
| iOS Simulator | Xcode + Apple Clang | arm64, x86_64 | `cmake -GXcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator` |
| macOS | Xcode + Apple Clang | arm64, x86_64 (Universal) | `cmake -GXcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` |
| Android arm64 | NDK r26+ + Clang | arm64-v8a | `cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a` |
| Android x86_64 | NDK r26+ + Clang | x86_64 | Same with `-DANDROID_ABI=x86_64` |
| Windows | MSVC 2022 / Clang-cl | x64 | `cmake -G"Visual Studio 17 2022" -A x64` |

### 22.3 Binary Size Targets

| Platform | Target Size | Optimization |
|---|---|---|
| iOS (arm64) | < 18 MB | LTO, dead code elimination, symbol stripping |
| macOS (Universal) | < 25 MB | LTO, dead code elimination |
| Android (arm64) | < 16 MB | LTO, -Oz, symbol stripping |
| Windows (x64) | < 22 MB | LTO, /Ox, symbol stripping |

### 22.4 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 1 | Wk 1-2 | IE-277 to IE-288 | Build system, initial setup |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-277 | As a developer, I want CMake project setup with C++20 and C17 so that the codebase uses modern language standards | - cmake_minimum_required 3.22+<br/>- CMAKE_CXX_STANDARD 20, CMAKE_C_STANDARD 17<br/>- Project builds on at least one platform | 3 | Sprint 1 | — |
| IE-278 | As a developer, I want shared core source linking from libgopost_ve so that IE reuses core modules | - GP_SHARED_CORE_DIR option for core path<br/>- Shared modules: core, rendering, effects, animation, text, color, masking, ai, platform, cache<br/>- IE target links/includes shared sources | 5 | Sprint 1 | IE-277 |
| IE-279 | As a developer, I want an iOS build (Xcode, arm64) so that the engine runs on physical iOS devices | - CMake generates Xcode project for iOS<br/>- arm64 architecture<br/>- Successful build and link | 5 | Sprint 1 | IE-277 |
| IE-280 | As a developer, I want an iOS Simulator build (arm64, x86_64) so that the engine runs in the simulator | - CMAKE_OSX_SYSROOT=iphonesimulator<br/>- arm64 and x86_64 support<br/>- Successful build | 3 | Sprint 1 | IE-279 |
| IE-281 | As a developer, I want a macOS build producing a Universal binary so that the engine runs on Intel and Apple Silicon Macs | - CMAKE_OSX_ARCHITECTURES="arm64;x86_64"<br/>- Universal binary output<br/>- Metal backend enabled | 3 | Sprint 1 | IE-277 |
| IE-282 | As a developer, I want an Android build (NDK r26+, arm64-v8a) so that the engine runs on Android devices | - android.toolchain.cmake, ANDROID_ABI=arm64-v8a<br/>- NDK r26+ Clang<br/>- Vulkan/GLES backends | 5 | Sprint 1 | IE-277 |
| IE-283 | As a developer, I want an Android x86_64 build for the emulator so that the engine can be tested in the Android emulator | - ANDROID_ABI=x86_64<br/>- Same toolchain as arm64<br/>- Successful build | 2 | Sprint 1 | IE-282 |
| IE-284 | As a developer, I want a Windows build (MSVC 2022 / Clang-cl) so that the engine runs on Windows | - Visual Studio 17 2022, x64<br/>- MSVC or Clang-cl support<br/>- Vulkan backend | 5 | Sprint 1 | IE-277 |
| IE-285 | As a developer, I want third-party dependency integration for all libs so that image codecs, fonts, and utilities are linked | - libjpeg-turbo, PNG, WebP, LibRaw, FreeType, HarfBuzz, msgpack-cxx, Clipper2, nanosvg, libharu, lcms2, lz4<br/>- Platform GPU (Metal/Vulkan) and ML (CoreML/TFLite/ONNX) frameworks<br/>- All find_package/find_library succeed | 5 | Sprint 1 | IE-277 |
| IE-286 | As a developer, I want a CI/CD pipeline (GitHub Actions, 4-platform matrix) so that every commit is built on all platforms | - GitHub Actions workflow<br/>- Matrix: iOS, macOS, Android, Windows<br/>- Build artifacts or status reported | 5 | Sprint 1 | IE-279, IE-281, IE-282, IE-284 |
| IE-287 | As a developer, I want binary size optimization (LTO, dead code, strip) so that binaries meet size targets | - LTO enabled in release<br/>- Dead code elimination<br/>- Symbol stripping; sizes within 22.3 targets | 3 | Sprint 1 | IE-279, IE-281, IE-282, IE-284 |
| IE-288 | As a developer, I want build configuration options (feature flags) so that features can be toggled per build | - GP_ENABLE_METAL, GP_ENABLE_VULKAN, GP_ENABLE_AI, GP_ENABLE_RAW, GP_ENABLE_SVG, GP_ENABLE_PDF, GP_ENABLE_PSD, GP_BUILD_TESTS, GP_BUILD_BENCHMARKS<br/>- Options propagate to compile definitions<br/>- Conditional source inclusion | 3 | Sprint 1 | IE-277 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Core Foundation |
| **Sprint(s)** | IE-Sprint 1 (Weeks 1-2) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | — |
| **Successor** | [20-platform-integration](20-platform-integration.md) |
| **Story Points Total** | 50 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-326 | As a developer, I want CMakeLists.txt for libgopost_ie so that the IE engine builds | - cmake_minimum_required 3.22+<br/>- CMAKE_CXX_STANDARD 20<br/>- Project builds on at least one platform | 3 | P0 | — |
| IE-327 | As a developer, I want shared core linking (target_link_libraries gopost_ve_core) so that IE reuses core modules | - GP_SHARED_CORE_DIR option<br/>- Shared modules: core, rendering, effects, etc.<br/>- IE target links shared sources | 5 | P0 | IE-326 |
| IE-328 | As a developer, I want IE-specific source collection so that IE modules are included | - canvas, composition, image, vector, brush, selection, transform, templates, export, project, api<br/>- file(GLOB_RECURSE) for each<br/>- Platform-specific sources | 5 | P0 | IE-326 |
| IE-329 | As a developer, I want platform detection and GPU backend flags so that the correct backend is used | - GP_PLATFORM: ios, macos, android, windows<br/>- GP_GPU_BACKEND: metal, vulkan<br/>- Compile definitions per platform | 3 | P0 | IE-326 |
| IE-330 | As a developer, I want LibRaw find_package (RAW processing) so that RAW images are supported | - find_package(LibRaw REQUIRED)<br/>- target_link_libraries with LibRaw::LibRaw<br/>- GP_ENABLE_RAW option | 3 | P0 | IE-326 |
| IE-331 | As a developer, I want Clipper2 find_package (boolean operations) so that vector boolean ops work | - find_package(Clipper2 REQUIRED)<br/>- target_link_libraries with Clipper2::Clipper2<br/>- Vector module integration | 3 | P0 | IE-326 |
| IE-332 | As a developer, I want nanosvg integration (SVG import) so that SVG files can be imported | - find_package(nanosvg) or integration<br/>- GP_ENABLE_SVG option<br/>- SVG parsing for vector layers | 3 | P0 | IE-326 |
| IE-333 | As a developer, I want iOS build (Xcode, arm64) so that the engine runs on iOS devices | - CMake generates Xcode project for iOS<br/>- arm64 architecture<br/>- Successful build and link | 5 | P0 | IE-326 |
| IE-334 | As a developer, I want Android build (NDK, arm64-v8a) so that the engine runs on Android | - android.toolchain.cmake<br/>- ANDROID_ABI=arm64-v8a<br/>- NDK r26+ Clang, Vulkan/GLES | 5 | P0 | IE-326 |
| IE-335 | As a developer, I want macOS build (Universal) and Windows build (MSVC/Clang x64) so that the engine runs on desktop | - macOS: CMAKE_OSX_ARCHITECTURES="arm64;x86_64", Metal<br/>- Windows: Visual Studio 17 2022 x64, Vulkan<br/>- Successful build on both | 5 | P0 | IE-326 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
