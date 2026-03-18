import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/**
 * VideoEditor2Screen — main layout composing all VE2 panels.
 *
 * Layout (matches video editor 1 style):
 *   ┌──────────────────────────────────────────────────────┐
 *   │ VE2Toolbar (52px)                                    │
 *   ├──────────┬───┬───────────────────────────────────────┤
 *   │ Icon     │   │                                       │
 *   │ Rail  +  │ ┃ │  Preview Panel                        │
 *   │ Panel    │ ┃ │                                       │
 *   │ (sidebar)│ ┃ │                                       │
 *   ├──────────┴───┴── split handle ───────────────────────┤
 *   │                                                      │
 *   │  Timeline Panel                                      │
 *   │                                                      │
 *   └──────────────────────────────────────────────────────┘
 *
 * Cross-platform: resizable splitters, touch-friendly on mobile.
 * Initialises the timeline engine on load.
 */
Item {
    id: root

    property real splitFraction: 0.55     // preview/timeline fraction
    property real sidebarPanelWidth: 260  // panel area width (excludes icon rail)

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
    Shortcut { sequence: "Delete"; onActivated: {
        if (timelineNotifier) {
            if (timelineNotifier.selectedClipCount > 1) timelineNotifier.deleteSelectedClips()
            else internal.deleteSelected()
        }
    }}
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
    Shortcut { sequence: "Ctrl+A"; onActivated: if (timelineNotifier) timelineNotifier.selectAll() }
    Shortcut { sequence: "Ctrl+Y"; onActivated: if (timelineNotifier) timelineNotifier.redo() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // === Toolbar ===
        VE2Toolbar {
            id: toolbar
            Layout.fillWidth: true
            Layout.preferredHeight: 52
        }

        // === Main area: upper (sidebar + preview) / lower (timeline) ===
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Column {
                anchors.fill: parent

                // === Upper panel: icon rail + sidebar + video preview ===
                Item {
                    id: upperPanel
                    width: parent.width
                    height: {
                        var available = parent.height - splitHandle.height
                        return Math.max(0, available * root.splitFraction)
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Icon rail + sidebar panels (VE2EditorSidebar includes both)
                        VE2EditorSidebar {
                            id: editorSidebar
                            Layout.preferredWidth: editorSidebar.iconRailWidth + (editorSidebar.panelOpen ? root.sidebarPanelWidth : 0)
                            Layout.fillHeight: true
                        }

                        // Sidebar splitter handle
                        Rectangle {
                            Layout.preferredWidth: 6
                            Layout.fillHeight: true
                            color: sidebarSplitterMa.containsMouse || sidebarSplitterMa.pressed ? "#6C63FF" : "#252540"

                            Rectangle {
                                anchors.centerIn: parent
                                width: 2; height: 40
                                radius: 1
                                color: sidebarSplitterMa.containsMouse || sidebarSplitterMa.pressed ? Qt.rgba(1,1,1,0.6) : "#404060"
                            }

                            MouseArea {
                                id: sidebarSplitterMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.SplitHCursor
                                property real startX: 0
                                property real startWidth: 0

                                onPressed: function(mouse) {
                                    startX = mouse.x
                                    startWidth = root.sidebarPanelWidth
                                }
                                onPositionChanged: function(mouse) {
                                    if (pressed) {
                                        var newWidth = startWidth + (mouse.x - startX)
                                        if (newWidth < 80) newWidth = 0
                                        root.sidebarPanelWidth = Math.max(0, Math.min(newWidth, 500))
                                        if (newWidth < 80) editorSidebar.panelOpen = false
                                        else if (!editorSidebar.panelOpen) editorSidebar.panelOpen = true
                                    }
                                }
                                onDoubleClicked: {
                                    if (editorSidebar.panelOpen) {
                                        editorSidebar.panelOpen = false
                                    } else {
                                        editorSidebar.panelOpen = true
                                        root.sidebarPanelWidth = 260
                                    }
                                }
                            }
                        }

                        // Video preview
                        VE2PreviewPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }
                }

                // === Horizontal split handle ===
                Rectangle {
                    id: splitHandle
                    width: parent.width
                    height: 6
                    color: splitHandleMa.containsMouse || splitHandleMa.pressed ? "#6C63FF" : "#252540"

                    Rectangle {
                        anchors.centerIn: parent
                        width: 40; height: 2; radius: 1
                        color: splitHandleMa.containsMouse || splitHandleMa.pressed
                               ? Qt.rgba(1, 1, 1, 0.6) : "#404060"
                    }

                    MouseArea {
                        id: splitHandleMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.SizeVerCursor

                        property real dragStartY: 0
                        property real dragStartFrac: 0

                        onPressed: function(mouse) {
                            dragStartY = mapToItem(upperPanel.parent, 0, mouse.y).y
                            dragStartFrac = root.splitFraction
                        }
                        onPositionChanged: function(mouse) {
                            if (pressed) {
                                var globalY = mapToItem(upperPanel.parent, 0, mouse.y).y
                                var totalHeight = upperPanel.parent.height - splitHandle.height
                                if (totalHeight <= 0) return
                                var delta = (globalY - dragStartY) / totalHeight
                                root.splitFraction = Math.max(0.15, Math.min(0.85, dragStartFrac + delta))
                            }
                        }
                    }
                }

                // === Lower panel: timeline ===
                Item {
                    id: lowerPanel
                    width: parent.width
                    height: {
                        var available = parent.height - splitHandle.height
                        return Math.max(80, available - available * root.splitFraction)
                    }

                    VE2TimelinePanel {
                        id: timelinePanel
                        anchors.fill: parent
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
