set(CORE_SOURCES
    src/core/engine.cpp
    src/core/allocator.cpp
    src/core/thread_pool.cpp
    src/core/logger.cpp
    src/gpu/gpu_factory.cpp
    src/crypto/aes_gcm.cpp
    src/crypto/rsa_oaep.cpp
    src/crypto/secure_memory.cpp
    src/template/template_parser.cpp
    src/image/image_decoder.cpp
    src/image/image_encoder.cpp
)


if(NOT APPLE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/stb_image.h")
        list(APPEND CORE_SOURCES third_party/stb/stb_impl.c)
        set(GOPOST_HAS_STB TRUE PARENT_SCOPE)
        set(GOPOST_HAS_STB TRUE)
    endif()
endif()

add_library(gopost_core STATIC ${CORE_SOURCES})

set_target_properties(gopost_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(gopost_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(GOPOST_HAS_STB)
    target_include_directories(gopost_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    )
endif()

target_compile_definitions(gopost_core PRIVATE
    GOPOST_GPU_${GOPOST_GPU_BACKEND}
    $<$<BOOL:${GOPOST_PLATFORM_IOS}>:GOPOST_PLATFORM_IOS>
    $<$<BOOL:${GOPOST_PLATFORM_MACOS}>:GOPOST_PLATFORM_MACOS>
    $<$<BOOL:${GOPOST_PLATFORM_ANDROID}>:GOPOST_PLATFORM_ANDROID>
    $<$<BOOL:${GOPOST_PLATFORM_WINDOWS}>:GOPOST_PLATFORM_WINDOWS>
    $<$<BOOL:${GOPOST_PLATFORM_LINUX}>:GOPOST_PLATFORM_LINUX>
    $<$<BOOL:${GOPOST_PLATFORM_WEB}>:GOPOST_PLATFORM_WEB>
)

if(MSVC)
    target_compile_options(gopost_core PRIVATE /W4
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:/O2 /GL>)
else()
    target_compile_options(gopost_core PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:-O3 -flto>
        $<$<CONFIG:Debug>:-g>
    )
endif()

if(NOT MSVC)
    set_property(TARGET gopost_core PROPERTY
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET gopost_core PROPERTY
        INTERPROCEDURAL_OPTIMIZATION_PROFILE TRUE)
endif()

if(OpenSSL_FOUND)
    target_compile_definitions(gopost_core PRIVATE GOPOST_USE_OPENSSL)
    target_link_libraries(gopost_core PRIVATE OpenSSL::Crypto)
endif()

if(WIN32)
    target_link_libraries(gopost_core PRIVATE bcrypt)
endif()

if(GOPOST_HAS_WEBP)
    target_compile_definitions(gopost_core PRIVATE GOPOST_HAS_WEBP)
    target_include_directories(gopost_core PRIVATE ${WEBP_INCLUDE_DIR})
    target_link_libraries(gopost_core PRIVATE ${WEBP_LIB})
endif()

if(APPLE)
    target_link_libraries(gopost_core PRIVATE
        "-framework Security"
        "-framework CoreGraphics"
        "-framework CoreFoundation"
        "-framework ImageIO"
        "-framework Foundation"
    )
    if(GOPOST_PLATFORM_IOS OR GOPOST_PLATFORM_MACOS)
        target_link_libraries(gopost_core PRIVATE "-framework Metal")
    endif()
endif()
