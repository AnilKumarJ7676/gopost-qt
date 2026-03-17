import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import GopostApp

/// Template customization screen.
/// Opens a downloaded template, displays placeholder fields for the user to
/// edit, shows a live preview, and allows export of the customized result.
Item {
    id: root

    property string templateId: ""
    property string templateName: ""
    property var editableFields: []  // list of {key, label, fieldType, defaultValue}

    signal goBack()
    signal exportRequested()

    // Local state
    property var fieldValues: ({})
    property var imagePaths: ({})
    property bool initialized: false
    property int highlightedLayerId: -1

    Component.onCompleted: {
        // Initialize field values from template defaults
        var vals = {}
        for (var i = 0; i < editableFields.length; ++i) {
            var field = editableFields[i]
            vals[field.key] = field.defaultValue || ""
        }
        fieldValues = vals
        initialized = true
    }

    function resetAllFields() {
        var vals = {}
        for (var i = 0; i < editableFields.length; ++i) {
            var field = editableFields[i]
            vals[field.key] = field.defaultValue || ""
        }
        fieldValues = vals
        imagePaths = {}
    }

    Rectangle {
        anchors.fill: parent
        color: "#121218"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "#1E1E2E"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 0.5
                color: Qt.rgba(1, 1, 1, 0.1)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8; anchors.rightMargin: 8
                spacing: 8

                ToolButton {
                    icon.name: "go-previous"
                    icon.color: Qt.rgba(1, 1, 1, 0.7)
                    onClicked: router.pop()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Label {
                        text: root.templateName
                        color: "white"
                        font.pixelSize: 14; font.weight: Font.DemiBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: "Customize Template"
                        color: Qt.rgba(1, 1, 1, 0.5)
                        font.pixelSize: 11
                    }
                }

                ToolButton {
                    icon.name: "edit-undo"
                    icon.color: Qt.rgba(1, 1, 1, 0.38)
                    ToolTip.text: "Reset All"
                    ToolTip.visible: hovered
                    onClicked: root.resetAllFields()
                }

                Button {
                    text: "Export"
                    icon.name: "document-save"
                    flat: true
                    onClicked: root.exportRequested()
                }
            }
        }

        // Main content - responsive layout
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Wide layout
            RowLayout {
                anchors.fill: parent
                spacing: 0
                visible: parent.width > 600

                // Preview
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    CanvasPreview {
                        anchors.fill: parent
                    }
                }

                // Placeholder panel
                Rectangle {
                    Layout.preferredWidth: 300
                    Layout.fillHeight: true
                    color: "#1E1E2E"

                    Rectangle {
                        anchors.left: parent.left
                        width: 0.5; height: parent.height
                        color: Qt.rgba(1, 1, 1, 0.1)
                    }

                    Loader {
                        anchors.fill: parent
                        sourceComponent: placeholderPanelContent
                    }
                }
            }

            // Narrow layout
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                visible: parent.width <= 600

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    CanvasPreview {
                        anchors.fill: parent
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 240
                    color: "#1E1E2E"

                    Rectangle {
                        anchors.top: parent.top
                        width: parent.width; height: 0.5
                        color: Qt.rgba(1, 1, 1, 0.1)
                    }

                    Loader { sourceComponent: placeholderPanelContent; anchors.fill: parent }
                }
            }
        }

        // Bottom bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#1E1E2E"

            Rectangle {
                anchors.top: parent.top
                width: parent.width; height: 0.5
                color: Qt.rgba(1, 1, 1, 0.1)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16; anchors.rightMargin: 16
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    text: "Cancel"
                    flat: true
                    onClicked: router.pop()
                }

                Button {
                    Layout.fillWidth: true
                    text: "Reset"
                    flat: true
                    visible: internal.fieldsModified
                    onClicked: root.resetAllFields()
                }

                Button {
                    Layout.fillWidth: true; Layout.preferredWidth: 200
                    text: "Export Customized"
                    highlighted: true
                    icon.name: "emblem-ok-symbolic"
                    onClicked: root.exportRequested()
                }
            }
        }
    }

    // --- Placeholder panel content ---
    Component {
        id: placeholderPanelContent

        ColumnLayout {
            spacing: 0

            // Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: "transparent"

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 0.5
                    color: Qt.rgba(1, 1, 1, 0.1)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12

                    Label {
                        text: "\u270F"
                        font.pixelSize: 14; color: "#6C63FF"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: "Edit Placeholders"
                        color: "white"; font.pixelSize: 14; font.weight: Font.DemiBold
                    }
                    Label {
                        text: root.editableFields.length + " field" + (root.editableFields.length > 1 ? "s" : "")
                        color: Qt.rgba(1, 1, 1, 0.4)
                        font.pixelSize: 11
                    }
                }
            }

            // Fields list
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: root.editableFields
                clip: true
                spacing: 12

                delegate: ColumnLayout {
                    width: parent ? parent.width - 24 : 200
                    x: 12
                    spacing: 4

                    RowLayout {
                        Label {
                            text: modelData.label
                            color: "white"; font.pixelSize: 12; font.weight: Font.Medium
                        }
                        Item { Layout.fillWidth: true }
                    }

                    TextField {
                        Layout.fillWidth: true
                        text: root.fieldValues[modelData.key] || ""
                        placeholderText: modelData.defaultValue || "Enter value..."
                        color: "white"
                        font.pixelSize: 13

                        background: Rectangle {
                            color: Qt.rgba(1, 1, 1, 0.05)
                            radius: 6
                            border.color: parent.activeFocus ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                            border.width: parent.activeFocus ? 1.5 : 1
                        }

                        onTextChanged: {
                            var vals = root.fieldValues
                            vals[modelData.key] = text
                            root.fieldValues = vals
                        }
                    }
                }
            }
        }
    }

    QtObject {
        id: internal
        readonly property bool fieldsModified: {
            for (var i = 0; i < root.editableFields.length; ++i) {
                var field = root.editableFields[i]
                var current = root.fieldValues[field.key] || ""
                if (current !== (field.defaultValue || "")) return true
            }
            return false
        }
    }
}
