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
FocusScope {
    id: root
    focus: true

    property bool isGridView: true
    property bool isDragOver: false
    property real thumbWidth: 120  // thumbnail width (resizable 80–240)

    // Keyboard: Ctrl+A is handled in VideoEditor2Screen.qml (routes to media pool
    // or timeline based on active panel). Delete/F2/Enter are local to this panel
    // and use Keys.onPressed below to avoid Shortcut ambiguity.

    // Handle keyboard shortcuts via Keys (scoped to when this FocusScope has activeFocus)
    Keys.onPressed: event => {
        if (!mediaPoolNotifier) return

        if (event.key === Qt.Key_Delete && mediaPoolNotifier.selectedAssetCount > 0) {
            var ids = mediaPoolNotifier.selectedAssetIds
            for (var i = 0; i < ids.length; i++)
                mediaPoolNotifier.removeAsset(ids[i])
            event.accepted = true
        } else if (event.key === Qt.Key_F2 && mediaPoolNotifier.selectedAssetCount === 1) {
            renameDialog.open()
            event.accepted = true
        } else if (event.key === Qt.Key_Return && mediaPoolNotifier.selectedAssetCount > 0) {
            var selIds = mediaPoolNotifier.selectedAssetIds
            var assets = mediaPoolNotifier.filteredAssets
            for (var j = 0; j < assets.length; j++) {
                if (assets[j].id === selIds[0]) {
                    internal.addAssetToTimeline(assets[j])
                    break
                }
            }
            event.accepted = true
        }
    }

    // ---- Drag proxy — parented to window overlay so it floats above all panels ----
    Item {
        id: dragProxy
        parent: Overlay.overlay
        visible: false
        width: 120; height: 80
        z: 999999
        opacity: 0.92

        property var assetData: ({})
        property string mediaType: ""
        property string displayName: ""
        property int dragCount: 1

        Drag.hotSpot: Qt.point(width / 2, height / 2)
        Drag.keys: ["application/x-gopost-media"]

        Rectangle {
            anchors.fill: parent; radius: 6
            color: "#2A2A50"
            border.color: "#6C63FF"; border.width: 1.5

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 4; spacing: 2

                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    radius: 4; color: "#0E0E1C"
                    Label {
                        anchors.centerIn: parent
                        text: dragProxy.mediaType === "video" ? "\uD83C\uDFA5"
                            : dragProxy.mediaType === "audio" ? "\uD83C\uDFB5" : "\uD83D\uDDBC"
                        font.pixelSize: 18; color: "#8888B0"
                    }
                }
                Label {
                    text: dragProxy.displayName
                    font.pixelSize: 9; color: "#D0D0E8"
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }
            // Badge showing count when dragging multiple items
            Rectangle {
                visible: dragProxy.dragCount > 1
                anchors.top: parent.top; anchors.right: parent.right
                anchors.topMargin: -6; anchors.rightMargin: -6
                width: 22; height: 22; radius: 11
                color: "#6C63FF"
                Label {
                    anchors.centerIn: parent
                    text: dragProxy.dragCount
                    font.pixelSize: 11; font.weight: Font.Bold
                    color: "#FFFFFF"
                }
            }
        }
    }

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

    // Grab focus when clicking anywhere in the media pool panel
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        propagateComposedEvents: true
        onPressed: mouse => { root.forceActiveFocus(); mouse.accepted = false }
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

        // ---- Breadcrumb + Bin bar ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: mediaPoolNotifier && (mediaPoolNotifier.activeBinId !== "" || mediaPoolNotifier.bins.length > 0) ? 28 : 0
            visible: height > 0
            color: "#0E0E1C"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#252540" }

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 4

                // Back button
                Label {
                    visible: mediaPoolNotifier && mediaPoolNotifier.activeBinId !== ""
                    text: "\u2190"; font.pixelSize: 14; color: "#8888A0"
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: { if (mediaPoolNotifier) mediaPoolNotifier.navigateUp() } }
                }

                // Breadcrumb path
                Label {
                    text: mediaPoolNotifier ? mediaPoolNotifier.binBreadcrumb : "All Media"
                    font.pixelSize: 10; color: "#6B6B88"
                    elide: Text.ElideMiddle; Layout.fillWidth: true
                }

                // New bin button
                Rectangle {
                    width: newBinLbl.implicitWidth + 10; height: 20; radius: 4
                    color: newBinHov.hovered ? Qt.rgba(1,1,1,0.06) : "transparent"
                    Label { id: newBinLbl; anchors.centerIn: parent; text: "+ Bin"; font.pixelSize: 9; color: "#6B6B88" }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: newBinDialog.open() }
                    HoverHandler { id: newBinHov }
                }
            }
        }

        // ---- Bins in current level ----
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: mediaPoolNotifier && mediaPoolNotifier.bins.length > 0 ? implicitHeight : 0
            visible: mediaPoolNotifier && mediaPoolNotifier.bins.length > 0
            Layout.leftMargin: 8; Layout.rightMargin: 8; Layout.topMargin: 4
            spacing: 4

            Repeater {
                model: mediaPoolNotifier ? mediaPoolNotifier.bins : []
                delegate: Rectangle {
                    width: binItemLbl.implicitWidth + 28; height: 24; radius: 4
                    color: binItemHov.hovered ? Qt.rgba(0.424, 0.388, 1.0, 0.12) : "#1A1A34"
                    border.color: "#303050"

                    RowLayout {
                        anchors.centerIn: parent; spacing: 3
                        Label { text: "\uD83D\uDCC1"; font.pixelSize: 10 }
                        Label { id: binItemLbl; text: modelData.name || "Bin"; font.pixelSize: 10; color: "#D0D0E8" }
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onDoubleClicked: { if (mediaPoolNotifier) mediaPoolNotifier.navigateIntoBin(modelData.id) } }
                    HoverHandler { id: binItemHov }
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

            // ---- Marquee drag-select overlay (above grid/list, passes through to items) ----
            MouseArea {
                id: marqueeArea
                anchors.fill: parent; anchors.margins: 6
                visible: !root.isDragOver
                z: 2  // above grid/list so we always get the press first
                acceptedButtons: Qt.LeftButton

                property bool isMarquee: false
                property point marqueeStart: Qt.point(0, 0)
                property point marqueeCurrent: Qt.point(0, 0)
                property var startSelectedIds: []  // IDs selected before marquee started

                // Check if the press is on a populated grid/list cell
                function isOnItem(mx, my) {
                    if (!mediaPoolNotifier) return false
                    var count = mediaPoolNotifier.filteredAssets.length
                    if (count === 0) return false

                    if (root.isGridView) {
                        var cols = Math.max(1, Math.floor(gridView.width / gridView.cellWidth))
                        var col = Math.floor(mx / gridView.cellWidth)
                        var row = Math.floor((my + gridView.contentY) / gridView.cellHeight)
                        if (col < 0 || col >= cols) return false
                        var idx = row * cols + col
                        return idx >= 0 && idx < count
                    } else {
                        var itemH = 38 + 2  // height + spacing
                        var idx2 = Math.floor((my + listView.contentY) / itemH)
                        return idx2 >= 0 && idx2 < count
                    }
                }

                onPressed: mouse => {
                    root.forceActiveFocus()

                    // If press is on a grid/list item, pass through to the item delegate
                    if (isOnItem(mouse.x, mouse.y)) {
                        mouse.accepted = false
                        return
                    }

                    // Empty space — start marquee tracking
                    isMarquee = false
                    marqueeStart = Qt.point(mouse.x, mouse.y)
                    marqueeCurrent = Qt.point(mouse.x, mouse.y)

                    // If no modifier, deselect all on empty-space click
                    if (!(mouse.modifiers & Qt.ControlModifier) && !(mouse.modifiers & Qt.ShiftModifier)) {
                        if (mediaPoolNotifier) mediaPoolNotifier.deselectAll()
                    }
                    startSelectedIds = mediaPoolNotifier ? mediaPoolNotifier.selectedAssetIds : []
                }
                onPositionChanged: mouse => {
                    if (!pressed) return
                    marqueeCurrent = Qt.point(mouse.x, mouse.y)
                    var dx = marqueeCurrent.x - marqueeStart.x
                    var dy = marqueeCurrent.y - marqueeStart.y
                    if (!isMarquee && (dx*dx + dy*dy > 36)) {
                        isMarquee = true
                    }
                    if (isMarquee) {
                        internal.marqueeSelect(marqueeStart, marqueeCurrent, startSelectedIds)
                    }
                }
                onReleased: {
                    isMarquee = false
                    marqueeRect.visible = false
                }

                // Marquee rectangle visual
                Rectangle {
                    id: marqueeRect
                    visible: marqueeArea.isMarquee
                    color: Qt.rgba(0.424, 0.388, 1.0, 0.15)
                    border.color: "#6C63FF"; border.width: 1; radius: 2
                    x: Math.min(marqueeArea.marqueeStart.x, marqueeArea.marqueeCurrent.x)
                    y: Math.min(marqueeArea.marqueeStart.y, marqueeArea.marqueeCurrent.y)
                    width: Math.abs(marqueeArea.marqueeCurrent.x - marqueeArea.marqueeStart.x)
                    height: Math.abs(marqueeArea.marqueeCurrent.y - marqueeArea.marqueeStart.y)
                    z: 100
                }
            }

            // ---- Grid View ----
            GridView {
                id: gridView
                anchors.fill: parent; anchors.margins: 6
                visible: root.isGridView && !root.isDragOver
                clip: true
                z: 1  // above marquee area so items get press events first
                cellWidth: Math.max(80, Math.min(root.thumbWidth + 8, (width - 6) / Math.max(1, Math.floor((width - 6) / (root.thumbWidth + 8)))))
                cellHeight: cellWidth * 0.625 + 28

                model: mediaPoolNotifier ? mediaPoolNotifier.filteredAssets : []

                delegate: Item {
                    width: GridView.view.cellWidth - 4
                    height: GridView.view.cellHeight - 4

                    readonly property var asset: modelData || ({})
                    // Bind selection to selectionGeneration so it re-evaluates without model rebuild
                    readonly property bool isItemSelected: {
                        void(mediaPoolNotifier ? mediaPoolNotifier.selectionGeneration : 0)
                        return mediaPoolNotifier ? mediaPoolNotifier.isAssetSelected(asset.id) : false
                    }

                    Rectangle {
                        anchors.fill: parent; radius: 6
                        color: isItemSelected ? Qt.rgba(0.424, 0.388, 1.0, 0.12) : "#1A1A34"
                        border.color: isItemSelected ? "#6C63FF" : "#303050"

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 3; spacing: 2

                            // Thumbnail with hover-scrub preview
                            Rectangle {
                                id: thumbRect
                                Layout.fillWidth: true; Layout.fillHeight: true
                                radius: 4; color: "#0E0E1C"; clip: true

                                property bool isVideo: asset.mediaType === "video"
                                property bool isScrubbing: isVideo && scrubArea.containsMouse
                                property double scrubTime: 0.0
                                property string thumbBase64: isVideo && asset.filePath
                                                            ? Qt.btoa(asset.filePath) : ""

                                // Poster frame for videos (shown when not hovering)
                                Image {
                                    id: posterImage
                                    anchors.fill: parent
                                    visible: thumbRect.isVideo && !thumbRect.isScrubbing
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                    cache: true
                                    source: thumbRect.thumbBase64 !== ""
                                            ? "image://videothumbnail/" + thumbRect.thumbBase64
                                              + "@0@" + Math.round(parent.height)
                                            : ""
                                }

                                // Scrub frame for videos (shown during hover)
                                Image {
                                    id: scrubImage
                                    anchors.fill: parent
                                    visible: thumbRect.isScrubbing
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                    cache: true
                                    source: thumbRect.isScrubbing && thumbRect.thumbBase64 !== ""
                                            ? "image://videothumbnail/" + thumbRect.thumbBase64
                                              + "@" + thumbRect.scrubTime.toFixed(1)
                                              + "@" + Math.round(parent.height)
                                            : ""
                                }

                                // Fallback icon (non-video, or while loading)
                                Label {
                                    anchors.centerIn: parent
                                    visible: !thumbRect.isVideo
                                             || (posterImage.status !== Image.Ready
                                                 && !thumbRect.isScrubbing)
                                    text: asset.mediaType === "video" ? "\uD83C\uDFA5"
                                        : asset.mediaType === "audio" ? "\uD83C\uDFB5" : "\uD83D\uDDBC"
                                    font.pixelSize: 20; color: "#505068"
                                }

                                // Hover-scrub mouse area
                                MouseArea {
                                    id: scrubArea
                                    anchors.fill: parent
                                    hoverEnabled: thumbRect.isVideo
                                    acceptedButtons: Qt.NoButton
                                    z: 2

                                    onPositionChanged: mouse => {
                                        if (!containsMouse || !thumbRect.isVideo) return
                                        var dur = asset.duration || 0
                                        if (dur <= 0) return
                                        var ratio = Math.max(0, Math.min(1, mouse.x / width))
                                        // Quantize to 0.5s steps to limit unique requests
                                        var raw = ratio * dur
                                        thumbRect.scrubTime = Math.round(raw * 2) / 2
                                    }
                                    onExited: thumbRect.scrubTime = 0
                                }

                                // Scrub progress bar
                                Rectangle {
                                    anchors.left: parent.left; anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    height: 3; color: Qt.rgba(0, 0, 0, 0.5)
                                    visible: thumbRect.isScrubbing
                                    z: 3

                                    Rectangle {
                                        width: {
                                            var dur = asset.duration || 1
                                            return parent.width * Math.min(1, thumbRect.scrubTime / dur)
                                        }
                                        height: parent.height
                                        color: "#6C63FF"
                                    }
                                }

                                // Timecode overlay
                                Rectangle {
                                    anchors.top: parent.top; anchors.left: parent.left
                                    anchors.margins: 3
                                    visible: thumbRect.isScrubbing
                                    width: tcLabel.implicitWidth + 6; height: 14; radius: 3
                                    color: Qt.rgba(0, 0, 0, 0.8)
                                    z: 4

                                    Label {
                                        id: tcLabel; anchors.centerIn: parent
                                        text: internal.formatTimecode(thumbRect.scrubTime)
                                        font.pixelSize: 8; font.family: "monospace"
                                        color: "#E0E0F0"
                                    }
                                }

                                // Duration badge
                                Rectangle {
                                    anchors.bottom: parent.bottom; anchors.right: parent.right
                                    anchors.margins: 3
                                    visible: !thumbRect.isScrubbing
                                             && asset.duration !== undefined && asset.duration > 0
                                    width: durText.implicitWidth + 6; height: 14; radius: 3
                                    color: Qt.rgba(0, 0, 0, 0.75)
                                    z: 4

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

                        // Click / double-click / drag-to-timeline
                        MouseArea {
                            id: gridDragArea
                            anchors.fill: parent; z: -1
                            cursorShape: Qt.PointingHandCursor
                            preventStealing: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton

                            property point pressPos
                            property bool isDragging: false
                            property int pressModifiers: 0
                            property bool deferredSelect: false  // true = do single-select on release

                            onPressed: mouse => {
                                root.forceActiveFocus()
                                pressPos = Qt.point(mouse.x, mouse.y)
                                isDragging = false
                                deferredSelect = false
                                pressModifiers = mouse.modifiers

                                // Select on press for immediate visual feedback
                                if (mouse.button === Qt.LeftButton && mediaPoolNotifier && asset.id) {
                                    if (pressModifiers & Qt.ControlModifier) {
                                        mediaPoolNotifier.toggleAssetSelection(asset.id)
                                    } else if (pressModifiers & Qt.ShiftModifier) {
                                        mediaPoolNotifier.rangeSelectAsset(asset.id)
                                    } else if (mediaPoolNotifier.isAssetSelected(asset.id)
                                               && mediaPoolNotifier.selectedAssetCount > 1) {
                                        // Already selected in a multi-selection — don't clear yet.
                                        // If user releases without dragging, we single-select then.
                                        deferredSelect = true
                                    } else {
                                        mediaPoolNotifier.selectAsset(asset.id)
                                    }
                                }
                            }
                            onPositionChanged: mouse => {
                                if (!pressed) return
                                if (!isDragging) {
                                    var dx = mouse.x - pressPos.x
                                    var dy = mouse.y - pressPos.y
                                    if (dx*dx + dy*dy > 64) {
                                        isDragging = true
                                        deferredSelect = false
                                        internal.beginDrag(asset)
                                    }
                                }
                                if (isDragging) {
                                    var gp = mapToItem(dragProxy.parent, mouse.x, mouse.y)
                                    dragProxy.x = gp.x - dragProxy.width / 2
                                    dragProxy.y = gp.y - dragProxy.height / 2
                                }
                            }
                            onReleased: mouse => {
                                if (isDragging) {
                                    dragProxy.Drag.drop()
                                    internal.endDrag()
                                    isDragging = false
                                    return
                                }
                                // Deferred single-select: user clicked a multi-selected item
                                // without dragging, so narrow selection to just this item
                                if (deferredSelect && mediaPoolNotifier && asset.id) {
                                    mediaPoolNotifier.selectAsset(asset.id)
                                    deferredSelect = false
                                }
                                if (mouse.button === Qt.RightButton) {
                                    if (mediaPoolNotifier && asset.id) {
                                        if (!mediaPoolNotifier.isAssetSelected(asset.id))
                                            mediaPoolNotifier.selectAsset(asset.id)
                                        poolContextMenu._targetAssetId = asset.id
                                        poolContextMenu._targetAssetName = asset.displayName || ""
                                        poolContextMenu.popup()
                                    }
                                    return
                                }
                            }
                            onDoubleClicked: {
                                if (!isDragging)
                                    internal.addAssetToTimeline(asset)
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
                z: 1

                model: mediaPoolNotifier ? mediaPoolNotifier.filteredAssets : []

                delegate: Rectangle {
                    width: ListView.view.width; height: 38; radius: 4

                    readonly property var asset: modelData || ({})
                    readonly property bool isItemSelected: {
                        void(mediaPoolNotifier ? mediaPoolNotifier.selectionGeneration : 0)
                        return mediaPoolNotifier ? mediaPoolNotifier.isAssetSelected(asset.id) : false
                    }

                    color: isItemSelected ? Qt.rgba(0.424, 0.388, 1.0, 0.12) : "transparent"
                    border.color: isItemSelected ? "#6C63FF" : "transparent"
                    border.width: isItemSelected ? 1 : 0

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

                    // Click / double-click / drag for list items
                    MouseArea {
                        id: listDragArea
                        anchors.fill: parent; z: -1
                        cursorShape: Qt.PointingHandCursor
                        preventStealing: true
                        acceptedButtons: Qt.LeftButton

                        property point pressPos
                        property bool isDragging: false
                        property int pressModifiers: 0
                        property bool deferredSelect: false

                        onPressed: mouse => {
                            root.forceActiveFocus()
                            pressPos = Qt.point(mouse.x, mouse.y)
                            isDragging = false
                            deferredSelect = false
                            pressModifiers = mouse.modifiers

                            if (mediaPoolNotifier && asset.id) {
                                if (pressModifiers & Qt.ControlModifier) {
                                    mediaPoolNotifier.toggleAssetSelection(asset.id)
                                } else if (pressModifiers & Qt.ShiftModifier) {
                                    mediaPoolNotifier.rangeSelectAsset(asset.id)
                                } else if (mediaPoolNotifier.isAssetSelected(asset.id)
                                           && mediaPoolNotifier.selectedAssetCount > 1) {
                                    deferredSelect = true
                                } else {
                                    mediaPoolNotifier.selectAsset(asset.id)
                                }
                            }
                        }
                        onPositionChanged: mouse => {
                            if (!pressed) return
                            if (!isDragging) {
                                var dx = mouse.x - pressPos.x
                                var dy = mouse.y - pressPos.y
                                if (dx*dx + dy*dy > 64) {
                                    isDragging = true
                                    deferredSelect = false
                                    internal.beginDrag(asset)
                                }
                            }
                            if (isDragging) {
                                var gp = mapToItem(dragProxy.parent, mouse.x, mouse.y)
                                dragProxy.x = gp.x - dragProxy.width / 2
                                dragProxy.y = gp.y - dragProxy.height / 2
                            }
                        }
                        onReleased: mouse => {
                            if (isDragging) {
                                dragProxy.Drag.drop()
                                internal.endDrag()
                                isDragging = false
                                return
                            }
                            if (deferredSelect && mediaPoolNotifier && asset.id) {
                                mediaPoolNotifier.selectAsset(asset.id)
                                deferredSelect = false
                            }
                        }
                        onDoubleClicked: {
                            if (!isDragging)
                                internal.addAssetToTimeline(asset)
                        }
                    }
                }
            }
        }

        // ---- Footer: asset count + thumbnail slider ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: "#0D0D1A"
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#252540" }

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6

                Label {
                    text: {
                        var count = mediaPoolNotifier ? mediaPoolNotifier.assetCount : 0
                        var sel = mediaPoolNotifier ? mediaPoolNotifier.selectedAssetCount : 0
                        return sel > 0 ? (sel + " of " + count + " selected") : (count + " assets")
                    }
                    font.pixelSize: 10; color: "#6B6B88"
                }

                Item { Layout.fillWidth: true }

                // Thumbnail size slider (grid view only)
                Label { visible: root.isGridView; text: "\u25A3"; font.pixelSize: 9; color: "#6B6B88" }
                Slider {
                    visible: root.isGridView
                    Layout.preferredWidth: 60
                    from: 80; to: 240; value: root.thumbWidth
                    onMoved: root.thumbWidth = value
                    background: Rectangle {
                        x: parent.leftPadding; y: parent.topPadding + parent.availableHeight / 2 - 1
                        width: parent.availableWidth; height: 2; radius: 1; color: "#252540"
                    }
                }
                Label { visible: root.isGridView; text: "\u25A3"; font.pixelSize: 13; color: "#6B6B88" }
            }
        }
    }

    // ---- Helpers ----
    QtObject {
        id: internal

        function beginDrag(asset) {
            // Build payload for all selected assets (or just the dragged one)
            var items = []
            var selectedIds = mediaPoolNotifier ? mediaPoolNotifier.selectedAssetIds : []
            var isDraggedSelected = false
            for (var i = 0; i < selectedIds.length; ++i) {
                if (selectedIds[i] === asset.id) { isDraggedSelected = true; break }
            }

            if (isDraggedSelected && selectedIds.length > 1) {
                // Build an id→true lookup map for O(1) checks instead of O(S) inner loop
                var selectedSet = {}
                for (var si = 0; si < selectedIds.length; ++si)
                    selectedSet[selectedIds[si]] = true

                // Single pass over filteredAssets — O(A) instead of O(S × A)
                var allAssets = mediaPoolNotifier.filteredAssets
                for (var a = 0; a < allAssets.length; ++a) {
                    if (selectedSet[allAssets[a].id]) {
                        items.push({
                            assetId: allAssets[a].id || "",
                            mediaType: allAssets[a].mediaType || "video",
                            displayName: allAssets[a].displayName || "Untitled",
                            sourcePath: allAssets[a].filePath || "",
                            duration: allAssets[a].duration || 5.0
                        })
                    }
                }
            } else {
                items.push({
                    assetId: asset.id || "",
                    mediaType: asset.mediaType || "video",
                    displayName: asset.displayName || "Untitled",
                    sourcePath: asset.filePath || "",
                    duration: asset.duration || 5.0
                })
            }

            var payload = {
                type: "media",
                // Keep single-item fields for backward compat
                assetId: items[0].assetId,
                mediaType: items[0].mediaType,
                displayName: items.length > 1 ? (items.length + " items") : items[0].displayName,
                sourcePath: items[0].sourcePath,
                duration: items[0].duration,
                // Multi-item array
                assets: items
            }
            dragProxy.assetData = payload
            dragProxy.mediaType = payload.mediaType
            dragProxy.displayName = payload.displayName
            dragProxy.dragCount = items.length
            dragProxy.Drag.mimeData = { "text/plain": JSON.stringify(payload) }
            dragProxy.visible = true
            dragProxy.Drag.active = true
            console.log("[VE2MediaPool] drag started:", items.length, "item(s)")
        }

        function endDrag() {
            dragProxy.Drag.active = false
            dragProxy.visible = false
            dragProxy.assetData = {}
            dragProxy.dragCount = 1
        }

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

        // Marquee rubber-band selection: select items whose grid cells intersect the rect
        function marqueeSelect(start, end, preserveIds) {
            if (!mediaPoolNotifier) return
            var assets = mediaPoolNotifier.filteredAssets
            if (!assets || assets.length === 0) return

            var rx = Math.min(start.x, end.x)
            var ry = Math.min(start.y, end.y)
            var rw = Math.abs(end.x - start.x)
            var rh = Math.abs(end.y - start.y)

            // Use the active view to determine item positions
            var view = root.isGridView ? gridView : listView

            // Deselect all first, then re-select preserved + intersected
            mediaPoolNotifier.deselectAll()
            // Restore previously selected
            for (var p = 0; p < preserveIds.length; p++)
                mediaPoolNotifier.toggleAssetSelection(preserveIds[p])

            if (root.isGridView) {
                var cols = Math.max(1, Math.floor(gridView.width / gridView.cellWidth))
                for (var i = 0; i < assets.length; i++) {
                    var col = i % cols
                    var row = Math.floor(i / cols)
                    var cx = col * gridView.cellWidth
                    var cy = row * gridView.cellHeight - gridView.contentY
                    var cw = gridView.cellWidth - 4
                    var ch = gridView.cellHeight - 4

                    // Rectangle intersection test
                    if (cx < rx + rw && cx + cw > rx && cy < ry + rh && cy + ch > ry) {
                        if (!mediaPoolNotifier.isAssetSelected(assets[i].id))
                            mediaPoolNotifier.toggleAssetSelection(assets[i].id)
                    }
                }
            } else {
                for (var j = 0; j < assets.length; j++) {
                    var ly = j * (38 + 2) - listView.contentY
                    var lh = 38
                    if (ly < ry + rh && ly + lh > ry) {
                        if (!mediaPoolNotifier.isAssetSelected(assets[j].id))
                            mediaPoolNotifier.toggleAssetSelection(assets[j].id)
                    }
                }
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

        function formatTimecode(sec) {
            if (sec === undefined || sec < 0) sec = 0
            var h = Math.floor(sec / 3600)
            var m = Math.floor((sec % 3600) / 60)
            var s = Math.floor(sec % 60)
            var f = Math.round((sec % 1) * 30)  // assume 30fps for frame count
            return (h < 10 ? "0" : "") + h + ":"
                 + (m < 10 ? "0" : "") + m + ":"
                 + (s < 10 ? "0" : "") + s + ":"
                 + (f < 10 ? "0" : "") + f
        }
    }

    // ================================================================
    // Right-click context menu
    // ================================================================
    Menu {
        id: poolContextMenu
        property string _targetAssetId: ""
        property string _targetAssetName: ""

        MenuItem {
            text: "Add to Timeline"
            onTriggered: {
                var assets = mediaPoolNotifier ? mediaPoolNotifier.filteredAssets : []
                for (var i = 0; i < assets.length; i++) {
                    if (assets[i].id === poolContextMenu._targetAssetId) {
                        internal.addAssetToTimeline(assets[i])
                        break
                    }
                }
            }
        }
        MenuSeparator {}
        MenuItem {
            text: "Rename"
            onTriggered: renameDialog.open()
        }
        MenuItem {
            text: "Reveal in File Explorer"
            onTriggered: {
                if (mediaPoolNotifier)
                    mediaPoolNotifier.revealInExplorer(poolContextMenu._targetAssetId)
            }
        }
        MenuSeparator {}
        MenuItem {
            text: "Move to Bin..."
            enabled: mediaPoolNotifier && mediaPoolNotifier.bins.length > 0
            onTriggered: moveToBinMenu.popup()
        }
        MenuItem {
            text: {
                var count = mediaPoolNotifier ? mediaPoolNotifier.selectedAssetCount : 0
                return count > 1 ? ("Delete " + count + " items") : "Delete from Pool"
            }
            onTriggered: {
                if (!mediaPoolNotifier) return
                var ids = mediaPoolNotifier.selectedAssetIds
                for (var i = 0; i < ids.length; i++)
                    mediaPoolNotifier.removeAsset(ids[i])
            }
        }
    }

    // Move to Bin sub-menu
    Menu {
        id: moveToBinMenu
        Repeater {
            model: mediaPoolNotifier ? mediaPoolNotifier.bins : []
            delegate: MenuItem {
                text: modelData.name || "Bin"
                onTriggered: {
                    if (mediaPoolNotifier) {
                        var ids = mediaPoolNotifier.selectedAssetIds
                        for (var i = 0; i < ids.length; i++)
                            mediaPoolNotifier.moveAssetToBin(ids[i], modelData.id)
                    }
                }
            }
        }
    }

    // ================================================================
    // Rename dialog (F2)
    // ================================================================
    Dialog {
        id: renameDialog
        title: "Rename Asset"
        modal: true; anchors.centerIn: Overlay.overlay
        width: 280; standardButtons: Dialog.Cancel | Dialog.Ok
        background: Rectangle { radius: 10; color: "#1A1A34"; border.color: "#303050" }

        contentItem: TextField {
            id: renameField
            font.pixelSize: 14; color: "#E0E0F0"
            background: Rectangle { radius: 6; color: "#12122A"; border.color: "#6C63FF" }
            onAccepted: renameDialog.accept()
        }
        onOpened: {
            renameField.text = poolContextMenu._targetAssetName
            renameField.forceActiveFocus()
            renameField.selectAll()
        }
        onAccepted: {
            if (mediaPoolNotifier && renameField.text.trim().length > 0) {
                var ids = mediaPoolNotifier.selectedAssetIds
                if (ids.length > 0)
                    mediaPoolNotifier.renameAsset(ids[0], renameField.text.trim())
            }
        }
    }

    // ================================================================
    // New Bin dialog
    // ================================================================
    Dialog {
        id: newBinDialog
        title: "New Bin"
        modal: true; anchors.centerIn: Overlay.overlay
        width: 260; standardButtons: Dialog.Cancel | Dialog.Ok
        background: Rectangle { radius: 10; color: "#1A1A34"; border.color: "#303050" }

        contentItem: TextField {
            id: newBinField
            placeholderText: "Bin name"
            font.pixelSize: 14; color: "#E0E0F0"
            background: Rectangle { radius: 6; color: "#12122A"; border.color: "#6C63FF" }
            onAccepted: newBinDialog.accept()
        }
        onOpened: { newBinField.text = ""; newBinField.forceActiveFocus() }
        onAccepted: {
            if (mediaPoolNotifier && newBinField.text.trim().length > 0)
                mediaPoolNotifier.createBin(newBinField.text.trim())
        }
    }

    // ================================================================
    // Toast notifications (import rejected, duplicate)
    // ================================================================
    Rectangle {
        id: poolToast
        visible: false; z: 200
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 30
        width: poolToastLbl.implicitWidth + 20; height: 28; radius: 6
        color: Qt.rgba(0.94, 0.33, 0.31, 0.9)

        Label { id: poolToastLbl; anchors.centerIn: parent; font.pixelSize: 11; color: "#FFFFFF" }

        function show(msg) { poolToastLbl.text = msg; visible = true; poolToastTimer.restart() }
        Timer { id: poolToastTimer; interval: 3000; onTriggered: poolToast.visible = false }
    }

    Connections {
        target: mediaPoolNotifier
        function onImportRejected(fileName, reason) { poolToast.show(reason) }
        function onDuplicateDetected(fileName) { poolToast.show("Already imported: " + fileName) }
    }
}
