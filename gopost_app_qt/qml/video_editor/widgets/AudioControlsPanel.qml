import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Panel for audio mixing controls: clip audio and track mixer.
Item {
    id: root

    ListView {
        anchors.fill: parent
        clip: true
        spacing: 0

        header: ColumnLayout {
            width: parent ? parent.width : 0
            spacing: 0

            // Clip Audio section
            Pane {
                Layout.fillWidth: true
                padding: 12

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: "Clip Audio"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: Qt.rgba(1, 1, 1, 0.9)
                    }

                    // No clip hint
                    RowLayout {
                        visible: timelineNotifier && timelineNotifier.selectedClipId < 0
                        spacing: 6
                        Label {
                            text: "\u24D8"
                            font.pixelSize: 14
                            opacity: 0.6
                        }
                        Label {
                            text: "Select a clip to adjust audio"
                            font.pixelSize: 13
                            opacity: 0.6
                        }
                    }

                    // Clip audio sliders
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        opacity: timelineNotifier && timelineNotifier.selectedClipId >= 0 ? 1.0 : 0.45
                        enabled: timelineNotifier && timelineNotifier.selectedClipId >= 0

                        // Volume
                        RowLayout {
                            spacing: 4
                            Label { text: "Volume"; font.pixelSize: 13; color: "#8888A0"; Layout.preferredWidth: 82 }
                            Slider {
                                Layout.fillWidth: true
                                from: 0; to: 2.0
                                value: timelineNotifier ? timelineNotifier.clipVolume : 1.0
                                onMoved: timelineNotifier.setClipVolume(timelineNotifier.selectedClipId, value)
                            }
                            Label {
                                text: (timelineNotifier ? timelineNotifier.clipVolume : 1.0).toFixed(1)
                                font.pixelSize: 12; color: "#8888A0"
                                Layout.preferredWidth: 38
                            }
                        }

                        // Pan
                        RowLayout {
                            spacing: 4
                            Label { text: "Pan"; font.pixelSize: 13; color: "#8888A0"; Layout.preferredWidth: 82 }
                            Label { text: "L"; font.pixelSize: 12; color: "#8888A0" }
                            Slider {
                                Layout.fillWidth: true
                                from: -1.0; to: 1.0
                                value: timelineNotifier ? timelineNotifier.clipPan : 0
                                onMoved: timelineNotifier.setClipPan(timelineNotifier.selectedClipId, value)
                            }
                            Label { text: "R"; font.pixelSize: 12; color: "#8888A0" }
                            Label {
                                text: (timelineNotifier ? timelineNotifier.clipPan : 0).toFixed(1)
                                font.pixelSize: 12; color: "#8888A0"
                                Layout.preferredWidth: 38
                            }
                        }

                        // Fade In
                        RowLayout {
                            spacing: 4
                            Label { text: "Fade In (s)"; font.pixelSize: 13; color: "#8888A0"; Layout.preferredWidth: 82 }
                            Slider {
                                Layout.fillWidth: true
                                from: 0; to: 5
                                value: timelineNotifier ? timelineNotifier.clipFadeIn : 0
                                onMoved: timelineNotifier.setClipFadeIn(timelineNotifier.selectedClipId, value)
                            }
                            Label {
                                text: (timelineNotifier ? timelineNotifier.clipFadeIn : 0).toFixed(1)
                                font.pixelSize: 12; color: "#8888A0"
                                Layout.preferredWidth: 38
                            }
                        }

                        // Fade Out
                        RowLayout {
                            spacing: 4
                            Label { text: "Fade Out (s)"; font.pixelSize: 13; color: "#8888A0"; Layout.preferredWidth: 82 }
                            Slider {
                                Layout.fillWidth: true
                                from: 0; to: 5
                                value: timelineNotifier ? timelineNotifier.clipFadeOut : 0
                                onMoved: timelineNotifier.setClipFadeOut(timelineNotifier.selectedClipId, value)
                            }
                            Label {
                                text: (timelineNotifier ? timelineNotifier.clipFadeOut : 0).toFixed(1)
                                font.pixelSize: 12; color: "#8888A0"
                                Layout.preferredWidth: 38
                            }
                        }

                        // Mute toggle
                        RowLayout {
                            spacing: 8
                            Label { text: "Mute"; font.pixelSize: 14; color: "#8888A0" }
                            Switch {
                                checked: timelineNotifier ? timelineNotifier.clipIsMuted : false
                                onToggled: timelineNotifier.setClipMuted(timelineNotifier.selectedClipId, checked)
                            }
                        }
                    }

                    Item { height: 16 }

                    // Track Mixer section
                    Label {
                        text: "Track Mixer"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: Qt.rgba(1, 1, 1, 0.9)
                    }
                }
            }
        }

        model: timelineNotifier ? timelineNotifier.tracks : []

        delegate: Rectangle {
            readonly property var track: modelData || ({})

            width: ListView.view.width - 24
            x: 12
            height: mixerCol.implicitHeight + 20
            radius: 6
            color: "#1E1E38"

            Component.onCompleted: console.log("[AudioMixer] track delegate created:",
                "label=", track.label, "index=", track.index, "vol=", track.volume)

            ColumnLayout {
                id: mixerCol
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: track.label || ("Track " + (track.index !== undefined ? track.index : "?"))
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: "#E0E0F0"
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        width: 40; height: 20
                        radius: 2
                        color: "#252540"
                        Label {
                            anchors.centerIn: parent
                            text: "---"
                            font.pixelSize: 11
                            opacity: 0.6
                        }
                    }
                }

                // Volume
                RowLayout {
                    spacing: 4
                    Label { text: "Vol"; font.pixelSize: 12; color: "#8888A0"; Layout.preferredWidth: 34 }
                    Slider {
                        Layout.fillWidth: true
                        from: 0; to: 2.0
                        value: track.volume !== undefined ? track.volume : 1.0
                        onMoved: {
                            console.log("[AudioMixer] setTrackVolume:", track.index, "\u2192", value)
                            timelineNotifier.setTrackVolume(track.index, value)
                        }
                    }
                    Label {
                        text: (track.volume !== undefined ? track.volume : 1.0).toFixed(1)
                        font.pixelSize: 12; color: "#8888A0"
                        Layout.preferredWidth: 30
                    }
                }

                // Pan
                RowLayout {
                    spacing: 4
                    Label { text: "Pan"; font.pixelSize: 12; color: "#8888A0"; Layout.preferredWidth: 34 }
                    Slider {
                        Layout.fillWidth: true
                        from: -1; to: 1
                        value: track.pan !== undefined ? track.pan : 0
                        onMoved: {
                            console.log("[AudioMixer] setTrackPan:", track.index, "\u2192", value)
                            timelineNotifier.setTrackPan(track.index, value)
                        }
                    }
                    Label {
                        text: (track.pan !== undefined ? track.pan : 0).toFixed(1)
                        font.pixelSize: 12; color: "#8888A0"
                        Layout.preferredWidth: 30
                    }
                }

                // Mute / Solo buttons
                RowLayout {
                    spacing: 6

                    Rectangle {
                        width: mLabel.implicitWidth + 20
                        height: 26; radius: 4
                        color: track.isMuted ? "#2A2050" : "#252540"

                        Label {
                            id: mLabel
                            anchors.centerIn: parent
                            text: "M"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: track.isMuted ? "#B0B0C8" : "#6B6B88"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.log("[AudioMixer] toggleTrackMute:", track.index)
                                timelineNotifier.toggleTrackMute(track.index)
                            }
                        }
                    }

                    Rectangle {
                        width: sLabel.implicitWidth + 20
                        height: 26; radius: 4
                        color: track.isSolo ? "#2A2050" : "#252540"

                        Label {
                            id: sLabel
                            anchors.centerIn: parent
                            text: "S"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: track.isSolo ? "#B0B0C8" : "#6B6B88"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.log("[AudioMixer] toggleTrackSolo:", track.index)
                                timelineNotifier.toggleTrackSolo(track.index)
                            }
                        }
                    }
                }
            }
        }
    }
}
