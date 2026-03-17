import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * SpeedControlsPanel — speed presets, custom slider, reverse, freeze frame,
 *                       rate stretch, speed ramp presets.
 *
 * Converted 1:1 from speed_controls_panel.dart.
 */
Item {
    id: root

    readonly property var speedPresets: [0.1, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0, 8.0]

    property int selectedClipId: timelineNotifier ? timelineNotifier.selectedClipId : -1
    property real currentClipSpeed: selectedClipId >= 0 && timelineNotifier && timelineNotifier.trackCount >= 0 ? timelineNotifier.clipSpeed(selectedClipId) : 1.0
    property real customSpeed: currentClipSpeed
    property bool isDraggingSlider: false

    onCurrentClipSpeedChanged: customSpeed = currentClipSpeed

    Flickable {
        anchors.fill: parent
        contentHeight: mainCol.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: mainCol
            width: parent.width
            spacing: 0

            // No clip hint
            RowLayout {
                visible: !timelineNotifier || timelineNotifier.selectedClipId < 0
                Layout.margins: 12
                spacing: 6
                Label { text: "\u2139"; font.pixelSize: 16; color: "#6B6B88" }
                Label { text: "Select a clip to adjust speed"; font.pixelSize: 13; color: "#6B6B88" }
            }

            // All controls (dimmed when no clip)
            ColumnLayout {
                Layout.margins: 12
                spacing: 14
                opacity: timelineNotifier && timelineNotifier.selectedClipId >= 0 ? 1.0 : 0.45
                enabled: timelineNotifier && timelineNotifier.selectedClipId >= 0

                // Current info box
                Rectangle {
                    Layout.fillWidth: true
                    height: 68
                    radius: 8
                    color: "#1A1A34"
                    border.color: "#303050"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Rectangle {
                            width: 48; height: 48; radius: 8
                            color: Qt.rgba(0.424, 0.388, 1.0, 0.15)
                            Label {
                                anchors.centerIn: parent
                                text: currentClipSpeed.toFixed(1) + "x"
                                font.pixelSize: 15; font.weight: Font.Bold
                                color: "#6C63FF"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: currentClipSpeed.toFixed(2) + "x"
                                font.pixelSize: 14; font.weight: Font.DemiBold
                                color: "#E0E0F0"
                            }
                            Label {
                                text: "Speed: " + (currentClipSpeed < 1 ? "Slow Motion" : (currentClipSpeed > 1 ? "Fast Forward" : "Normal"))
                                font.pixelSize: 12; color: "#8888A0"
                            }
                        }
                    }
                }

                // Speed Presets
                SectionLabel { text: "Speed Presets" }

                Flow {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: speedPresets
                        delegate: Rectangle {
                            property real speed: modelData
                            property bool isActive: Math.abs(currentClipSpeed - speed) < 0.01
                            property bool isSlowMo: speed < 1.0
                            property bool isFast: speed > 1.0
                            property color accentColor: isSlowMo ? "#26C6DA" : (isFast ? "#FF7043" : "#6C63FF")

                            width: 58
                            height: isSlowMo || isFast ? 46 : 36
                            radius: 6
                            color: isActive ? Qt.rgba(
                                parseInt(accentColor.toString().substr(1,2), 16) / 255,
                                parseInt(accentColor.toString().substr(3,2), 16) / 255,
                                parseInt(accentColor.toString().substr(5,2), 16) / 255,
                                0.2
                            ) : "#1A1A34"
                            border.color: isActive ? accentColor : "#303050"

                            Column {
                                anchors.centerIn: parent
                                spacing: 2
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: speed + "x"
                                    font.pixelSize: 13; font.weight: Font.DemiBold
                                    color: isActive ? accentColor : "#B0B0C8"
                                }
                                Label {
                                    visible: isSlowMo
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "slow"
                                    font.pixelSize: 9
                                    color: Qt.rgba(0.149, 0.776, 0.855, 0.6)
                                }
                                Label {
                                    visible: isFast
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "fast"
                                    font.pixelSize: 9
                                    color: Qt.rgba(1.0, 0.439, 0.263, 0.6)
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (timelineNotifier.selectedClipId >= 0) {
                                        timelineNotifier.setClipSpeed(timelineNotifier.selectedClipId, speed);
                                    }
                                }
                            }
                        }
                    }
                }

                // Custom Speed slider
                SectionLabel { text: "Custom Speed" }

                Slider {
                    id: speedSlider
                    Layout.fillWidth: true
                    from: 0.1; to: 8.0
                    value: customSpeed
                    stepSize: 0.1
                    onMoved: {
                        customSpeed = value;
                    }
                    onPressedChanged: {
                        if (!pressed && timelineNotifier.selectedClipId >= 0) {
                            timelineNotifier.setClipSpeed(timelineNotifier.selectedClipId, value);
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "0.1x"; font.pixelSize: 11; color: "#6B6B88" }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: customSpeed.toFixed(2) + "x"
                        font.pixelSize: 13; font.weight: Font.DemiBold
                        font.family: "monospace"
                        color: "#6C63FF"
                    }
                    Item { Layout.fillWidth: true }
                    Label { text: "8.0x"; font.pixelSize: 11; color: "#6B6B88" }
                }

                // Special buttons
                SectionLabel { text: "Special" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    // Reverse
                    Rectangle {
                        Layout.fillWidth: true
                        height: 52; radius: 8
                        color: Qt.rgba(0.671, 0.278, 0.737, 0.08)

                        Column {
                            anchors.centerIn: parent
                            spacing: 2
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "\u21BA"
                                font.pixelSize: 18; color: "#AB47BC"
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "Reverse"
                                font.pixelSize: 12; font.weight: Font.DemiBold; color: "#AB47BC"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (timelineNotifier.selectedClipId >= 0)
                                    timelineNotifier.reverseClip(timelineNotifier.selectedClipId);
                            }
                        }
                    }

                    // Freeze frame
                    Rectangle {
                        Layout.fillWidth: true
                        height: 52; radius: 8
                        color: Qt.rgba(0.149, 0.776, 0.855, 0.08)

                        Column {
                            anchors.centerIn: parent
                            spacing: 2
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "\u23F8"
                                font.pixelSize: 18; color: "#26C6DA"
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "Freeze Frame"
                                font.pixelSize: 12; font.weight: Font.DemiBold; color: "#26C6DA"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (timelineNotifier.selectedClipId >= 0)
                                    timelineNotifier.freezeFrame(timelineNotifier.selectedClipId);
                            }
                        }
                    }
                }

                // Rate Stretch
                SectionLabel { text: "Rate Stretch" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Repeater {
                        model: [0.5, 0.75, 1.5, 2.0]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            height: 36; radius: 6
                            color: "#1A1A34"

                            Label {
                                anchors.centerIn: parent
                                text: modelData + "x dur"
                                font.pixelSize: 11; font.weight: Font.Medium
                                color: "#B0B0C8"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (timelineNotifier.selectedClipId >= 0)
                                        timelineNotifier.rateStretch(timelineNotifier.selectedClipId, 10.0 * modelData);
                                }
                            }
                        }
                    }
                }

                // Speed Ramp Presets
                SectionLabel { text: "Speed Ramp Presets" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RampPresetBtn {
                        Layout.fillWidth: true
                        label: "Ramp Up"
                        subtitle: "0.5x \u2192 2.0x"
                        accentColor: "#FF7043"
                        rampIndex: 0
                    }
                    RampPresetBtn {
                        Layout.fillWidth: true
                        label: "Ramp Down"
                        subtitle: "2.0x \u2192 0.5x"
                        accentColor: "#26C6DA"
                        rampIndex: 1
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RampPresetBtn {
                        Layout.fillWidth: true
                        label: "Pulse"
                        subtitle: "1x \u2192 3x \u2192 1x"
                        accentColor: "#AB47BC"
                        rampIndex: 2
                    }
                    RampPresetBtn {
                        Layout.fillWidth: true
                        label: "Slow-mo Hit"
                        subtitle: "1x \u2192 0.2x \u2192 1x"
                        accentColor: "#66BB6A"
                        rampIndex: 3
                    }
                }

                Item { height: 12 }
            }
        }
    }

    // Reusable section label
    component SectionLabel: Label {
        Layout.fillWidth: true
        font.pixelSize: 13
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        color: "#8888A0"
    }

    // Speed ramp preset button
    component RampPresetBtn: Rectangle {
        property string label: ""
        property string subtitle: ""
        property color accentColor: "#FF7043"
        property int rampIndex: 0

        height: 62; radius: 8
        color: Qt.rgba(
            accentColor.r, accentColor.g, accentColor.b, 0.06
        )

        Column {
            anchors.centerIn: parent
            spacing: 2
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: label
                font.pixelSize: 12; font.weight: Font.DemiBold
                color: accentColor
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: subtitle
                font.pixelSize: 10
                color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.6)
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (selectedClipId >= 0 && timelineNotifier)
                    timelineNotifier.applySpeedRamp(selectedClipId, rampIndex)
            }
        }
    }
}
