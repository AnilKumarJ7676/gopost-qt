import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * ColorGradingPanel — 10 sliders, preset pills with drag, reset.
 *
 * Converted 1:1 from color_grading_panel.dart.
 */
Item {
    id: root

    property int selectedClipId: timelineNotifier ? timelineNotifier.selectedClipId : -1
    property var grading: selectedClipId >= 0 && timelineNotifier && timelineNotifier.trackCount >= 0 ? timelineNotifier.clipColorGrading(selectedClipId) : ({})

    readonly property var sliders: [
        { label: "Brightness",  prop: "brightness",  min: -100, max: 100 },
        { label: "Contrast",    prop: "contrast",    min: -100, max: 100 },
        { label: "Saturation",  prop: "saturation",  min: -100, max: 100 },
        { label: "Temperature", prop: "temperature", min: -100, max: 100 },
        { label: "Tint",        prop: "tint",        min: -100, max: 100 },
        { label: "Shadows",     prop: "shadows",     min: -100, max: 100 },
        { label: "Highlights",  prop: "highlights",  min: -100, max: 100 },
        { label: "Vibrance",    prop: "vibrance",    min: -100, max: 100 },
        { label: "Fade",        prop: "fade",        min: 0,    max: 100 },
        { label: "Vignette",    prop: "vignette",    min: 0,    max: 100 },
    ]

    readonly property var presets: [
        "Cinematic", "Vintage", "Warm Sunset", "Cool Blue", "Desaturated",
        "High Contrast", "Soft Pastel", "Film Noir", "Bleach Bypass",
        "Cross Process", "Orange Teal", "Moody", "Dreamy", "Retro 70s",
        "Polaroid", "HDR", "Matte", "Cyberpunk", "Golden", "Arctic"
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Info when no clip
        RowLayout {
            visible: selectedClipId < 0
            Layout.margins: 12
            spacing: 6
            Label { text: "\u2139"; font.pixelSize: 14; color: "#6B6B88" }
            Label { text: "Select a clip to grade"; font.pixelSize: 13; color: "#6B6B88" }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            header: Column {
                width: parent ? parent.width : 0
                padding: 12

                // Preset pills
                Label { text: "PRESETS"; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#8888A0" }
                Flow {
                    width: parent.width - 24
                    spacing: 6
                    topPadding: 8
                    bottomPadding: 12

                    Repeater {
                        model: presets
                        delegate: Rectangle {
                            width: presetLabel.implicitWidth + 16
                            height: 28
                            radius: 14
                            color: "#1A1A34"
                            border.color: "#303050"

                            Label {
                                id: presetLabel
                                anchors.centerIn: parent
                                text: modelData
                                font.pixelSize: 12
                                color: "#B0B0C8"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: timelineNotifier.applyPresetFilter(selectedClipId, index + 1)
                            }
                        }
                    }
                }

                Rectangle { width: parent.width - 24; height: 1; color: "#252540" }
            }

            model: sliders.length
            delegate: Item {
                width: ListView.view.width
                height: 44
                property var s: sliders[index]

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Label {
                        text: s.label
                        font.pixelSize: 12
                        color: "#8888A0"
                        Layout.preferredWidth: 80
                    }

                    Slider {
                        Layout.fillWidth: true
                        from: s.min; to: s.max
                        value: grading[s.prop] !== undefined ? grading[s.prop] : 0
                        onMoved: {
                            if (selectedClipId >= 0)
                                timelineNotifier.setColorGradingProperty(selectedClipId, s.prop, value)
                        }
                    }

                    Label {
                        text: Math.round(parent.children[1].value)
                        font.pixelSize: 12
                        font.family: "monospace"
                        color: "#B0B0C8"
                        Layout.preferredWidth: 30
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            footer: Item {
                width: parent ? parent.width : 0
                height: 50

                Button {
                    anchors.centerIn: parent
                    text: "Reset Color Grading"
                    icon.name: "view-refresh"
                    onClicked: timelineNotifier.resetColorGrading(selectedClipId)
                }
            }
        }
    }
}
