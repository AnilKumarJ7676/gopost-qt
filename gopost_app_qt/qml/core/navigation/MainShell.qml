import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: root

    property int selectedIndex: {
        var route = router.currentRoute
        if (route.startsWith("/templates")) return 1
        if (route.startsWith("/create") || route.startsWith("/editor")) return 2
        if (route.startsWith("/profile")) return 3
        return 0
    }

    property string currentTitle: {
        switch (selectedIndex) {
        case 0: return "Home"
        case 1: return "Templates"
        case 2: return "Go Craft"
        case 3: return "Profile"
        default: return "Gopost"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Top App Bar with back button ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Material.background

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: Qt.rgba(0.5, 0.5, 0.5, 0.2)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4; anchors.rightMargin: 16
                spacing: 4

                ToolButton {
                    id: backBtn
                    icon.name: "go-previous"
                    icon.width: 20; icon.height: 20
                    visible: router.canGoBack
                    onClicked: router.pop()
                    ToolTip.text: "Back"
                    ToolTip.visible: hovered
                }

                Label {
                    text: root.currentTitle
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    Layout.fillWidth: true
                    leftPadding: backBtn.visible ? 0 : 12
                }
            }
        }

        // Guest banner
        GuestBanner {
            Layout.fillWidth: true
            visible: authNotifier ? authNotifier.isGuest : false
            onSignInClicked: router.go("/auth/login")
        }

        // Content area with nested StackView for shell routes
        StackView {
            id: shellStack
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Resolve which component to show based on current route at creation time
            initialItem: {
                var route = router.currentRoute
                if (route === "/templates") return browseComponent
                if (route.startsWith("/templates/")) return templateDetailComponent
                if (route === "/create") return goCraftComponent
                if (route === "/profile") return profileComponent
                return homeComponent
            }

            Connections {
                target: router
                function onNavigationRequested(route) {
                    if (route === "/") shellStack.replace(homeComponent)
                    else if (route === "/templates") shellStack.replace(browseComponent)
                    else if (route.startsWith("/templates/")) shellStack.push(templateDetailComponent)
                    else if (route === "/create") shellStack.replace(goCraftComponent)
                    else if (route === "/profile") shellStack.replace(profileComponent)
                }
            }
        }

        // Bottom navigation bar
        TabBar {
            id: bottomNav
            Layout.fillWidth: true
            currentIndex: selectedIndex

            TabButton {
                icon.name: "home"
                text: "Home"
                onClicked: router.go("/")
            }

            TabButton {
                icon.name: "grid"
                text: "Templates"
                onClicked: router.go("/templates")
            }

            TabButton {
                icon.name: "auto-fix"
                text: "Go Craft"
                onClicked: router.go("/create")
            }

            TabButton {
                icon.name: "person"
                text: "Profile"
                onClicked: router.go("/profile")
            }
        }
    }

    // Shell route components
    Component {
        id: homeComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/template_browser/screens/HomeScreen.qml" }
    }

    Component {
        id: browseComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/template_browser/screens/BrowseScreen.qml" }
    }

    Component {
        id: templateDetailComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/template_browser/screens/TemplateDetailScreen.qml" }
    }

    Component {
        id: goCraftComponent
        Loader { source: "qrc:/qt/qml/GopostApp/qml/go_craft/screens/GoCraftScreen.qml" }
    }

    Component {
        id: profileComponent
        Page {
            Label {
                anchors.centerIn: parent
                text: "Profile"
                font.pixelSize: 24
            }
        }
    }
}
