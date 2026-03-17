## VE-20. Build System and Cross-Compilation

### 20.1 CMake Project Structure

```cmake
cmake_minimum_required(VERSION 3.22)
project(gopost_ve VERSION 1.0.0 LANGUAGES C CXX OBJC OBJCXX)

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
option(GP_ENABLE_EXPRESSIONS "Enable Lua expression engine" ON)
option(GP_ENABLE_PLUGINS "Enable plugin system" ON)
option(GP_BUILD_TESTS "Build unit tests" OFF)
option(GP_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(GP_ASAN "Enable AddressSanitizer" OFF)

# ─── Source Files ───────────────────────────────
file(GLOB_RECURSE CORE_SOURCES     "src/core/*.cpp")
file(GLOB_RECURSE TIMELINE_SOURCES "src/timeline/*.cpp")
file(GLOB_RECURSE COMP_SOURCES     "src/composition/*.cpp")
file(GLOB_RECURSE RENDER_SOURCES   "src/rendering/*.cpp")
file(GLOB_RECURSE EFFECTS_SOURCES  "src/effects/*.cpp")
file(GLOB_RECURSE ANIM_SOURCES     "src/animation/*.cpp")
file(GLOB_RECURSE MOTION_SOURCES   "src/motion/*.cpp")
file(GLOB_RECURSE TEXT_SOURCES     "src/text/*.cpp")
file(GLOB_RECURSE AUDIO_SOURCES    "src/audio/*.cpp")
file(GLOB_RECURSE TRANS_SOURCES    "src/transitions/*.cpp")
file(GLOB_RECURSE COLOR_SOURCES    "src/color/*.cpp")
file(GLOB_RECURSE MEDIA_SOURCES    "src/media/*.cpp")
file(GLOB_RECURSE TRACK_SOURCES    "src/tracking/*.cpp")
file(GLOB_RECURSE MASK_SOURCES     "src/masking/*.cpp")
file(GLOB_RECURSE PROJECT_SOURCES  "src/project/*.cpp")
file(GLOB_RECURSE CACHE_SOURCES    "src/cache/*.cpp")
file(GLOB_RECURSE API_SOURCES      "src/api/*.cpp")

set(ALL_SOURCES
    ${CORE_SOURCES} ${TIMELINE_SOURCES} ${COMP_SOURCES} ${RENDER_SOURCES}
    ${EFFECTS_SOURCES} ${ANIM_SOURCES} ${MOTION_SOURCES} ${TEXT_SOURCES}
    ${AUDIO_SOURCES} ${TRANS_SOURCES} ${COLOR_SOURCES} ${MEDIA_SOURCES}
    ${TRACK_SOURCES} ${MASK_SOURCES} ${PROJECT_SOURCES} ${CACHE_SOURCES}
    ${API_SOURCES}
)

# ─── Platform-Specific Sources ──────────────────
if(GP_PLATFORM STREQUAL "ios" OR GP_PLATFORM STREQUAL "macos")
    file(GLOB_RECURSE METAL_SOURCES "src/rendering/metal/*.mm")
    file(GLOB_RECURSE APPLE_PLATFORM "src/platform/apple/*.mm")
    list(APPEND ALL_SOURCES ${METAL_SOURCES} ${APPLE_PLATFORM})
    set(GP_ENABLE_METAL ON)
elseif(GP_PLATFORM STREQUAL "android")
    file(GLOB_RECURSE VULKAN_SOURCES "src/rendering/vulkan/*.cpp")
    file(GLOB_RECURSE ANDROID_PLATFORM "src/platform/android/*.cpp")
    list(APPEND ALL_SOURCES ${VULKAN_SOURCES} ${ANDROID_PLATFORM})
    set(GP_ENABLE_VULKAN ON)
    set(GP_ENABLE_GLES ON)
elseif(GP_PLATFORM STREQUAL "windows")
    file(GLOB_RECURSE VULKAN_SOURCES "src/rendering/vulkan/*.cpp")
    file(GLOB_RECURSE WIN_PLATFORM "src/platform/windows/*.cpp")
    list(APPEND ALL_SOURCES ${VULKAN_SOURCES} ${WIN_PLATFORM})
    set(GP_ENABLE_VULKAN ON)
endif()

# ─── AI Module ──────────────────────────────────
if(GP_ENABLE_AI)
    file(GLOB_RECURSE AI_SOURCES "src/ai/*.cpp")
    list(APPEND ALL_SOURCES ${AI_SOURCES})
endif()

# ─── Plugin Module ──────────────────────────────
if(GP_ENABLE_PLUGINS)
    file(GLOB_RECURSE PLUGIN_SOURCES "src/plugin/*.cpp")
    list(APPEND ALL_SOURCES ${PLUGIN_SOURCES})
endif()

# ─── Library Target ─────────────────────────────
add_library(gopost_ve SHARED ${ALL_SOURCES})

target_include_directories(gopost_ve PUBLIC include)
target_include_directories(gopost_ve PRIVATE src)

target_compile_definitions(gopost_ve PRIVATE
    GP_PLATFORM_${GP_PLATFORM}
    $<$<BOOL:${GP_ENABLE_METAL}>:GP_GPU_METAL>
    $<$<BOOL:${GP_ENABLE_VULKAN}>:GP_GPU_VULKAN>
    $<$<BOOL:${GP_ENABLE_GLES}>:GP_GPU_GLES>
    $<$<BOOL:${GP_ENABLE_DX12}>:GP_GPU_DX12>
    $<$<BOOL:${GP_ENABLE_AI}>:GP_ENABLE_AI>
    $<$<BOOL:${GP_ENABLE_EXPRESSIONS}>:GP_ENABLE_EXPRESSIONS>
)

# ─── Third-Party Dependencies ───────────────────
# FFmpeg (pre-built per platform)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat avutil swscale swresample avfilter)
target_link_libraries(gopost_ve PRIVATE FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil
                                        FFmpeg::swscale FFmpeg::swresample FFmpeg::avfilter)

# FreeType + HarfBuzz (text rendering)
find_package(Freetype REQUIRED)
find_package(HarfBuzz REQUIRED)
target_link_libraries(gopost_ve PRIVATE Freetype::Freetype HarfBuzz::HarfBuzz)

# Lua (expression engine)
if(GP_ENABLE_EXPRESSIONS)
    find_package(Lua REQUIRED)
    target_link_libraries(gopost_ve PRIVATE ${LUA_LIBRARIES})
endif()

# MessagePack (project serialization)
find_package(msgpack-cxx REQUIRED)
target_link_libraries(gopost_ve PRIVATE msgpack-cxx)

# Platform GPU frameworks
if(GP_ENABLE_METAL)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(gopost_ve PRIVATE ${METAL_FRAMEWORK} ${METALKIT_FRAMEWORK} ${QUARTZCORE_FRAMEWORK})
endif()

if(GP_ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(gopost_ve PRIVATE Vulkan::Vulkan)
endif()

# Platform ML frameworks
if(GP_ENABLE_AI)
    if(GP_PLATFORM STREQUAL "ios" OR GP_PLATFORM STREQUAL "macos")
        find_library(COREML_FRAMEWORK CoreML REQUIRED)
        find_library(VISION_FRAMEWORK Vision REQUIRED)
        target_link_libraries(gopost_ve PRIVATE ${COREML_FRAMEWORK} ${VISION_FRAMEWORK})
    elseif(GP_PLATFORM STREQUAL "android")
        # TFLite linked via pre-built AAR
    elseif(GP_PLATFORM STREQUAL "windows")
        find_package(onnxruntime REQUIRED)
        target_link_libraries(gopost_ve PRIVATE onnxruntime::onnxruntime)
    endif()
endif()

# ─── Tests ──────────────────────────────────────
if(GP_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()
```

### 20.2 Build Matrix

| Platform | Toolchain | Architecture | Build Command |
|---|---|---|---|
| iOS | Xcode + Apple Clang | arm64 | `cmake -GXcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64` |
| iOS Simulator | Xcode + Apple Clang | arm64, x86_64 | `cmake -GXcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator` |
| macOS | Xcode + Apple Clang | arm64, x86_64 (Universal) | `cmake -GXcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` |
| Android arm64 | NDK r26+ + Clang | arm64-v8a | `cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a` |
| Android x86_64 | NDK r26+ + Clang | x86_64 | Same with `-DANDROID_ABI=x86_64` |
| Windows | MSVC 2022 / Clang-cl | x64 | `cmake -G"Visual Studio 17 2022" -A x64` |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Core Foundation |
| **Sprint(s)** | VE-Sprint 1 (Weeks 1-2) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [19-memory-performance](19-memory-performance.md) |
| **Successor** | [21-public-c-api](21-public-c-api.md) |
| **Story Points Total** | 55 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-319 | As a C++ engine developer, I want CMakeLists.txt root project setup (C++20, C17) so that the project builds with modern standards | - cmake_minimum_required 3.22<br/>- CMAKE_CXX_STANDARD 20, CMAKE_C_STANDARD 17<br/>- project(gopost_ve) with version | 3 | P0 | — |
| VE-320 | As a C++ engine developer, I want platform detection (iOS/macOS/Android/Windows) so that the correct config is used | - CMAKE_SYSTEM_NAME for iOS, Darwin, Android, Windows<br/>- GP_PLATFORM variable set<br/>- GP_GPU_BACKEND (metal, vulkan) | 3 | P0 | VE-319 |
| VE-321 | As a C++ engine developer, I want build options (GP_ENABLE_METAL/VULKAN/GLES/DX12/AI/EXPRESSIONS/PLUGINS) so that features can be toggled | - option() for each feature flag<br/>- Conditional compilation via target_compile_definitions<br/>- Defaults per platform | 5 | P0 | VE-319 |
| VE-322 | As a C++ engine developer, I want source file collection (GLOB_RECURSE per module) so that all sources are included | - file(GLOB_RECURSE) for core, timeline, composition, etc.<br/>- ALL_SOURCES aggregation<br/>- Exclude test/benchmark by default | 3 | P0 | VE-319 |
| VE-323 | As a C++ engine developer, I want platform-specific source inclusion (Metal .mm, Vulkan .cpp) so that platform code compiles | - Metal .mm for iOS/macOS<br/>- Vulkan .cpp for Android/Windows<br/>- APPEND to ALL_SOURCES per platform | 3 | P0 | VE-320 |
| VE-324 | As a C++ engine developer, I want FFmpeg find_package and linking so that media I/O works | - find_package(FFmpeg REQUIRED avcodec avformat avutil swscale swresample avfilter)<br/>- target_link_libraries with FFmpeg components<br/>- Version check | 5 | P0 | VE-319 |
| VE-325 | As a C++ engine developer, I want FreeType + HarfBuzz find_package so that text rendering works | - find_package(Freetype REQUIRED), find_package(HarfBuzz REQUIRED)<br/>- target_link_libraries<br/>- Include directories | 3 | P0 | VE-319 |
| VE-326 | As a C++ engine developer, I want Lua find_package (conditional) so that expressions work when enabled | - find_package(Lua) when GP_ENABLE_EXPRESSIONS<br/>- target_link_libraries conditionally<br/>- Graceful skip when disabled | 2 | P1 | VE-321 |
| VE-327 | As a C++ engine developer, I want MessagePack find_package so that project serialization works | - find_package(msgpack-cxx REQUIRED)<br/>- target_link_libraries<br/>- Include for msgpack headers | 2 | P0 | VE-319 |
| VE-328 | As a C++ engine developer, I want Metal/Vulkan/DX12 framework linking so that GPU backends link correctly | - Metal: Metal, MetalKit, QuartzCore frameworks<br/>- Vulkan: find_package(Vulkan)<br/>- DX12: when GP_ENABLE_DX12 | 5 | P0 | VE-321 |
| VE-329 | As a C++ engine developer, I want build matrix: iOS (Xcode, arm64) so that iOS builds succeed | - cmake -GXcode -DCMAKE_SYSTEM_NAME=iOS<br/>- arm64 architecture<br/>- Validated build | 5 | P0 | VE-320 |
| VE-330 | As a C++ engine developer, I want build matrix: macOS (Xcode, Universal) so that macOS builds succeed | - cmake -GXcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"<br/>- Universal binary output<br/>- Validated build | 5 | P0 | VE-320 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
