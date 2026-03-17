import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var categories: []
    property string selectedCategoryId: ""

    signal categorySelected(string id)

    implicitHeight: 40

    ListView {
        id: chipList
        anchors.fill: parent
        orientation: ListView.Horizontal
        spacing: 8
        leftMargin: 16
        rightMargin: 16
        clip: true

        model: {
            var items = [{ id: "", name: "All" }]
            if (categories) {
                for (var i = 0; i < categories.length; i++) {
                    items.push(categories[i])
                }
            }
            return items
        }

        delegate: Rectangle {
            required property var modelData
            required property int index

            property bool isSelected: (modelData.id === "" && root.selectedCategoryId === "")
                || modelData.id === root.selectedCategoryId

            width: chipLabel.implicitWidth + 32
            height: 36
            radius: height / 2
            color: isSelected ? palette.highlight : palette.alternateBase

            Behavior on color { ColorAnimation { duration: 200 } }

            Label {
                id: chipLabel
                anchors.centerIn: parent
                text: modelData.name || ""
                font.pixelSize: 13
                font.weight: isSelected ? Font.DemiBold : Font.Medium
                color: isSelected ? "white" : palette.placeholderText
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (modelData.id === root.selectedCategoryId) {
                        root.categorySelected("")
                    } else {
                        root.categorySelected(modelData.id)
                    }
                }
            }
        }
    }
}
