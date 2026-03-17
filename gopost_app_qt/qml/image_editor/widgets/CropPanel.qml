import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Panel to apply center crop by aspect ratio and replace canvas with result.
Rectangle {
    id: root

    property int selectedIndex: 0
    property bool isApplying: false

    height: 120
    color: "#1E1E2E"

    Rectangle {
        anchors.top: parent.top
        width: parent.width; height: 0.5
        color: Qt.rgba(1, 1, 1, 0.1)
    }

    readonly property var cropAspects: [
        { label: "Original", widthRatio: 0, heightRatio: 0 },
        { label: "1:1",      widthRatio: 1, heightRatio: 1 },
        { label: "4:5",      widthRatio: 4, heightRatio: 5 },
        { label: "16:9",     widthRatio: 16, heightRatio: 9 }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: "Crop"
            color: Qt.rgba(1, 1, 1, 0.7)
            font.pixelSize: 13; font.weight: Font.Medium
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                contentWidth: chipRow.width
                clip: true
                flickableDirection: Flickable.HorizontalFlick

                Row {
                    id: chipRow
                    spacing: 8

                    Repeater {
                        model: root.cropAspects

                        delegate: Rectangle {
                            width: chipLabel.implicitWidth + 24
                            height: 32
                            radius: 16
                            color: root.selectedIndex === index
                                   ? Qt.rgba(0.42, 0.39, 1.0, 0.3)
                                   : Qt.rgba(1, 1, 1, 0.08)
                            border.color: root.selectedIndex === index
                                          ? "#6C63FF"
                                          : Qt.rgba(1, 1, 1, 0.24)
                            border.width: root.selectedIndex === index ? 1.5 : 1

                            Label {
                                id: chipLabel
                                anchors.centerIn: parent
                                text: modelData.label
                                color: root.selectedIndex === index ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.7)
                                font.pixelSize: 13
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.selectedIndex = index
                            }
                        }
                    }
                }
            }

            Button {
                text: root.isApplying ? "" : "Apply"
                highlighted: true
                enabled: canvasNotifier && canvasNotifier.hasCanvas && !root.isApplying
                onClicked: internal.applyCrop()

                BusyIndicator {
                    anchors.centerIn: parent
                    running: root.isApplying
                    visible: root.isApplying
                    implicitWidth: 20; implicitHeight: 20
                }
            }
        }
    }

    QtObject {
        id: internal

        function applyCrop() {
            if (!canvasNotifier || !canvasNotifier.hasCanvas) return
            root.isApplying = true
            var aspect = root.cropAspects[root.selectedIndex]
            canvasNotifier.applyCrop(aspect.widthRatio, aspect.heightRatio)
            root.isApplying = false
        }
    }
}
