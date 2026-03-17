import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Layer panel displayed as a side sheet.
/// Header with "Layers" + Add button. Reorderable ListView of layer tiles.
/// Visibility toggle, context menu (duplicate/remove).
Rectangle {
    id: root

    color: "#1E1E2E"

    Rectangle {
        anchors.left: parent.left
        width: 0.5; height: parent.height
        color: Qt.rgba(1, 1, 1, 0.1)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12; Layout.rightMargin: 12
            Layout.topMargin: 8; Layout.bottomMargin: 8

            Label {
                text: "Layers"
                color: "white"
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                icon.name: "list-add"
                icon.color: Qt.rgba(1, 1, 1, 0.7)
                ToolTip.text: "Add layer"
                ToolTip.visible: hovered
                implicitWidth: 28; implicitHeight: 28
                onClicked: {
                    if (canvasNotifier) canvasNotifier.addSolidLayer(1, 1, 1, 1)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 1
            color: Qt.rgba(1, 1, 1, 0.1)
        }

        // Layer list or empty state
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Empty state
            Label {
                anchors.centerIn: parent
                visible: !canvasNotifier || canvasNotifier.layerCount === 0
                text: "No layers yet.\nImport an image to get started."
                color: Qt.rgba(1, 1, 1, 0.38)
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                width: parent.width - 48
            }

            // Layer list
            ListView {
                id: layerListView
                anchors.fill: parent
                visible: canvasNotifier && canvasNotifier.layerCount > 0
                model: canvasNotifier ? canvasNotifier.layerList() : []
                clip: true
                spacing: 0

                // Show layers in reverse order (top layer first)
                delegate: Item {
                    id: layerDelegate
                    width: layerListView.width
                    height: layerContent.height

                    readonly property var layerData: modelData
                    readonly property bool isSelected: canvasNotifier && layerData.id === canvasNotifier.selectedLayerId
                    property bool expanded: false

                    Rectangle {
                        id: layerContent
                        width: parent.width - 8
                        height: mainRow.height + (layerDelegate.expanded && layerDelegate.isSelected ? detailsCol.height : 0)
                        x: 4
                        radius: 6
                        color: layerDelegate.isSelected ? Qt.rgba(0.42, 0.39, 1.0, 0.15) : "transparent"
                        border.color: layerDelegate.isSelected ? Qt.rgba(0.42, 0.39, 1.0, 0.6) : "transparent"
                        border.width: layerDelegate.isSelected ? 1 : 0

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // Main row
                            RowLayout {
                                id: mainRow
                                Layout.fillWidth: true
                                Layout.preferredHeight: 52
                                Layout.leftMargin: 8; Layout.rightMargin: 8
                                spacing: 4

                                // Visibility toggle
                                Label {
                                    text: layerData.visible ? "\uD83D\uDC41" : "\u2014"
                                    font.pixelSize: 14
                                    color: layerData.visible ? Qt.rgba(1, 1, 1, 0.6) : Qt.rgba(1, 1, 1, 0.24)

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (canvasNotifier)
                                                canvasNotifier.setLayerVisible(layerData.id, !layerData.visible)
                                        }
                                    }
                                }

                                // Type icon
                                Rectangle {
                                    width: 32; height: 32; radius: 4
                                    color: Qt.rgba(1, 1, 1, 0.1)
                                    Label {
                                        anchors.centerIn: parent
                                        text: internal.layerTypeIcon(layerData.type)
                                        font.pixelSize: 14; color: Qt.rgba(1, 1, 1, 0.54)
                                    }
                                }

                                // Name and opacity
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0

                                    Label {
                                        text: layerData.name || ("Layer " + layerData.id)
                                        color: layerData.visible ? "white" : Qt.rgba(1, 1, 1, 0.38)
                                        font.pixelSize: 12; font.weight: Font.Medium
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        visible: layerData.opacity < 1.0
                                        text: Math.round(layerData.opacity * 100) + "%"
                                        color: Qt.rgba(1, 1, 1, 0.3)
                                        font.pixelSize: 10
                                    }
                                }

                                // Expand toggle
                                Label {
                                    visible: layerDelegate.isSelected
                                    text: layerDelegate.expanded ? "\u25B2" : "\u25BC"
                                    font.pixelSize: 10; color: Qt.rgba(1, 1, 1, 0.54)
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: layerDelegate.expanded = !layerDelegate.expanded
                                    }
                                }

                                // Delete button
                                Label {
                                    text: "\u2715"
                                    font.pixelSize: 12; color: Qt.rgba(1, 1, 1, 0.24)
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (canvasNotifier)
                                                canvasNotifier.removeLayer(layerData.id)
                                        }
                                    }
                                }
                            }

                            // Details (expanded)
                            ColumnLayout {
                                id: detailsCol
                                Layout.fillWidth: true
                                Layout.leftMargin: 8; Layout.rightMargin: 8; Layout.bottomMargin: 8
                                visible: layerDelegate.expanded && layerDelegate.isSelected
                                spacing: 4

                                Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }

                                // Opacity slider
                                RowLayout {
                                    Label { text: "Opacity"; color: Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 11 }
                                    Slider {
                                        Layout.fillWidth: true
                                        from: 0; to: 1; value: layerData.opacity
                                        onMoved: {
                                            if (canvasNotifier)
                                                canvasNotifier.setLayerOpacity(layerData.id, value)
                                        }
                                    }
                                    Label {
                                        text: Math.round(layerData.opacity * 100)
                                        color: Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 10
                                        Layout.preferredWidth: 32
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }

                                // Blend mode
                                RowLayout {
                                    Label { text: "Blend"; color: Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 11 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Normal", "Multiply", "Screen", "Overlay"]
                                        currentIndex: layerData.blendMode
                                        onCurrentIndexChanged: {
                                            if (canvasNotifier)
                                                canvasNotifier.setLayerBlendMode(layerData.id, currentIndex)
                                        }
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            onClicked: {
                                if (canvasNotifier)
                                    canvasNotifier.selectLayer(layerData.id)
                            }
                        }
                    }
                }
            }
        }
    }

    QtObject {
        id: internal
        function layerTypeIcon(type) {
            switch (type) {
            case 0: return "\uD83D\uDDBC"  // Image
            case 1: return "\u25A0"          // SolidColor
            case 2: return "T"               // Text
            case 3: return "\u25CF"          // Shape
            case 4: return "\uD83D\uDCC1"   // Group
            case 5: return "\u2699"          // Adjustment
            case 6: return "\u25B2"          // Gradient
            case 7: return "\uD83D\uDE00"   // Sticker
            default: return "?"
            }
        }
    }
}
