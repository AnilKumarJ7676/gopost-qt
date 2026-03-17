cmake_minimum_required(VERSION 3.27)
project(gopost_native VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Platform detection
if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(GOPOST_PLATFORM_IOS TRUE)
        set(GOPOST_GPU_BACKEND "metal")
    else()
        set(GOPOST_PLATFORM_MACOS TRUE)
        set(GOPOST_GPU_BACKEND "metal")
    endif()
elseif(ANDROID)
    set(GOPOST_PLATFORM_ANDROID TRUE)
    set(GOPOST_GPU_BACKEND "vulkan")
elseif(WIN32)
    set(GOPOST_PLATFORM_WINDOWS TRUE)
    set(GOPOST_GPU_BACKEND "vulkan")
elseif(UNIX)
    set(GOPOST_PLATFORM_LINUX TRUE)
    set(GOPOST_GPU_BACKEND "vulkan")
elseif(EMSCRIPTEN)
    set(GOPOST_PLATFORM_WEB TRUE)
    set(GOPOST_GPU_BACKEND "webgl")
endif()

message(STATUS "GPU Backend: ${GOPOST_GPU_BACKEND}")

option(ENABLE_ASAN "Enable AddressSanitizer (standalone testing only)" OFF)

# OpenSSL (shared by core)
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL found — AES-GCM, RSA-OAEP, SHA-256 enabled")
else()
    if(APPLE)
        message(STATUS "OpenSSL not found — install via Homebrew (brew install openssl) for crypto support")
    else()
        message(STATUS "OpenSSL not found — AES-GCM/RSA-OAEP unavailable")
    endif()
endif()

# WebP encoding on non-Apple platforms
if(NOT APPLE)
    find_library(WEBP_LIB webp HINTS ${WEBP_ROOT}/lib $ENV{WEBP_ROOT}/lib)
    find_path(WEBP_INCLUDE_DIR webp/encode.h HINTS ${WEBP_ROOT}/include $ENV{WEBP_ROOT}/include)
    if(WEBP_LIB AND WEBP_INCLUDE_DIR)
        set(GOPOST_HAS_WEBP TRUE)
        message(STATUS "libwebp found — WebP encoding enabled")
    else()
        message(STATUS "libwebp not found — WebP encoding unavailable on this platform")
    endif()
endif()

# ── FreeType (vector font rasterisation) ──────────────────────────────────────
# Build from source via FetchContent for reproducible, version-locked builds
# across all platforms (desktop, mobile, WASM).
option(GOPOST_USE_SYSTEM_FREETYPE "Use system FreeType instead of FetchContent" OFF)

if(GOPOST_USE_SYSTEM_FREETYPE)
    find_package(Freetype QUIET)
endif()

if(NOT Freetype_FOUND)
    message(STATUS "FreeType: fetching v2.13.3 via FetchContent...")
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
        GIT_TAG        VER-2-13-3
        GIT_SHALLOW    TRUE
    )
    # Disable optional FreeType deps we don't need (keeps build minimal)
    set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)  # HarfBuzz links FreeType, not the reverse
    set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(freetype)
    # FetchContent target is "freetype", alias to Freetype::Freetype for uniformity
    if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
    endif()
    set(Freetype_FOUND TRUE)
    message(STATUS "FreeType: built from source (v2.13.3)")
else()
    message(STATUS "FreeType: using system installation")
endif()

# ── HarfBuzz (text shaping — bidirectional, ligatures, kerning) ───────────────
# Must be fetched AFTER FreeType so it can find the FreeType target.
option(GOPOST_USE_SYSTEM_HARFBUZZ "Use system HarfBuzz instead of FetchContent" OFF)

if(GOPOST_USE_SYSTEM_HARFBUZZ)
    find_package(harfbuzz QUIET)
endif()

if(NOT harfbuzz_FOUND)
    message(STATUS "HarfBuzz: fetching v10.2.0 via FetchContent...")
    FetchContent_Declare(
        harfbuzz
        GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
        GIT_TAG        10.2.0
        GIT_SHALLOW    TRUE
    )
    # Minimal HarfBuzz config — only enable FreeType integration
    set(HB_HAVE_FREETYPE  ON  CACHE BOOL "" FORCE)
    set(HB_HAVE_GLIB      OFF CACHE BOOL "" FORCE)
    set(HB_HAVE_ICU       OFF CACHE BOOL "" FORCE)
    set(HB_HAVE_GOBJECT   OFF CACHE BOOL "" FORCE)
    set(HB_BUILD_UTILS    OFF CACHE BOOL "" FORCE)
    set(HB_BUILD_SUBSET   OFF CACHE BOOL "" FORCE)
    set(HB_HAVE_CAIRO     OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(harfbuzz)
    set(harfbuzz_FOUND TRUE)
    message(STATUS "HarfBuzz: built from source (v10.2.0)")
else()
    message(STATUS "HarfBuzz: using system installation")
endif()

# Export availability flags for sub-projects
set(GOPOST_HAS_FREETYPE ${Freetype_FOUND})
set(GOPOST_HAS_HARFBUZZ ${harfbuzz_FOUND})

# FFmpeg (video/audio decode on non-Apple platforms)
if(NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFMPEG_LIBAV QUIET
            libavformat libavcodec libswscale libswresample libavutil)
    endif()

    if(NOT FFMPEG_LIBAV_FOUND)
        find_path(FFMPEG_INCLUDE_DIR libavformat/avformat.h
            HINTS ${FFMPEG_ROOT}/include $ENV{FFMPEG_ROOT}/include)
        find_library(AVFORMAT_LIB avformat
            HINTS ${FFMPEG_ROOT}/lib $ENV{FFMPEG_ROOT}/lib)
        find_library(AVCODEC_LIB avcodec
            HINTS ${FFMPEG_ROOT}/lib $ENV{FFMPEG_ROOT}/lib)
        find_library(SWSCALE_LIB swscale
            HINTS ${FFMPEG_ROOT}/lib $ENV{FFMPEG_ROOT}/lib)
        find_library(SWRESAMPLE_LIB swresample
            HINTS ${FFMPEG_ROOT}/lib $ENV{FFMPEG_ROOT}/lib)
        find_library(AVUTIL_LIB avutil
            HINTS ${FFMPEG_ROOT}/lib $ENV{FFMPEG_ROOT}/lib)

        if(FFMPEG_INCLUDE_DIR AND AVFORMAT_LIB AND AVCODEC_LIB
           AND SWSCALE_LIB AND SWRESAMPLE_LIB AND AVUTIL_LIB)
            set(GOPOST_HAS_FFMPEG TRUE)
            set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
            set(FFMPEG_LIBRARIES
                ${AVFORMAT_LIB} ${AVCODEC_LIB} ${SWSCALE_LIB}
                ${SWRESAMPLE_LIB} ${AVUTIL_LIB})
        endif()
    else()
        set(GOPOST_HAS_FFMPEG TRUE)
        set(FFMPEG_INCLUDE_DIRS ${FFMPEG_LIBAV_INCLUDE_DIRS})
        set(FFMPEG_LIBRARIES ${FFMPEG_LIBAV_LIBRARIES})
        set(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBAV_LIBRARY_DIRS})
    endif()

    if(GOPOST_HAS_FFMPEG)
        message(STATUS "FFmpeg found — real video/audio decoding enabled")
    else()
        message(STATUS "FFmpeg not found — video/audio decoders will use stubs")
        message(STATUS "  Install FFmpeg dev libraries or set -DFFMPEG_ROOT=<path>")
    endif()
endif()

# version.h generation (in gopost_core)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/gopost_core/include/gopost/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/gopost_core/include/gopost/version.h
)

# --- Sub-projects ---
add_subdirectory(gopost_core)
add_subdirectory(gopost_image_engine)
add_subdirectory(gopost_video_engine)

# --- Tests (Google Test via FetchContent — no system install required) ---
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS AND NOT ANDROID AND NOT GOPOST_PLATFORM_IOS)
    enable_testing()

    # Prefer system GTest if available, fall back to FetchContent
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GTest: fetching v1.15.2 via FetchContent...")
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.15.2
            GIT_SHALLOW    TRUE
        )
        # Prevent GTest from overriding parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
        message(STATUS "GTest: built from source (v1.15.2)")
    else()
        message(STATUS "GTest: using system installation")
    endif()

    add_executable(gopost_engine_tests
        tests/test_allocator.cpp
        tests/test_thread_pool.cpp
        tests/test_gpu_context.cpp
        tests/test_crypto.cpp
        tests/test_template_parser.cpp
        tests/test_image_codec.cpp
        tests/test_canvas.cpp
        tests/test_artistic_filters.cpp
        tests/test_video_engine.cpp
        tests/test_ring_frame_buffer.cpp
        tests/test_decode_thread.cpp
        tests/test_graphics_backend_mock.cpp
        tests/test_playback_controller.cpp
    )
    target_link_libraries(gopost_engine_tests PRIVATE
        gopost_image_engine
        gopost_video_engine
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
    )
    target_include_directories(gopost_engine_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gopost_video_engine/src
    )
    if(OpenSSL_FOUND)
        target_compile_definitions(gopost_engine_tests PRIVATE GOPOST_USE_OPENSSL)
    endif()
    if(GOPOST_HAS_STB)
        target_include_directories(gopost_engine_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/gopost_core/third_party/stb
        )
    endif()
    add_test(NAME gopost_engine_tests COMMAND gopost_engine_tests)
endif()
