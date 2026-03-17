add_library(gopost_image_editor_lib STATIC
    # Domain — commands
    domain/commands/layer_commands.cpp

    # Domain — use cases
    domain/usecases/create_canvas_usecase.cpp
    domain/usecases/import_image_usecase.cpp
    domain/usecases/manage_layers_usecase.cpp
    domain/usecases/render_canvas_usecase.cpp

    # Data — datasources
    data/datasources/engine_canvas_datasource.cpp

    # Data — repositories
    data/repositories/canvas_repository_impl.cpp
    data/repositories/export_repository_impl.cpp
    data/repositories/filter_repository_impl.cpp
    data/repositories/mask_repository_impl.cpp
    data/repositories/project_repository_impl.cpp
    data/repositories/text_repository_impl.cpp

    # Presentation — notifiers
    presentation/models/canvas_notifier.cpp
    presentation/models/tool_notifier.cpp
    presentation/models/undo_redo_notifier.cpp
    presentation/models/filter_notifier.cpp
    presentation/models/adjustment_notifier.cpp
    presentation/models/photo_import_notifier.cpp
    presentation/models/text_notifier.cpp
    presentation/models/editor_providers.cpp
)

target_include_directories(gopost_image_editor_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(gopost_image_editor_lib PUBLIC
    Qt6::Core
    gopost_core_lib
    gopost_rendering_bridge_lib
)
