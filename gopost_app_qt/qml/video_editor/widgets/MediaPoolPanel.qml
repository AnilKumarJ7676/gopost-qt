import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import GopostApp

/// Media pool panel: desktop drop, file picker, search, grid/list view, draggable assets, bins.
/// Converted from media_pool_panel.dart.
Item {
    id: root

    property bool isGridView: true
    property string searchText: ""
    property bool isDragOver: false

    // File dialog for importing media
    FileDialog {
        id: importFileDialog
        title: "Import Media Files"
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            "All media files (*.mp4 *.mov *.avi *.mkv *.webm *.m4v *.flv *.wmv *.3gp *.jpg *.jpeg *.png *.webp *.gif *.bmp *.heic *.heif *.tiff *.mp3 *.aac *.wav *.m4a *.flac *.ogg *.wma *.opus *.aiff)",
            "Video files (*.mp4 *.mov *.avi *.mkv *.webm *.m4v *.flv *.wmv *.3gp)",
            "Image files (*.jpg *.jpeg *.png *.webp *.gif *.bmp *.heic *.heif *.tiff)",
            "Audio files (*.mp3 *.aac *.wav *.m4a *.flac *.ogg *.wma *.opus *.aiff)",
            "All files (*)"
        ]
        onAccepted: {
            if (selectedFiles.length > 0) {
                mediaPoolNotifier.importFiles(selectedFiles)
            }
        }
    }

    Connections {
        target: mediaPoolNotifier
        function onFilePickerRequested() {
            importFileDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header: search + view toggle + import ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            color: "#12122A"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1; color: "#252540"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10; anchors.rightMargin: 10
                spacing: 6

                // Search
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    placeholderText: "\uD83D\uDD0D Search media..."
                    font.pixelSize: 13
                    color: "#E0E0F0"
                    placeholderTextColor: "#6B6B88"

                    background: Rectangle {
                        radius: 6
                        color: "#1A1A34"
                        border.color: "#303050"
                    }

                    onTextChanged: root.searchText = text
                }

                // Grid/List toggle
                ToolButton {
                    icon.name: root.isGridView ? "view-list-details" : "view-grid"
                    icon.width: 18; icon.height: 18; icon.color: "#8888A0"
                    ToolTip.text: root.isGridView ? "List view" : "Grid view"
                    onClicked: root.isGridView = !root.isGridView
                }

                // Import button
                Rectangle {
                    width: importRow.implicitWidth + 16
                    height: 30; radius: 6
                    color: "#6C63FF"

                    RowLayout {
                        id: importRow
                        anchors.centerIn: parent
                        spacing: 4

                        Label { text: "+"; font.pixelSize: 14; color: "white" }
                        Label { text: "Import"; font.pixelSize: 12; font.weight: Font.DemiBold; color: "white" }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mediaPoolNotifier.showFilePicker()
                    }
                }
            }
        }

        // --- Filter chips ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "#12122A"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1; color: "#252540"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10; anchors.rightMargin: 10
                spacing: 6

                Repeater {
                    model: [
                        { label: "All", filter: "all" },
                        { label: "Video", filter: "video" },
                        { label: "Image", filter: "image" },
                        { label: "Audio", filter: "audio" }
                    ]
                    delegate: Rectangle {
                        property bool isActive: mediaPoolNotifier && mediaPoolNotifier.currentFilter === modelData.filter
                        width: filterLabel.implicitWidth + 16
                        height: 24; radius: 12
                        color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "transparent"
                        border.color: isActive ? "#6C63FF" : "#353550"

                        Label {
                            id: filterLabel
                            anchors.centerIn: parent
                            text: modelData.label
                            font.pixelSize: 11
                            font.weight: isActive ? Font.DemiBold : Font.Normal
                            color: isActive ? "#6C63FF" : "#8888A0"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: mediaPoolNotifier.setFilter(modelData.filter)
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Sort
                ToolButton {
                    icon.name: "view-sort"
                    icon.width: 16; icon.height: 16; icon.color: "#8888A0"
                    ToolTip.text: "Sort"
                    onClicked: mediaPoolNotifier.cycleSort()
                }
            }
        }

        // --- Drop zone + content area ---
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Desktop drop target
            DropArea {
                id: dropArea
                anchors.fill: parent
                keys: ["text/uri-list"]

                onEntered: root.isDragOver = true
                onExited: root.isDragOver = false
                onDropped: drop => {
                    root.isDragOver = false
                    if (drop.hasUrls) {
                        mediaPoolNotifier.importFiles(drop.urls)
                    }
                }
            }

            // Drop highlight
            Rectangle {
                anchors.fill: parent
                anchors.margins: 8
                radius: 12
                color: "transparent"
                border.color: root.isDragOver ? "#6C63FF" : "transparent"
                border.width: 2
                visible: root.isDragOver

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Label {
                        text: "\uD83D\uDCE5"
                        font.pixelSize: 48; color: "#6C63FF"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        text: "Drop media files here"
                        font.pixelSize: 14; font.weight: Font.DemiBold; color: "#6C63FF"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // Empty state
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                visible: !root.isDragOver && mediaPoolNotifier && mediaPoolNotifier.assetCount === 0

                Label {
                    text: "\uD83C\uDFA5"
                    font.pixelSize: 48; color: "#404060"
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "No media imported"
                    font.pixelSize: 14; color: "#6B6B88"
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "Drop files here or click Import"
                    font.pixelSize: 12; color: "#505070"
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Grid view
            GridView {
                id: gridView
                anchors.fill: parent
                anchors.margins: 8
                visible: root.isGridView && !root.isDragOver
                clip: true
                cellWidth: (width - 8) / 3
                cellHeight: cellWidth * 0.75 + 24

                model: mediaPoolNotifier ? mediaPoolNotifier.filteredAssets : []

                delegate: Item {
                    width: GridView.view.cellWidth - 4
                    height: GridView.view.cellHeight - 4

                    // Extract model data — QVariantList of QVariantMap requires modelData access
                    readonly property var asset: modelData || ({})

                    Rectangle {
                        anchors.fill: parent
                        radius: 8
                        color: "#1A1A34"
                        border.color: "#303050"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 2

                            // Thumbnail placeholder
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 6
                                color: "#0E0E1C"

                                Label {
                                    anchors.centerIn: parent
                                    text: {
                                        if (asset.mediaType === "video") return "\uD83C\uDFA5"
                                        if (asset.mediaType === "audio") return "\uD83C\uDFB5"
                                        return "\uD83D\uDDBC"
                                    }
                                    font.pixelSize: 24
                                    color: "#505068"
                                }

                                // Duration badge
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    anchors.right: parent.right
                                    anchors.margins: 4
                                    visible: asset.duration !== undefined && asset.duration > 0
                                    width: durLabel.implicitWidth + 8
                                    height: 16; radius: 3
                                    color: Qt.rgba(0, 0, 0, 0.7)

                                    Label {
                                        id: durLabel
                                        anchors.centerIn: parent
                                        text: asset.duration !== undefined ? asset.duration.toFixed(1) + "s" : ""
                                        font.pixelSize: 9; color: "#D0D0E8"
                                    }
                                }
                            }

                            Label {
                                text: asset.displayName || ""
                                font.pixelSize: 10
                                color: "#D0D0E8"
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        // Add to timeline button
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottomMargin: 4
                            width: addLabel.implicitWidth + 12
                            height: 22; radius: 4
                            color: addBtnMa.pressed ? "#5A52DD" : "#6C63FF"

                            Label {
                                id: addLabel
                                anchors.centerIn: parent
                                text: "+ Timeline"
                                font.pixelSize: 9; font.weight: Font.DemiBold
                                color: "white"
                            }

                            MouseArea {
                                id: addBtnMa
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: mediaPoolNotifier.addToTimeline(asset.id)
                            }
                        }

                        MouseArea {
                            id: gridDragArea
                            anchors.fill: parent
                            z: -1
                            cursorShape: Qt.PointingHandCursor
                            drag.target: dragItem

                            onClicked: mediaPoolNotifier.selectAsset(asset.id)
                            onDoubleClicked: mediaPoolNotifier.addToTimeline(asset.id)
                            onPressed: console.log("[MediaPool] drag started for:", asset.displayName || "unknown")
                        }
                    }

                    // Drag handle
                    Item {
                        id: dragItem
                        Drag.active: gridDragArea.drag.active
                        Drag.hotSpot: Qt.point(width / 2, height / 2)
                        Drag.mimeData: { "text/plain": JSON.stringify({
                            type: "media",
                            assetId: asset.id || "",
                            mediaType: asset.mediaType || "video",
                            displayName: asset.displayName || "Untitled",
                            sourcePath: asset.filePath || "",
                            duration: asset.duration || 5.0
                        }) }
                        Drag.keys: ["application/x-gopost-media"]
                    }
                }
            }

            // List view
            ListView {
                id: listView
                anchors.fill: parent
                anchors.margins: 4
                visible: !root.isGridView && !root.isDragOver
                clip: true
                spacing: 2

                model: mediaPoolNotifier ? mediaPoolNotifier.filteredAssets : []

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 42
                    radius: 6

                    readonly property var asset: modelData || ({})

                    color: asset.isSelected ? Qt.rgba(0.424, 0.388, 1.0, 0.08) : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 8

                        Rectangle {
                            width: 32; height: 32; radius: 4
                            color: "#0E0E1C"

                            Label {
                                anchors.centerIn: parent
                                text: {
                                    if (asset.mediaType === "video") return "\uD83C\uDFA5"
                                    if (asset.mediaType === "audio") return "\uD83C\uDFB5"
                                    return "\uD83D\uDDBC"
                                }
                                font.pixelSize: 16; color: "#505068"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Label {
                                text: asset.displayName || ""
                                font.pixelSize: 12; color: "#E0E0F0"
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                            Label {
                                text: (asset.mediaType || "") + (asset.duration > 0 ? ("  \u2022  " + asset.duration.toFixed(1) + "s") : "")
                                font.pixelSize: 10; color: "#6B6B88"
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mediaPoolNotifier.selectAsset(asset.id)
                        onDoubleClicked: mediaPoolNotifier.addToTimeline(asset.id)
                    }
                }
            }
        }
    }
}
