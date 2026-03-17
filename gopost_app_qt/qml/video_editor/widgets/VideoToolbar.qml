import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

/**
 * VideoToolbar — import/add tools, NLE edit tools, panel shortcuts, track management.
 *
 * Connects all remaining Q_INVOKABLE methods not covered by sidebar panels:
 *   - splitClipAtPlayhead, rippleDelete
 *   - setInPoint, setOutPoint, clearInOutPoints
 *   - toggleProxyPlayback
 *   - addTrack (video/audio/title/effect)
 *   - createAdjustmentClip
 */
Item {
    id: root
    height: 48

    property bool importing: false

    Rectangle {
        anchors.fill: parent
        color: "#1E1E38"
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width; height: 1
            color: "#303050"
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 0

        // Initializing state
        RowLayout {
            visible: timelineNotifier.phase === 1 // initializing
            spacing: 8
            BusyIndicator { running: true; Layout.preferredWidth: 16; Layout.preferredHeight: 16 }
            Label { text: "Initializing..."; font.pixelSize: 12; color: "#8888A0" }
        }

        // Error state
        Button {
            visible: timelineNotifier.phase === 3 // error
            text: "Retry"
            icon.name: "view-refresh"
            onClicked: timelineNotifier.initTimeline()
        }

        // Ready state tools
        RowLayout {
            visible: timelineNotifier.isReady
            spacing: 0

            // === Import section ===
            ToolBtn { iconText: "\uD83C\uDFA5"; label: "Video"; enabled: !importing; onClicked: videoDialog.open() }
            ToolBtn { iconText: "\uD83D\uDDBC"; label: "Photo"; enabled: !importing; onClicked: imageDialog.open() }

            ToolSep {}

            // === NLE Edit Tools ===
            ToolBtn {
                iconText: "\u2702"
                label: "Split"
                tooltip: "Split clip at playhead (Ctrl+B)"
                enabled: timelineNotifier && timelineNotifier.selectedClipId >= 0
                onClicked: timelineNotifier.splitClipAtPlayhead()
            }
            ToolBtn {
                iconText: "\u232B"
                label: "Ripple"
                tooltip: "Ripple delete selected clip (Shift+Del)"
                enabled: timelineNotifier && timelineNotifier.selectedClipId >= 0
                onClicked: timelineNotifier.rippleDelete(timelineNotifier.selectedClipId)
            }

            ToolSep {}

            // === In/Out Points ===
            ToolBtn {
                iconText: "["
                label: "In"
                tooltip: "Set In point (I)"
                onClicked: timelineNotifier.setInPoint()
            }
            ToolBtn {
                iconText: "]"
                label: "Out"
                tooltip: "Set Out point (O)"
                onClicked: timelineNotifier.setOutPoint()
            }
            ToolBtn {
                iconText: "\u2715"
                label: "Clear"
                tooltip: "Clear In/Out points (Alt+X)"
                onClicked: timelineNotifier.clearInOutPoints()
            }

            ToolSep {}

            // === Content creation ===
            ToolBtn { iconText: "T"; label: "Text"; onClicked: internal.addTextClip() }
            ToolBtn {
                iconText: "\u25A3"
                label: "Adjust"
                tooltip: "Create adjustment layer"
                onClicked: timelineNotifier.createAdjustmentClip()
            }

            ToolSep {}

            // === Panel shortcuts ===
            ToolBtn { iconText: "\u2728"; label: "Effects"; onClicked: timelineNotifier.setActivePanel(3) }
            ToolBtn { iconText: "\uD83C\uDFA8"; label: "Color"; onClicked: timelineNotifier.setActivePanel(4) }
            ToolBtn { iconText: "\u21C4"; label: "Trans."; onClicked: timelineNotifier.setActivePanel(5) }
            ToolBtn { iconText: "\u23F1"; label: "Keyframe"; onClicked: timelineNotifier.setActivePanel(8) }
            ToolBtn { iconText: "\u266B"; label: "Audio"; onClicked: timelineNotifier.setActivePanel(9) }

            ToolSep {}

            // === Track management ===
            ToolBtn { iconText: "+"; label: "Track"; onClicked: addTrackMenu.open() }
        }

        // Importing indicator
        RowLayout {
            visible: importing
            spacing: 6
            Layout.leftMargin: 8
            BusyIndicator { running: true; Layout.preferredWidth: 16; Layout.preferredHeight: 16 }
            Label { text: "Importing..."; font.pixelSize: 12; color: "#8888A0" }
        }

        Item { Layout.fillWidth: true }

        // === Right side: Proxy toggle + Delete ===
        RowLayout {
            visible: timelineNotifier.isReady
            spacing: 0

            // Proxy playback toggle
            Rectangle {
                width: proxyRow.implicitWidth + 12
                height: 32
                radius: 6
                color: timelineNotifier && timelineNotifier.useProxyPlayback
                       ? Qt.rgba(0.4, 0.78, 0.55, 0.15) : "transparent"
                border.color: timelineNotifier && timelineNotifier.useProxyPlayback
                              ? "#66BB6A" : "#303050"
                border.width: 1

                RowLayout {
                    id: proxyRow
                    anchors.centerIn: parent
                    spacing: 4
                    Label {
                        text: "\u25B6"
                        font.pixelSize: 10
                        color: timelineNotifier && timelineNotifier.useProxyPlayback ? "#66BB6A" : "#6B6B88"
                    }
                    Label {
                        text: "Proxy"
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: timelineNotifier && timelineNotifier.useProxyPlayback ? "#66BB6A" : "#8888A0"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: timelineNotifier.toggleProxyPlayback()
                }

                ToolTip.visible: proxyHover.hovered
                ToolTip.text: "Toggle proxy playback for smoother editing"
                HoverHandler { id: proxyHover }
            }

            Item { width: 8 }

            // Delete selected clip
            ToolBtn {
                visible: timelineNotifier.selectedClipId >= 0
                iconText: "\uD83D\uDDD1"
                label: "Delete"
                onClicked: timelineNotifier.removeClip(timelineNotifier.selectedClipId)
            }
        }
    }

    // Add track popup menu
    Menu {
        id: addTrackMenu
        MenuItem { text: "Video Track"; onTriggered: timelineNotifier.addTrack(1) }
        MenuItem { text: "Audio Track"; onTriggered: timelineNotifier.addTrack(2) }
        MenuItem { text: "Title Track"; onTriggered: timelineNotifier.addTrack(3) }
        MenuItem { text: "Effect Track"; onTriggered: timelineNotifier.addTrack(0) }
    }

    // File dialogs
    FileDialog {
        id: videoDialog
        title: "Select Video"
        nameFilters: ["Video files (*.mp4 *.mov *.avi *.mkv *.webm *.m4v *.flv *.wmv *.3gp)"]
        onAccepted: internal.addClipFromFile(selectedFile, true)
    }

    FileDialog {
        id: imageDialog
        title: "Select Image"
        nameFilters: ["Image files (*.jpg *.jpeg *.png *.webp *.gif *.bmp *.heic *.tiff)"]
        onAccepted: internal.addClipFromFile(selectedFile, false)
    }

    QtObject {
        id: internal

        function addTextClip() {
            timelineNotifier.addTrack(3); // title
            var clipId = timelineNotifier.addClip(0, 3, "", "Text", 5.0);
            if (clipId >= 0) timelineNotifier.selectClip(clipId);
        }

        function addClipFromFile(fileUrl, isVideo) {
            root.importing = true;
            var path = fileUrl.toString().replace("file:///", "");
            var name = path.split("/").pop();
            var duration = isVideo ? 10.0 : 5.0;
            var sourceType = isVideo ? 0 : 1; // video=0, image=1
            var clipId = timelineNotifier.addClip(1, sourceType, path, name, duration);
            if (clipId >= 0 && isVideo) {
                timelineNotifier.generateProxyForClip(clipId);
            }
            root.importing = false;
        }
    }

    // Reusable tool button component
    component ToolBtn: Item {
        property string iconText: ""
        property string label: ""
        property string tooltip: ""
        property bool enabled: true
        signal clicked()
        width: col.implicitWidth + 16
        height: 44
        Layout.leftMargin: 2; Layout.rightMargin: 2
        opacity: enabled ? 1.0 : 0.3

        Column {
            id: col
            anchors.centerIn: parent
            spacing: 2
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: iconText
                font.pixelSize: 16
                color: "#E0E0F0"
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: label
                font.pixelSize: 9
                color: "#8888A0"
            }
        }
        MouseArea {
            anchors.fill: parent
            enabled: parent.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }

        ToolTip.visible: tooltip !== "" && btnHover.hovered
        ToolTip.text: tooltip
        HoverHandler { id: btnHover }
    }

    // Separator component
    component ToolSep: Rectangle {
        width: 1; height: 28; color: "#303050"
        Layout.leftMargin: 4; Layout.rightMargin: 4
    }
}
