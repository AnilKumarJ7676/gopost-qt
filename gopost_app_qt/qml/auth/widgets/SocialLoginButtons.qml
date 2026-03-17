import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    // authNotifier available via context property

    spacing: 8

    Button {
        id: googleButton
        Layout.fillWidth: true
        Layout.preferredHeight: 48
        flat: true

        contentItem: RowLayout {
            spacing: 8
            Item { Layout.fillWidth: true }

            // Google "G" icon placeholder
            Label {
                text: "G"
                font.pixelSize: 20
                font.weight: Font.Bold
                color: "#4285F4"
            }

            Label {
                text: qsTr("Continue with Google")
                font.pixelSize: 14
            }

            Item { Layout.fillWidth: true }
        }

        background: Rectangle {
            radius: 6
            border.width: 1
            border.color: palette.mid
            color: googleButton.hovered ? palette.alternateBase : "transparent"
        }

        onClicked: {
            // TODO: initiate Google OAuth flow
            if (root.authNotifier) {
                // authNotifier would handle OAuth externally
            }
        }
    }

    Button {
        id: appleButton
        Layout.fillWidth: true
        Layout.preferredHeight: 48
        flat: true

        contentItem: RowLayout {
            spacing: 8
            Item { Layout.fillWidth: true }

            // Apple icon placeholder
            Label {
                text: "\uF8FF"  // Apple logo (platform-dependent)
                font.pixelSize: 20
            }

            Label {
                text: qsTr("Continue with Apple")
                font.pixelSize: 14
            }

            Item { Layout.fillWidth: true }
        }

        background: Rectangle {
            radius: 6
            border.width: 1
            border.color: palette.mid
            color: appleButton.hovered ? palette.alternateBase : "transparent"
        }

        onClicked: {
            // TODO: initiate Apple OAuth flow
            if (root.authNotifier) {
                // authNotifier would handle OAuth externally
            }
        }
    }
}
