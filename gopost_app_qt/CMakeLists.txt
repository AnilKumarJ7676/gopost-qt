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
)

set_source_files_properties(qml/core/theme/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_policy(SET QTP0004 OLD)

qt_add_qml_module(gopost_app_qt
    URI GopostApp
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/core/theme/Theme.qml
        qml/core/theme/NeonGlow.qml
        qml/core/navigation/MainShell.qml
        qml/core/navigation/GuestBanner.qml
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
        qml/video_editor/screens/VideoEditorScreen.qml
        qml/video_editor/screens/ProjectListScreen.qml
        qml/video_editor/screens/VideoExportScreen.qml
        qml/video_editor/screens/VideoTemplateCustomizationScreen.qml
        qml/video_editor/widgets/AdjustmentClipPainter.qml
        qml/video_editor/widgets/AdjustmentLayerPanel.qml
        qml/video_editor/widgets/AudioControlsPanel.qml
        qml/video_editor/widgets/ClipInspectorPanel.qml
        qml/video_editor/widgets/ClipWidget.qml
        qml/video_editor/widgets/ColorGradingPanel.qml
        qml/video_editor/widgets/CropTransformPanel.qml
        qml/video_editor/widgets/DockSystem.qml
        qml/video_editor/widgets/EditorSidebar.qml
        qml/video_editor/widgets/EffectsPanel.qml
        qml/video_editor/widgets/KeyframeEditor.qml
        qml/video_editor/widgets/MarkersPanel.qml
        qml/video_editor/widgets/MediaPoolPanel.qml
        qml/video_editor/widgets/Playhead.qml
        qml/video_editor/widgets/ResizableSplit.qml
        qml/video_editor/widgets/SaveAsVideoTemplateDialog.qml
        qml/video_editor/widgets/SpeedControlsPanel.qml
        qml/video_editor/widgets/TextEditorPanel.qml
        qml/video_editor/widgets/TimeRuler.qml
        qml/video_editor/widgets/TimelinePanel.qml
        qml/video_editor/widgets/TrackWidget.qml
        qml/video_editor/widgets/TransitionPicker.qml
        qml/video_editor/widgets/VideoPreviewPanel.qml
        qml/video_editor/widgets/VideoToolbar.qml
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
