import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Panel providing draggable adjustment layer tiles (Effects, Presets, Custom tabs).
Item {
    id: root

    // Static effect type data
    readonly property var effectTypeList: [
        { id: 0, label: "Blur", icon: "\u25CE", color: "#AB47BC" },
        { id: 1, label: "Sharpen", icon: "\u2736", color: "#FF7043" },
        { id: 2, label: "Vignette", icon: "\u25C9", color: "#26C6DA" },
        { id: 3, label: "Film Grain", icon: "\u2591", color: "#FFCA28" },
        { id: 4, label: "Chromatic", icon: "\u25C8", color: "#42A5F5" },
        { id: 5, label: "Pixelate", icon: "\u25A3", color: "#66BB6A" },
        { id: 6, label: "Glitch", icon: "\u26A1", color: "#EF5350" },
        { id: 7, label: "Sepia", icon: "\u263C", color: "#8D6E63" }
    ]

    readonly property var presetFilterList: [
        { id: 1, label: "Cinematic" }, { id: 2, label: "Vintage" },
        { id: 3, label: "Warm Sunset" }, { id: 4, label: "Cool Blue" },
        { id: 5, label: "Desaturated" }, { id: 6, label: "High Contrast" },
        { id: 7, label: "Film Noir" }, { id: 8, label: "Dreamy" },
        { id: 9, label: "Retro 70s" }, { id: 10, label: "Polaroid" },
        { id: 11, label: "HDR" }, { id: 12, label: "Matte" }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "#16162E"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: "#252540"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12
                spacing: 8

                Label {
                    text: "\u2728"
                    font.pixelSize: 16
                    color: "#6C63FF"
                }
                Label {
                    text: "Adjustment Layers"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: "#D0D0E8"
                    Layout.fillWidth: true
                }
                Rectangle {
                    width: addRow.implicitWidth + 16
                    height: 26
                    radius: 4
                    color: "transparent"
                    border.color: "#303050"

                    RowLayout {
                        id: addRow
                        anchors.centerIn: parent
                        spacing: 4

                        Label {
                            text: "+"
                            font.pixelSize: 14
                            color: "#6C63FF"
                        }
                        Label {
                            text: "Add"
                            font.pixelSize: 12
                            color: "#D0D0E8"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: timelineNotifier.createAdjustmentClip()
                    }
                }
            }
        }

        // Tab bar
        TabBar {
            id: tabBar
            Layout.fillWidth: true

            background: Rectangle {
                color: "#14142B"
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 1
                    color: "#252540"
                }
            }

            TabButton {
                text: "Effects"
                width: implicitWidth
            }
            TabButton {
                text: "Presets"
                width: implicitWidth
            }
        }

        // Tab content
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Effects tab
            ListView {
                clip: true
                spacing: 6

                model: root.effectTypeList

                delegate: Rectangle {
                    width: ListView.view.width - 16
                    x: 8
                    height: 52
                    radius: 6
                    color: Qt.rgba(0.4, 0.3, 0.8, 0.12)
                    border.color: Qt.rgba(0.4, 0.3, 0.8, 0.3)

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 4

                        Label {
                            text: modelData.icon
                            font.pixelSize: 22
                            color: modelData.color
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: modelData.label
                            font.pixelSize: 11
                            color: "#D0D0E8"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (timelineNotifier && timelineNotifier.selectedClipId >= 0)
                                timelineNotifier.addEffect(timelineNotifier.selectedClipId, modelData.id, 0.5)
                        }
                    }
                }
            }

            // Presets tab
            GridView {
                clip: true
                cellWidth: (width - 16) / 3
                cellHeight: 40

                model: root.presetFilterList

                delegate: Rectangle {
                    width: GridView.view.cellWidth - 6
                    height: 34
                    radius: 6
                    color: Qt.rgba(0.671, 0.294, 0.737, 0.1)
                    border.color: Qt.rgba(0.671, 0.294, 0.737, 0.3)

                    Label {
                        anchors.centerIn: parent
                        text: modelData.label
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: "#D0D0E8"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (timelineNotifier && timelineNotifier.selectedClipId >= 0)
                                timelineNotifier.applyPresetFilter(timelineNotifier.selectedClipId, modelData.id)
                        }
                    }
                }
            }
        }
    }
}
