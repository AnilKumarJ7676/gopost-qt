import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/// Bottom sheet prompting login for restricted features.
/// Usage:
///   LoginRequiredSheet {
///       id: loginSheet
///       feature: "export your project"
///       onSignInRequested: navigateToLogin()
///   }
///   loginSheet.open()
Popup {
    id: root

    property string feature: ""

    signal signInRequested()
    signal dismissed()

    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    x: (parent.width - width) / 2
    y: parent.height - height
    width: parent.width
    height: contentColumn.implicitHeight + 48

    enter: Transition {
        NumberAnimation { property: "y"; from: root.parent.height; to: root.parent.height - root.height; duration: 300; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "y"; to: root.parent.height; duration: 250; easing.type: Easing.InCubic }
    }

    background: Rectangle {
        color: palette.window
        radius: 20

        // Only round the top corners
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 20
            color: parent.color
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 24
        anchors.topMargin: 12
        spacing: 0

        // Drag handle
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 40
            height: 4
            radius: 2
            color: palette.mid
            opacity: 0.5
        }

        Item { Layout.preferredHeight: 24 }

        // Lock icon circle
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 64
            height: 64
            radius: 32
            color: palette.highlight
            opacity: 0.15

            Label {
                anchors.centerIn: parent
                text: "\uD83D\uDD12"   // lock emoji fallback
                font.pixelSize: 28
            }
        }

        Item { Layout.preferredHeight: 24 }

        Label {
            text: qsTr("Sign In Required")
            font.pixelSize: 22
            font.weight: Font.Bold
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Item { Layout.preferredHeight: 8 }

        Label {
            text: qsTr("To %1, you need to sign in or create an account. It only takes a moment.").arg(root.feature)
            font.pixelSize: 14
            opacity: 0.6
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            lineHeight: 1.5
            Layout.fillWidth: true
        }

        Item { Layout.preferredHeight: 32 }

        Button {
            text: qsTr("Sign In")
            highlighted: true
            Layout.fillWidth: true
            Layout.preferredHeight: 48

            onClicked: {
                root.close()
                root.signInRequested()
            }
        }

        Item { Layout.preferredHeight: 8 }

        Button {
            text: qsTr("Not Now")
            flat: true
            Layout.fillWidth: true
            Layout.preferredHeight: 44

            onClicked: {
                root.close()
                root.dismissed()
            }
        }
    }
}
