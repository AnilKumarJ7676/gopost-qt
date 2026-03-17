cmake_minimum_required(VERSION 3.27)
project(test_decoder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_executable(test_decoder
    test_decoder.cpp
    src/rendering_bridge/qprocess_video_decoder.cpp
    src/rendering_bridge/render_decode_thread.cpp
)

target_include_directories(test_decoder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_decoder PRIVATE Qt6::Core Qt6::Gui)
