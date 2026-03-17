import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Dock system components: DockablePanelHeader, FloatingPanel, DockZoneOverlay,
/// DockTabGroup, LayoutPresetSelector.
/// Converted from dock_system.dart.
Item {
    id: root

    // =========================================================================
    // DockablePanelHeader — drag-to-undock + double-click maximize
    // =========================================================================
    component DockablePanelHeader: Rectangle {
        id: dockHeader

        property string panelId: ""
        property string title: ""
        property bool isMaximized: false
        signal maximize()
        signal undock()
        signal dragUpdated(real dx, real dy)
        signal dragEnded()
        signal closePanel()

        height: 32
        color: "#16162E"

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width; height: 1
            color: "#252540"
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10; anchors.rightMargin: 10
            spacing: 6

            Label {
                text: "\u2630"
                font.pixelSize: 14
                color: "#505068"
            }
            Label {
                text: dockHeader.title
                font.pixelSize: 12; font.weight: Font.DemiBold
                color: "#D0D0E8"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            // Maximize button
            Rectangle {
                width: 22; height: 22; radius: 4
                color: "transparent"
                visible: true

                Label {
                    anchors.centerIn: parent
                    text: dockHeader.isMaximized ? "\u2B1C" : "\u26F6"
                    font.pixelSize: 14; color: "#8888A0"
                }
                ToolTip.text: dockHeader.isMaximized ? "Restore" : "Maximize"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dockHeader.maximize()
                }
            }

            // Close button
            Rectangle {
                width: 22; height: 22; radius: 4
                color: "transparent"

                Label {
                    anchors.centerIn: parent
                    text: "\u2715"
                    font.pixelSize: 12; color: "#8888A0"
                }
                ToolTip.text: "Close"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dockHeader.closePanel()
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            z: -1
            property real startX: 0
            property real startY: 0
            property bool isDragging: false

            onDoubleClicked: dockHeader.maximize()

            onPressed: mouse => {
                startX = mouse.x; startY = mouse.y
                isDragging = false
            }
            onPositionChanged: mouse => {
                var dist = Math.sqrt(Math.pow(mouse.x - startX, 2) + Math.pow(mouse.y - startY, 2))
                if (!isDragging && dist > 10) {
                    isDragging = true
                    dockHeader.undock()
                }
                if (isDragging) {
                    dockHeader.dragUpdated(mouse.x - startX, mouse.y - startY)
                }
            }
            onReleased: {
                if (isDragging) {
                    isDragging = false
                    dockHeader.dragEnded()
                }
            }
        }
    }

    // =========================================================================
    // FloatingPanel — overlaid on editor when undocked
    // =========================================================================
    component FloatingPanel: Rectangle {
        id: floatingPanel

        property string title: ""
        property alias content: contentLoader.sourceComponent
        signal redock()
        signal closePanel()

        property real panelX: 100
        property real panelY: 100
        property real panelW: 320
        property real panelH: 280

        x: panelX; y: panelY
        width: panelW; height: panelH
        radius: 8
        color: "#12122A"
        border.color: Qt.rgba(0.424, 0.388, 1.0, 0.4)
        clip: true

        // Shadow
        layer.enabled: true
        layer.effect: Item {}

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Title bar — drag to move
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: "#1A1A38"

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 1; color: "#252540"
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10; anchors.rightMargin: 10
                    spacing: 6

                    Label { text: "\u2630"; font.pixelSize: 12; color: "#505068" }
                    Label {
                        text: floatingPanel.title
                        font.pixelSize: 11; font.weight: Font.DemiBold
                        color: "#D0D0E8"; elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Re-dock
                    Rectangle {
                        width: 20; height: 20; radius: 4; color: "transparent"
                        Label { anchors.centerIn: parent; text: "\uD83D\uDCCC"; font.pixelSize: 10; color: "#8888A0" }
                        ToolTip.text: "Re-dock"
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: floatingPanel.redock() }
                    }

                    // Close
                    Rectangle {
                        width: 20; height: 20; radius: 4; color: "transparent"
                        Label { anchors.centerIn: parent; text: "\u2715"; font.pixelSize: 10; color: "#8888A0" }
                        ToolTip.text: "Close"
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: floatingPanel.closePanel() }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    z: -1
                    onDoubleClicked: floatingPanel.redock()
                    property real dragStartX: 0
                    property real dragStartY: 0
                    onPressed: mouse => { dragStartX = mouse.x; dragStartY = mouse.y }
                    onPositionChanged: mouse => {
                        if (pressed) {
                            floatingPanel.panelX += mouse.x - dragStartX
                            floatingPanel.panelY += mouse.y - dragStartY
                        }
                    }
                }
            }

            // Content
            Loader {
                id: contentLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Resize handle
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 16

                MouseArea {
                    anchors.right: parent.right
                    width: 16; height: 16
                    cursorShape: Qt.SizeFDiagCursor

                    onPositionChanged: mouse => {
                        if (pressed) {
                            floatingPanel.panelW = Math.max(200, Math.min(800, floatingPanel.panelW + mouse.x))
                            floatingPanel.panelH = Math.max(150, Math.min(800, floatingPanel.panelH + mouse.y))
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "\u2922"
                        font.pixelSize: 10; color: "#505068"
                    }
                }
            }
        }
    }

    // =========================================================================
    // DockZoneOverlay — highlight when dragging over dock target
    // =========================================================================
    component DockZoneOverlay: Rectangle {
        id: dockZone

        property bool isActive: false
        property string label: "Drop to dock"

        visible: isActive
        color: Qt.rgba(0.424, 0.388, 1.0, 0.1)
        border.color: Qt.rgba(0.424, 0.388, 1.0, 0.6)
        border.width: 2
        radius: 8

        Rectangle {
            anchors.centerIn: parent
            width: zoneLabel.implicitWidth + 32
            height: zoneLabel.implicitHeight + 16
            radius: 6
            color: Qt.rgba(0.424, 0.388, 1.0, 0.2)

            Label {
                id: zoneLabel
                anchors.centerIn: parent
                text: dockZone.label
                font.pixelSize: 13; font.weight: Font.DemiBold
                color: "#6C63FF"
            }
        }
    }

    // =========================================================================
    // DockTabGroup — multiple panels as tabs
    // =========================================================================
    component DockTabGroup: ColumnLayout {
        id: tabGroup

        property var tabLabels: []
        property int activeIndex: 0
        property alias content: tabContentLoader.sourceComponent
        signal tabSelected(int index)
        signal tabClosed(int index)

        spacing: 0

        // Tab bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#0E0E1C"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1; color: "#252540"
            }

            Row {
                anchors.fill: parent

                Repeater {
                    model: tabGroup.tabLabels
                    delegate: Rectangle {
                        property bool isActive: index === tabGroup.activeIndex
                        width: tabText.implicitWidth + 24 + (closeBtn.visible ? 18 : 0)
                        height: 30
                        color: isActive ? "#12122A" : "transparent"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width; height: 2
                            color: isActive ? "#6C63FF" : "transparent"
                        }

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4

                            Label {
                                id: tabText
                                text: modelData
                                font.pixelSize: 11
                                font.weight: isActive ? Font.DemiBold : Font.Normal
                                color: isActive ? "#D0D0E8" : "#6B6B88"
                            }

                            Label {
                                id: closeBtn
                                visible: tabGroup.tabLabels.length > 1
                                text: "\u2715"
                                font.pixelSize: 8
                                color: isActive ? "#8888A0" : "#505068"

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: tabGroup.tabClosed(index)
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            cursorShape: Qt.PointingHandCursor
                            onClicked: tabGroup.tabSelected(index)
                        }
                    }
                }
            }
        }

        // Content
        Loader {
            id: tabContentLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // =========================================================================
    // LayoutPresetSelector
    // =========================================================================
    component LayoutPresetSelector: RowLayout {
        id: presetSelector

        property var presets: ["Default", "Minimal", "Stacked", "Custom"]
        property int activePreset: 0
        signal presetSelected(int index)
        signal saveCustom()

        spacing: 4

        Repeater {
            model: presetSelector.presets
            delegate: Rectangle {
                property bool isActive: index === presetSelector.activePreset
                width: chipLabel.implicitWidth + 20
                height: 24
                radius: 4
                color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "transparent"
                border.color: isActive ? "#6C63FF" : "#353550"

                Label {
                    id: chipLabel
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 11
                    font.weight: isActive ? Font.DemiBold : Font.Normal
                    color: isActive ? "#6C63FF" : "#8888A0"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: presetSelector.presetSelected(index)
                }
            }
        }

        // Save custom button
        Rectangle {
            width: 24; height: 24; radius: 4; color: "transparent"

            Label {
                anchors.centerIn: parent
                text: "\uD83D\uDCBE"
                font.pixelSize: 14; color: "#6B6B88"
            }
            ToolTip.text: "Save current layout"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: presetSelector.saveCustom()
            }
        }
    }
}
