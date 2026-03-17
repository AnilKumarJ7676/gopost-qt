import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string initialQuery: ""

    signal searchChanged(string query)
    signal searchCleared()

    implicitHeight: 52

    TextField {
        id: searchField
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        placeholderText: qsTr("Search templates...")
        text: root.initialQuery
        font.pixelSize: 14

        leftPadding: 44
        rightPadding: clearButton.visible ? 44 : 16

        background: Rectangle {
            radius: 24
            color: palette.alternateBase
            border.width: 0
        }

        // Search icon
        Label {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: "\ue8b6" // search
            font.pixelSize: 20
            color: palette.placeholderText
        }

        // Clear button
        ToolButton {
            id: clearButton
            visible: searchField.text.length > 0
            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            width: 36; height: 36

            Label {
                anchors.centerIn: parent
                text: "\ue5cd" // close
                font.pixelSize: 20
                color: palette.placeholderText
            }

            onClicked: {
                searchField.text = ""
                root.searchChanged("")
                root.searchCleared()
            }
        }

        onTextChanged: {
            root.searchChanged(text)
        }
    }
}
