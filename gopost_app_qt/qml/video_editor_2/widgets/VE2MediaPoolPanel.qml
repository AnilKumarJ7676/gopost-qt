import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

/**
 * VE2MediaPoolPanel — media import, browse, drag-to-timeline.
 *
 * Features:
 *   - Desktop file drop (drag files from Explorer/Finder)
 *   - File picker dialog (Import button)
 *   - Grid / List view toggle
 *   - Search + type filter (All / Video / Image / Audio)
 *   - Drag-and-drop to timeline (MIME: application/x-gopost-media)
 *   - Double-click or "+ Timeline" button to add to timeline
 *   - Proper error handling and log tracing
 *
 * Cross-platform: uses Qt FileDialog (native on Win/Mac/Linux).
 */
Item {
    id: root

    property bool isGridView: true
    property bool isDragOver: false

    // ---- File dialog ----
    FileDialog {
        id: importFileDialog
        title: "Import Media Files"
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            "All media (*.mp4 *.mov *.avi *.mkv *.webm *.m4v *.flv *.wmv *.3gp *.jpg *.jpeg *.png *.webp *.gif *.bmp *.heic *.heif *.tiff *.mp3 *.aac *.wav *.m4a *.flac *.ogg *.wma *.opus *.aiff)",
            "Video (*.mp4 *.mov *.avi *.mkv *.webm *.m4v *.flv *.wmv *.3gp)",
            "Image (*.jpg *.jpeg *.png *.webp *.gif *.bmp *.heic *.heif *.tiff)",
            "Audio (*.mp3 *.aac *.wav *.m4a *.flac *.ogg *.wma *.opus *.aiff)",
            "All files (*)"
        ]
        onAccepted: {
            if (selectedFiles.length > 0) {
                console.log("[VE2MediaPool] file dialog accepted:", selectedFiles.length, "files")
                if (mediaPoolNotifier) {
                    mediaPoolNotifier.importFiles(selectedFiles)
                } else {
                    console.warn("[VE2MediaPool] cannot import: mediaPoolNotifier is null")
                }
            }
        }
    }

    // Listen for file picker requests from C++
    Connections {
        target: mediaPoolNotifier
        function onFilePickerRequested() {
            importFileDialog.open()
        }
    }

    Component.onCompleted: {
        console.log("[VE2MediaPool] created — mediaPoolNotifier:",
                    mediaPoolNotifier !== null,
                    "assets:", mediaPoolNotifier ? mediaPoolNotifier.assetCount : 0)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Header: search + view toggle + import ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#12122A"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#252540" }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8; anchors.rightMargin: 8
                spacing: 4

                // Search
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    placeholderText: "Search..."
                    font.pixelSize: 12
                    color: "#E0E0F0"
                    placeholderTextColor: "#6B6B88"
                    background: Rectangle { radius: 6; color: "#1A1A34"; border.color: "#303050" }
                    onTextChanged: {
                        if (mediaPoolNotifier) mediaPoolNotifier.setSearchQuery(text)
                    }
                }

                // Grid/List toggle
                ToolButton {
                    Layout.preferredWidth: 28; Layout.preferredHeight: 28
                    contentItem: Label {
                        text: root.isGridView ? "\u2630" : "\u25A6"  // list / grid icon
                        font.pixelSize: 14; color: "#8888A0"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    ToolTip.text: root.isGridView ? "List view" : "Grid view"
                    ToolTip.visible: hovered
                    onClicked: root.isGridView = !root.isGridView
                }

                // Import button
                Rectangle {
                    width: 60; height: 28; radius: 6
                    color: importMa.pressed ? "#5A52DD" : "#6C63FF"

                    Label {
                        anchors.centerIn: parent
                        text: "+ Import"
                        font.pixelSize: 11; font.weight: Font.DemiBold; color: "white"
                    }

                    MouseArea {
                        id: importMa
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (mediaPoolNotifier) {
                                mediaPoolNotifier.showFilePicker()
                            } else {
                                console.warn("[VE2MediaPool] import: mediaPoolNotifier is null")
                            }
                        }
                    }
                }
            }
        }

        // ---- Filter chips ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: "#12122A"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#252540" }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8; anchors.rightMargin: 8
                spacing: 4

                Repeater {
                    model: [
                        { label: "All",   filter: "all" },
                        { label: "Video", filter: "video" },
                        { label: "Image", filter: "image" },
                        { label: "Audio", filter: "audio" }
                    ]
                    delegate: Rectangle {
                        property bool isActive: mediaPoolNotifier
                                                && mediaPoolNotifier.currentFilter === modelData.filter
                        width: chipLabel.implicitWidth + 14
                        height: 22; radius: 11
                        color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "transparent"
                        border.color: isActive ? "#6C63FF" : "#353550"

                        Label {
                            id: chipLabel
                            anchors.centerIn: parent
                            text: modelData.label
                            font.pixelSize: 10
                            font.weight: isActive ? Font.DemiBold : Font.Normal
                            color: isActive ? "#6C63FF" : "#8888A0"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (mediaPoolNotifier) mediaPoolNotifier.setFilter(modelData.filter)
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Sort toggle
                Label {
                    text: "\u21C5"
                    font.pixelSize: 14; color: "#8888A0"
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { if (mediaPoolNotifier) mediaPoolNotifier.cycleSort() }
                    }
                    ToolTip.visible: sortHov.hovered; ToolTip.text: "Sort"
                    HoverHandler { id: sortHov }
                }
            }
        }

        // ---- Content area ----
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Desktop file drop zone
            DropArea {
                anchors.fill: parent
                keys: ["text/uri-list"]

                onEntered: root.isDragOver = true
                onExited: root.isDragOver = false
                onDropped: drop => {
                    root.isDragOver = false
                    if (drop.hasUrls) {
                        console.log("[VE2MediaPool] desktop drop:", drop.urls.length, "files")
                        if (mediaPoolNotifier) {
                            mediaPoolNotifier.importFiles(drop.urls)
                        }
                    }
                }
            }

            // Drop highlight overlay
            Rectangle {
                anchors.fill: parent; anchors.margins: 6
                radius: 10; color: "transparent"
                border.color: root.isDragOver ? "#6C63FF" : "transparent"
                border.width: 2
                visible: root.isDragOver; z: 100

                ColumnLayout {
                    anchors.centerIn: parent; spacing: 6
                    Label { text: "\u2B07"; font.pixelSize: 36; color: "#6C63FF"; Layout.alignment: Qt.AlignHCenter }
                    Label { text: "Drop files here"; font.pixelSize: 13; font.weight: Font.DemiBold; color: "#6C63FF"; Layout.alignment: Qt.AlignHCenter }
                }
            }

            // Empty state
            ColumnLayout {
                anchors.centerIn: parent; spacing: 6
                visible: !root.isDragOver && mediaPoolNotifier && mediaPoolNotifier.assetCount === 0

                Label { text: "\uD83C\uDFA5"; font.pixelSize: 36; color: "#404060"; Layout.alignment: Qt.AlignHCenter }
                Label { text: "No media imported"; font.pixelSize: 13; color: "#6B6B88"; Layout.alignment: Qt.AlignHCenter }
                Label { text: "Drop files or click Import"; font.pixelSize: 11; color: "#505070"; Layout.alignment: Qt.AlignHCenter }
            }

            // ---- Grid View ----
            GridView {
                id: gridView
                anchors.fill: parent; anchors.margins: 6
                visible: root.isGridView && !root.isDragOver
                clip: true
                cellWidth: Math.max(80, (width - 6) / 2)
                cellHeight: cellWidth * 0.7 + 26

                model: mediaPoolNotifier ? mediaPoolNotifier.filteredAssets : []

                delegate: Item {
                    width: GridView.view.cellWidth - 4
                    height: GridView.view.cellHeight - 4

                    readonly property var asset: modelData || ({})

                    Rectangle {
                        anchors.fill: parent; radius: 6
                        color: asset.isSelected ? Qt.rgba(0.424, 0.388, 1.0, 0.12) : "#1A1A34"
                        border.color: asset.isSelected ? "#6C63FF" : "#303050"

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 3; spacing: 2

                            // Thumbnail
                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                radius: 4; color: "#0E0E1C"

                                Label {
                                    anchors.centerIn: parent
                                    text: asset.mediaType === "video" ? "\uD83C\uDFA5"
                                        : asset.mediaType === "audio" ? "\uD83C\uDFB5" : "\uD83D\uDDBC"
                                    font.pixelSize: 20; color: "#505068"
                                }

                                // Duration badge
                                Rectangle {
                                    anchors.bottom: parent.bottom; anchors.right: parent.right
                                    anchors.margins: 3
                                    visible: asset.duration !== undefined && asset.duration > 0
                                    width: durText.implicitWidth + 6; height: 14; radius: 3
                                    color: Qt.rgba(0, 0, 0, 0.75)

                                    Label {
                                        id: durText; anchors.centerIn: parent
                                        text: internal.formatDuration(asset.duration || 0)
                                        font.pixelSize: 8; color: "#D0D0E8"
                                    }
                                }
                            }

                            // File name
                            Label {
                                text: asset.displayName || ""
                                font.pixelSize: 9; color: "#D0D0E8"
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        // "+ Timeline" button
                        Rectangle {
                            anchors.bottom: parent.bottom; anchors.right: parent.right
                            anchors.margins: 3
                            width: 22; height: 22; radius: 4
                            color: addBtnMa.containsMouse ? "#6C63FF" : Qt.rgba(0.424, 0.388, 1.0, 0.25)
                            z: 5

                            Label {
                                anchors.centerIn: parent
                                text: "+"; font.pixelSize: 14; font.weight: Font.Bold
                                color: "white"
                            }

                            MouseArea {
                                id: addBtnMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: internal.addAssetToTimeline(asset)
                            }

                            ToolTip.visible: addBtnMa.containsMouse
                            ToolTip.text: "Add to timeline"
                        }

                        // Click / double-click / drag
                        MouseArea {
                            id: gridDragArea
                            anchors.fill: parent; z: -1
                            cursorShape: Qt.PointingHandCursor
                            drag.target: dragItem

                            onClicked: {
                                if (mediaPoolNotifier && asset.id)
                                    mediaPoolNotifier.selectAsset(asset.id)
                            }
                            onDoubleClicked: internal.addAssetToTimeline(asset)
                        }
                    }

                    // Drag handle — provides data for timeline DropArea
                    Item {
                        id: dragItem
                        Drag.active: gridDragArea.drag.active
                        Drag.hotSpot: Qt.point(width / 2, height / 2)
                        Drag.mimeData: ({
                            "text/plain": JSON.stringify({
                                type: "media",
                                assetId: asset.id || "",
                                mediaType: asset.mediaType || "video",
                                displayName: asset.displayName || "Untitled",
                                sourcePath: asset.filePath || "",
                                duration: asset.duration || 5.0
                            })
                        })
                        Drag.keys: ["application/x-gopost-media"]

                        Connections {
                            target: dragItem.Drag
                            function onActiveChanged() {
                                if (dragItem.Drag.active) {
                                    console.log("[VE2MediaPool] drag started:", asset.displayName)
                                }
                            }
                        }
                    }
                }
            }

            // ---- List View ----
            ListView {
                id: listView
                anchors.fill: parent; anchors.margins: 4
                visible: !root.isGridView && !root.isDragOver
                clip: true; spacing: 2

                model: mediaPoolNotifier ? mediaPoolNotifier.filteredAssets : []

                delegate: Rectangle {
                    width: ListView.view.width; height: 38; radius: 4

                    readonly property var asset: modelData || ({})

                    color: asset.isSelected ? Qt.rgba(0.424, 0.388, 1.0, 0.08) : "transparent"

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 6

                        // Icon
                        Rectangle {
                            width: 28; height: 28; radius: 4; color: "#0E0E1C"
                            Label {
                                anchors.centerIn: parent
                                text: asset.mediaType === "video" ? "\uD83C\uDFA5"
                                    : asset.mediaType === "audio" ? "\uD83C\uDFB5" : "\uD83D\uDDBC"
                                font.pixelSize: 14; color: "#505068"
                            }
                        }

                        // Name + info
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 1
                            Label {
                                text: asset.displayName || ""
                                font.pixelSize: 11; color: "#E0E0F0"
                                elide: Text.ElideMiddle; Layout.fillWidth: true
                            }
                            Label {
                                text: (asset.mediaType || "")
                                      + (asset.duration > 0 ? (" \u2022 " + internal.formatDuration(asset.duration)) : "")
                                font.pixelSize: 9; color: "#6B6B88"
                            }
                        }

                        // Add button
                        Rectangle {
                            width: 20; height: 20; radius: 4
                            color: listAddMa.containsMouse ? "#6C63FF" : Qt.rgba(0.424, 0.388, 1.0, 0.2)

                            Label { anchors.centerIn: parent; text: "+"; font.pixelSize: 12; font.weight: Font.Bold; color: "white" }
                            MouseArea {
                                id: listAddMa; anchors.fill: parent
                                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: internal.addAssetToTimeline(asset)
                            }
                        }
                    }

                    // Click / double-click for list items
                    MouseArea {
                        id: listDragArea
                        anchors.fill: parent; z: -1
                        cursorShape: Qt.PointingHandCursor
                        drag.target: listDragItem

                        onClicked: {
                            if (mediaPoolNotifier && asset.id)
                                mediaPoolNotifier.selectAsset(asset.id)
                        }
                        onDoubleClicked: internal.addAssetToTimeline(asset)
                    }

                    // Drag handle for list items
                    Item {
                        id: listDragItem
                        Drag.active: listDragArea.drag.active
                        Drag.hotSpot: Qt.point(width / 2, height / 2)
                        Drag.mimeData: ({
                            "text/plain": JSON.stringify({
                                type: "media",
                                assetId: asset.id || "",
                                mediaType: asset.mediaType || "video",
                                displayName: asset.displayName || "Untitled",
                                sourcePath: asset.filePath || "",
                                duration: asset.duration || 5.0
                            })
                        })
                        Drag.keys: ["application/x-gopost-media"]
                    }
                }
            }
        }

        // ---- Footer: asset count ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            color: "#0D0D1A"
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#252540" }

            Label {
                anchors.centerIn: parent
                text: mediaPoolNotifier ? (mediaPoolNotifier.assetCount + " assets") : "0 assets"
                font.pixelSize: 10; color: "#6B6B88"
            }
        }
    }

    // ---- Helpers ----
    QtObject {
        id: internal

        // Add an asset directly to the timeline — single code path, no signal indirection.
        function addAssetToTimeline(asset) {
            if (!asset || !asset.filePath || asset.filePath === "") {
                console.warn("[VE2MediaPool] addAssetToTimeline: invalid asset or empty path")
                return
            }
            if (!timelineNotifier || !timelineNotifier.isReady) {
                console.warn("[VE2MediaPool] addAssetToTimeline: timeline not ready")
                return
            }

            var sourceType = 0  // video
            var trackIndex = 1  // video track
            if (asset.mediaType === "image") {
                sourceType = 1
            } else if (asset.mediaType === "audio") {
                sourceType = 5
                trackIndex = 2  // audio track
            }

            var dur = (asset.duration && asset.duration > 0) ? asset.duration : 5.0
            var name = asset.displayName || asset.filePath.split("/").pop() || "Untitled"

            console.log("[VE2MediaPool] adding to timeline:", name,
                        "track=", trackIndex, "type=", asset.mediaType, "dur=", dur.toFixed(1))

            var clipId = timelineNotifier.addClip(trackIndex, sourceType, asset.filePath, name, dur)
            if (clipId >= 0) {
                timelineNotifier.selectClip(clipId)
                console.log("[VE2MediaPool] clip added: id=", clipId)
            } else {
                console.warn("[VE2MediaPool] addClip failed for:", name)
            }
        }

        function formatDuration(sec) {
            if (sec === undefined || sec <= 0) return ""
            var mins = Math.floor(sec / 60)
            var secs = Math.floor(sec % 60)
            if (mins > 0) {
                return mins + ":" + (secs < 10 ? "0" : "") + secs
            }
            return sec.toFixed(1) + "s"
        }
    }
}
