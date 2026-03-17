# gopost_rendering_bridge_lib — Phase 3: Rendering Bridge
# Provides engine types, abstract interfaces, native + stub implementations.

set(RENDERING_BRIDGE_HEADERS
    engine_types.h
    engine_api.h
    qprocess_video_decoder.h
    render_decode_thread.h
    stub_image_editor_engine.h
    stub_video_timeline_engine.h
    template_bridge.h
)

set(RENDERING_BRIDGE_SOURCES
    qprocess_video_decoder.cpp
    render_decode_thread.cpp
    stub_image_editor_engine.cpp
    stub_video_timeline_engine.cpp
    template_bridge.cpp
)

# Native engine implementations require the gopost_core C library.
# Only compile them when that target is available.
if(TARGET gopost_core)
    list(APPEND RENDERING_BRIDGE_HEADERS
        image_editor_engine_native.h
        video_timeline_engine_native.h
    )
    list(APPEND RENDERING_BRIDGE_SOURCES
        image_editor_engine_native.cpp
        video_timeline_engine_native.cpp
    )
endif()

add_library(gopost_rendering_bridge_lib STATIC
    ${RENDERING_BRIDGE_HEADERS}
    ${RENDERING_BRIDGE_SOURCES}
)

target_include_directories(gopost_rendering_bridge_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(gopost_rendering_bridge_lib
    PUBLIC
        Qt6::Core
        Qt6::Gui
)

# When the native gopost_engine library is available, link it.
# Otherwise the stub engines provide fallback functionality.
if(TARGET gopost_engine)
    target_link_libraries(gopost_rendering_bridge_lib PRIVATE gopost_engine)
    target_compile_definitions(gopost_rendering_bridge_lib PRIVATE GOPOST_HAS_NATIVE_ENGINE=1)
endif()

target_compile_features(gopost_rendering_bridge_lib PUBLIC cxx_std_20)
