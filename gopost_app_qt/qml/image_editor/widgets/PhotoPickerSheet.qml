import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import GopostApp 1.0

/// Photo picker popup with gallery and multi-select options.
/// Displayed as a Popup (replaces Flutter showModalBottomSheet).
Popup {
    id: root

    signal imageSelected(string path)

    anchors.centerIn: Overlay.overlay
    width: Math.min(parent.width - 32, 400)
    modal: true
    dim: true
    padding: 0

    property bool isDecoding: false
    property string errorMessage: ""

    background: Rectangle {
        color: "#1E1E2E"
        radius: 16
    }

    contentItem: ColumnLayout {
        spacing: 0

        // Drag handle
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            Rectangle {
                anchors.centerIn: parent
                width: 36; height: 4; radius: 2
                color: Qt.rgba(1, 1, 1, 0.24)
            }
        }

        // Title
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Import Image"
            color: "white"
            font.pixelSize: 18; font.weight: Font.DemiBold
        }

        Item { Layout.preferredHeight: 24 }

        // Decoding state
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 64
            running: root.isDecoding
            visible: root.isDecoding
        }

        // Import options
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16; Layout.rightMargin: 16
            visible: !root.isDecoding
            spacing: 0

            // Open File
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 64; height: 64; radius: 16
                        color: Qt.rgba(0.42, 0.39, 1.0, 0.15)

                        Label {
                            anchors.centerIn: parent
                            text: "\uD83D\uDCC2"
                            font.pixelSize: 28
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Open File"
                        color: Qt.rgba(1, 1, 1, 0.7)
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: fileDialog.open()
                }
            }

            // Multiple
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 64; height: 64; radius: 16
                        color: Qt.rgba(0.42, 0.39, 1.0, 0.15)

                        Label {
                            anchors.centerIn: parent
                            text: "\uD83D\uDDBC"
                            font.pixelSize: 28
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Multiple"
                        color: Qt.rgba(1, 1, 1, 0.7)
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: multiFileDialog.open()
                }
            }
        }

        // Error display
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 16; Layout.rightMargin: 16
            visible: root.errorMessage !== ""
            text: root.errorMessage
            color: "#CF6679"
            font.pixelSize: 13
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }

        Item { Layout.preferredHeight: 24 }
    }

    FileDialog {
        id: fileDialog
        title: "Select Image"
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.webp *.bmp)"]
        onAccepted: {
            root.imageSelected(selectedFile.toString().replace("file:///", ""))
            root.close()
        }
    }

    FileDialog {
        id: multiFileDialog
        title: "Select Images"
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.webp *.bmp)"]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            for (var i = 0; i < selectedFiles.length; ++i) {
                root.imageSelected(selectedFiles[i].toString().replace("file:///", ""))
            }
            root.close()
        }
    }
}
