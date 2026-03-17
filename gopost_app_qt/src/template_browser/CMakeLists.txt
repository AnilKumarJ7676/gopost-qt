add_library(gopost_template_browser_lib STATIC
    # Domain entities (Q_GADGET moc stubs)
    domain/entities/category_entity.cpp
    domain/entities/template_entity.cpp
    domain/entities/template_filter.cpp
    domain/entities/template_access.cpp

    # Domain use cases
    domain/usecases/get_categories_usecase.cpp
    domain/usecases/get_featured_templates_usecase.cpp
    domain/usecases/get_template_detail_usecase.cpp
    domain/usecases/get_templates_usecase.cpp
    domain/usecases/request_template_access_usecase.cpp
    domain/usecases/search_templates_usecase.cpp

    # Data models
    data/models/template_model.cpp
    data/models/category_model.cpp
    data/models/template_access_model.cpp

    # Data datasources
    data/datasources/template_remote_datasource.cpp

    # Data repositories
    data/repositories/template_repository_impl.cpp

    # Presentation notifiers
    presentation/models/template_list_notifier.cpp
    presentation/models/template_search_notifier.cpp
    presentation/models/template_detail_notifier.cpp
    presentation/models/home_notifier.cpp
    presentation/models/download_notifier.cpp
    presentation/models/template_providers.cpp
)

target_include_directories(gopost_template_browser_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(gopost_template_browser_lib PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    gopost_core_lib
)
