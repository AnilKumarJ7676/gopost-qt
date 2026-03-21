add_library(gopost_core_lib STATIC
    error/failures.cpp
    config/app_environment.cpp
    logging/app_logger.cpp
    theme/app_theme.cpp
    cache/image_cache_manager.cpp
    security/secure_storage_service.cpp
    network/http_client.cpp
    network/ssl_pinning.cpp
    network/interceptors/auth_interceptor.cpp
    network/interceptors/error_interceptor.cpp
    network/interceptors/logging_interceptor.cpp
    network/interceptors/retry_interceptor.cpp
    network/interceptors/ssl_pinning_interceptor.cpp
    navigation/router.cpp
    di/service_locator.cpp
    engines/platform_capability_engine.cpp
    engines/memory_management_engine.cpp
    engines/logging_engine.cpp
    engines/configuration_engine.cpp
    engines/diagnostics_engine.cpp
    engines/event_bus_engine.cpp
)

target_include_directories(gopost_core_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(gopost_core_lib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
)
