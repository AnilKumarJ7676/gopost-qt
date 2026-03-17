option(GOPOST_USE_HW_DECODER "Enable GPU-accelerated video decoding via FFmpeg hw_device_ctx" OFF)

set(VIDEO_ENGINE_SOURCES
    src/video/timeline_model.cpp
    src/video/frame_cache.cpp
    src/video/video_compositor.cpp
    src/video/timeline_evaluator.cpp
    src/video/video_engine.cpp
    src/video/audio_mixer.cpp
    src/video/video_effects.cpp
    src/video/transition_renderer.cpp
    src/video/keyframe_evaluator.cpp
    src/video/video_exporter.cpp
    src/video/decoder_pool.cpp
    src/video/thumbnail_generator.cpp
    src/video/decoder_pool_api.cpp
    src/video/ring_frame_buffer.cpp
    src/video/decode_thread.cpp
    src/video/playback_controller.cpp
    src/texture/texture_bridge_cpu.cpp
)

if(APPLE)
    enable_language(OBJCXX)
    list(APPEND VIDEO_ENGINE_SOURCES
        src/video/video_decoder_avf.mm
        src/video/media_probe_avf.mm
        src/video/audio_decoder_avf.mm
    )
elseif(GOPOST_HAS_FFMPEG)
    list(APPEND VIDEO_ENGINE_SOURCES
        src/video/video_decoder_ffmpeg.cpp
        src/video/audio_decoder_ffmpeg.cpp
        src/video/media_probe_ffmpeg.cpp
    )
    # GPU-accelerated decoder (optional, requires FFmpeg hw accel support)
    if(GOPOST_USE_HW_DECODER)
        list(APPEND VIDEO_ENGINE_SOURCES
            src/video/hw_video_decoder.cpp
        )
    endif()
else()
    list(APPEND VIDEO_ENGINE_SOURCES
        src/video/video_decoder_stub.cpp
        src/video/media_probe_stub.cpp
        src/video/audio_decoder_stub.cpp
    )
endif()

add_library(gopost_video_engine SHARED ${VIDEO_ENGINE_SOURCES})
set_target_properties(gopost_video_engine PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_include_directories(gopost_video_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(gopost_video_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/video
    ${CMAKE_CURRENT_SOURCE_DIR}/src/texture
)

target_link_libraries(gopost_video_engine PUBLIC gopost_core)

if(GOPOST_HAS_FFMPEG)
    target_compile_definitions(gopost_video_engine PRIVATE GOPOST_HAS_FFMPEG)
    target_include_directories(gopost_video_engine PRIVATE ${FFMPEG_INCLUDE_DIRS})
    if(FFMPEG_LIBRARY_DIRS)
        target_link_directories(gopost_video_engine PRIVATE ${FFMPEG_LIBRARY_DIRS})
    endif()
    target_link_libraries(gopost_video_engine PRIVATE ${FFMPEG_LIBRARIES})

    # GPU-accelerated decoder: platform-specific hw accel libraries
    if(GOPOST_USE_HW_DECODER)
        target_compile_definitions(gopost_video_engine PRIVATE GOPOST_USE_HW_DECODER)

        if(WIN32)
            # D3D11VA: Direct3D 11 Video Acceleration
            target_link_libraries(gopost_video_engine PRIVATE d3d11 dxgi)
        endif()

        if(APPLE)
            # VideoToolbox (hw decode is handled via AVFoundation on Apple,
            # but FFmpeg can also use VT as an hw accel backend)
            find_library(VIDEOTOOLBOX_FW VideoToolbox)
            if(VIDEOTOOLBOX_FW)
                target_link_libraries(gopost_video_engine PRIVATE ${VIDEOTOOLBOX_FW})
            endif()
        endif()

        if(UNIX AND NOT APPLE AND NOT ANDROID)
            # VAAPI / VDPAU on Linux
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(VAAPI QUIET libva libva-drm)
                if(VAAPI_FOUND)
                    target_link_libraries(gopost_video_engine PRIVATE ${VAAPI_LIBRARIES})
                    target_include_directories(gopost_video_engine PRIVATE ${VAAPI_INCLUDE_DIRS})
                endif()
            endif()
        endif()
    endif()
endif()

# Force-load entire static archive so FFI entry points (called only from Dart) are exported
if(APPLE)
    target_link_options(gopost_video_engine PRIVATE
        "LINKER:-force_load,$<TARGET_FILE:gopost_core>")
elseif(NOT MSVC)
    target_link_options(gopost_video_engine PRIVATE
        "LINKER:--whole-archive" $<TARGET_FILE:gopost_core> "LINKER:--no-whole-archive")
endif()

target_compile_definitions(gopost_video_engine PRIVATE
    GOPOST_GPU_${GOPOST_GPU_BACKEND}
    $<$<BOOL:${GOPOST_PLATFORM_IOS}>:GOPOST_PLATFORM_IOS>
    $<$<BOOL:${GOPOST_PLATFORM_MACOS}>:GOPOST_PLATFORM_MACOS>
    $<$<BOOL:${GOPOST_PLATFORM_ANDROID}>:GOPOST_PLATFORM_ANDROID>
    $<$<BOOL:${GOPOST_PLATFORM_WINDOWS}>:GOPOST_PLATFORM_WINDOWS>
    $<$<BOOL:${GOPOST_PLATFORM_LINUX}>:GOPOST_PLATFORM_LINUX>
    $<$<BOOL:${GOPOST_PLATFORM_WEB}>:GOPOST_PLATFORM_WEB>
)

if(MSVC)
    target_compile_options(gopost_video_engine PRIVATE /W4
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:/O2 /GL>)
    target_link_options(gopost_video_engine PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:/LTCG>)
else()
    target_compile_options(gopost_video_engine PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:-O3 -flto>
        $<$<CONFIG:Debug>:-g>
    )
    if(ENABLE_ASAN AND NOT ANDROID AND NOT WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(gopost_video_engine PRIVATE -fsanitize=address)
        target_link_options(gopost_video_engine PRIVATE -fsanitize=address)
    endif()
endif()

if(APPLE)
    set_target_properties(gopost_video_engine PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )
    target_link_libraries(gopost_video_engine PRIVATE
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework AudioToolbox"
        "-framework Foundation"
    )
endif()

if(ANDROID)
    target_link_libraries(gopost_video_engine PRIVATE android log)
endif()
