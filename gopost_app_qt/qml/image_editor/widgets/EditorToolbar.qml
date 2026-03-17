import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Bottom toolbar with tool icons.
/// Horizontal tool buttons: layers, addImage, addText, sticker, filter, adjust, crop, draw, export.
/// NeonGlow effect on selected tool.
Rectangle {
    id: root

    readonly property int activeTool: toolNotifier ? toolNotifier.activeTool : 0

    height: 64
    color: "#1E1E2E"

    Rectangle {
        anchors.top: parent.top
        width: parent.width; height: 0.5
        color: Qt.rgba(1, 1, 1, 0.1)
    }

    ListView {
        anchors.fill: parent
        anchors.leftMargin: 8; anchors.rightMargin: 8
        orientation: ListView.Horizontal
        spacing: 0
        clip: true

        model: ListModel {
            ListElement { toolId: 2;  label: "Layers";  iconText: "\u25A3" }
            ListElement { toolId: 3;  label: "Image";   iconText: "\uD83D\uDDBC" }
            ListElement { toolId: 4;  label: "Text";    iconText: "T" }
            ListElement { toolId: 6;  label: "Sticker"; iconText: "\uD83D\uDE00" }
            ListElement { toolId: 7;  label: "Filter";  iconText: "\u2728" }
            ListElement { toolId: 8;  label: "Adjust";  iconText: "\u2699" }
            ListElement { toolId: 9;  label: "Crop";    iconText: "\u2702" }
            ListElement { toolId: 11; label: "Draw";    iconText: "\uD83D\uDD8C" }
            ListElement { toolId: 13; label: "Export";  iconText: "\uD83D\uDCBE" }
        }

        delegate: Item {
            width: 60
            height: root.height

            // NeonGlow effect for active tool
            Rectangle {
                anchors.fill: parent
                anchors.margins: 4
                radius: 8
                color: root.activeTool === model.toolId ? Qt.rgba(0.42, 0.39, 1.0, 0.12) : "transparent"
                border.color: root.activeTool === model.toolId ? Qt.rgba(0.42, 0.39, 1.0, 0.4) : "transparent"
                border.width: root.activeTool === model.toolId ? 1 : 0

                // Glow layer
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    radius: parent.radius + 2
                    color: "transparent"
                    border.color: root.activeTool === model.toolId ? Qt.rgba(0.42, 0.39, 1.0, 0.2) : "transparent"
                    border.width: root.activeTool === model.toolId ? 2 : 0
                    visible: root.activeTool === model.toolId
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 3

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: model.iconText
                    font.pixelSize: 20
                    color: root.activeTool === model.toolId ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.6)
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: model.label
                    font.pixelSize: 10
                    font.weight: root.activeTool === model.toolId ? Font.DemiBold : Font.Normal
                    color: root.activeTool === model.toolId ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.54)
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (toolNotifier) toolNotifier.selectTool(model.toolId)
                }
            }
        }
    }
}
