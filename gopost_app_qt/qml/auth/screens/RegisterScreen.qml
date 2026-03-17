import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal navigateToLogin()
    signal navigateToHome()

    // authNotifier available via context property

    Connections {
        target: authNotifier

        function onStateChanged() {
            // Navigation handled by C++ ServiceLocator wiring
        }

        function onAuthError(message) {
            errorSnackBar.text = message
            errorSnackBar.open()
        }
    }

    function passwordStrength(password) {
        if (password.length === 0) return 0.0
        var strength = 0.0
        if (password.length >= 8) strength += 0.25
        if (password.length >= 12) strength += 0.15
        if (/[A-Z]/.test(password)) strength += 0.2
        if (/[0-9]/.test(password)) strength += 0.2
        if (/[!@#$%^&*(),.?":{}|<>]/.test(password)) strength += 0.2
        return Math.min(strength, 1.0)
    }

    function strengthColor(strength) {
        if (strength < 0.3) return "#F44336"   // red / error
        if (strength < 0.6) return "#FF9800"   // orange / warning
        return "#4CAF50"                        // green / success
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
                    text: qsTr("Create Account")
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Start creating amazing content")
                    font.pixelSize: 16
                    opacity: 0.6
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                }

                Item { Layout.preferredHeight: 40 }

                // --- Full Name ---
                Label {
                    text: qsTr("Full Name")
                    font.pixelSize: 14
                    Layout.fillWidth: true
                }

                TextField {
                    id: nameField
                    placeholderText: qsTr("Enter your full name")
                    Layout.fillWidth: true
                    Layout.topMargin: 4

                    property string errorText: ""

                    function validate() {
                        if (text.trim().length === 0) {
                            errorText = qsTr("Name is required")
                            return false
                        }
                        errorText = ""
                        return true
                    }
                }

                Label {
                    text: nameField.errorText
                    color: "#F44336"
                    font.pixelSize: 12
                    visible: nameField.errorText.length > 0
                    Layout.fillWidth: true
                    Layout.topMargin: 2
                }

                Item { Layout.preferredHeight: 16 }

                // --- Email ---
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

                // --- Password ---
                Label {
                    text: qsTr("Password")
                    font.pixelSize: 14
                    Layout.fillWidth: true
                }

                TextField {
                    id: passwordField
                    placeholderText: qsTr("Enter your password")
                    echoMode: showPassword1.checked ? TextInput.Normal : TextInput.Password
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

                    rightPadding: showPassword1.width + 8

                    ToolButton {
                        id: showPassword1
                        checkable: true
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        icon.name: checked ? "visibility-off" : "visibility"
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

                // --- Password strength indicator ---
                ProgressBar {
                    id: strengthBar
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    visible: passwordField.text.length > 0
                    from: 0
                    to: 1.0
                    value: root.passwordStrength(passwordField.text)

                    background: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 4
                        radius: 2
                        color: palette.mid
                    }

                    contentItem: Item {
                        implicitWidth: 200
                        implicitHeight: 4

                        Rectangle {
                            width: strengthBar.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: root.strengthColor(strengthBar.value)

                            Behavior on width {
                                NumberAnimation { duration: 200 }
                            }

                            Behavior on color {
                                ColorAnimation { duration: 200 }
                            }
                        }
                    }
                }

                Item { Layout.preferredHeight: 16 }

                // --- Confirm Password ---
                Label {
                    text: qsTr("Confirm Password")
                    font.pixelSize: 14
                    Layout.fillWidth: true
                }

                TextField {
                    id: confirmPasswordField
                    placeholderText: qsTr("Confirm your password")
                    echoMode: showPassword2.checked ? TextInput.Normal : TextInput.Password
                    Layout.fillWidth: true
                    Layout.topMargin: 4

                    property string errorText: ""

                    function validate() {
                        if (text !== passwordField.text) {
                            errorText = qsTr("Passwords do not match")
                            return false
                        }
                        errorText = ""
                        return true
                    }

                    rightPadding: showPassword2.width + 8

                    ToolButton {
                        id: showPassword2
                        checkable: true
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        icon.name: checked ? "visibility-off" : "visibility"
                        display: AbstractButton.IconOnly
                        width: 40
                        height: 40
                    }
                }

                Label {
                    text: confirmPasswordField.errorText
                    color: "#F44336"
                    font.pixelSize: 12
                    visible: confirmPasswordField.errorText.length > 0
                    Layout.fillWidth: true
                    Layout.topMargin: 2
                }

                Item { Layout.preferredHeight: 24 }

                // --- Create Account button ---
                Button {
                    id: createAccountButton
                    text: authNotifier.isLoading ? "" : qsTr("Create Account")
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
                        var nameOk = nameField.validate()
                        var emailOk = emailField.validate()
                        var passOk = passwordField.validate()
                        var confirmOk = confirmPasswordField.validate()
                        if (nameOk && emailOk && passOk && confirmOk) {
                            authNotifier.register_(
                                nameField.text.trim(),
                                emailField.text.trim(),
                                passwordField.text
                            )
                        }
                    }
                }

                Item { Layout.preferredHeight: 32 }

                // --- Sign In link ---
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 4

                    Label {
                        text: qsTr("Already have an account?")
                        font.pixelSize: 14
                    }

                    Label {
                        text: qsTr("Sign In")
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: palette.highlight

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: router.go("/auth/login")
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
