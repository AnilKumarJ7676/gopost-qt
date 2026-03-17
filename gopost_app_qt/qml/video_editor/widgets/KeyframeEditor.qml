import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * KeyframeEditor — property selector, keyframe strip with diamond markers.
 *
 * Converted 1:1 from keyframe_editor.dart.
 */
Item {
    id: root

    property int selectedClipId: timelineNotifier ? timelineNotifier.selectedClipId : -1

    readonly property var properties: [
        { id: 0, label: "Position X" },
        { id: 1, label: "Position Y" },
        { id: 2, label: "Scale" },
        { id: 3, label: "Rotation" },
        { id: 4, label: "Opacity" },
        { id: 5, label: "Volume" },
        { id: 6, label: "Speed" },
    ]

    property int selectedProp: 0
    property var keyframeList: selectedClipId >= 0 && timelineNotifier && timelineNotifier.trackCount >= 0
        ? timelineNotifier.clipKeyframeTimes(selectedClipId, selectedProp) : []

    // Clip time range for mapping keyframes to strip position
    property var clipInfo: selectedClipId >= 0 && timelineNotifier && timelineNotifier.trackCount >= 0
        ? timelineNotifier.clipData(internal.trackForClip(), internal.indexForClip()) : ({})
    property double clipStart: clipInfo.timelineIn !== undefined ? clipInfo.timelineIn : 0
    property double clipEnd: clipInfo.timelineOut !== undefined ? clipInfo.timelineOut : 1
    property double clipDuration: Math.max(0.01, clipEnd - clipStart)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // No clip hint
        RowLayout {
            visible: selectedClipId < 0
            Layout.margins: 12
            spacing: 6
            Label { text: "\u2139"; font.pixelSize: 14; color: "#6B6B88" }
            Label { text: "Select a clip to edit keyframes"; font.pixelSize: 13; color: "#6B6B88" }
        }

        // Property selector
        Flow {
            Layout.margins: 12
            spacing: 6

            Repeater {
                model: properties
                delegate: Rectangle {
                    property bool isActive: selectedProp === modelData.id
                    width: propLabel.implicitWidth + 16
                    height: 28
                    radius: 14
                    color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "#1A1A34"
                    border.color: isActive ? "#6C63FF" : "#303050"

                    Label {
                        id: propLabel
                        anchors.centerIn: parent
                        text: modelData.label
                        font.pixelSize: 12
                        color: isActive ? "#6C63FF" : "#B0B0C8"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: selectedProp = modelData.id
                    }
                }
            }
        }

        // Keyframe count
        Label {
            Layout.leftMargin: 12
            text: keyframeList.length + " keyframe" + (keyframeList.length !== 1 ? "s" : "")
            font.pixelSize: 11
            color: "#6B6B88"
            visible: selectedClipId >= 0
        }

        // Keyframe strip with diamond markers
        Rectangle {
            id: kfStrip
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.margins: 12
            color: "#1A1A34"
            radius: 6
            border.color: "#303050"

            // Placeholder text when empty
            Label {
                anchors.centerIn: parent
                text: "Click to add keyframe at playhead"
                font.pixelSize: 12
                color: "#6B6B88"
                visible: keyframeList.length === 0
            }

            // Playhead position indicator
            Rectangle {
                visible: selectedClipId >= 0 && timelineNotifier
                x: {
                    var pos = timelineNotifier ? timelineNotifier.position : 0
                    var frac = clipDuration > 0 ? (pos - clipStart) / clipDuration : 0
                    return Math.max(0, Math.min(1, frac)) * (kfStrip.width - 2) + 1
                }
                y: 0
                width: 1
                height: parent.height
                color: Qt.rgba(1, 1, 1, 0.3)
            }

            // Diamond keyframe markers
            Repeater {
                model: keyframeList
                delegate: Item {
                    property double kfTime: modelData.time !== undefined ? modelData.time : 0
                    property double frac: clipDuration > 0 ? (kfTime - clipStart) / clipDuration : 0
                    x: Math.max(4, Math.min(kfStrip.width - 12, frac * (kfStrip.width - 8)))
                    y: (kfStrip.height - 12) / 2
                    width: 12; height: 12

                    // Diamond shape via rotation
                    Rectangle {
                        anchors.centerIn: parent
                        width: 8; height: 8
                        rotation: 45
                        color: "#6C63FF"
                        border.color: "#9590FF"
                        border.width: 1

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                // Navigate to keyframe time
                                if (timelineNotifier) timelineNotifier.seek(kfTime)
                            }
                            onDoubleClicked: {
                                // Delete keyframe
                                if (timelineNotifier)
                                    timelineNotifier.removeKeyframe(selectedClipId, selectedProp, kfTime)
                            }
                        }
                    }

                    ToolTip {
                        parent: parent
                        visible: diamondMa.containsMouse
                        text: "t=" + kfTime.toFixed(2) + "s  v=" + (modelData.value !== undefined ? modelData.value.toFixed(2) : "?")
                    }

                    MouseArea {
                        id: diamondMa
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                z: -1
                onClicked: function(mouse) {
                    if (selectedClipId < 0 || !timelineNotifier) return
                    // Calculate time from click position
                    var frac = mouse.x / kfStrip.width
                    var time = clipStart + frac * clipDuration
                    timelineNotifier.addKeyframe(selectedClipId, selectedProp, time, 0)
                }
            }
        }

        // Action buttons
        RowLayout {
            Layout.margins: 12
            spacing: 8

            Button {
                text: "Add at Playhead"
                icon.name: "list-add"
                onClicked: {
                    if (timelineNotifier)
                        timelineNotifier.addKeyframe(selectedClipId, selectedProp,
                            timelineNotifier.position, 0)
                }
            }
            Button {
                text: "Delete"
                icon.name: "list-remove"
                onClicked: {
                    if (timelineNotifier)
                        timelineNotifier.removeKeyframe(selectedClipId, selectedProp,
                            timelineNotifier.position)
                }
            }
            Button {
                text: "Clear All"
                icon.name: "edit-clear"
                onClicked: {
                    if (timelineNotifier)
                        timelineNotifier.clearKeyframes(selectedClipId, selectedProp)
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    QtObject {
        id: internal

        // Helper to find track/index for selected clip (for clipData call)
        function trackForClip() {
            if (!timelineNotifier || selectedClipId < 0) return 0
            var tracks = timelineNotifier.tracks
            if (!tracks) return 0
            for (var t = 0; t < tracks.length; t++) {
                var count = timelineNotifier.clipCountForTrack(t)
                for (var c = 0; c < count; c++) {
                    var d = timelineNotifier.clipData(t, c)
                    if (d.clipId === selectedClipId) return t
                }
            }
            return 0
        }
        function indexForClip() {
            if (!timelineNotifier || selectedClipId < 0) return 0
            var tracks = timelineNotifier.tracks
            if (!tracks) return 0
            for (var t = 0; t < tracks.length; t++) {
                var count = timelineNotifier.clipCountForTrack(t)
                for (var c = 0; c < count; c++) {
                    var d = timelineNotifier.clipData(t, c)
                    if (d.clipId === selectedClipId) return c
                }
            }
            return 0
        }
    }
}
