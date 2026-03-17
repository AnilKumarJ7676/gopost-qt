import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Main image editor screen.
/// Composes canvas preview, toolbar, layer panel, and top action bar.
Item {
    id: root

    property bool showLayerPanel: false
    property string activeBottomPanel: ""

    signal exitRequested()

    Component.onCompleted: {
        if (canvasNotifier && !canvasNotifier.hasCanvas) {
            canvasNotifier.createCanvas(1080, 1080, 72.0, true)
            if (undoRedoNotifier) undoRedoNotifier.clear()
        }
    }

    // Keyboard shortcuts
    Shortcut {
        sequence: "Ctrl+Z"
        onActivated: if (undoRedoNotifier) undoRedoNotifier.undo()
    }
    Shortcut {
        sequence: "Ctrl+Shift+Z"
        onActivated: if (undoRedoNotifier) undoRedoNotifier.redo()
    }
    Shortcut {
        sequence: "Ctrl+Y"
        onActivated: if (undoRedoNotifier) undoRedoNotifier.redo()
    }

    // Handle tool changes
    Connections {
        target: toolNotifier
        function onToolChanged(tool) {
            switch (tool) {
            case ToolNotifier.Layers:
                root.showLayerPanel = !root.showLayerPanel
                root.activeBottomPanel = ""
                break
            case ToolNotifier.AddImage:
                photoPickerSheet.open()
                break
            case ToolNotifier.Filter:
                root.activeBottomPanel = root.activeBottomPanel === "filter" ? "" : "filter"
                root.showLayerPanel = false
                break
            case ToolNotifier.Adjust:
                root.activeBottomPanel = root.activeBottomPanel === "adjust" ? "" : "adjust"
                root.showLayerPanel = false
                break
            case ToolNotifier.AddText:
                root.activeBottomPanel = root.activeBottomPanel === "text" ? "" : "text"
                root.showLayerPanel = false
                break
            case ToolNotifier.Sticker:
                stickerPickerSheet.open()
                break
            case ToolNotifier.Crop:
                root.activeBottomPanel = root.activeBottomPanel === "crop" ? "" : "crop"
                root.showLayerPanel = false
                break
            case ToolNotifier.Mask:
                root.activeBottomPanel = root.activeBottomPanel === "mask" ? "" : "mask"
                root.showLayerPanel = false
                break
            case ToolNotifier.Export:
                exportLoader.active = true
                break
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Top Bar ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "#1E1E2E"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 0.5
                color: Qt.rgba(1, 1, 1, 0.1)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8; anchors.rightMargin: 8
                spacing: 8

                ToolButton {
                    icon.name: "go-previous"
                    icon.color: Qt.rgba(1, 1, 1, 0.7)
                    ToolTip.text: "Back"
                    ToolTip.visible: hovered
                    onClicked: internal.handleBack()
                }

                Label {
                    Layout.fillWidth: true
                    text: canvasNotifier && canvasNotifier.hasCanvas
                          ? canvasNotifier.canvasWidth + "\u00D7" + canvasNotifier.canvasHeight
                          : "Image Editor"
                    color: Qt.rgba(1, 1, 1, 0.7)
                    font.pixelSize: 14
                    font.weight: Font.Medium
                }

                ToolButton {
                    icon.name: "edit-undo"
                    icon.color: enabled ? Qt.rgba(1, 1, 1, 0.54) : Qt.rgba(1, 1, 1, 0.12)
                    enabled: undoRedoNotifier ? undoRedoNotifier.canUndo : false
                    ToolTip.text: "Undo (Ctrl+Z)"
                    ToolTip.visible: hovered
                    onClicked: undoRedoNotifier.undo()
                }

                ToolButton {
                    icon.name: "edit-redo"
                    icon.color: enabled ? Qt.rgba(1, 1, 1, 0.54) : Qt.rgba(1, 1, 1, 0.12)
                    enabled: undoRedoNotifier ? undoRedoNotifier.canRedo : false
                    ToolTip.text: "Redo (Ctrl+Shift+Z)"
                    ToolTip.visible: hovered
                    onClicked: undoRedoNotifier.redo()
                }

                ToolButton {
                    icon.name: "document-save"
                    icon.color: Qt.rgba(1, 1, 1, 0.54)
                    ToolTip.text: "Save Project"
                    ToolTip.visible: hovered
                    onClicked: internal.saveProject()
                }

                ToolButton {
                    icon.name: "document-save-as"
                    icon.color: Qt.rgba(1, 1, 1, 0.54)
                    ToolTip.text: "Save as Template"
                    ToolTip.visible: hovered
                    onClicked: saveAsTemplateDialog.open()
                }

                // Zoom indicator
                Rectangle {
                    Layout.preferredWidth: zoomLabel.implicitWidth + 16
                    Layout.preferredHeight: 24
                    color: Qt.rgba(1, 1, 1, 0.1)
                    radius: 4

                    Label {
                        id: zoomLabel
                        anchors.centerIn: parent
                        text: canvasNotifier ? Math.round(canvasNotifier.zoom * 100) + "%" : "100%"
                        color: Qt.rgba(1, 1, 1, 0.54)
                        font.pixelSize: 11
                    }
                }
            }
        }

        // --- Main area: Canvas + optional Layer Panel ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Canvas area
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Loading state
                BusyIndicator {
                    anchors.centerIn: parent
                    running: canvasNotifier ? canvasNotifier.isLoading : false
                    visible: running
                }

                // Error state
                Column {
                    anchors.centerIn: parent
                    spacing: 16
                    visible: canvasNotifier && canvasNotifier.error !== "" && !canvasNotifier.isLoading

                    Label {
                        text: "\u26A0"
                        font.pixelSize: 48
                        color: "#CF6679"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Label {
                        text: canvasNotifier ? canvasNotifier.error : ""
                        color: Qt.rgba(1, 1, 1, 0.54)
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        width: 300
                    }
                    Button {
                        text: "Retry"
                        anchors.horizontalCenter: parent.horizontalCenter
                        onClicked: canvasNotifier.createCanvas(1080, 1080, 72.0, true)
                    }
                }

                // Canvas preview
                CanvasPreview {
                    anchors.fill: parent
                    visible: canvasNotifier && !canvasNotifier.isLoading && canvasNotifier.error === ""

                }
            }

            // Layer panel
            LayerPanel {
                Layout.preferredWidth: root.width > 1024 ? 300 : (root.width > 600 ? 260 : 220)
                Layout.fillHeight: true
                visible: root.showLayerPanel
            }
        }

        // --- Bottom panel ---
        Loader {
            Layout.fillWidth: true
            active: root.activeBottomPanel !== ""
            sourceComponent: {
                switch (root.activeBottomPanel) {
                case "filter": return filterPanelComponent
                case "adjust": return adjustmentPanelComponent
                case "text": return textToolPanelComponent
                case "crop": return cropPanelComponent
                case "mask": return maskPanelComponent
                default: return null
                }
            }
        }

        // --- Toolbar ---
        EditorToolbar {
            Layout.fillWidth: true
        }
    }

    // --- Components for bottom panel loader ---
    Component {
        id: filterPanelComponent
        FilterPanel {}
    }
    Component {
        id: adjustmentPanelComponent
        AdjustmentPanel {}
    }
    Component {
        id: textToolPanelComponent
        TextToolPanel {}
    }
    Component {
        id: cropPanelComponent
        CropPanel {}
    }
    Component {
        id: maskPanelComponent
        MaskPanel {}
    }

    // --- Popups ---
    PhotoPickerSheet {
        id: photoPickerSheet
    }

    StickerPickerSheet {
        id: stickerPickerSheet
    }

    SaveAsTemplateDialog {
        id: saveAsTemplateDialog
    }

    // Export screen loader
    Loader {
        id: exportLoader
        anchors.fill: parent
        active: false
        sourceComponent: Component {
            ExportScreen {
                onBack: exportLoader.active = false
            }
        }
    }

    // Exit confirmation dialog
    Dialog {
        id: exitDialog
        anchors.centerIn: parent
        title: "Unsaved changes"
        modal: true
        standardButtons: Dialog.Cancel | Dialog.Discard

        Label {
            text: "You have unsaved changes. Are you sure you want to exit?"
            wrapMode: Text.Wrap
        }

        onDiscarded: {
            if (undoRedoNotifier) undoRedoNotifier.clear()
            if (canvasNotifier) canvasNotifier.destroyCanvas()
            router.pop()
            close()
        }
    }

    QtObject {
        id: internal

        function handleBack() {
            if (canvasNotifier && canvasNotifier.layerCount === 0) {
                router.pop()
                return
            }
            exitDialog.open()
        }

        function saveProject() {
            // Placeholder for save functionality
        }
    }
}
