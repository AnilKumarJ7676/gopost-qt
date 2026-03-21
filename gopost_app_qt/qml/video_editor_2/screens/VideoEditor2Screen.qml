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
 *   │ VE2TimelineToolbar (36px)                            │
 *   ├──────────────────────────────────────────────────────┤
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

    Component.onDestruction: {
        // Exit fullscreen and stop playback when leaving the editor
        if (previewPanel) previewPanel.isFullscreen = false
        if (timelineNotifier && timelineNotifier.isPlaying)
            timelineNotifier.togglePlayPause()
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
    Shortcut { sequence: "Shift+Left"; onActivated: if (timelineNotifier) timelineNotifier.stepBackwardN(5) }
    Shortcut { sequence: "Shift+Right"; onActivated: if (timelineNotifier) timelineNotifier.stepForwardN(5) }
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
    Shortcut { sequence: "Up"; onActivated: if (timelineNotifier) timelineNotifier.jumpToPreviousEditPoint() }
    Shortcut { sequence: "Down"; onActivated: if (timelineNotifier) timelineNotifier.jumpToNextEditPoint() }
    Shortcut { sequence: "Shift+Up"; onActivated: if (timelineNotifier) timelineNotifier.jumpToPreviousMarkerOnly() }
    Shortcut { sequence: "Shift+Down"; onActivated: if (timelineNotifier) timelineNotifier.jumpToNextMarkerOnly() }
    Shortcut { sequence: "Return"; onActivated: if (timelineNotifier) timelineNotifier.playInToOut() }
    Shortcut { sequence: "."; onActivated: if (timelineNotifier) timelineNotifier.playAroundCurrent(2.0, 2.0) }
    Shortcut { sequence: "Ctrl+="; onActivated: if (timelineNotifier) timelineNotifier.zoomIn() }
    Shortcut { sequence: "Ctrl+-"; onActivated: if (timelineNotifier) timelineNotifier.zoomOut() }
    Shortcut {
        sequence: "Ctrl+A"
        onActivated: {
            // Route to media pool when media panel is active, otherwise select all timeline clips
            if (editorSidebar.activePanel === 0 && editorSidebar.panelOpen
                && mediaPoolNotifier)
                mediaPoolNotifier.selectAllInBin()
            else if (timelineNotifier)
                timelineNotifier.selectAll()
        }
    }
    Shortcut { sequence: "Ctrl+Y"; onActivated: if (timelineNotifier) timelineNotifier.redo() }
    Shortcut { sequence: "Alt+S"; onActivated: if (timelineNotifier) timelineNotifier.soloSelectedTrack() }
    Shortcut { sequence: "Ctrl+T"; onActivated: { if (timelineNotifier && timelineNotifier.selectedClipId >= 0) timelineNotifier.applyDefaultTransition(timelineNotifier.selectedClipId) } }
    Shortcut { sequence: "B"; onActivated: if (timelineNotifier) timelineNotifier.setTrimEditMode(1) }  // Ripple
    Shortcut { sequence: "N"; onActivated: if (timelineNotifier) timelineNotifier.setTrimEditMode(2) }  // Roll
    Shortcut { sequence: "Y"; onActivated: if (timelineNotifier) timelineNotifier.setTrimEditMode(3) }  // Slip
    Shortcut { sequence: "U"; onActivated: if (timelineNotifier) timelineNotifier.setTrimEditMode(4) }  // Slide

    // Clipboard shortcuts
    Shortcut { sequence: "Ctrl+X"; onActivated: if (timelineNotifier) timelineNotifier.cutSelectedClips() }
    Shortcut { sequence: "Ctrl+C"; onActivated: if (timelineNotifier) timelineNotifier.copySelectedClips() }
    Shortcut { sequence: "Ctrl+V"; onActivated: if (timelineNotifier) timelineNotifier.pasteClipsAtPlayhead() }
    Shortcut { sequence: "Ctrl+Shift+V"; onActivated: if (timelineNotifier) timelineNotifier.pasteClipsOverwrite() }
    Shortcut { sequence: "Ctrl+Alt+V"; onActivated: pasteAttributesDialog.open() }

    // Save shortcuts
    Shortcut { sequence: "Ctrl+S"; onActivated: internal.quickSave() }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: {
        if (timelineNotifier) timelineNotifier.saveVersion("Manual save")
        saveToast.show("Version saved")
    }}

    // Selection & Navigation shortcuts
    Shortcut { sequence: "Ctrl+Shift+A"; onActivated: if (timelineNotifier) timelineNotifier.deselectAll() }
    Shortcut { sequence: "Tab"; onActivated: if (timelineNotifier) timelineNotifier.selectNextClipOnTrack() }
    Shortcut { sequence: "Shift+Tab"; onActivated: if (timelineNotifier) timelineNotifier.selectPrevClipOnTrack() }
    Shortcut { sequence: "Shift+Z"; onActivated: { if (timelineNotifier) timelineNotifier.zoomToFit(timelinePanel.width - 100) } }
    Shortcut { sequence: "\\"; onActivated: { if (timelineNotifier) timelineNotifier.toggleZoomFit(timelinePanel.width - 100) } }
    Shortcut { sequence: "Ctrl+G"; onActivated: timecodeDialog.open() }

    // Fullscreen preview
    Shortcut { sequence: "F"; onActivated: previewPanel.isFullscreen = !previewPanel.isFullscreen }
    Shortcut { sequence: "Escape"; onActivated: { if (previewPanel.isFullscreen) previewPanel.isFullscreen = false } }

    // ---- Timecode input dialog (Ctrl+G) ----
    Dialog {
        id: timecodeDialog
        title: "Go to Timecode"
        modal: true; anchors.centerIn: Overlay.overlay
        width: 300; standardButtons: Dialog.Cancel | Dialog.Ok
        background: Rectangle { radius: 10; color: "#1A1A34"; border.color: "#303050" }

        contentItem: ColumnLayout {
            spacing: 8
            Label { text: "Enter timecode (MM:SS:FF or seconds)"; font.pixelSize: 12; color: "#8888A0" }
            TextField {
                id: timecodeField
                Layout.fillWidth: true
                placeholderText: "00:00:00"
                font.pixelSize: 16; font.family: "monospace"; color: "#E0E0F0"
                horizontalAlignment: Text.AlignHCenter
                background: Rectangle { radius: 6; color: "#12122A"; border.color: "#6C63FF" }
                onAccepted: timecodeDialog.accept()
            }
        }
        onOpened: { timecodeField.text = ""; timecodeField.forceActiveFocus() }
        onAccepted: {
            if (timelineNotifier && timecodeField.text.trim().length > 0)
                timelineNotifier.seekToTimecode(timecodeField.text.trim())
        }
    }

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
                        var available = parent.height - splitHandle.height - timelineToolbar.height
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
                            id: previewPanel
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

                // === Timeline secondary toolbar ===
                VE2TimelineToolbar {
                    id: timelineToolbar
                    width: parent.width
                    height: 36
                }

                // === Lower panel: timeline ===
                Item {
                    id: lowerPanel
                    width: parent.width
                    height: {
                        var available = parent.height - splitHandle.height - timelineToolbar.height
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

        function quickSave() {
            if (!projectListNotifier || !timelineNotifier) return
            projectListNotifier.saveProject(timelineNotifier.projectJson(), "Quick Save")
            saveToast.show("Project saved")
        }
    }

    // ================================================================
    // Paste Attributes Dialog (Ctrl+Alt+V)
    // ================================================================
    Dialog {
        id: pasteAttributesDialog
        title: "Paste Attributes"
        modal: true; anchors.centerIn: Overlay.overlay
        width: 320; standardButtons: Dialog.Cancel | Dialog.Ok
        background: Rectangle { radius: 10; color: "#1A1A34"; border.color: "#303050" }

        contentItem: ColumnLayout {
            spacing: 6
            Label { text: "Select attributes to paste:"; font.pixelSize: 12; color: "#8888A0" }
            CheckBox { id: chkEffects; text: "Effects"; checked: true
                contentItem: Label { text: parent.text; color: "#D0D0E8"; font.pixelSize: 13; leftPadding: parent.indicator.width + 6 } }
            CheckBox { id: chkColorGrading; text: "Color Grading"; checked: true
                contentItem: Label { text: parent.text; color: "#D0D0E8"; font.pixelSize: 13; leftPadding: parent.indicator.width + 6 } }
            CheckBox { id: chkTransforms; text: "Transforms"; checked: true
                contentItem: Label { text: parent.text; color: "#D0D0E8"; font.pixelSize: 13; leftPadding: parent.indicator.width + 6 } }
            CheckBox { id: chkSpeed; text: "Speed"; checked: false
                contentItem: Label { text: parent.text; color: "#D0D0E8"; font.pixelSize: 13; leftPadding: parent.indicator.width + 6 } }
            CheckBox { id: chkAudio; text: "Audio Settings"; checked: false
                contentItem: Label { text: parent.text; color: "#D0D0E8"; font.pixelSize: 13; leftPadding: parent.indicator.width + 6 } }
            CheckBox { id: chkTransitions; text: "Transitions"; checked: false
                contentItem: Label { text: parent.text; color: "#D0D0E8"; font.pixelSize: 13; leftPadding: parent.indicator.width + 6 } }
        }
        onAccepted: {
            if (timelineNotifier)
                timelineNotifier.pasteAttributes(chkEffects.checked, chkColorGrading.checked,
                    chkTransforms.checked, chkSpeed.checked, chkAudio.checked, chkTransitions.checked)
        }
    }

    // ================================================================
    // Save toast notification
    // ================================================================
    Rectangle {
        id: saveToast
        visible: false; z: 200
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 60
        width: toastLabel.implicitWidth + 24; height: 32; radius: 8
        color: Qt.rgba(0.4, 0.73, 0.42, 0.9)
        border.color: "#66BB6A"

        Label {
            id: toastLabel; anchors.centerIn: parent
            font.pixelSize: 12; font.weight: Font.Bold; color: "#FFFFFF"
        }

        function show(msg) {
            toastLabel.text = msg
            visible = true
            toastTimer.restart()
        }

        Timer {
            id: toastTimer; interval: 2000; onTriggered: saveToast.visible = false
        }

        Behavior on visible {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 300 }
        }
    }

    // ================================================================
    // Import error toast (orange/red for errors)
    // ================================================================
    Rectangle {
        id: importToast
        visible: false; z: 200
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 100
        width: importToastLabel.implicitWidth + 24; height: 32; radius: 8
        color: Qt.rgba(0.94, 0.33, 0.31, 0.9)
        border.color: "#EF5350"

        Label {
            id: importToastLabel; anchors.centerIn: parent
            font.pixelSize: 12; font.weight: Font.Bold; color: "#FFFFFF"
        }

        function show(msg) {
            importToastLabel.text = msg
            visible = true
            importToastTimer.restart()
        }

        Timer {
            id: importToastTimer; interval: 3000; onTriggered: importToast.visible = false
        }
    }

    // Wire media pool error signals to toast
    Connections {
        target: mediaPoolNotifier
        function onImportRejected(fileName, reason) {
            importToast.show(fileName + ": " + reason)
        }
        function onDuplicateDetected(fileName) {
            importToast.show(fileName + " already imported")
        }
        function onBatchTooLarge(count, maxAllowed) {
            importToast.show("Too many files (" + count + "). Max " + maxAllowed + " at once.")
        }
        function onImportBatchCompleted(succeeded, failed, failedNames) {
            if (failed > 0)
                importToast.show(failed + " of " + (succeeded + failed) + " files could not be imported")
        }
    }

    // ---- Fullscreen preview overlay ----
    Rectangle {
        id: fullscreenOverlay
        anchors.fill: parent
        color: "#000000"
        visible: previewPanel.isFullscreen
        z: 1000

        // Reparent the preview panel into the overlay when fullscreen
        states: [
            State {
                name: "fullscreen"
                when: previewPanel.isFullscreen
                ParentChange {
                    target: previewPanel
                    parent: fullscreenOverlay
                }
                AnchorChanges {
                    target: previewPanel
                    anchors.left: fullscreenOverlay.left
                    anchors.right: fullscreenOverlay.right
                    anchors.top: fullscreenOverlay.top
                    anchors.bottom: fullscreenOverlay.bottom
                }
            }
        ]

        // Double-click anywhere to exit fullscreen
        MouseArea {
            anchors.fill: parent
            z: -1
            onDoubleClicked: previewPanel.isFullscreen = false
        }
    }
}
