import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

Page {
    id: root

    // homeNotifier available via context property from ServiceLocator

    Component.onCompleted: {
        if (homeNotifier)
            homeNotifier.load()
    }

    ScrollView {
        anchors.fill: parent

        Flickable {
            contentWidth: parent.width
            contentHeight: contentColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.DragOverBounds

            // Pull-to-refresh
            onContentYChanged: {
                if (contentY < -80 && !refreshTimer.running && dragging) {
                    refreshTimer.start()
                    if (homeNotifier)
                        homeNotifier.refresh()
                }
            }

            Timer {
                id: refreshTimer
                interval: 1000
            }

            ColumnLayout {
                id: contentColumn
                width: parent.width
                spacing: 0

                // Loading state
                Loader {
                    Layout.fillWidth: true
                    active: homeNotifier && homeNotifier.isLoading &&
                            (!homeNotifier.featured || homeNotifier.featured.length === 0)
                    visible: active
                    sourceComponent: loadingComponent
                }

                // Header
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    visible: !homeNotifier || !homeNotifier.isLoading ||
                             (homeNotifier.featured && homeNotifier.featured.length > 0)

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: "Gopost"
                            font.pixelSize: 28
                            font.weight: Font.ExtraBold
                            color: palette.highlight
                        }

                        Label {
                            text: "Create something amazing"
                            font.pixelSize: 14
                            color: palette.placeholderText
                        }
                    }

                    ToolButton {
                        width: 40; height: 40

                        Label {
                            anchors.centerIn: parent
                            text: "\ue7f4" // notifications_outlined
                            font.pixelSize: 22
                            color: palette.placeholderText
                        }
                    }
                }

                // Featured Banner Carousel
                Loader {
                    Layout.fillWidth: true
                    Layout.preferredHeight: active ? 240 : 0
                    active: homeNotifier && homeNotifier.featured && homeNotifier.featured.length > 0
                    visible: active
                    sourceComponent: featuredBannerComponent
                }

                // Quick Create Buttons
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    spacing: 12
                    visible: !homeNotifier || !homeNotifier.isLoading ||
                             (homeNotifier.featured && homeNotifier.featured.length > 0)

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        radius: 12
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: palette.highlight }
                            GradientStop { position: 1.0; color: Qt.darker(palette.highlight, 1.15) }
                        }

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 8

                            Label {
                                text: "\ue04b" // videocam_rounded
                                font.pixelSize: 20
                                color: "white"
                            }

                            Label {
                                text: "New Video"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: "white"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: router.push("/editor/video2")
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        radius: 12
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#00B894" }
                            GradientStop { position: 1.0; color: Qt.darker("#00B894", 1.15) }
                        }

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 8

                            Label {
                                text: "\ue3f4" // image_rounded
                                font.pixelSize: 20
                                color: "white"
                            }

                            Label {
                                text: "New Image"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: "white"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: router.push("/editor/image")
                        }
                    }
                }

                // Categories Quick Access
                Loader {
                    Layout.fillWidth: true
                    active: homeNotifier && homeNotifier.categories && homeNotifier.categories.length > 0
                    visible: active
                    sourceComponent: categoryQuickAccessComponent
                }

                // Trending Section
                Loader {
                    Layout.fillWidth: true
                    active: homeNotifier && homeNotifier.trending && homeNotifier.trending.length > 0
                    visible: active
                    sourceComponent: Component {
                        HorizontalTemplateSection {
                            title: "Trending Now"
                            iconText: "\ue80e" // local_fire_department
                            templates: homeNotifier.trending
                            onViewAllClicked: router.push("/templates")
                            onTemplateTapped: function(id) { router.push("/templates/" + id) }
                        }
                    }
                }

                // Recently Added Section
                Loader {
                    Layout.fillWidth: true
                    active: homeNotifier && homeNotifier.recent && homeNotifier.recent.length > 0
                    visible: active
                    sourceComponent: Component {
                        HorizontalTemplateSection {
                            title: "Recently Added"
                            iconText: "\ue8b5" // schedule
                            templates: homeNotifier.recent
                            onViewAllClicked: router.push("/templates")
                            onTemplateTapped: function(id) { router.push("/templates/" + id) }
                        }
                    }
                }

                // Bottom spacer
                Item { Layout.preferredHeight: 48 }
            }
        }
    }

    // Featured Banner Component
    Component {
        id: featuredBannerComponent

        ColumnLayout {
            spacing: 8

            SwipeView {
                id: featuredSwipe
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                clip: true

                Repeater {
                    model: homeNotifier ? homeNotifier.featured : []

                    delegate: Item {
                        required property var modelData
                        required property int index
                        width: featuredSwipe.width
                        height: featuredSwipe.height

                        Rectangle {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            radius: 16
                            clip: true
                            color: palette.base

                            Image {
                                anchors.fill: parent
                                source: modelData.thumbnailUrl || ""
                                fillMode: Image.PreserveAspectCrop
                                visible: modelData.thumbnailUrl ? true : false
                                asynchronous: true
                            }

                            // Fallback gradient
                            Rectangle {
                                anchors.fill: parent
                                visible: !modelData.thumbnailUrl
                                gradient: Gradient {
                                    orientation: Gradient.Horizontal
                                    GradientStop { position: 0.0; color: palette.highlight }
                                    GradientStop { position: 1.0; color: Qt.lighter(palette.highlight, 1.4) }
                                }
                            }

                            // Bottom gradient overlay
                            Rectangle {
                                anchors.fill: parent
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "transparent" }
                                    GradientStop { position: 0.6; color: "transparent" }
                                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.7) }
                                }
                            }

                            // Banner text
                            ColumnLayout {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.margins: 16
                                spacing: 4

                                Rectangle {
                                    visible: modelData.isPremium || false
                                    width: featuredLabel.implicitWidth + 12
                                    height: featuredLabel.implicitHeight + 4
                                    radius: 4
                                    color: palette.highlight

                                    Label {
                                        id: featuredLabel
                                        anchors.centerIn: parent
                                        text: "FEATURED"
                                        font.pixelSize: 10
                                        font.weight: Font.Bold
                                        color: "white"
                                    }
                                }

                                Label {
                                    text: modelData.name || ""
                                    font.pixelSize: 16
                                    font.weight: Font.Bold
                                    color: "white"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: router.push("/templates/" + modelData.id)
                            }
                        }
                    }
                }
            }

            // Page indicators
            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 6
                visible: homeNotifier && homeNotifier.featured && homeNotifier.featured.length > 1

                Repeater {
                    model: homeNotifier ? homeNotifier.featured.length : 0

                    delegate: Rectangle {
                        required property int index
                        width: featuredSwipe.currentIndex === index ? 20 : 6
                        height: 6
                        radius: 3
                        color: featuredSwipe.currentIndex === index
                               ? palette.highlight
                               : Qt.rgba(palette.text.r, palette.text.g, palette.text.b, 0.3)

                        Behavior on width {
                            NumberAnimation { duration: 200 }
                        }
                    }
                }
            }
        }
    }

    // Category Quick Access Component
    Component {
        id: categoryQuickAccessComponent

        ColumnLayout {
            Layout.leftMargin: 16
            Layout.topMargin: 24
            spacing: 12

            Label {
                text: "Categories"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                orientation: ListView.Horizontal
                spacing: 12
                clip: true
                model: homeNotifier ? homeNotifier.categories : []

                delegate: Item {
                    required property var modelData
                    width: 72; height: 80

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 48; height: 48
                            radius: 12
                            color: palette.alternateBase

                            Label {
                                anchors.centerIn: parent
                                text: {
                                    var icons = {
                                        "social-media": "\ue80d",
                                        "marketing": "\ue0c9",
                                        "presentation": "\ue41b",
                                        "youtube": "\ue039",
                                        "instagram": "\ue412",
                                        "tiktok": "\ue405",
                                        "education": "\ue80c",
                                        "business": "\ue8f9",
                                        "personal": "\ue7fd"
                                    }
                                    return icons[modelData.slug] || "\ue574"
                                }
                                font.pixelSize: 22
                                color: palette.highlight
                            }
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 72
                            text: modelData.name || ""
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: router.push("/templates")
                    }
                }
            }
        }
    }

    // Horizontal Template Section (inline component)
    component HorizontalTemplateSection: ColumnLayout {
        property string title
        property string iconText
        property var templates

        signal viewAllClicked()
        signal templateTapped(string id)

        Layout.topMargin: 24
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 8

            Label {
                text: iconText
                font.pixelSize: 20
                color: palette.highlight
            }

            Label {
                text: title
                font.pixelSize: 16
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }

            Label {
                text: "View All"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: palette.highlight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: viewAllClicked()
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            orientation: ListView.Horizontal
            spacing: 12
            clip: true
            leftMargin: 16
            rightMargin: 16
            model: templates

            delegate: Item {
                required property var modelData
                width: 140; height: 200

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 12
                        clip: true
                        color: palette.alternateBase

                        Image {
                            anchors.fill: parent
                            source: modelData.thumbnailUrl || ""
                            fillMode: Image.PreserveAspectCrop
                            visible: modelData.thumbnailUrl ? true : false
                            asynchronous: true
                        }

                        // Play icon overlay for video
                        Rectangle {
                            visible: modelData.isVideo || false
                            anchors.centerIn: parent
                            width: 32; height: 32
                            radius: 16
                            color: Qt.rgba(0, 0, 0, 0.4)

                            Label {
                                anchors.centerIn: parent
                                text: "\ue037" // play_arrow
                                font.pixelSize: 16
                                color: "white"
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: !modelData.thumbnailUrl && !(modelData.isVideo || false)
                            text: "\ue3f4" // image
                            font.pixelSize: 24
                            color: palette.placeholderText
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.name || ""
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        spacing: 4

                        Rectangle {
                            visible: modelData.isPremium || false
                            width: proLabel.implicitWidth + 8
                            height: proLabel.implicitHeight + 2
                            radius: 3
                            color: palette.highlight

                            Label {
                                id: proLabel
                                anchors.centerIn: parent
                                text: "PRO"
                                font.pixelSize: 8
                                font.weight: Font.Bold
                                color: "white"
                            }
                        }

                        Label {
                            text: modelData.typeName || (modelData.isVideo ? "video" : "image")
                            font.pixelSize: 11
                            color: palette.placeholderText
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: templateTapped(modelData.id)
                }
            }
        }
    }

    // Loading Component
    Component {
        id: loadingComponent

        ColumnLayout {
            spacing: 0

            Item { Layout.preferredHeight: 80 }

            ShimmerLoading {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                radius: 0
            }

            Item { Layout.preferredHeight: 24 }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 12

                ShimmerLoading { implicitWidth: 150; implicitHeight: 20 }

                ShimmerLoading {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 160
                }

                Item { Layout.preferredHeight: 12 }

                ShimmerLoading { implicitWidth: 150; implicitHeight: 20 }

                ShimmerLoading {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 160
                }
            }
        }
    }
}
