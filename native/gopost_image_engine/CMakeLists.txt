set(IMAGE_ENGINE_SOURCES
    src/canvas/canvas.cpp
    src/composition/compositor.cpp
    src/composition/blend_modes.cpp
    src/rendering/tile_renderer.cpp
    src/text/text_renderer.cpp
    src/effects/effect_registry.cpp
    src/effects/adjustment_filters.cpp
    src/effects/artistic_filters.cpp
    src/effects/blur_sharpen_filters.cpp
    src/effects/preset_filters.cpp
    src/effects/layer_effects.cpp
    src/mask/mask_system.cpp
    src/export/export_pipeline.cpp
    src/project/project_serializer.cpp
)

if(APPLE)
    list(APPEND IMAGE_ENGINE_SOURCES src/gpu/metal/metal_renderer.cpp)
endif()

add_library(gopost_image_engine SHARED ${IMAGE_ENGINE_SOURCES})
set_target_properties(gopost_image_engine PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_include_directories(gopost_image_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(gopost_image_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gopost_image_engine PUBLIC gopost_core)

# FreeType + HarfBuzz for vector text rendering (conditionally linked)
if(GOPOST_HAS_FREETYPE)
    target_link_libraries(gopost_image_engine PRIVATE Freetype::Freetype)
    target_compile_definitions(gopost_image_engine PRIVATE GOPOST_HAS_FREETYPE)
endif()
if(GOPOST_HAS_HARFBUZZ)
    target_link_libraries(gopost_image_engine PRIVATE harfbuzz)
    target_compile_definitions(gopost_image_engine PRIVATE GOPOST_HAS_HARFBUZZ)
endif()

# Force-load entire static archive so FFI entry points (called only from Dart) are exported
if(APPLE)
    target_link_options(gopost_image_engine PRIVATE
        "LINKER:-force_load,$<TARGET_FILE:gopost_core>")
elseif(NOT MSVC)
    target_link_options(gopost_image_engine PRIVATE
        "LINKER:--whole-archive" $<TARGET_FILE:gopost_core> "LINKER:--no-whole-archive")
endif()

target_compile_definitions(gopost_image_engine PRIVATE
    GOPOST_GPU_${GOPOST_GPU_BACKEND}
    $<$<BOOL:${GOPOST_PLATFORM_IOS}>:GOPOST_PLATFORM_IOS>
    $<$<BOOL:${GOPOST_PLATFORM_MACOS}>:GOPOST_PLATFORM_MACOS>
    $<$<BOOL:${GOPOST_PLATFORM_ANDROID}>:GOPOST_PLATFORM_ANDROID>
    $<$<BOOL:${GOPOST_PLATFORM_WINDOWS}>:GOPOST_PLATFORM_WINDOWS>
    $<$<BOOL:${GOPOST_PLATFORM_LINUX}>:GOPOST_PLATFORM_LINUX>
    $<$<BOOL:${GOPOST_PLATFORM_WEB}>:GOPOST_PLATFORM_WEB>
)

if(MSVC)
    target_compile_options(gopost_image_engine PRIVATE /W4
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:/O2 /GL>)
    target_link_options(gopost_image_engine PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:/LTCG>)
else()
    target_compile_options(gopost_image_engine PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:-O3 -flto>
        $<$<CONFIG:Debug>:-g>
    )
    if(ENABLE_ASAN AND NOT ANDROID AND NOT WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(gopost_image_engine PRIVATE -fsanitize=address)
        target_link_options(gopost_image_engine PRIVATE -fsanitize=address)
    endif()
endif()

if(APPLE)
    set_target_properties(gopost_image_engine PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )
endif()

if(ANDROID)
    target_link_libraries(gopost_image_engine PRIVATE android log jnigraphics)
endif()

if(WIN32)
    target_link_libraries(gopost_image_engine PRIVATE shlwapi)
endif()
