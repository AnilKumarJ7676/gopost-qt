import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Panel for mask editing on the selected layer.
/// Mask toggle, paint/erase mode, brush size, hardness, opacity, action buttons, preview mode.
Rectangle {
    id: root

    property bool hasMask: false
    property real brushSize: 20
    property real hardness: 0.8
    property bool isPaintMode: true
    property real brushOpacity: 1.0
    property bool redPreview: true

    height: 300
    color: "#1E1E2E"

    Rectangle {
        anchors.top: parent.top
        width: parent.width; height: 0.5
        color: Qt.rgba(1, 1, 1, 0.1)
    }

    Label {
        anchors.centerIn: parent
        visible: !canvasNotifier || canvasNotifier.selectedLayerId < 0
        text: "Select a layer to edit mask"
        color: Qt.rgba(1, 1, 1, 0.54)
        font.pixelSize: 14
    }

    Flickable {
        anchors.fill: parent
        contentHeight: contentCol.height + 24
        clip: true
        visible: canvasNotifier && canvasNotifier.selectedLayerId >= 0

        ColumnLayout {
            id: contentCol
            anchors {
                left: parent.left; right: parent.right
                margins: 12; top: parent.top; topMargin: 12
            }
            spacing: 16

            // Mask toggle
            RowLayout {
                Layout.fillWidth: true
                Label { text: "Mask"; color: "#E0E0E0"; font.pixelSize: 14; font.weight: Font.Medium }
                Item { Layout.preferredWidth: 12 }
                Switch { checked: root.hasMask; onToggled: root.hasMask = checked }
            }

            // Mask controls
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.hasMask
                spacing: 12

                // Paint / Erase mode
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Label { text: "Mode"; color: "#E0E0E0"; font.pixelSize: 13 }

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 36; radius: 8
                        color: root.isPaintMode ? Qt.rgba(0.42, 0.39, 1.0, 0.2) : Qt.rgba(1, 1, 1, 0.06)
                        border.color: root.isPaintMode ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                        Label { anchors.centerIn: parent; text: "Paint"; color: root.isPaintMode ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 13 }
                        MouseArea { anchors.fill: parent; onClicked: root.isPaintMode = true }
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 36; radius: 8
                        color: !root.isPaintMode ? Qt.rgba(0.42, 0.39, 1.0, 0.2) : Qt.rgba(1, 1, 1, 0.06)
                        border.color: !root.isPaintMode ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                        Label { anchors.centerIn: parent; text: "Erase"; color: !root.isPaintMode ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 13 }
                        MouseArea { anchors.fill: parent; onClicked: root.isPaintMode = false }
                    }
                }

                // Brush size
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Brush size"; color: "#E0E0E0"; font.pixelSize: 13 }
                        Item { Layout.fillWidth: true }
                        Label { text: Math.round(root.brushSize); color: "#6C63FF"; font.pixelSize: 12; font.weight: Font.Medium }
                    }
                    Slider { Layout.fillWidth: true; from: 1; to: 100; value: root.brushSize; onMoved: root.brushSize = value }
                }

                // Hardness
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Hardness"; color: "#E0E0E0"; font.pixelSize: 13 }
                        Item { Layout.fillWidth: true }
                        Label { text: Math.round(root.hardness * 100); color: "#6C63FF"; font.pixelSize: 12; font.weight: Font.Medium }
                    }
                    Slider { Layout.fillWidth: true; from: 0; to: 1; value: root.hardness; onMoved: root.hardness = value }
                }

                // Opacity
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Opacity"; color: "#E0E0E0"; font.pixelSize: 13 }
                        Item { Layout.fillWidth: true }
                        Label { text: Math.round(root.brushOpacity * 100); color: "#6C63FF"; font.pixelSize: 12; font.weight: Font.Medium }
                    }
                    Slider { Layout.fillWidth: true; from: 0; to: 1; value: root.brushOpacity; onMoved: root.brushOpacity = value }
                }

                // Action buttons
                Flow {
                    Layout.fillWidth: true; spacing: 8
                    Button { text: "Invert Mask"; flat: true }
                    Button { text: "Fill White"; flat: true }
                    Button { text: "Fill Black"; flat: true }
                }

                // Preview mode
                RowLayout {
                    spacing: 12
                    Label { text: "Preview"; color: "#E0E0E0"; font.pixelSize: 13 }
                    Rectangle {
                        width: 60; height: 32; radius: 8
                        color: root.redPreview ? Qt.rgba(0.42, 0.39, 1.0, 0.2) : Qt.rgba(1, 1, 1, 0.06)
                        border.color: root.redPreview ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                        Label { anchors.centerIn: parent; text: "Red"; color: root.redPreview ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 13 }
                        MouseArea { anchors.fill: parent; onClicked: root.redPreview = true }
                    }
                    Rectangle {
                        width: 60; height: 32; radius: 8
                        color: !root.redPreview ? Qt.rgba(0.42, 0.39, 1.0, 0.2) : Qt.rgba(1, 1, 1, 0.06)
                        border.color: !root.redPreview ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                        Label { anchors.centerIn: parent; text: "Black"; color: !root.redPreview ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 13 }
                        MouseArea { anchors.fill: parent; onClicked: root.redPreview = false }
                    }
                }
            }
        }
    }
}
