import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/**
 * VideoEditor2Screen — main layout composing all VE2 panels.
 *
 * Layout:
 *   ┌──────────────────────────────────────────────────────┐
 *   │ VE2Toolbar                                           │
 *   ├──────────────┬───────────────────────────────────────┤
 *   │ [Media Pool] │                                       │
 *   │ [Properties] │  Preview Panel                        │
 *   │  (tabbed     │                                       │
 *   │   sidebar)   │                                       │
 *   │              ├── split handle ───────────────────────┤
 *   │              │                                       │
 *   │              │  Timeline Panel                       │
 *   │              │                                       │
 *   └──────────────┴───────────────────────────────────────┘
 *
 * Cross-platform: resizable splitters, touch-friendly on mobile.
 * Initialises the timeline engine on load.
 */
Item {
    id: root

    property real verticalSplit: 0.55   // preview/timeline fraction
    property real sidebarWidth: 280     // property panel width
    property int  sidebarTab: 0         // 0 = Media Pool, 1 = Properties

    Component.onCompleted: {
        console.log("[VE2Screen] onCompleted — size:", width, "x", height)
        console.log("[VE2Screen] timelineNotifier:", timelineNotifier !== null,
                    "mediaPoolNotifier:", mediaPoolNotifier !== null)
        if (timelineNotifier) {
            timelineNotifier.initTimeline()
            console.log("[VE2Screen] initTimeline done — isReady:",
                        timelineNotifier.isReady, "trackCount:", timelineNotifier.trackCount)
        }
    }

    // ---- Keyboard shortcuts (NLE standard) ----
    Shortcut { sequence: "Space"; onActivated: if (timelineNotifier) timelineNotifier.togglePlayback() }
    Shortcut { sequence: "Ctrl+Z"; onActivated: if (timelineNotifier) timelineNotifier.undo() }
    Shortcut { sequence: "Ctrl+Shift+Z"; onActivated: if (timelineNotifier) timelineNotifier.redo() }
    Shortcut { sequence: "Ctrl+B"; onActivated: if (timelineNotifier) timelineNotifier.splitClipAtPlayhead() }
    Shortcut { sequence: "Delete"; onActivated: internal.deleteSelected() }
    Shortcut { sequence: "Shift+Delete"; onActivated: { if (timelineNotifier && timelineNotifier.selectedClipId >= 0) timelineNotifier.rippleDelete(timelineNotifier.selectedClipId) } }
    Shortcut { sequence: "Ctrl+D"; onActivated: { if (timelineNotifier && timelineNotifier.selectedClipId >= 0) timelineNotifier.duplicateClip(timelineNotifier.selectedClipId) } }
    Shortcut { sequence: "Left"; onActivated: if (timelineNotifier) timelineNotifier.stepBackward() }
    Shortcut { sequence: "Right"; onActivated: if (timelineNotifier) timelineNotifier.stepForward() }
    Shortcut { sequence: "Shift+Left"; onActivated: if (timelineNotifier) timelineNotifier.stepBackwardN(10) }
    Shortcut { sequence: "Shift+Right"; onActivated: if (timelineNotifier) timelineNotifier.stepForwardN(10) }
    Shortcut { sequence: "Home"; onActivated: if (timelineNotifier) timelineNotifier.jumpToStart() }
    Shortcut { sequence: "End"; onActivated: if (timelineNotifier) timelineNotifier.jumpToEnd() }
    Shortcut { sequence: "J"; onActivated: if (timelineNotifier) timelineNotifier.shuttleReverse() }
    Shortcut { sequence: "K"; onActivated: if (timelineNotifier) timelineNotifier.shuttleStop() }
    Shortcut { sequence: "L"; onActivated: if (timelineNotifier) timelineNotifier.shuttleForward() }
    Shortcut { sequence: "I"; onActivated: if (timelineNotifier) timelineNotifier.setInPoint() }
    Shortcut { sequence: "O"; onActivated: if (timelineNotifier) timelineNotifier.setOutPoint() }
    Shortcut { sequence: "Alt+X"; onActivated: if (timelineNotifier) timelineNotifier.clearInOutPoints() }
    Shortcut { sequence: "M"; onActivated: if (timelineNotifier) timelineNotifier.addMarker() }
    Shortcut { sequence: "["; onActivated: if (timelineNotifier) timelineNotifier.navigateToPreviousMarker() }
    Shortcut { sequence: "]"; onActivated: if (timelineNotifier) timelineNotifier.navigateToNextMarker() }
    Shortcut { sequence: "Up"; onActivated: if (timelineNotifier) timelineNotifier.jumpToPreviousSnapPoint() }
    Shortcut { sequence: "Down"; onActivated: if (timelineNotifier) timelineNotifier.jumpToNextSnapPoint() }
    Shortcut { sequence: "Ctrl+="; onActivated: if (timelineNotifier) timelineNotifier.zoomIn() }
    Shortcut { sequence: "Ctrl+-"; onActivated: if (timelineNotifier) timelineNotifier.zoomOut() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // === Toolbar ===
        VE2Toolbar {
            id: toolbar
            Layout.fillWidth: true
            Layout.preferredHeight: 52
        }

        // === Main content ===
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ---- Left sidebar (tabbed: Media Pool / Properties) ----
            Rectangle {
                id: sidebar
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: sidebarWidth
                color: "#0D0D1A"
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Tab bar
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        color: "#0A0A18"
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#252540" }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            // Media Pool tab
                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: sidebarTab === 0 ? "#12122A" : "transparent"
                                Rectangle {
                                    anchors.bottom: parent.bottom; width: parent.width; height: 2
                                    color: sidebarTab === 0 ? "#6C63FF" : "transparent"
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: "Media Pool"
                                    font.pixelSize: 11
                                    font.weight: sidebarTab === 0 ? Font.DemiBold : Font.Normal
                                    color: sidebarTab === 0 ? "#E0E0F0" : "#6B6B88"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.sidebarTab = 0
                                }
                            }

                            // Properties tab
                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: sidebarTab === 1 ? "#12122A" : "transparent"
                                Rectangle {
                                    anchors.bottom: parent.bottom; width: parent.width; height: 2
                                    color: sidebarTab === 1 ? "#6C63FF" : "transparent"
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: "Properties"
                                    font.pixelSize: 11
                                    font.weight: sidebarTab === 1 ? Font.DemiBold : Font.Normal
                                    color: sidebarTab === 1 ? "#E0E0F0" : "#6B6B88"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.sidebarTab = 1
                                }
                            }
                        }
                    }

                    // Panel content
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        VE2MediaPoolPanel {
                            id: mediaPoolPanel
                            anchors.fill: parent
                            visible: sidebarTab === 0
                        }

                        VE2PropertyPanel {
                            id: propertyPanel
                            anchors.fill: parent
                            visible: sidebarTab === 1
                        }
                    }
                }
            }

            // Sidebar resize handle
            Rectangle {
                id: sidebarHandle
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                x: sidebarWidth
                width: 5
                color: sidebarHandleMa.containsMouse || sidebarHandleMa.pressed ? "#6C63FF" : "#1E1E38"
                z: 10

                Rectangle {
                    anchors.centerIn: parent
                    width: 2; height: 30; radius: 1
                    color: sidebarHandleMa.containsMouse || sidebarHandleMa.pressed ? Qt.rgba(1,1,1,0.5) : "#353550"
                }

                MouseArea {
                    id: sidebarHandleMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SplitHCursor
                    property real startX: 0
                    property real startWidth: 0

                    onPressed: mouse => { startX = mapToItem(root, mouse.x, 0).x; startWidth = sidebarWidth }
                    onPositionChanged: mouse => {
                        if (pressed) {
                            var gx = mapToItem(root, mouse.x, 0).x
                            var newW = startWidth + (gx - startX)
                            root.sidebarWidth = Math.max(0, Math.min(newW, root.width * 0.4))
                        }
                    }
                    onDoubleClicked: root.sidebarWidth = root.sidebarWidth > 0 ? 0 : 280
                }
            }

            // Right content area (preview + timeline)
            Column {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: sidebarHandle.right
                anchors.right: parent.right

                // Preview panel
                VE2PreviewPanel {
                    id: previewPanel
                    width: parent.width
                    height: {
                        var avail = parent.height - hSplitHandle.height
                        return Math.max(100, avail * root.verticalSplit)
                    }
                }

                // Horizontal split handle
                Rectangle {
                    id: hSplitHandle
                    width: parent.width
                    height: 5
                    color: hSplitMa.containsMouse || hSplitMa.pressed ? "#6C63FF" : "#1E1E38"

                    Rectangle {
                        anchors.centerIn: parent
                        width: 36; height: 2; radius: 1
                        color: hSplitMa.containsMouse || hSplitMa.pressed ? Qt.rgba(1,1,1,0.5) : "#353550"
                    }

                    MouseArea {
                        id: hSplitMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.SizeVerCursor
                        property real dragStartY: 0
                        property real dragStartFrac: 0

                        onPressed: mouse => {
                            dragStartY = mapToItem(previewPanel.parent, 0, mouse.y).y
                            dragStartFrac = root.verticalSplit
                        }
                        onPositionChanged: mouse => {
                            if (pressed) {
                                var gy = mapToItem(previewPanel.parent, 0, mouse.y).y
                                var total = previewPanel.parent.height - hSplitHandle.height
                                if (total <= 0) return
                                var delta = (gy - dragStartY) / total
                                root.verticalSplit = Math.max(0.15, Math.min(0.85, dragStartFrac + delta))
                            }
                        }
                    }
                }

                // Timeline panel
                VE2TimelinePanel {
                    id: timelinePanel
                    width: parent.width
                    height: {
                        var avail = parent.height - hSplitHandle.height
                        return Math.max(80, avail - avail * root.verticalSplit)
                    }
                }
            }
        }
    }

    // ================================================================
    // Internal helpers
    // ================================================================
    QtObject {
        id: internal

        function deleteSelected() {
            if (timelineNotifier && timelineNotifier.selectedClipId >= 0)
                timelineNotifier.removeClip(timelineNotifier.selectedClipId)
        }
    }
}
