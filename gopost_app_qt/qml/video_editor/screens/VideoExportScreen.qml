import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Video export screen with preset picker, progress, and result views.
Page {
    id: root

    signal back()

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8; anchors.rightMargin: 8

            ToolButton {
                icon.name: "window-close"
                enabled: !exportNotifier.isExporting
                onClicked: router.pop()
            }
            Label {
                text: "Export Video"
                font.pixelSize: 18
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: exportNotifier.isExporting ? 1 : (exportNotifier.hasResult ? 2 : 0)

        // --- Preset picker ---
        ColumnLayout {
            spacing: 0

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 8

                header: ColumnLayout {
                    width: parent ? parent.width : 0
                    spacing: 4

                    Item { height: 16 }

                    Label {
                        text: "Choose Export Preset"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        leftPadding: 16
                    }
                    Label {
                        text: "Select a platform-optimized preset or customize your export settings."
                        font.pixelSize: 13
                        opacity: 0.6
                        leftPadding: 16
                    }

                    Item { height: 8 }
                }

                model: exportNotifier.presets
                delegate: Rectangle {
                    width: ListView.view.width - 32
                    x: 16
                    height: 64
                    radius: 12
                    color: Theme.editorSurface
                    border.color: model.isSelected ? "#6C63FF" : "#252540"
                    border.width: model.isSelected ? 2 : 0.5

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: exportNotifier.selectPreset(model.id)
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 12

                        Rectangle {
                            width: 40; height: 40
                            radius: 8
                            color: model.isSelected ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "#20203A"

                            Label {
                                anchors.centerIn: parent
                                text: model.iconChar || "\uD83C\uDFA5"
                                font.pixelSize: 18
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: model.name
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }
                            Label {
                                text: model.width + "x" + model.height + " / " + model.videoCodec + " / " + model.bitrate + " Mbps"
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        Label {
                            visible: model.isSelected
                            text: "\u2713"
                            font.pixelSize: 20
                            color: "#6C63FF"
                        }
                    }
                }
            }

            // Export footer
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: footerCol.implicitHeight + 32
                color: "#1A1A34"

                Rectangle {
                    anchors.top: parent.top
                    width: parent.width; height: 0.5
                    color: "#252540"
                }

                ColumnLayout {
                    id: footerCol
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: exportNotifier.presetInfo
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: "Est. size: " + exportNotifier.estimatedSize
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                    }

                    Label {
                        visible: exportNotifier.error.length > 0
                        text: exportNotifier.error
                        color: Theme.semanticError
                        font.pixelSize: 12
                    }

                    Item { height: 8 }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        text: "Start Export"
                        font.pixelSize: 15
                        font.weight: Font.DemiBold

                        background: Rectangle {
                            radius: 8
                            color: "#6C63FF"
                        }
                        contentItem: Label {
                            text: "\u2913  Start Export"
                            color: "white"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: exportNotifier.startExport()
                    }
                }
            }
        }

        // --- Progress view ---
        Item {
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24
                width: 240

                // Circular progress
                Item {
                    Layout.alignment: Qt.AlignHCenter
                    width: 120; height: 120

                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()
                            ctx.lineWidth = 6
                            ctx.strokeStyle = "#20203A"
                            ctx.beginPath()
                            ctx.arc(60, 60, 54, 0, 2 * Math.PI)
                            ctx.stroke()

                            ctx.strokeStyle = "#6C63FF"
                            ctx.beginPath()
                            ctx.arc(60, 60, 54, -Math.PI / 2, -Math.PI / 2 + (exportNotifier.progressPercent / 100) * 2 * Math.PI)
                            ctx.stroke()
                        }

                        Connections {
                            target: exportNotifier
                            function onProgressPercentChanged() { parent.requestPaint() }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: Math.round(exportNotifier.progressPercent) + "%"
                        font.pixelSize: 24
                        font.weight: Font.Bold
                    }
                }

                Label {
                    text: exportNotifier.phaseLabel
                    font.pixelSize: 16
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    visible: exportNotifier.estimatedRemaining.length > 0
                    text: "About " + exportNotifier.estimatedRemaining + " remaining"
                    font.pixelSize: 13
                    opacity: 0.6
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Elapsed: " + exportNotifier.elapsed
                    font.pixelSize: 13
                    opacity: 0.6
                    Layout.alignment: Qt.AlignHCenter
                }

                Button {
                    text: "Cancel"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: exportNotifier.cancelExport()

                    background: Rectangle {
                        radius: 8
                        color: "transparent"
                        border.color: "#6C63FF"
                    }
                }
            }
        }

        // --- Result view ---
        Item {
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 16
                width: 300

                Label {
                    text: "\u2713"
                    font.pixelSize: 72
                    color: "#6C63FF"
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Export Complete!"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }

                // Info rows
                Repeater {
                    model: exportNotifier.resultInfo

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: modelData.label
                            font.pixelSize: 13
                            opacity: 0.6
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: modelData.value
                            font.pixelSize: 13
                            font.weight: Font.Medium
                        }
                    }
                }

                // File path
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: 8
                    color: "#20203A"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: "\uD83D\uDCC1"
                            font.pixelSize: 16
                        }
                        Label {
                            text: exportNotifier.resultFileName
                            font.pixelSize: 13
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }
                }

                Item { height: 8 }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 12

                    Button {
                        text: "Done"
                        onClicked: {
                            exportNotifier.reset()
                            root.back()
                        }
                        background: Rectangle {
                            radius: 8
                            color: "transparent"
                            border.color: "#6C63FF"
                        }
                    }

                    Button {
                        text: "Share"
                        onClicked: exportNotifier.shareResult()

                        background: Rectangle {
                            radius: 8
                            color: "#6C63FF"
                        }
                        contentItem: Label {
                            text: "\u2197  Share"
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }
    }
}
