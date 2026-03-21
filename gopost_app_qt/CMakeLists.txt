cmake_minimum_required(VERSION 3.21)
project(gopost_app_qt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    QuickDialogs2
    Network
    Multimedia
    Concurrent
    ShaderTools
    Sql
    Test
)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(src/core)
add_subdirectory(src/auth)
add_subdirectory(src/rendering_bridge)
add_subdirectory(src/image_editor)
add_subdirectory(src/video_editor)
add_subdirectory(src/template_browser)
add_subdirectory(src/go_craft)

qt_add_executable(gopost_app_qt
    src/main.cpp
    src/app.h
    src/app.cpp
    resources/gopost.rc
)

set_source_files_properties(qml/core/ui_support/theme/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_policy(SET QTP0004 OLD)

qt_add_qml_module(gopost_app_qt
    URI GopostApp
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/core/ui_support/theme/Theme.qml
        qml/core/ui_support/theme/NeonGlow.qml
        qml/core/ui_support/navigation/MainShell.qml
        qml/core/ui_support/navigation/GuestBanner.qml
        qml/core/ui_support/diagnostics/FpsOverlay.qml
        qml/auth/screens/LoginScreen.qml
        qml/auth/screens/RegisterScreen.qml
        qml/auth/widgets/LoginRequiredSheet.qml
        qml/auth/widgets/SocialLoginButtons.qml
        qml/image_editor/screens/ImageEditorScreen.qml
        qml/image_editor/screens/ExportScreen.qml
        qml/image_editor/screens/TemplateCustomizationScreen.qml
        qml/image_editor/widgets/AdjustmentPanel.qml
        qml/image_editor/widgets/CanvasPreview.qml
        qml/image_editor/widgets/CropPanel.qml
        qml/image_editor/widgets/EditorToolbar.qml
        qml/image_editor/widgets/FilterPanel.qml
        qml/image_editor/widgets/LayerPanel.qml
        qml/image_editor/widgets/MaskPanel.qml
        qml/image_editor/widgets/PhotoPickerSheet.qml
        qml/image_editor/widgets/SaveAsTemplateDialog.qml
        qml/image_editor/widgets/StickerPickerSheet.qml
        qml/image_editor/widgets/TextToolPanel.qml
        qml/image_editor/widgets/TransformOverlay.qml
        qml/image_editor/widgets/TransformHandle.qml
        qml/template_browser/screens/HomeScreen.qml
        qml/template_browser/screens/BrowseScreen.qml
        qml/template_browser/screens/TemplateDetailScreen.qml
        qml/template_browser/widgets/CategoryChips.qml
        qml/template_browser/widgets/FilterBar.qml
        qml/template_browser/widgets/PreviewPlayer.qml
        qml/template_browser/widgets/SearchBarWidget.qml
        qml/template_browser/widgets/ShimmerLoading.qml
        qml/template_browser/widgets/TemplateCard.qml
        qml/go_craft/screens/GoCraftScreen.qml
        qml/video_editor_2/screens/VideoEditor2Screen.qml
        qml/video_editor_2/widgets/VE2Toolbar.qml
        qml/video_editor_2/widgets/VE2PreviewPanel.qml
        qml/video_editor_2/widgets/VE2TimelinePanel.qml
        qml/video_editor_2/widgets/VE2PropertyPanel.qml
        qml/video_editor_2/widgets/VE2MediaPoolPanel.qml
        qml/video_editor_2/widgets/VE2EditorSidebar.qml
        qml/video_editor_2/widgets/VE2TimelineToolbar.qml
)

qt6_add_shaders(gopost_app_qt "gopost_shaders"
    BATCHABLE
    PREFIX "/GopostApp"
    FILES
        shaders/colorgrading.frag
)

target_link_libraries(gopost_app_qt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Multimedia
    Qt6::Concurrent
    Qt6::QuickDialogs2
    Qt6::ShaderTools
    gopost_core_lib
    gopost_auth_lib
    gopost_rendering_bridge_lib
    gopost_image_editor_lib
    gopost_video_editor_lib
    gopost_template_browser_lib
    gopost_go_craft_lib
)

target_include_directories(gopost_app_qt PRIVATE src)

install(TARGETS gopost_app_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

enable_testing()
set(GOPOST_TEST_ENV "PATH=${Qt6_DIR}/../../../bin\;$ENV{PATH}" "QT_PLUGIN_PATH=${Qt6_DIR}/../../../plugins")

# Common test link libraries (core pulls in service_locator which needs all feature libs)
set(GOPOST_TEST_LIBS
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Concurrent Qt6::Network Qt6::Qml Qt6::Test
    gopost_core_lib gopost_auth_lib gopost_rendering_bridge_lib
    gopost_image_editor_lib gopost_video_editor_lib
    gopost_template_browser_lib gopost_go_craft_lib
    gopost_bridge gopost_permissions gopost_storage gopost_mmap gopost_io gopost_import
)

# Test: Platform Capability Engine
add_executable(test_platform_capability tests/platform/TestPlatformCapability.cpp)
target_link_libraries(test_platform_capability PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_platform_capability PRIVATE src)
add_test(NAME TestPlatformCapability COMMAND test_platform_capability)
set_tests_properties(TestPlatformCapability PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Memory Budget Engine
add_executable(test_memory_budget tests/memory/TestMemoryBudget.cpp)
target_link_libraries(test_memory_budget PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_memory_budget PRIVATE src)
add_test(NAME TestMemoryBudget COMMAND test_memory_budget)
set_tests_properties(TestMemoryBudget PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Diagnostics Engine
add_executable(test_diagnostics tests/diagnostics/TestDiagnostics.cpp)
target_link_libraries(test_diagnostics PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_diagnostics PRIVATE src)
add_test(NAME TestDiagnostics COMMAND test_diagnostics)
set_tests_properties(TestDiagnostics PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Logger Engine
add_executable(test_logger tests/logging/TestLogger.cpp)
target_link_libraries(test_logger PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_logger PRIVATE src)
add_test(NAME TestLogger COMMAND test_logger)
set_tests_properties(TestLogger PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Config Store Engine
add_executable(test_config_store tests/config/TestConfigStore.cpp)
target_link_libraries(test_config_store PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_config_store PRIVATE src)
add_test(NAME TestConfigStore COMMAND test_config_store)
set_tests_properties(TestConfigStore PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Event Bus Engine
add_executable(test_event_bus tests/eventbus/TestEventBus.cpp)
target_link_libraries(test_event_bus PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_event_bus PRIVATE src)
add_test(NAME TestEventBus COMMAND test_event_bus)
set_tests_properties(TestEventBus PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# ── Level 1 Tests ──────────────────────────────────────────────────────

# Test: Platform Bridge
add_executable(test_bridge tests/bridges/TestBridge.cpp)
target_link_libraries(test_bridge PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_bridge PRIVATE src)
add_test(NAME TestBridge COMMAND test_bridge)
set_tests_properties(TestBridge PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Permission Engine
add_executable(test_permissions tests/permissions/TestPermissions.cpp)
target_link_libraries(test_permissions PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_permissions PRIVATE src)
add_test(NAME TestPermissions COMMAND test_permissions)
set_tests_properties(TestPermissions PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Storage Scheduler Engine
add_executable(test_storage tests/storage/TestStorage.cpp)
target_link_libraries(test_storage PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_storage PRIVATE src)
add_test(NAME TestStorage COMMAND test_storage)
set_tests_properties(TestStorage PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Mmap Engine
add_executable(test_mmap tests/mmap/TestMmap.cpp)
target_link_libraries(test_mmap PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_mmap PRIVATE src)
add_test(NAME TestMmap COMMAND test_mmap)
set_tests_properties(TestMmap PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: IO Engine
add_executable(test_io tests/io/TestIO.cpp)
target_link_libraries(test_io PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_io PRIVATE src)
add_test(NAME TestIO COMMAND test_io)
set_tests_properties(TestIO PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")

# Test: Media Import Engine
add_executable(test_import tests/import/TestImport.cpp)
target_link_libraries(test_import PRIVATE ${GOPOST_TEST_LIBS})
target_include_directories(test_import PRIVATE src)
add_test(NAME TestImport COMMAND test_import)
set_tests_properties(TestImport PROPERTIES ENVIRONMENT "${GOPOST_TEST_ENV}")
