import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/// VE2EditorSidebar — Icon rail (9 tabs) + Panel area.
/// Mirrors the EditorSidebar from video_editor but uses VE2 panels.
/// Tab mapping:
///   0 = Media Pool       (VE2MediaPoolPanel)
///   1 = Inspector         (VE2PropertyPanel tab 0)
///   2 = Effects           (VE2PropertyPanel tab 1)
///   3 = Color             (VE2PropertyPanel tab 2)
///   4 = Audio             (VE2PropertyPanel tab 3)
///   5 = Transform         (VE2PropertyPanel tab 4)
///   6 = Speed             (VE2PropertyPanel tab 5)
///   7 = Markers           (VE2PropertyPanel tab 6)
///   8 = Transitions       (VE2PropertyPanel tab 7)
Item {
    id: root

    readonly property real iconRailWidth: 56
    property bool panelOpen: true
    property int activePanel: 0

    readonly property var tabs: [
        { tab: 0,  icon: "\uD83C\uDFA5", label: "Media" },
        { tab: 1,  icon: "\u24D8",       label: "Inspector" },
        { tab: 2,  icon: "\u2728",       label: "Effects" },
        { tab: 3,  icon: "\uD83C\uDFA8", label: "Color" },
        { tab: 4,  icon: "\uD83C\uDFB5", label: "Audio" },
        { tab: 5,  icon: "\u2B1C",       label: "Transform" },
        { tab: 6,  icon: "\u26A1",       label: "Speed" },
        { tab: 7,  icon: "\uD83D\uDCCD", label: "Markers" },
        { tab: 8,  icon: "\uD83D\uDD17", label: "Transitions" }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // --- Icon Rail ---
        Rectangle {
            Layout.preferredWidth: root.iconRailWidth
            Layout.fillHeight: true
            color: "#0E0E1C"

            Rectangle {
                anchors.right: parent.right
                width: 1; height: parent.height
                color: "#252540"
            }

            Flickable {
                anchors.fill: parent
                anchors.topMargin: 4
                contentHeight: iconColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: iconColumn
                    width: parent.width

                    Repeater {
                        model: root.tabs
                        delegate: Rectangle {
                            property bool isActive: root.activePanel === modelData.tab && root.panelOpen
                            width: root.iconRailWidth
                            height: Math.min(Math.max(44, (root.height - 8) / root.tabs.length), 64)
                            radius: 6
                            color: isActive ? "#1A1A38" : "transparent"

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 3

                                Label {
                                    text: modelData.icon
                                    font.pixelSize: parent.parent.height > 50 ? 22 : 18
                                    color: isActive ? "#6C63FF" : "#6B6B88"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Label {
                                    text: modelData.label
                                    font.pixelSize: 10
                                    font.weight: isActive ? Font.DemiBold : Font.Normal
                                    color: isActive ? "#D0D0E8" : "#6B6B88"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }

                            ToolTip.visible: hoverHandler.hovered
                            ToolTip.text: modelData.label
                            HoverHandler { id: hoverHandler }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (root.activePanel === modelData.tab) {
                                        root.panelOpen = !root.panelOpen
                                    } else {
                                        root.activePanel = modelData.tab
                                        if (!root.panelOpen) root.panelOpen = true
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- Panel Area ---
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.panelOpen
            color: "#12122A"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Panel header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "#16162E"

                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width; height: 1; color: "#252540"
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14; anchors.rightMargin: 14
                        spacing: 6

                        Label {
                            text: "\u2630"
                            font.pixelSize: 14; color: "#505068"
                        }
                        Label {
                            text: {
                                for (var i = 0; i < root.tabs.length; i++) {
                                    if (root.tabs[i].tab === root.activePanel) return root.tabs[i].label
                                }
                                return "Panel"
                            }
                            font.pixelSize: 15; font.weight: Font.DemiBold
                            color: "#E0E0F0"
                            Layout.fillWidth: true
                        }

                        ToolButton {
                            contentItem: Label {
                                text: "\u2715"
                                font.pixelSize: 14; color: "#8888A0"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            ToolTip.text: "Collapse"
                            ToolTip.visible: hovered
                            onClicked: root.panelOpen = false
                        }
                    }
                }

                // Panel content
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // Media Pool (panel 0)
                    VE2MediaPoolPanel {
                        anchors.fill: parent
                        visible: root.activePanel === 0
                    }

                    // Property panels (panels 1-8) — mapped to VE2PropertyPanel tabs 0-7
                    VE2PropertyPanel {
                        anchors.fill: parent
                        visible: root.activePanel > 0
                        forceTabIndex: root.activePanel > 0 ? root.activePanel - 1 : -1
                    }
                }
            }
        }
    }
}
