add_library(gopost_core_lib STATIC
    error/failures.cpp
    config/app_environment.cpp
    config/config_layer.cpp
    config/layered_config_store.cpp
    config/config_schema.cpp
    config/schema_migrator.cpp
    config/configuration_engine_new.cpp
    logging/app_logger.cpp
    logging/log_formatter.cpp
    logging/console_log_sink.cpp
    logging/file_log_sink.cpp
    logging/log_filter.cpp
    logging/async_log_queue.cpp
    logging/logging_engine_new.cpp
    ui_support/theme/app_theme.cpp
    ui_support/cache/image_cache_manager.cpp
    security/secure_storage_service.cpp
    network/http_client.cpp
    network/ssl_pinning.cpp
    network/interceptors/auth_interceptor.cpp
    network/interceptors/error_interceptor.cpp
    network/interceptors/logging_interceptor.cpp
    network/interceptors/retry_interceptor.cpp
    network/interceptors/ssl_pinning_interceptor.cpp
    ui_support/navigation/router.cpp
    di/service_locator.cpp
    platform/cpu_detector.cpp
    platform/gpu_detector.cpp
    platform/ram_detector.cpp
    platform/storage_detector.cpp
    platform/hardware_decode_detector.cpp
    platform/tier_classifier.cpp
    platform/platform_capability_engine.cpp
    memory/tier_budget_policy.cpp
    memory/budget_calculator.cpp
    memory/memory_pool_registry.cpp
    memory/budget_enforcer.cpp
    memory/memory_engine.cpp
    diagnostics/frame_timer.cpp
    diagnostics/scope_profiler.cpp
    diagnostics/overlay_data_provider.cpp
    diagnostics/diagnostic_engine.cpp
    diagnostics/boot_info_provider.cpp
    engines/memory_management_engine.cpp
    engines/logging_engine.cpp
    engines/configuration_engine.cpp
    engines/diagnostics_engine.cpp
    engines/event_bus_engine.cpp
    events/subscription_registry.cpp
    events/queued_delivery_bridge.cpp
    events/event_bus_engine_new.cpp
)

target_include_directories(gopost_core_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(gopost_core_lib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
)

if(WIN32)
    target_link_libraries(gopost_core_lib PRIVATE dxgi mfplat mf mfuuid)
endif()

# ── Level 1: File Access & Import ──────────────────────────────────────

# gopost_bridge — platform-conditional (only current platform compiles)
set(BRIDGE_COMMON
    bridges/common/bridge_factory.cpp
    bridges/common/path_sanitizer.cpp
)
if(WIN32)
    set(BRIDGE_PLATFORM bridges/windows/windows_bridge.cpp)
elseif(APPLE AND IOS)
    set(BRIDGE_PLATFORM bridges/ios/ios_bridge.mm)
elseif(APPLE)
    set(BRIDGE_PLATFORM bridges/macos/macos_bridge.mm)
elseif(ANDROID)
    set(BRIDGE_PLATFORM bridges/android/android_bridge.cpp)
elseif(UNIX)
    set(BRIDGE_PLATFORM bridges/linux/linux_bridge.cpp)
endif()
add_library(gopost_bridge STATIC ${BRIDGE_COMMON} ${BRIDGE_PLATFORM})
target_include_directories(gopost_bridge PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(gopost_bridge PUBLIC Qt6::Core)
if(WIN32)
    target_link_libraries(gopost_bridge PRIVATE shell32 ole32 advapi32)
endif()

# gopost_permissions
add_library(gopost_permissions STATIC
    permissions/permission_engine.cpp
    permissions/permission_state_cache.cpp
    permissions/platform_permission_adapter.cpp
    permissions/permission_rationale_provider.cpp
    permissions/denial_handler.cpp
)
target_include_directories(gopost_permissions PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(gopost_permissions PUBLIC Qt6::Core gopost_bridge gopost_core_lib)

# gopost_storage
add_library(gopost_storage STATIC
    storage/storage_scheduler_engine.cpp
    storage/storage_profiler.cpp
    storage/read_ahead_predictor.cpp
    storage/io_bandwidth_monitor.cpp
    storage/read_scheduler.cpp
)
target_include_directories(gopost_storage PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(gopost_storage PUBLIC Qt6::Core gopost_core_lib)

# gopost_mmap
add_library(gopost_mmap STATIC
    mmap/mmap_engine.cpp
    mmap/mmap_provider.cpp
    mmap/advisory_hint_manager.cpp
    mmap/sliding_window_mapper.cpp
    mmap/mmap_fallback_reader.cpp
)
target_include_directories(gopost_mmap PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(gopost_mmap PUBLIC Qt6::Core gopost_core_lib gopost_bridge)

# gopost_io
add_library(gopost_io STATIC
    io/io_engine.cpp
    io/async_read_pool.cpp
    io/buffered_reader.cpp
    io/write_coalescer.cpp
    io/file_handle_pool.cpp
    io/hash_calculator.cpp
    io/project_database.cpp
    io/duplicate_detector.cpp
    io/file_validator.cpp
)
target_include_directories(gopost_io PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(gopost_io PUBLIC Qt6::Core Qt6::Sql gopost_core_lib gopost_storage gopost_mmap)

# gopost_import
add_library(gopost_import STATIC import/media_import_engine.cpp)
target_include_directories(gopost_import PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(gopost_import PUBLIC Qt6::Core gopost_io gopost_permissions)
