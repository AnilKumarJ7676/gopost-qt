import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    signal signInClicked()

    height: visible ? bannerRow.implicitHeight + 16 : 0
    color: "#FDE8F0" // tertiaryContainer light

    RowLayout {
        id: bannerRow
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 8

        Label {
            text: "\u2139" // info icon
            font.pixelSize: 16
            color: "#5E2A3A"
        }

        Label {
            Layout.fillWidth: true
            text: "Browsing as guest. Sign in to export and use templates."
            font.pixelSize: 12
            color: "#5E2A3A"
            wrapMode: Text.Wrap
        }

        Button {
            text: "Sign In"
            flat: true
            font.pixelSize: 12
            font.weight: Font.Bold
            onClicked: root.signInClicked()

            contentItem: Text {
                text: parent.text
                font: parent.font
                color: "#5E2A3A"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
