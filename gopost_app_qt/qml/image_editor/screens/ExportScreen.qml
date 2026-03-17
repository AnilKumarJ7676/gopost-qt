import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Full-screen export dialog for the GoPost image editor.
/// Format picker (JPEG/PNG/WebP), quality slider, resolution dropdown, DPI selector.
Item {
    id: root

    signal back()

    // Local state
    property int selectedFormat: 0   // 0=JPEG, 1=PNG, 2=WebP
    property int quality: 85
    property int selectedResolution: 0 // 0=Original, 1=4K, 2=1080p, 3=720p, 4=InstaSquare, 5=InstaStory
    property double dpi: 72
    property bool exporting: false
    property double progress: 0
    property bool complete: false
    property string exportedPath: ""
    property int estimatedSize: 0
    property string exportError: ""

    readonly property bool showQualitySlider: selectedFormat === 0 || selectedFormat === 2
    readonly property var formatLabels: ["JPEG", "PNG", "WebP"]
    readonly property var resolutionLabels: ["Original", "4K", "1080p", "720p", "Instagram Square", "Instagram Story"]
    readonly property var dpiOptions: [72, 150, 300]

    function formatFileSize(bytes) {
        if (bytes >= 1024 * 1024)
            return "~" + (bytes / (1024 * 1024)).toFixed(1) + " MB"
        return "~" + Math.round(bytes / 1024) + " KB"
    }

    function estimateFileSize() {
        var base = selectedFormat === 1 ? 2048 : 512
        var qualityFactor = showQualitySlider ? (quality / 100.0) : 1.0
        return Math.round(base * qualityFactor) * 1024
    }

    Rectangle {
        anchors.fill: parent
        color: "#121218"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // App bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: "#1E1E2E"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8

                    ToolButton {
                        icon.name: "go-previous"
                        icon.color: "#E0E0E0"
                        onClicked: root.back()
                    }

                    Label {
                        text: "Export Image"
                        color: "#E0E0E0"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                    }
                }
            }

            // Content
            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: root.complete ? successView : exportForm
            }
        }
    }

    // --- Export Form ---
    Component {
        id: exportForm

        Flickable {
            contentHeight: formColumn.height + 32
            clip: true

            ColumnLayout {
                id: formColumn
                anchors {
                    left: parent.left; right: parent.right
                    margins: 16; top: parent.top; topMargin: 16
                }
                spacing: 24

                // Format picker
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: root.formatLabels
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 56
                            color: "#1E1E2E"
                            radius: 8
                            border.color: root.selectedFormat === index ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                            border.width: root.selectedFormat === index ? 2 : 1

                            Label {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.selectedFormat === index ? "#6C63FF" : "#E0E0E0"
                                font.pixelSize: 16
                                font.weight: root.selectedFormat === index ? Font.DemiBold : Font.Medium
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.selectedFormat = index
                            }
                        }
                    }
                }

                // Quality slider
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.showQualitySlider
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Quality"; color: "#E0E0E0"; font.pixelSize: 14 }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: root.quality + "%"
                            color: "#6C63FF"; font.pixelSize: 14; font.weight: Font.Medium
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 1; to: 100; value: root.quality
                        onMoved: root.quality = Math.round(value)
                    }
                }

                // Resolution dropdown
                ComboBox {
                    Layout.fillWidth: true
                    model: root.resolutionLabels
                    currentIndex: root.selectedResolution
                    onCurrentIndexChanged: root.selectedResolution = currentIndex
                }

                // Estimated size
                Label {
                    text: root.formatFileSize(root.estimatedSize > 0 ? root.estimatedSize : root.estimateFileSize())
                    color: Qt.rgba(1, 1, 1, 0.54)
                    font.pixelSize: 13
                }

                // DPI selector
                RowLayout {
                    spacing: 8
                    Repeater {
                        model: root.dpiOptions
                        delegate: Button {
                            text: modelData + " DPI"
                            highlighted: root.dpi === modelData
                            onClicked: root.dpi = modelData
                        }
                    }
                }

                // Error display
                Label {
                    Layout.fillWidth: true
                    visible: root.exportError !== ""
                    text: root.exportError
                    color: "#CF6679"
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                }

                // Export button / progress
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ProgressBar {
                        Layout.fillWidth: true
                        visible: root.exporting
                        value: root.progress
                    }

                    Button {
                        Layout.fillWidth: true
                        text: root.exporting ? "Cancel" : "Export"
                        highlighted: !root.exporting
                        onClicked: {
                            if (root.exporting) {
                                root.exporting = false
                                root.progress = 0
                            } else {
                                root.exporting = true
                                root.progress = 0
                                root.exportError = ""
                                // Export would be triggered here via engine
                            }
                        }
                    }
                }
            }
        }
    }

    // --- Success View ---
    Component {
        id: successView

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 24
            width: parent.width - 32

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "\u2714"
                font.pixelSize: 64
                color: "#66BB6A"
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "Export complete"
                color: "#E0E0E0"
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: root.formatFileSize(root.estimatedSize)
                color: Qt.rgba(1, 1, 1, 0.54)
                font.pixelSize: 14
            }

            Button {
                Layout.fillWidth: true
                text: "Share"
                highlighted: true
                onClicked: root.back()
            }

            Button {
                Layout.fillWidth: true
                text: "Done"
                flat: true
                onClicked: root.back()
            }
        }
    }
}
