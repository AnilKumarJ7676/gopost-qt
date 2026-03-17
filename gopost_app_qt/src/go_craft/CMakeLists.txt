add_library(gopost_go_craft_lib STATIC
    data/image_project_local_datasource.cpp
    presentation/models/go_craft_notifier.cpp
)

target_include_directories(gopost_go_craft_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(gopost_go_craft_lib PUBLIC
    Qt6::Core
    Qt6::Gui
)
