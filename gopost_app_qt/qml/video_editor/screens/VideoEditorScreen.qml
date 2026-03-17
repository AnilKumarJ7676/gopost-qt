import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Main video editor screen.
/// Composes icon rail, sidebar panels, video preview, and timeline.
Item {
    id: root

    property string projectId: ""
    property string projectName: "Untitled project"
    property real splitFraction: editorLayoutNotifier ? editorLayoutNotifier.upperFraction : 0.6

    Component.onCompleted: {
        console.log("[VideoEditor] onCompleted — size:", width, "x", height)
        console.log("[VideoEditor] timelineNotifier:", timelineNotifier !== null,
                    "editorLayoutNotifier:", editorLayoutNotifier !== null)
        if (projectId && projectId.length > 0) {
            if (projectListNotifier) projectListNotifier.loadProject(projectId)
        } else {
            if (timelineNotifier) {
                timelineNotifier.initTimeline()
                console.log("[VideoEditor] initTimeline done — isReady:",
                            timelineNotifier.isReady, "trackCount:", timelineNotifier.trackCount)
            }
        }
    }

    // Keyboard shortcuts
    Shortcut { sequence: "Ctrl+Shift+Z"; onActivated: if (timelineNotifier) timelineNotifier.redo() }
    Shortcut { sequence: "Ctrl+Z"; onActivated: if (timelineNotifier) timelineNotifier.undo() }
    Shortcut { sequence: "Ctrl+S"; onActivated: internal.saveProject() }
    Shortcut { sequence: "Space"; onActivated: if (timelineNotifier) timelineNotifier.togglePlayback() }
    Shortcut { sequence: "J"; onActivated: if (timelineNotifier) timelineNotifier.shuttleReverse() }
    Shortcut { sequence: "K"; onActivated: if (timelineNotifier) timelineNotifier.shuttleStop() }
    Shortcut { sequence: "L"; onActivated: if (timelineNotifier) timelineNotifier.shuttleForward() }
    Shortcut { sequence: "Left"; onActivated: if (timelineNotifier) timelineNotifier.stepBackward() }
    Shortcut { sequence: "Shift+Left"; onActivated: if (timelineNotifier) timelineNotifier.stepBackwardN(10) }
    Shortcut { sequence: "Right"; onActivated: if (timelineNotifier) timelineNotifier.stepForward() }
    Shortcut { sequence: "Shift+Right"; onActivated: if (timelineNotifier) timelineNotifier.stepForwardN(10) }
    Shortcut { sequence: "Home"; onActivated: if (timelineNotifier) timelineNotifier.jumpToStart() }
    Shortcut { sequence: "End"; onActivated: if (timelineNotifier) timelineNotifier.jumpToEnd() }
    Shortcut { sequence: "I"; onActivated: if (timelineNotifier) timelineNotifier.setInPoint() }
    Shortcut { sequence: "O"; onActivated: if (timelineNotifier) timelineNotifier.setOutPoint() }
    Shortcut { sequence: "Alt+X"; onActivated: if (timelineNotifier) timelineNotifier.clearInOutPoints() }
    Shortcut { sequence: "Up"; onActivated: if (timelineNotifier) timelineNotifier.jumpToPreviousSnapPoint() }
    Shortcut { sequence: "Down"; onActivated: if (timelineNotifier) timelineNotifier.jumpToNextSnapPoint() }
    Shortcut { sequence: "Delete"; onActivated: internal.deleteSelected() }
    Shortcut { sequence: "M"; onActivated: if (timelineNotifier) timelineNotifier.addMarker() }
    Shortcut { sequence: "["; onActivated: if (timelineNotifier) timelineNotifier.navigateToPreviousMarker() }
    Shortcut { sequence: "]"; onActivated: if (timelineNotifier) timelineNotifier.navigateToNextMarker() }
    Shortcut { sequence: "Ctrl+D"; onActivated: internal.duplicateSelected() }
    Shortcut { sequence: "Ctrl+="; onActivated: if (timelineNotifier) timelineNotifier.zoomIn() }
    Shortcut { sequence: "Ctrl+-"; onActivated: if (timelineNotifier) timelineNotifier.zoomOut() }
    Shortcut { sequence: "Ctrl+B"; onActivated: if (timelineNotifier) timelineNotifier.splitClipAtPlayhead() }
    Shortcut { sequence: "Shift+Delete"; onActivated: if (timelineNotifier && timelineNotifier.selectedClipId >= 0) timelineNotifier.rippleDelete(timelineNotifier.selectedClipId) }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Top Bar ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "#0E0E1C"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: "#252540"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14; anchors.rightMargin: 14
                spacing: 8

                ToolButton {
                    icon.name: "go-previous"
                    icon.color: "#8888A0"
                    icon.width: 20; icon.height: 20
                    ToolTip.text: "Back"
                    ToolTip.visible: hovered
                    onClicked: router.pop()
                }

                Rectangle { width: 1; height: 26; color: "#252540" }

                Label {
                    text: "\u25B6"
                    color: "#6C63FF"
                    font.pixelSize: 20
                }

                TextField {
                    Layout.preferredWidth: 200
                    text: root.projectName
                    color: "#E0E0F0"
                    font.pixelSize: 16
                    font.weight: Font.Medium
                    background: null
                    onEditingFinished: root.projectName = text
                }

                // Layout preset selector
                Repeater {
                    model: editorLayoutNotifier ? editorLayoutNotifier.presetNames : []
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        property bool isActive: editorLayoutNotifier ? editorLayoutNotifier.activePreset === index : false
                        width: presetLabel.implicitWidth + 20
                        height: 28
                        radius: 4
                        color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "transparent"
                        border.color: isActive ? "#6C63FF" : "#353550"
                        border.width: 1

                        Label {
                            id: presetLabel
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: 11
                            font.weight: isActive ? Font.DemiBold : Font.Normal
                            color: isActive ? "#6C63FF" : "#8888A0"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (editorLayoutNotifier) editorLayoutNotifier.applyPreset(modelData)
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Resolution badge
                Rectangle {
                    visible: timelineNotifier ? (timelineNotifier.isReady && timelineNotifier.projectWidth > 0) : false
                    Layout.preferredWidth: resBadge.implicitWidth + 20
                    Layout.preferredHeight: 24
                    color: "#1A1A34"
                    radius: 4
                    border.color: "#303050"

                    Label {
                        id: resBadge
                        anchors.centerIn: parent
                        text: timelineNotifier ? (timelineNotifier.projectWidth + "x" + timelineNotifier.projectHeight) : ""
                        font.pixelSize: 12
                        font.family: Theme.fontFamilyMono
                        color: "#8888A0"
                    }
                }

                ToolButton {
                    icon.name: "edit-undo"
                    icon.color: enabled ? "#B0B0C8" : "#404060"
                    enabled: timelineNotifier ? timelineNotifier.canUndo : false
                    ToolTip.text: "Undo (Ctrl+Z)"
                    ToolTip.visible: hovered
                    onClicked: if (timelineNotifier) timelineNotifier.undo()
                }

                ToolButton {
                    icon.name: "edit-redo"
                    icon.color: enabled ? "#B0B0C8" : "#404060"
                    enabled: timelineNotifier ? timelineNotifier.canRedo : false
                    ToolTip.text: "Redo (Ctrl+Shift+Z)"
                    ToolTip.visible: hovered
                    onClicked: if (timelineNotifier) timelineNotifier.redo()
                }

                Rectangle { width: 1; height: 26; color: "#252540" }

                ToolButton {
                    icon.name: "view-more-horizontal"
                    icon.color: "#8888A0"
                    onClicked: moreMenu.open()

                    Menu {
                        id: moreMenu
                        MenuItem {
                            text: "Save Project"
                            onTriggered: internal.saveProject()
                        }
                        MenuItem {
                            text: "Save as Template"
                            onTriggered: saveAsTemplateDialog.open()
                        }
                    }
                }

                Button {
                    text: "Export"
                    icon.name: "document-save"
                    enabled: timelineNotifier ? timelineNotifier.isReady : false
                    font.pixelSize: 15
                    font.weight: Font.DemiBold

                    background: Rectangle {
                        radius: 8
                        color: parent.enabled ? "#6C63FF" : Qt.rgba(0.424, 0.388, 1.0, 0.3)
                    }
                    contentItem: RowLayout {
                        spacing: 6
                        Label {
                            text: "\u2913"
                            color: "white"
                            font.pixelSize: 16
                        }
                        Label {
                            text: "Export"
                            color: "white"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                    }

                    onClicked: router.push("/export/video")
                }
            }
        }

        // --- Main area: upper (sidebar + preview) / lower (timeline) ---
        // Inline vertical split — no ResizableSplit component needed
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
                        var available = parent.height - splitHandle.height - videoToolbar.height
                        return Math.max(0, available * root.splitFraction)
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Icon rail + sidebar panels (EditorSidebar includes both)
                        EditorSidebar {
                            id: iconRail
                            Layout.preferredWidth: iconRail.iconRailWidth + (iconRail.panelOpen ? (editorLayoutNotifier ? editorLayoutNotifier.sidebarWidth : 300) : 0)
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
                                    startWidth = editorLayoutNotifier ? editorLayoutNotifier.sidebarWidth : 0
                                }
                                onPositionChanged: function(mouse) {
                                    if (pressed && editorLayoutNotifier) {
                                        var newWidth = startWidth + (mouse.x - startX)
                                        if (newWidth < 80) newWidth = 0
                                        editorLayoutNotifier.setSidebarWidth(Math.max(0, Math.min(newWidth, 500)))
                                    }
                                }
                                onDoubleClicked: if (editorLayoutNotifier) editorLayoutNotifier.resetVerticalSplitter()
                            }
                        }

                        // Video preview
                        VideoPreviewPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }
                }

                // === Video toolbar (import, tools, track management) ===
                VideoToolbar {
                    id: videoToolbar
                    width: parent.width
                    height: 48
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
                                var newFrac = Math.max(0.2, Math.min(0.85, dragStartFrac + delta))
                                root.splitFraction = newFrac
                                if (editorLayoutNotifier) editorLayoutNotifier.setUpperFraction(newFrac)
                            }
                        }
                    }
                }

                // === Lower panel: timeline ===
                Item {
                    id: lowerPanel
                    width: parent.width
                    height: {
                        var available = parent.height - splitHandle.height - videoToolbar.height
                        return Math.max(0, available - available * root.splitFraction)
                    }

                    TimelinePanel {
                        id: timelinePanel
                        anchors.fill: parent
                    }
                }
            }
        }
    }

    // --- Dialogs ---
    SaveAsVideoTemplateDialog {
        id: saveAsTemplateDialog
    }

    // Save project dialog
    Dialog {
        id: saveDialog
        anchors.centerIn: parent
        title: "Save Project"
        modal: true
        standardButtons: Dialog.Cancel | Dialog.Save
        width: 360

        ColumnLayout {
            spacing: 12
            width: parent.width

            TextField {
                id: saveNameField
                Layout.fillWidth: true
                text: root.projectName
                placeholderText: "Project name"
            }
        }

        onAccepted: {
            var name = saveNameField.text.trim()
            if (name.length > 0) {
                root.projectName = name
                if (projectListNotifier) projectListNotifier.saveProject(name, timelineNotifier.project)
            }
        }
    }

    QtObject {
        id: internal

        function saveProject() {
            saveNameField.text = root.projectName
            saveDialog.open()
        }

        function deleteSelected() {
            if (timelineNotifier && timelineNotifier.selectedClipId >= 0) {
                timelineNotifier.removeClip(timelineNotifier.selectedClipId)
            }
        }

        function duplicateSelected() {
            if (timelineNotifier && timelineNotifier.selectedClipId >= 0) {
                timelineNotifier.duplicateClip(timelineNotifier.selectedClipId)
            }
        }
    }
}
