import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

/**
 * VE2Toolbar — top bar with back, import, NLE tools, panel toggle, export.
 *
 * Cross-platform: uses Qt FileDialog (native on Win/Mac/Linux, overlay on mobile).
 * All engine methods are guarded with null-checks on timelineNotifier.
 */
Item {
    id: root
    height: 52

    signal exportRequested()

    property bool importing: false
    property bool isCompact: width < 700  // mobile/tablet layout

    Component.onCompleted: {
        console.log("[VE2Toolbar] loaded, timelineNotifier:", timelineNotifier !== null,
                    "isReady:", timelineNotifier ? timelineNotifier.isReady : "n/a",
                    "mediaPoolNotifier:", mediaPoolNotifier !== null)
    }

    Rectangle {
        anchors.fill: parent
        color: "#0D0D1A"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#1E1E38" }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8; anchors.rightMargin: 8
        spacing: 0

        // Back button
        ToolButton {
            icon.name: "go-previous"
            icon.color: "#B0B0C8"; icon.width: 20; icon.height: 20
            ToolTip.text: "Back"; ToolTip.visible: hovered
            onClicked: router.pop()
        }

        ToolSep {}

        // === Save Project ===
        Button {
            text: "\uD83D\uDCBE Save"
            font.pixelSize: 12
            enabled: timelineNotifier && timelineNotifier.isReady
            onClicked: saveProjectDialog.open()
            background: Rectangle { radius: 6; color: parent.enabled ? "#1A1A34" : "#0D0D1A"; border.color: "#303050" }
            contentItem: Label { text: parent.text; color: parent.enabled ? "#D0D0E8" : "#505068"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
        }

        // === Save as Template ===
        Button {
            text: "\uD83D\uDCC4 Template"
            font.pixelSize: 12
            enabled: timelineNotifier && timelineNotifier.isReady
            onClicked: saveTemplateDialog.open()
            background: Rectangle { radius: 6; color: parent.enabled ? "#1A1A34" : "#0D0D1A"; border.color: "#303050" }
            contentItem: Label { text: parent.text; color: parent.enabled ? "#D0D0E8" : "#505068"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
        }

        // Importing indicator
        RowLayout {
            visible: root.importing
            spacing: 6; Layout.leftMargin: 8
            BusyIndicator { running: true; Layout.preferredWidth: 16; Layout.preferredHeight: 16 }
            Label { text: "Importing..."; font.pixelSize: 12; color: "#8888A0" }
        }

        Item { Layout.fillWidth: true }

        // === Right side ===

        // Undo / Redo
        ToolButton {
            icon.name: "edit-undo"; icon.color: enabled ? "#B0B0C8" : "#404060"
            enabled: timelineNotifier ? timelineNotifier.canUndo : false
            ToolTip.text: "Undo"; ToolTip.visible: hovered
            onClicked: timelineNotifier.undo()
        }
        ToolButton {
            icon.name: "edit-redo"; icon.color: enabled ? "#B0B0C8" : "#404060"
            enabled: timelineNotifier ? timelineNotifier.canRedo : false
            ToolTip.text: "Redo"; ToolTip.visible: hovered
            onClicked: timelineNotifier.redo()
        }

        ToolSep {}

        // Export
        Button {
            text: "Export"
            font.pixelSize: 14; font.weight: Font.DemiBold
            enabled: timelineNotifier ? timelineNotifier.isReady : false
            background: Rectangle { radius: 8; color: parent.enabled ? "#6C63FF" : Qt.rgba(0.424, 0.388, 1.0, 0.3) }
            contentItem: Label { text: "Export"; color: "white"; font.pixelSize: 14; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter }
            onClicked: router.push("/export/video")
        }
    }

    // ---- File Dialogs (cross-platform, multi-select) ----
    FileDialog {
        id: videoDialog
        title: "Select Video(s)"
        fileMode: FileDialog.OpenFiles  // multi-select
        nameFilters: ["Video files (*.mp4 *.mov *.avi *.mkv *.webm *.m4v *.flv *.wmv *.3gp)"]
        onAccepted: {
            console.log("[VE2] videoDialog accepted, count:", selectedFiles.length)
            internal.importFiles(selectedFiles, "video")
        }
    }
    FileDialog {
        id: imageDialog
        title: "Select Image(s)"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Image files (*.jpg *.jpeg *.png *.webp *.gif *.bmp *.heic *.tiff)"]
        onAccepted: {
            console.log("[VE2] imageDialog accepted, count:", selectedFiles.length)
            internal.importFiles(selectedFiles, "image")
        }
    }
    FileDialog {
        id: audioDialog
        title: "Select Audio File(s)"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Audio files (*.mp3 *.wav *.aac *.m4a *.flac *.ogg *.wma *.opus *.aiff)"]
        onAccepted: {
            console.log("[VE2] audioDialog accepted, count:", selectedFiles.length)
            internal.importFiles(selectedFiles, "audio")
        }
    }

    QtObject {
        id: internal

        // Import files into the media pool only.
        // Clips are added to the timeline from the media pool panel (double-click, drag, or "+" button).
        function importFiles(fileUrls, mediaType) {
            if (!mediaPoolNotifier) {
                console.warn("[VE2] importFiles ABORT: mediaPoolNotifier is null")
                return
            }

            root.importing = true
            var count = fileUrls.length

            console.log("[VE2] importing", count, mediaType, "file(s) into media pool")

            var urls = []
            for (var i = 0; i < count; i++) {
                var path = normaliseUrl(fileUrls[i])
                if (path === "") continue
                urls.push(path)
            }

            // Import all files into the media pool (ffprobe probes duration/resolution)
            for (var j = 0; j < urls.length; j++) {
                mediaPoolNotifier.importFile(urls[j])
                console.log("[VE2] imported", (j + 1) + "/" + urls.length, urls[j].split("/").pop())
            }

            root.importing = false
            console.log("[VE2] import done —", urls.length, "files added to media pool")
        }

        // Single file import (convenience wrapper)
        function importFile(fileUrl, mediaType) {
            importFiles([fileUrl], mediaType)
        }

        // Convert file:///URL → local path (cross-platform)
        function normaliseUrl(fileUrl) {
            var path = fileUrl.toString()
            path = decodeURIComponent(path)
            if (path.startsWith("file:///")) {
                var stripped = path.substring(8)
                if (stripped.match(/^[A-Za-z]:/)) {
                    path = stripped  // Windows: C:/Users/...
                } else {
                    path = "/" + stripped  // Unix: /home/...
                }
            }
            return path
        }

        // Zoom timeline so all clips are visible
        function zoomToFit() {
            var totalDur = timelineNotifier.totalDuration
            if (totalDur <= 0) return
            var visibleWidth = root.width > 200 ? (root.width * 0.6) : 600
            var fitPps = visibleWidth / totalDur
            fitPps = Math.max(0.01, Math.min(400, fitPps))
            timelineNotifier.setPixelsPerSecond(fitPps)
        }

        function addTextClip() {
            if (!timelineNotifier) return
            timelineNotifier.addTrack(3)
            var clipId = timelineNotifier.addClip(0, 2, "", "Text", 5.0)
            if (clipId >= 0) timelineNotifier.selectClip(clipId)
        }

        function addColorClip() {
            if (!timelineNotifier) return
            var clipId = timelineNotifier.addClip(1, 3, "", "Color", 5.0)
            if (clipId >= 0) timelineNotifier.selectClip(clipId)
        }
    }

    // ---- Reusable components ----
    component ToolBtn: Item {
        property string iconText: ""
        property string label: ""
        property string tooltip: ""
        property bool enabled: true
        signal clicked()
        width: col.implicitWidth + 14; height: 44
        Layout.leftMargin: 1; Layout.rightMargin: 1
        opacity: enabled ? 1.0 : 0.3

        Column { id: col; anchors.centerIn: parent; spacing: 2
            Label { anchors.horizontalCenter: parent.horizontalCenter; text: iconText; font.pixelSize: 15; color: "#E0E0F0" }
            Label { anchors.horizontalCenter: parent.horizontalCenter; text: label; font.pixelSize: 9; color: "#8888A0" }
        }
        MouseArea { anchors.fill: parent; enabled: parent.enabled; cursorShape: Qt.PointingHandCursor; onClicked: parent.clicked() }
        ToolTip.visible: tooltip !== "" && hov.hovered; ToolTip.text: tooltip
        HoverHandler { id: hov }
    }

    component ToolSep: Rectangle {
        width: 1; height: 24; color: "#252540"
        Layout.leftMargin: 4; Layout.rightMargin: 4
    }

    // ---- Save Project Dialog ----
    Dialog {
        id: saveProjectDialog
        title: "Save Project"
        modal: true; anchors.centerIn: Overlay.overlay
        width: 360; standardButtons: Dialog.Cancel | Dialog.Save
        background: Rectangle { radius: 10; color: "#1A1A34"; border.color: "#303050" }

        contentItem: ColumnLayout {
            spacing: 10
            Label { text: "Project name"; font.pixelSize: 13; color: "#8888A0" }
            TextField {
                id: projectNameField
                Layout.fillWidth: true
                placeholderText: "My Project"
                font.pixelSize: 14; color: "#E0E0F0"
                background: Rectangle { radius: 6; color: "#12122A"; border.color: "#303050" }
            }
        }
        onAccepted: {
            if (projectNameField.text.trim().length === 0) return
            if (projectListNotifier && timelineNotifier)
                projectListNotifier.saveProject(timelineNotifier.projectJson(), projectNameField.text.trim())
        }
    }

    // ---- Save as Template Dialog ----
    Dialog {
        id: saveTemplateDialog
        title: "Save as Template"
        modal: true; anchors.centerIn: Overlay.overlay
        width: 400; standardButtons: Dialog.Cancel | Dialog.Save
        background: Rectangle { radius: 10; color: "#1A1A34"; border.color: "#303050" }

        contentItem: ColumnLayout {
            spacing: 8
            Label { text: "Template name *"; font.pixelSize: 13; color: "#8888A0" }
            TextField {
                id: tplNameField; Layout.fillWidth: true
                placeholderText: "My Template"; font.pixelSize: 14; color: "#E0E0F0"
                background: Rectangle { radius: 6; color: "#12122A"; border.color: "#303050" }
            }
            Label { text: "Description"; font.pixelSize: 13; color: "#8888A0" }
            TextField {
                id: tplDescField; Layout.fillWidth: true
                placeholderText: "What this template is for"; font.pixelSize: 14; color: "#E0E0F0"
                background: Rectangle { radius: 6; color: "#12122A"; border.color: "#303050" }
            }
            Label { text: "Category"; font.pixelSize: 13; color: "#8888A0" }
            ComboBox {
                id: tplCategoryCombo; Layout.fillWidth: true
                model: ["General", "Social Media", "Promo", "Reel", "Story", "YouTube", "Ad", "Intro/Outro"]
            }
            Label { text: "Tags (comma-separated)"; font.pixelSize: 13; color: "#8888A0" }
            TextField {
                id: tplTagsField; Layout.fillWidth: true
                placeholderText: "social, promo"; font.pixelSize: 14; color: "#E0E0F0"
                background: Rectangle { radius: 6; color: "#12122A"; border.color: "#303050" }
            }
            Label {
                id: tplError; visible: text.length > 0
                color: "#EF5350"; font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
        }
        onAccepted: {
            if (tplNameField.text.trim().length === 0) { tplError.text = "Name required"; return }
            if (projectListNotifier && timelineNotifier)
                projectListNotifier.saveTemplate(
                    timelineNotifier.projectJson(), tplNameField.text.trim(),
                    tplDescField.text.trim(), tplCategoryCombo.currentText, tplTagsField.text.trim())
        }
        Connections {
            target: projectListNotifier
            function onTemplateSaved() { saveTemplateDialog.close() }
            function onTemplateSaveError(msg) { tplError.text = msg }
        }
    }
}
