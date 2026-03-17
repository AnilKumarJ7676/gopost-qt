import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Panel for project aspect ratio, clip position/scale/rotation, flip, Ken Burns presets.
Item {
    id: root

    readonly property var clip: timelineNotifier ? timelineNotifier.selectedClip : null

    property real posX: 0
    property real posY: 0
    property real scaleVal: 1.0
    property real rotation: 0
    property bool flipH: false
    property bool flipV: false

    readonly property var aspectRatios: [
        { label: "16:9", w: 1920, h: 1080 },
        { label: "9:16", w: 1080, h: 1920 },
        { label: "1:1",  w: 1080, h: 1080 },
        { label: "4:5",  w: 1080, h: 1350 },
        { label: "4:3",  w: 1440, h: 1080 },
        { label: "21:9", w: 2560, h: 1080 }
    ]

    ListView {
        anchors.fill: parent
        clip: true
        spacing: 0

        model: ListModel { id: dummyModel; ListElement { dummy: true } }

        delegate: ColumnLayout {
            width: ListView.view.width
            spacing: 4

            Item { height: 8 }

            // --- Project Aspect Ratio ---
            Label {
                text: "Project Aspect Ratio"
                font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0"
                Layout.leftMargin: 12
            }
            Item { height: 8 }

            Flow {
                Layout.fillWidth: true
                Layout.leftMargin: 12; Layout.rightMargin: 12
                spacing: 6

                Repeater {
                    model: root.aspectRatios
                    delegate: Rectangle {
                        property bool isActive: timelineNotifier && timelineNotifier.projectWidth === modelData.w
                                                && timelineNotifier.projectHeight === modelData.h
                        width: 68
                        height: 52
                        radius: 8
                        color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "#1A1A34"
                        border.color: isActive ? "#6C63FF" : "#303050"

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 4

                            Label {
                                text: "\u25A3"
                                font.pixelSize: 20
                                color: isActive ? "#6C63FF" : "#6B6B88"
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Label {
                                text: modelData.label
                                font.pixelSize: 12; font.weight: Font.DemiBold
                                color: isActive ? "#6C63FF" : "#B0B0C8"
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: timelineNotifier.setProjectDimensions(modelData.w, modelData.h)
                        }
                    }
                }
            }

            // Current dimensions
            Label {
                visible: timelineNotifier && timelineNotifier.projectWidth > 0
                text: timelineNotifier ? (timelineNotifier.projectWidth + " x " + timelineNotifier.projectHeight) : ""
                font.pixelSize: 12; font.family: Theme.fontFamilyMono; color: "#6B6B88"
                Layout.alignment: Qt.AlignHCenter
            }

            Item { height: 16 }

            // --- Clip Transform ---
            Label {
                text: "Clip Transform"
                font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0"
                Layout.leftMargin: 12
            }
            Item { height: 6 }

            // No clip hint
            RowLayout {
                visible: !root.clip
                Layout.leftMargin: 12
                spacing: 6
                Label { text: "\u24D8"; font.pixelSize: 14; color: "#6B6B88" }
                Label { text: "Select a clip to transform"; font.pixelSize: 11; color: "#6B6B88" }
            }

            // Sliders (disabled when no clip)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 12; Layout.rightMargin: 12
                spacing: 4
                opacity: root.clip ? 1.0 : 0.45
                enabled: !!root.clip

                // Position X
                RowLayout {
                    spacing: 4
                    Label { text: "Position X"; font.pixelSize: 12; color: "#8888A0"; Layout.preferredWidth: 62 }
                    Slider {
                        Layout.fillWidth: true
                        from: -1000; to: 1000; value: root.posX
                        onMoved: { root.posX = value; if (root.clip) timelineNotifier.setKeyframe(root.clip.id, 0, value) }
                    }
                    Label { text: root.posX.toFixed(1); font.pixelSize: 12; font.family: Theme.fontFamilyMono; color: "#B0B0C8"; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignRight }
                }

                // Position Y
                RowLayout {
                    spacing: 4
                    Label { text: "Position Y"; font.pixelSize: 12; color: "#8888A0"; Layout.preferredWidth: 62 }
                    Slider {
                        Layout.fillWidth: true
                        from: -1000; to: 1000; value: root.posY
                        onMoved: { root.posY = value; if (root.clip) timelineNotifier.setKeyframe(root.clip.id, 1, value) }
                    }
                    Label { text: root.posY.toFixed(1); font.pixelSize: 12; font.family: Theme.fontFamilyMono; color: "#B0B0C8"; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignRight }
                }

                // Scale
                RowLayout {
                    spacing: 4
                    Label { text: "Scale"; font.pixelSize: 12; color: "#8888A0"; Layout.preferredWidth: 62 }
                    Slider {
                        Layout.fillWidth: true
                        from: 0.1; to: 5.0; value: root.scaleVal
                        onMoved: { root.scaleVal = value; if (root.clip) timelineNotifier.setKeyframe(root.clip.id, 2, value) }
                    }
                    Label { text: root.scaleVal.toFixed(1); font.pixelSize: 12; font.family: Theme.fontFamilyMono; color: "#B0B0C8"; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignRight }
                }

                // Rotation
                RowLayout {
                    spacing: 4
                    Label { text: "Rotation"; font.pixelSize: 12; color: "#8888A0"; Layout.preferredWidth: 62 }
                    Slider {
                        Layout.fillWidth: true
                        from: -360; to: 360; value: root.rotation
                        onMoved: { root.rotation = value; if (root.clip) timelineNotifier.setKeyframe(root.clip.id, 3, value) }
                    }
                    Label { text: root.rotation.toFixed(1); font.pixelSize: 12; font.family: Theme.fontFamilyMono; color: "#B0B0C8"; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignRight }
                }

                Item { height: 12 }

                // Flip buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: 8
                        color: root.flipH ? Qt.rgba(0.424, 0.388, 1.0, 0.15) : "#1A1A34"

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Label { text: "\u2194"; font.pixelSize: 16; color: root.flipH ? "#6C63FF" : "#6B6B88" }
                            Label { text: "Flip H"; font.pixelSize: 13; color: root.flipH ? "#6C63FF" : "#B0B0C8" }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.flipH = !root.flipH
                                if (root.clip) timelineNotifier.setKeyframe(root.clip.id, 2, root.flipH ? -Math.abs(root.scaleVal) : Math.abs(root.scaleVal))
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: 8
                        color: root.flipV ? Qt.rgba(0.424, 0.388, 1.0, 0.15) : "#1A1A34"

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Label { text: "\u2195"; font.pixelSize: 16; color: root.flipV ? "#6C63FF" : "#6B6B88" }
                            Label { text: "Flip V"; font.pixelSize: 13; color: root.flipV ? "#6C63FF" : "#B0B0C8" }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.flipV = !root.flipV
                        }
                    }
                }

                Item { height: 12 }

                // Preset buttons: Fit, Fill, Reset
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: [
                            { label: "Fit",   action: "fit" },
                            { label: "Fill",  action: "fill" },
                            { label: "Reset", action: "reset" }
                        ]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            radius: 6
                            color: "#1A1A34"

                            Label {
                                anchors.centerIn: parent
                                text: modelData.label
                                font.pixelSize: 12; font.weight: Font.DemiBold; color: "#B0B0C8"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (modelData.action === "fit") {
                                        root.posX = 0; root.posY = 0; root.scaleVal = 1.0; root.rotation = 0
                                        if (root.clip) timelineNotifier.resetTransform(root.clip.id)
                                    } else if (modelData.action === "fill") {
                                        root.posX = 0; root.posY = 0; root.scaleVal = 1.2; root.rotation = 0
                                        if (root.clip) timelineNotifier.setKeyframe(root.clip.id, 2, 1.2)
                                    } else {
                                        root.posX = 0; root.posY = 0; root.scaleVal = 1.0; root.rotation = 0
                                        root.flipH = false; root.flipV = false
                                        if (root.clip) timelineNotifier.resetTransform(root.clip.id)
                                    }
                                }
                            }
                        }
                    }
                }

                Item { height: 16 }

                // --- Ken Burns ---
                Label {
                    text: "Ken Burns (Auto Pan & Zoom)"
                    font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0"
                }
                Item { height: 8 }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 36; radius: 8
                        color: Qt.rgba(0.149, 0.776, 0.855, 0.08)

                        Label { anchors.centerIn: parent; text: "Zoom In"; font.pixelSize: 13; font.weight: Font.Medium; color: "#26C6DA" }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { if (root.clip) timelineNotifier.applyKenBurns(root.clip.id, 1.0, 1.4) }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 36; radius: 8
                        color: Qt.rgba(0.149, 0.776, 0.855, 0.08)

                        Label { anchors.centerIn: parent; text: "Zoom Out"; font.pixelSize: 13; font.weight: Font.Medium; color: "#26C6DA" }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { if (root.clip) timelineNotifier.applyKenBurns(root.clip.id, 1.4, 1.0) }
                        }
                    }
                }
                Item { height: 6 }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 36; radius: 8
                        color: Qt.rgba(0.149, 0.776, 0.855, 0.08)

                        Label { anchors.centerIn: parent; text: "Pan Left"; font.pixelSize: 13; font.weight: Font.Medium; color: "#26C6DA" }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { if (root.clip) timelineNotifier.applyKenBurnsPan(root.clip.id, 100, 0, -100, 0) }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 36; radius: 8
                        color: Qt.rgba(0.149, 0.776, 0.855, 0.08)

                        Label { anchors.centerIn: parent; text: "Pan Right"; font.pixelSize: 13; font.weight: Font.Medium; color: "#26C6DA" }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { if (root.clip) timelineNotifier.applyKenBurnsPan(root.clip.id, -100, 0, 100, 0) }
                        }
                    }
                }

                Item { height: 16 }
            }
        }
    }
}
