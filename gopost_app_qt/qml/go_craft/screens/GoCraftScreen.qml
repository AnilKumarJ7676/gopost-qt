import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    // goCraftNotifier available via context property

    Component.onCompleted: {
        if (goCraftNotifier)
            goCraftNotifier.loadProjects()
    }

    ScrollView {
        anchors.fill: parent

        Flickable {
            contentWidth: parent.width
            contentHeight: contentColumn.implicitHeight

            ColumnLayout {
                id: contentColumn
                width: parent.width
                spacing: 0

                // Header
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 20
                    Layout.bottomMargin: 8
                    spacing: 2

                    Label {
                        text: "Go Craft"
                        font.pixelSize: 28
                        font.weight: Font.ExtraBold
                        color: palette.highlight
                    }

                    Label {
                        text: "Start creating or continue where you left off"
                        font.pixelSize: 14
                        color: palette.placeholderText
                    }
                }

                // Editor Cards
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 12
                    spacing: 12

                    // Video Editor Card
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 16
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: palette.highlight }
                            GradientStop { position: 1.0; color: Qt.darker(palette.highlight, 1.3) }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: router.push("/editor/video")
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8

                            Rectangle {
                                width: 40; height: 40
                                radius: 8
                                color: Qt.rgba(1, 1, 1, 0.2)

                                Label {
                                    anchors.centerIn: parent
                                    text: "\ue04b" // videocam icon
                                    font.pixelSize: 22
                                    color: "white"
                                }
                            }

                            Label {
                                text: "Video Editor"
                                font.pixelSize: 14
                                font.weight: Font.Bold
                                color: "white"
                            }

                            Label {
                                text: "Create & edit videos"
                                font.pixelSize: 12
                                color: Qt.rgba(1, 1, 1, 0.8)
                            }
                        }
                    }

                    // Image Editor Card
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 16
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00B894" }
                            GradientStop { position: 1.0; color: Qt.darker("#00B894", 1.3) }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: router.push("/editor/image")
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8

                            Rectangle {
                                width: 40; height: 40
                                radius: 8
                                color: Qt.rgba(1, 1, 1, 0.2)

                                Label {
                                    anchors.centerIn: parent
                                    text: "\ue3f4" // image icon
                                    font.pixelSize: 22
                                    color: "white"
                                }
                            }

                            Label {
                                text: "Image Editor"
                                font.pixelSize: 14
                                font.weight: Font.Bold
                                color: "white"
                            }

                            Label {
                                text: "Design & edit images"
                                font.pixelSize: 12
                                color: Qt.rgba(1, 1, 1, 0.8)
                            }
                        }
                    }
                }

                // Video Editor 2 Card (full width)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 12
                    Layout.preferredHeight: 80
                    radius: 16
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#6C5CE7" }
                        GradientStop { position: 1.0; color: "#00B894" }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: router.push("/editor/video2")
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Rectangle {
                            width: 48; height: 48; radius: 12
                            color: Qt.rgba(1, 1, 1, 0.2)
                            Label {
                                anchors.centerIn: parent
                                text: "\uD83C\uDFAC"
                                font.pixelSize: 24
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: "Video Editor 2"
                                font.pixelSize: 16; font.weight: Font.Bold; color: "white"
                            }
                            Label {
                                text: "New timeline editor with full engine integration"
                                font.pixelSize: 12; color: Qt.rgba(1, 1, 1, 0.8)
                            }
                        }

                        Label {
                            text: "\u2192"
                            font.pixelSize: 24; color: "white"
                        }
                    }
                }

                // My Projects Section Header
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 24
                    Layout.bottomMargin: 12
                    spacing: 8

                    Label {
                        text: "\ue2c7" // folder icon
                        font.pixelSize: 20
                        color: palette.highlight
                    }

                    Label {
                        text: "My Projects"
                        font.pixelSize: 16
                        font.weight: Font.Bold
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        visible: goCraftNotifier && goCraftNotifier.projects.length > 0
                        text: goCraftNotifier ? goCraftNotifier.projects.length + " project" +
                              (goCraftNotifier.projects.length === 1 ? "" : "s") : ""
                        font.pixelSize: 12
                        color: palette.placeholderText
                    }
                }

                // Loading indicator
                BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 48
                    visible: goCraftNotifier && goCraftNotifier.isLoading &&
                             (!goCraftNotifier.projects || goCraftNotifier.projects.length === 0)
                    running: visible
                }

                // Empty state
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 32
                    spacing: 8
                    visible: goCraftNotifier && !goCraftNotifier.isLoading &&
                             goCraftNotifier.projects.length === 0

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 72; height: 72
                        radius: 16
                        color: palette.alternateBase

                        Label {
                            anchors.centerIn: parent
                            text: "\ue2c8" // folder_open icon
                            font.pixelSize: 36
                            color: palette.placeholderText
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "No projects yet"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: palette.placeholderText
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Start creating with the editors above.\nYour work will appear here."
                        font.pixelSize: 12
                        color: palette.placeholderText
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Project list
                Repeater {
                    model: goCraftNotifier ? goCraftNotifier.projects : []

                    delegate: ItemDelegate {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 8

                        required property var modelData
                        property bool isVideo: modelData.type === 1

                        contentItem: RowLayout {
                            spacing: 12

                            Rectangle {
                                width: 52; height: 52
                                radius: 8
                                color: isVideo ? palette.highlight : "#00B894"
                                opacity: 0.15

                                Label {
                                    anchors.centerIn: parent
                                    text: isVideo ? "\ue04b" : "\ue3f4"
                                    font.pixelSize: 24
                                    color: isVideo ? palette.highlight : "#00B894"
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    Rectangle {
                                        width: typeLabel.implicitWidth + 12
                                        height: typeLabel.implicitHeight + 4
                                        radius: 4
                                        color: isVideo ? palette.highlight : "#00B894"
                                        opacity: 0.15

                                        Label {
                                            id: typeLabel
                                            anchors.centerIn: parent
                                            text: isVideo ? "VIDEO" : "IMAGE"
                                            font.pixelSize: 9
                                            font.weight: Font.Bold
                                            color: isVideo ? palette.highlight : "#00B894"
                                        }
                                    }
                                }

                                Label {
                                    text: isVideo
                                          ? (modelData.trackCount || "") + " tracks"
                                          : modelData.width + " x " + modelData.height
                                    font.pixelSize: 12
                                    color: palette.placeholderText
                                }

                                Label {
                                    text: Qt.formatDateTime(modelData.updatedAt, "MMM d, yyyy h:mm AP")
                                    font.pixelSize: 11
                                    color: palette.placeholderText
                                }
                            }

                            ToolButton {
                                text: "\u22ee"
                                font.pixelSize: 18
                                onClicked: deleteMenu.open()

                                Menu {
                                    id: deleteMenu

                                    MenuItem {
                                        text: "Delete"
                                        onTriggered: deleteDialog.open()
                                    }
                                }
                            }
                        }

                        onClicked: {
                            if (isVideo)
                                router.push("/editor/video?projectId=" + modelData.id)
                            else
                                router.push("/editor/image?projectId=" + modelData.id)
                        }

                        Dialog {
                            id: deleteDialog
                            title: "Delete Project"
                            modal: true
                            anchors.centerIn: Overlay.overlay
                            standardButtons: Dialog.Cancel | Dialog.Ok

                            Label {
                                text: "Are you sure you want to delete \"" + modelData.name + "\"? This cannot be undone."
                                wrapMode: Text.Wrap
                            }

                            onAccepted: {
                                if (goCraftNotifier)
                                    goCraftNotifier.deleteProject(modelData)
                            }
                        }
                    }
                }

                // Bottom spacer
                Item { Layout.preferredHeight: 48 }
            }
        }
    }
}
