import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: appWindow
    width: 1400
    height: 900
    visible: true
    title: "Gopost"

    Material.theme: Material.System
    Material.primary: "#6C5CE7"
    Material.accent: "#00B894"

    font.family: "Inter"

    // Track which top-level screen category is active to avoid
    // re-creating MainShell when switching between shell tabs.
    property string currentScreenCategory: "auth"

    function screenCategory(route) {
        if (route === "/" || route === "/templates" || route === "/create" || route === "/profile" ||
            route.startsWith("/templates/")) {
            return "shell"
        }
        if (route === "/auth/login" || route === "/auth/register") return "auth"
        return "standalone" // editors, export, etc.
    }

    // Route-based content switching
    StackView {
        id: mainStack
        anchors.fill: parent
        initialItem: authLoginComponent

        Connections {
            target: router
            function onNavigationRequested(route) {
                console.log("[Main] Navigation requested:", route)
                var category = screenCategory(route)

                if (category === currentScreenCategory && category === "shell") {
                    return
                }

                var component = resolveComponent(route)
                if (component) {
                    console.log("[Main] Replacing StackView with component for route:", route)
                    mainStack.replace(component)
                    currentScreenCategory = category
                } else {
                    console.warn("[Main] No component found for route:", route)
                }
            }
        }
    }

    function resolveComponent(route) {
        // Shell routes (with bottom nav)
        if (route === "/" || route === "/templates" || route === "/create" || route === "/profile" ||
            route.startsWith("/templates/")) {
            return mainShellComponent
        }

        // Auth routes
        if (route === "/auth/login") return authLoginComponent
        if (route === "/auth/register") return authRegisterComponent

        // Editor routes
        if (route === "/editor/video") return videoEditorComponent
        if (route === "/editor/video2") return videoEditor2Component
        if (route === "/editor/image") return imageEditorComponent
        if (route.startsWith("/editor/customize/")) return templateCustomizationComponent
        if (route.startsWith("/editor/video/customize/")) return videoTemplateCustomizationComponent
        if (route === "/projects/video") return projectListComponent

        // Export
        if (route === "/export/image") return imageExportComponent
        if (route === "/export/video") return videoExportComponent

        return null
    }

    // Component declarations for lazy loading
    Component {
        id: authLoginComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/auth/screens/LoginScreen.qml" }
    }

    Component {
        id: authRegisterComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/auth/screens/RegisterScreen.qml" }
    }

    Component {
        id: mainShellComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/core/navigation/MainShell.qml" }
    }

    Component {
        id: videoEditorComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/video_editor/screens/VideoEditorScreen.qml" }
    }

    Component {
        id: videoEditor2Component
        Loader { source: "qrc:/qt/qml/GopostApp/qml/video_editor_2/screens/VideoEditor2Screen.qml" }
    }

    Component {
        id: imageEditorComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/image_editor/screens/ImageEditorScreen.qml" }
    }

    Component {
        id: templateCustomizationComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/image_editor/screens/TemplateCustomizationScreen.qml" }
    }

    Component {
        id: videoTemplateCustomizationComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/video_editor/screens/VideoTemplateCustomizationScreen.qml" }
    }

    Component {
        id: projectListComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/video_editor/screens/ProjectListScreen.qml" }
    }

    Component {
        id: imageExportComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/image_editor/screens/ExportScreen.qml" }
    }

    Component {
        id: videoExportComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/video_editor/screens/VideoExportScreen.qml" }
    }
}
