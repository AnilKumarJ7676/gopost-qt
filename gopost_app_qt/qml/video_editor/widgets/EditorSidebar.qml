import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// EditorIconRail (12 tabs) + EditorPanelArea switching between all sidebar panels.
/// Converted from editor_sidebar.dart.
Item {
    id: root

    readonly property real iconRailWidth: 56
    property bool panelOpen: true

    readonly property var tabs: [
        { tab: 0,  icon: "\uD83C\uDFA5", label: "Media" },
        { tab: 1,  icon: "\u24D8",       label: "Inspector" },
        { tab: 2,  icon: "\uD83D\uDD24", label: "Text" },
        { tab: 3,  icon: "\u2728",       label: "Effects" },
        { tab: 4,  icon: "\uD83C\uDFA8", label: "Color" },
        { tab: 5,  icon: "\u21C4",       label: "Transitions" },
        { tab: 6,  icon: "\u2B6F",       label: "Transform" },
        { tab: 7,  icon: "\u26A1",       label: "Speed" },
        { tab: 8,  icon: "\u2014",       label: "Keyframes" },
        { tab: 9,  icon: "\uD83C\uDFB5", label: "Audio" },
        { tab: 10, icon: "\uD83D\uDD16", label: "Markers" },
        { tab: 11, icon: "\u25A3",       label: "Adjust" }
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
                            property bool isActive: timelineNotifier && timelineNotifier.activePanel === modelData.tab && root.panelOpen
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
                                    if (timelineNotifier.activePanel === modelData.tab) {
                                        root.panelOpen = !root.panelOpen
                                    } else {
                                        timelineNotifier.setActivePanel(modelData.tab)
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
                                var panel = timelineNotifier ? timelineNotifier.activePanel : 0
                                for (var i = 0; i < root.tabs.length; i++) {
                                    if (root.tabs[i].tab === panel) return root.tabs[i].label
                                }
                                return "Panel"
                            }
                            font.pixelSize: 15; font.weight: Font.DemiBold
                            color: "#E0E0F0"
                            Layout.fillWidth: true
                        }

                        ToolButton {
                            icon.name: "window-maximize"
                            icon.width: 18; icon.height: 18
                            icon.color: "#8888A0"
                            ToolTip.text: "Maximize"
                            onClicked: editorLayoutNotifier.toggleMaximize("sidebar")
                        }
                        ToolButton {
                            icon.name: "window-close"
                            icon.width: 18; icon.height: 18
                            icon.color: "#8888A0"
                            ToolTip.text: "Collapse"
                            onClicked: root.panelOpen = false
                        }
                    }
                }

                // Panel content switcher
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: timelineNotifier ? timelineNotifier.activePanel : 0

                    MediaPoolPanel {}        // 0 - Media
                    ClipInspectorPanel {}     // 1 - Inspector
                    TextEditorPanel {}        // 2 - Text
                    EffectsPanel {}           // 3 - Effects
                    ColorGradingPanel {}      // 4 - Color
                    TransitionPicker {}       // 5 - Transitions
                    CropTransformPanel {}     // 6 - Transform
                    SpeedControlsPanel {}     // 7 - Speed
                    KeyframeEditor {}         // 8 - Keyframes
                    AudioControlsPanel {}     // 9 - Audio
                    MarkersPanel {}           // 10 - Markers
                    AdjustmentLayerPanel {}   // 11 - Adjust
                }
            }
        }
    }
}
