import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal navigateToRegister()
    signal navigateToHome()

    // authNotifier is available via context property set by ServiceLocator

    Connections {
        target: authNotifier

        function onStateChanged() {
            // Navigation is handled by C++ ServiceLocator wiring:
            // authNotifier.stateChanged -> router.setCanAccessApp -> navigationRequested
            // No need to call router.go() here — it would race with the C++ slot.
        }

        function onAuthError(message) {
            errorSnackBar.text = message
            errorSnackBar.open()
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: formColumn.implicitHeight + 64
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Item {
            anchors.fill: parent

            ColumnLayout {
                id: formColumn
                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 400)
                spacing: 0

                // --- Header ---
                Label {
                    text: qsTr("Welcome Back")
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Sign in to continue")
                    font.pixelSize: 16
                    opacity: 0.6
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                }

                Item { Layout.preferredHeight: 40 }

                // --- Email field ---
                Label {
                    text: qsTr("Email")
                    font.pixelSize: 14
                    Layout.fillWidth: true
                }

                TextField {
                    id: emailField
                    placeholderText: qsTr("Enter your email")
                    inputMethodHints: Qt.ImhEmailCharactersOnly
                    Layout.fillWidth: true
                    Layout.topMargin: 4

                    property string errorText: ""

                    function validate() {
                        if (text.trim().length === 0) {
                            errorText = qsTr("Email is required")
                            return false
                        }
                        var emailRegex = /^[^@]+@[^@]+\.[^@]+$/
                        if (!emailRegex.test(text.trim())) {
                            errorText = qsTr("Enter a valid email")
                            return false
                        }
                        errorText = ""
                        return true
                    }
                }

                Label {
                    text: emailField.errorText
                    color: "#F44336"
                    font.pixelSize: 12
                    visible: emailField.errorText.length > 0
                    Layout.fillWidth: true
                    Layout.topMargin: 2
                }

                Item { Layout.preferredHeight: 16 }

                // --- Password field ---
                Label {
                    text: qsTr("Password")
                    font.pixelSize: 14
                    Layout.fillWidth: true
                }

                TextField {
                    id: passwordField
                    placeholderText: qsTr("Enter your password")
                    echoMode: showPasswordButton.checked ? TextInput.Normal : TextInput.Password
                    Layout.fillWidth: true
                    Layout.topMargin: 4

                    property string errorText: ""

                    function validate() {
                        if (text.length === 0) {
                            errorText = qsTr("Password is required")
                            return false
                        }
                        if (text.length < 8) {
                            errorText = qsTr("Password must be at least 8 characters")
                            return false
                        }
                        errorText = ""
                        return true
                    }

                    rightPadding: showPasswordButton.width + 8

                    ToolButton {
                        id: showPasswordButton
                        checkable: true
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        icon.name: checked ? "visibility-off" : "visibility"
                        text: checked ? qsTr("Hide") : qsTr("Show")
                        display: AbstractButton.IconOnly
                        width: 40
                        height: 40
                    }
                }

                Label {
                    text: passwordField.errorText
                    color: "#F44336"
                    font.pixelSize: 12
                    visible: passwordField.errorText.length > 0
                    Layout.fillWidth: true
                    Layout.topMargin: 2
                }

                // --- Forgot Password ---
                Label {
                    text: qsTr("Forgot Password?")
                    font.pixelSize: 14
                    color: palette.highlight
                    horizontalAlignment: Text.AlignRight
                    Layout.fillWidth: true
                    Layout.topMargin: 8

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // TODO: navigate to forgot password
                        }
                    }
                }

                Item { Layout.preferredHeight: 16 }

                // --- Sign In button ---
                Button {
                    id: signInButton
                    text: authNotifier.isLoading ? "" : qsTr("Sign In")
                    enabled: !authNotifier.isLoading
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    highlighted: true

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: authNotifier.isLoading
                        visible: authNotifier.isLoading
                        width: 24
                        height: 24
                    }

                    onClicked: {
                        var emailOk = emailField.validate()
                        var passOk = passwordField.validate()
                        if (emailOk && passOk) {
                            authNotifier.login(emailField.text.trim(), passwordField.text)
                        }
                    }
                }

                Item { Layout.preferredHeight: 24 }

                // --- OR divider ---
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: palette.mid
                    }

                    Label {
                        text: qsTr("OR")
                        font.pixelSize: 13
                        opacity: 0.6
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: palette.mid
                    }
                }

                Item { Layout.preferredHeight: 24 }

                // --- Social login buttons ---
                Loader {
                    Layout.fillWidth: true
                    source: "qrc:/qt/qml/GopostApp/qml/auth/widgets/SocialLoginButtons.qml"
                }

                Item { Layout.preferredHeight: 24 }

                // --- Skip & Explore ---
                Button {
                    text: qsTr("Skip & Explore")
                    flat: true
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    icon.name: "explore"

                    onClicked: {
                        console.log("SKIP BUTTON CLICKED")
                        console.log("authNotifier exists: " + (authNotifier !== null && authNotifier !== undefined))
                        authNotifier.skipAuth()
                        console.log("skipAuth() called, canAccessApp=" + authNotifier.canAccessApp)
                    }
                }

                Label {
                    text: qsTr("You can browse, but export and templates require sign-in")
                    font.pixelSize: 12
                    opacity: 0.4
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                }

                Item { Layout.preferredHeight: 24 }

                // --- Sign Up link ---
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 4

                    Label {
                        text: qsTr("Don't have an account?")
                        font.pixelSize: 14
                    }

                    Label {
                        text: qsTr("Sign Up")
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: palette.highlight

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: router.go("/auth/register")
                        }
                    }
                }
            }
        }
    }

    // --- Error SnackBar ---
    Popup {
        id: errorSnackBar
        property string text: ""

        x: (parent.width - width) / 2
        y: parent.height - height - 24
        width: Math.min(parent.width - 32, 400)
        height: snackBarLabel.implicitHeight + 24
        modal: false
        closePolicy: Popup.CloseOnPressOutside

        function open() {
            visible = true
            snackBarTimer.restart()
        }

        background: Rectangle {
            color: "#F44336"
            radius: 8
        }

        Label {
            id: snackBarLabel
            text: errorSnackBar.text
            color: "white"
            wrapMode: Text.WordWrap
            anchors.centerIn: parent
            width: parent.width - 24
        }

        Timer {
            id: snackBarTimer
            interval: 4000
            onTriggered: errorSnackBar.visible = false
        }
    }
}
