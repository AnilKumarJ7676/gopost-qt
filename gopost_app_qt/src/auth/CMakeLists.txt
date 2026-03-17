add_library(gopost_auth_lib STATIC
    data/models/auth_response_model.cpp
    data/datasources/auth_remote_datasource.cpp
    data/repositories/auth_repository_impl.cpp
    presentation/models/auth_notifier.cpp
)

target_include_directories(gopost_auth_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(gopost_auth_lib PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Qml
    gopost_core_lib
)
