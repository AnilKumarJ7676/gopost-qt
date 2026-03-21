import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: appWindow
    width: 1280
    height: 720
    visible: true
    title: "Gopost"
    color: "#1a1a1a"

    onClosing: function(close) {
        // Stop timeline playback before closing to prevent
        // audio/video continuing after the window is destroyed
        if (timelineNotifier && timelineNotifier.isPlaying)
            timelineNotifier.togglePlayPause()
    }

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
                    console.log("[Main] Replacing StackView with component for route:", route,
                                "category:", category, "prev:", currentScreenCategory)
                    mainStack.replace(null, component)
                    currentScreenCategory = category
                    console.log("[Main] StackView depth:", mainStack.depth,
                                "currentItem:", mainStack.currentItem,
                                "size:", mainStack.width, "x", mainStack.height)
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
        if (route.startsWith("/editor/video")) return videoEditor2Component
        if (route === "/editor/image") return imageEditorComponent
        if (route.startsWith("/editor/customize/")) return templateCustomizationComponent

        // Export
        if (route === "/export/image") return imageExportComponent
        // Video export removed with v1 editor

        return null
    }

    // Component declarations for lazy loading
    // Each uses Item wrapper so StackView can set width/height, Loader fills it
    Component {
        id: authLoginComponent
        Item { Loader { anchors.fill: parent; source: "qrc:/qt/qml/GopostApp/qml/auth/screens/LoginScreen.qml" } }
    }

    Component {
        id: authRegisterComponent
        Item { Loader { anchors.fill: parent; source: "qrc:/qt/qml/GopostApp/qml/auth/screens/RegisterScreen.qml" } }
    }

    Component {
        id: mainShellComponent
        Item { Loader { anchors.fill: parent; source: "qrc:/qt/qml/GopostApp/qml/core/ui_support/navigation/MainShell.qml" } }
    }

    Component {
        id: videoEditor2Component
        Item { Loader { anchors.fill: parent; source: "qrc:/qt/qml/GopostApp/qml/video_editor_2/screens/VideoEditor2Screen.qml" } }
    }

    Component {
        id: imageEditorComponent
        Item { Loader { anchors.fill: parent; source: "qrc:/qt/qml/GopostApp/qml/image_editor/screens/ImageEditorScreen.qml" } }
    }

    Component {
        id: templateCustomizationComponent
        Item { Loader { anchors.fill: parent; source: "qrc:/qt/qml/GopostApp/qml/image_editor/screens/TemplateCustomizationScreen.qml" } }
    }

    Component {
        id: imageExportComponent
        Item { Loader { anchors.fill: parent; source: "qrc:/qt/qml/GopostApp/qml/image_editor/screens/ExportScreen.qml" } }
    }

    // Boot info block — centered, visible on auth/initial screen
    Column {
        anchors.centerIn: parent
        spacing: 12
        z: 100
        visible: currentScreenCategory === "auth"

        Text {
            text: "GoPost"
            color: "white"
            font.pixelSize: 32
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            text: "Level 0 Bootstrap OK"
            color: "#888888"
            font.pixelSize: 18
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            text: bootInfo
                  ? "Tier: " + bootInfo.tier
                    + " | " + bootInfo.cpuInfo
                    + " | " + bootInfo.ramGb + "GB"
                    + " | " + bootInfo.storageType
                  : ""
            color: "#666666"
            font.pixelSize: 14
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            text: bootInfo ? "Boot: " + bootInfo.bootTimeMs.toFixed(0) + "ms" : ""
            color: "#E85D3A"
            font.pixelSize: 16
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    // FPS diagnostic overlay — floats on top of all content
    Loader {
        source: "qrc:/qt/qml/GopostApp/qml/core/ui_support/diagnostics/FpsOverlay.qml"
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 8
        z: 9999
    }
}
