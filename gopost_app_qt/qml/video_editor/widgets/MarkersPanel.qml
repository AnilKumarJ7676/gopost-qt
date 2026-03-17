import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Panel for adding markers (chapter/comment/todo/sync), navigation, and marker list.
/// Converted from markers_panel.dart.
Item {
    id: root

    property int selectedType: 0   // 0=chapter, 1=comment, 2=todo, 3=sync

    readonly property var markerTypes: [
        { id: 0, label: "Chapter", icon: "\uD83D\uDD16", color: "#6C63FF" },
        { id: 1, label: "Comment", icon: "\uD83D\uDCAC", color: "#26C6DA" },
        { id: 2, label: "To-Do",   icon: "\u2705",       color: "#FF7043" },
        { id: 3, label: "Sync",    icon: "\uD83D\uDD04", color: "#66BB6A" }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Add Marker section ---
        Pane {
            Layout.fillWidth: true
            padding: 12

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "Add Marker"
                        font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0"
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: internal.formatTime(timelineNotifier ? timelineNotifier.position : 0)
                        font.pixelSize: 12; font.family: Theme.fontFamilyMono; color: "#6B6B88"
                    }
                }

                // Marker type selector
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Repeater {
                        model: root.markerTypes
                        delegate: Rectangle {
                            property bool isActive: root.selectedType === modelData.id
                            readonly property color typeColor: modelData.color || "#6C63FF"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            radius: 6
                            color: isActive ? Qt.rgba(typeColor.r, typeColor.g, typeColor.b, 0.2) : "#1A1A34"
                            border.color: isActive ? typeColor : "#303050"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.icon
                                font.pixelSize: 16
                                color: isActive ? typeColor : "#6B6B88"
                            }

                            ToolTip.visible: hoverHandler.hovered
                            ToolTip.text: modelData.label
                            HoverHandler { id: hoverHandler }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectedType = modelData.id
                            }
                        }
                    }
                }

                // Label input + Add button
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    TextField {
                        id: labelInput
                        Layout.fillWidth: true
                        placeholderText: "Marker label (optional)"
                        font.pixelSize: 13
                        color: "#E0E0F0"
                        placeholderTextColor: "#6B6B88"

                        background: Rectangle {
                            radius: 6
                            color: "#1A1A34"
                            border.color: "#303050"
                        }
                    }

                    Button {
                        Layout.preferredHeight: 34
                        text: "+ Add"
                        font.pixelSize: 13

                        background: Rectangle {
                            radius: 6
                            color: "#6C63FF"
                        }
                        contentItem: Label {
                            text: "+ Add"
                            color: "white"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                        }

                        onClicked: {
                            timelineNotifier.addMarkerWithType(root.selectedType, labelInput.text.trim())
                            labelInput.clear()
                        }
                    }
                }
            }
        }

        // --- Navigation bar ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: "#14142B"

            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#252540" }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#252540" }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8; anchors.rightMargin: 8

                ToolButton {
                    icon.name: "media-skip-backward"
                    icon.width: 18; icon.height: 18; icon.color: "#B0B0C8"
                    ToolTip.text: "Previous marker"
                    onClicked: timelineNotifier.navigateToPreviousMarker()
                }
                ToolButton {
                    icon.name: "media-skip-forward"
                    icon.width: 18; icon.height: 18; icon.color: "#B0B0C8"
                    ToolTip.text: "Next marker"
                    onClicked: timelineNotifier.navigateToNextMarker()
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: (timelineNotifier ? timelineNotifier.markerCount : 0) + " markers"
                    font.pixelSize: 12; color: "#6B6B88"
                }
            }
        }

        // --- Marker list ---
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 0

            model: timelineNotifier ? timelineNotifier.markers : []

            // Empty state
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                visible: parent.count === 0

                Label {
                    text: "\uD83D\uDD16"
                    font.pixelSize: 38; color: "#404060"
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "No markers yet"
                    font.pixelSize: 14; color: "#6B6B88"
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "Add markers to annotate your timeline"
                    font.pixelSize: 12; color: "#505070"
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            delegate: Rectangle {
                readonly property var marker: modelData || ({})
                readonly property color markerColor: marker.color || "#6C63FF"

                width: ListView.view.width
                height: markerRow.implicitHeight + 16
                color: "transparent"

                Component.onCompleted: console.log("[Markers] delegate created: id=", marker.id,
                    "label=", marker.label, "pos=", marker.positionSeconds, "color=", marker.color)

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 0.5
                    color: "#1E1E38"
                }

                RowLayout {
                    id: markerRow
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12
                    anchors.topMargin: 8; anchors.bottomMargin: 8
                    spacing: 8

                    // Type icon
                    Rectangle {
                        width: 28; height: 28; radius: 6
                        color: Qt.rgba(markerColor.r, markerColor.g, markerColor.b, 0.12)

                        Label {
                            anchors.centerIn: parent
                            text: marker.icon || "\uD83D\uDD16"
                            font.pixelSize: 16; color: markerColor
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: marker.label || marker.typeName || "Marker"
                            font.pixelSize: 13; font.weight: Font.Medium
                            color: "#E0E0F0"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: internal.formatTime(marker.positionSeconds) + "  \u2022  " + (marker.typeName || "")
                            font.pixelSize: 11
                            color: Qt.rgba(markerColor.r, markerColor.g, markerColor.b, 0.7)
                        }
                    }

                    ToolButton {
                        icon.name: "document-edit"
                        icon.width: 16; icon.height: 16; icon.color: "#6B6B88"
                        onClicked: {
                            console.log("[Markers] edit marker:", marker.id)
                            timelineNotifier.editMarker(marker.id)
                        }
                    }
                    ToolButton {
                        icon.name: "window-close"
                        icon.width: 16; icon.height: 16; icon.color: "#6B6B88"
                        onClicked: {
                            console.log("[Markers] remove marker:", marker.id)
                            timelineNotifier.removeMarker(marker.id)
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    z: -1
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        console.log("[Markers] navigate to marker:", marker.id, "at", marker.positionSeconds)
                        timelineNotifier.navigateToMarker(marker.id)
                    }
                }
            }
        }
    }

    QtObject {
        id: internal

        function formatTime(s) {
            if (s === undefined || s === null) return "00:00.00"
            var m = Math.floor(s / 60)
            var sec = Math.floor(s % 60)
            var ms = Math.floor((s % 1) * 100)
            return String(m).padStart(2, '0') + ":" + String(sec).padStart(2, '0') + "." + String(ms).padStart(2, '0')
        }
    }
}
