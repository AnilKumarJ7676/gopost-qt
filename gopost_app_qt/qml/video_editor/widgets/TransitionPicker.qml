import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * TransitionPicker — grid of 20 types, duration slider, easing selector.
 *
 * Converted 1:1 from transition_picker.dart.
 */
Item {
    id: root

    property int selectedClipId: timelineNotifier ? timelineNotifier.selectedClipId : -1

    readonly property var transitionTypes: [
        "Fade", "Dissolve", "Slide Left", "Slide Right", "Slide Up",
        "Slide Down", "Wipe Left", "Wipe Right", "Wipe Up", "Wipe Down",
        "Zoom", "Push", "Reveal", "Iris", "Clock",
        "Blur", "Glitch", "Morph", "Flash", "Spin"
    ]

    readonly property var easingCurves: [
        "Linear", "Ease In", "Ease Out", "Ease In-Out",
        "Bounce", "Elastic", "Back"
    ]

    TabBar {
        id: tabBar
        anchors.top: parent.top
        width: parent.width

        TabButton { text: "Transition In" }
        TabButton { text: "Transition Out" }
    }

    StackLayout {
        anchors.top: tabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: tabBar.currentIndex

        Repeater {
            model: 2
            delegate: ScrollView {
                property bool isIn: index === 0

                ColumnLayout {
                    width: parent.width
                    spacing: 12

                    // Grid of transitions
                    GridLayout {
                        columns: 4
                        columnSpacing: 6
                        rowSpacing: 6
                        Layout.margins: 16

                        Repeater {
                            model: transitionTypes.length
                            delegate: Rectangle {
                                width: 68; height: 56
                                radius: 8
                                color: "#1E1E38"
                                border.color: "#303050"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    Label {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "\u25A0"
                                        font.pixelSize: 20
                                        color: "#6B6B88"
                                    }
                                    Label {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: transitionTypes[index]
                                        font.pixelSize: 10
                                        color: "#B0B0C8"
                                        elide: Text.ElideRight
                                        width: 64
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (isIn)
                                            timelineNotifier.setTransitionIn(selectedClipId, index + 1, durationSlider.value, easingIndex);
                                        else
                                            timelineNotifier.setTransitionOut(selectedClipId, index + 1, durationSlider.value, easingIndex);
                                    }
                                }
                            }
                        }
                    }

                    // Duration
                    RowLayout {
                        Layout.margins: 16
                        spacing: 8
                        Label { text: "Duration: " + durationSlider.value.toFixed(1) + "s"; font.pixelSize: 14; color: "#B0B0C8" }
                        Slider {
                            id: durationSlider
                            Layout.fillWidth: true
                            from: 0.1; to: 3.0; value: 0.5
                            stepSize: 0.1
                        }
                    }

                    // Easing
                    Label { text: "Easing"; font.pixelSize: 14; color: "#B0B0C8"; Layout.leftMargin: 16 }
                    property int easingIndex: 3
                    Flow {
                        Layout.margins: 16
                        spacing: 8
                        Repeater {
                            model: easingCurves
                            delegate: Rectangle {
                                property bool isActive: parent.parent.easingIndex === index
                                width: easLabel.implicitWidth + 16
                                height: 28
                                radius: 14
                                color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "#1A1A34"
                                border.color: isActive ? "#6C63FF" : "#303050"

                                Label {
                                    id: easLabel
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.pixelSize: 12
                                    color: isActive ? "#6C63FF" : "#B0B0C8"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: parent.parent.parent.easingIndex = index
                                }
                            }
                        }
                    }

                    // Remove
                    Button {
                        Layout.margins: 16
                        Layout.fillWidth: true
                        text: "Remove Transition"
                        icon.name: "list-remove"
                        onClicked: {
                            if (isIn) timelineNotifier.removeTransitionIn(selectedClipId);
                            else timelineNotifier.removeTransitionOut(selectedClipId);
                        }
                    }
                }
            }
        }
    }
}
