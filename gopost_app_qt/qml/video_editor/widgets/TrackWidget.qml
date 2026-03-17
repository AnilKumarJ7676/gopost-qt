import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * TrackWidget — track header + clip lane with positioned clips.
 *
 * Converted 1:1 from track_widget.dart.
 */
Item {
    id: root

    required property int trackIndex
    property real trackHeight: 68

    readonly property color trackBg: "#0F0F22"
    readonly property color headerBg: "#14142B"

    Row {
        anchors.fill: parent

        // --- Track header ---
        Rectangle {
            id: header
            width: 120
            height: parent.height
            color: headerBg
            border.color: "#252540"
            border.width: 0.5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2

                RowLayout {
                    spacing: 4

                    Label {
                        text: {
                            var icons = ["\u25B6", "\uD83C\uDFA5", "\uD83C\uDFB5", "\uD83D\uDCDD", "\u2728"];
                            return icons[Math.min(trackIndex, icons.length - 1)];
                        }
                        font.pixelSize: 14
                    }

                    Label {
                        text: "Track " + (trackIndex + 1)
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: "#D0D0E8"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    spacing: 2

                    ToolButton {
                        width: 22; height: 22
                        icon.name: "visibility"
                        icon.width: 14; icon.height: 14
                        icon.color: "#8888A0"
                        ToolTip.text: "Toggle visibility"
                        onClicked: timelineNotifier.toggleTrackVisibility(trackIndex)
                    }
                    ToolButton {
                        width: 22; height: 22
                        icon.name: "lock"
                        icon.width: 14; icon.height: 14
                        icon.color: "#8888A0"
                        ToolTip.text: "Toggle lock"
                        onClicked: timelineNotifier.toggleTrackLock(trackIndex)
                    }
                    ToolButton {
                        width: 22; height: 22
                        icon.name: "audio-volume-muted"
                        icon.width: 14; icon.height: 14
                        icon.color: "#8888A0"
                        ToolTip.text: "Toggle mute"
                        onClicked: timelineNotifier.toggleTrackMute(trackIndex)
                    }
                }
            }
        }

        // --- Clip lane ---
        Rectangle {
            width: parent.width - header.width
            height: parent.height
            color: trackBg
            clip: true

            // Clips positioned absolutely within the lane
            Repeater {
                id: clipRepeater
                // trackCount is a Q_PROPERTY with NOTIFY stateChanged — reading it
                // ensures QML re-evaluates this binding when timeline state changes.
                model: timelineNotifier && timelineNotifier.trackCount >= 0 ? timelineNotifier.clipCountForTrack(trackIndex) : 0
                onModelChanged: console.log("[TrackWidget] track", root.trackIndex, "clipRepeater model changed to:", model)
                delegate: ClipWidget {
                    required property int index
                    clipIndex: index
                    trackIndex: root.trackIndex
                    y: 2
                    height: parent.height - 4
                    Component.onCompleted: console.log("[ClipWidget] CREATED track:", trackIndex, "clip:", clipIndex, "id:", clipId, "x:", x, "w:", width)
                }
            }

            // Drop zone for track-level drops
            DropArea {
                anchors.fill: parent
                keys: ["application/x-gopost-media"]

                Rectangle {
                    anchors.fill: parent
                    color: parent.containsDrag ? Qt.rgba(0.424, 0.388, 1.0, 0.1) : "transparent"
                    border.color: parent.containsDrag ? "#6C63FF" : "transparent"
                    border.width: 2
                    radius: 4
                }
            }
        }
    }
}
