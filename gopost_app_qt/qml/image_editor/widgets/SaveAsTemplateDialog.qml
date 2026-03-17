import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Dialog for saving the current canvas as a reusable template.
/// Name, description, category, tags, and placeholder layer assignment.
Dialog {
    id: root

    anchors.centerIn: parent
    width: Math.min(parent.width - 32, 480)
    modal: true
    title: "Save as Template"

    property string templateName: ""
    property string templateDescription: ""
    property string templateTags: ""
    property string selectedCategoryId: ""
    property bool saving: false
    property string errorMessage: ""
    property var placeholders: []
    property var categoryList: []
    property bool loadingCategories: true

    Component.onCompleted: { loadingCategories = false }

    background: Rectangle {
        color: "#1E1E2E"
        radius: 12
        border.color: Qt.rgba(1, 1, 1, 0.12)
    }

    header: Rectangle {
        color: "transparent"
        height: 48
        Label {
            anchors.centerIn: parent
            text: "Save as Template"
            color: "#E0E0E0"
            font.pixelSize: 16; font.weight: Font.DemiBold
        }
    }

    contentItem: Flickable {
        contentHeight: formCol.height
        clip: true

        ColumnLayout {
            id: formCol
            width: parent.width
            spacing: 12

            // Template name
            TextField {
                Layout.fillWidth: true
                placeholderText: "Template name *"
                text: root.templateName
                color: "#E0E0E0"; font.pixelSize: 14
                onTextChanged: root.templateName = text
                enabled: !root.saving
                background: Rectangle {
                    color: Qt.rgba(1, 1, 1, 0.05); radius: 6
                    border.color: parent.activeFocus ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                    border.width: parent.activeFocus ? 1.5 : 1
                }
            }

            // Description
            TextField {
                Layout.fillWidth: true
                placeholderText: "Description"
                text: root.templateDescription
                color: "#E0E0E0"; font.pixelSize: 14
                onTextChanged: root.templateDescription = text
                enabled: !root.saving
                background: Rectangle {
                    color: Qt.rgba(1, 1, 1, 0.05); radius: 6
                    border.color: parent.activeFocus ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                    border.width: parent.activeFocus ? 1.5 : 1
                }
            }

            // Category dropdown
            ComboBox {
                Layout.fillWidth: true
                visible: !root.loadingCategories
                model: root.categoryList
                textRole: "name"
                valueRole: "id"
                enabled: !root.saving
            }

            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                visible: root.loadingCategories
                running: root.loadingCategories
                implicitWidth: 18; implicitHeight: 18
            }

            // Tags
            TextField {
                Layout.fillWidth: true
                placeholderText: "Tags (comma-separated)"
                text: root.templateTags
                color: "#E0E0E0"; font.pixelSize: 14
                onTextChanged: root.templateTags = text
                enabled: !root.saving
                background: Rectangle {
                    color: Qt.rgba(1, 1, 1, 0.05); radius: 6
                    border.color: parent.activeFocus ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.12)
                    border.width: parent.activeFocus ? 1.5 : 1
                }
            }

            // Placeholder layers section
            Label {
                text: "Placeholder Layers"
                color: "#E0E0E0"
                font.pixelSize: 14; font.weight: Font.DemiBold
            }

            Label {
                text: "Mark layers that users can customize when using this template."
                color: Qt.rgba(1, 1, 1, 0.54)
                font.pixelSize: 12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            // Placeholder list
            Repeater {
                model: root.placeholders

                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: phCol.implicitHeight + 16
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.04)
                    border.color: Qt.rgba(1, 1, 1, 0.08)

                    ColumnLayout {
                        id: phCol
                        anchors {
                            left: parent.left; right: parent.right
                            margins: 8; top: parent.top; topMargin: 8
                        }
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: modelData.layerName || ("Layer " + modelData.layerId)
                                color: "#E0E0E0"; font.pixelSize: 13; font.weight: Font.Medium
                                elide: Text.ElideRight
                            }
                            ToolButton {
                                icon.name: "list-remove"
                                icon.color: Qt.rgba(1, 1, 1, 0.54)
                                implicitWidth: 24; implicitHeight: 24
                                onClicked: internal.removePlaceholder(index)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            TextField {
                                Layout.fillWidth: true
                                text: modelData.label
                                placeholderText: "Label"
                                color: "#E0E0E0"; font.pixelSize: 12
                                onTextChanged: {
                                    var list = root.placeholders
                                    list[index].label = text
                                    root.placeholders = list
                                }
                                background: Rectangle {
                                    color: Qt.rgba(1, 1, 1, 0.05); radius: 4
                                    border.color: Qt.rgba(1, 1, 1, 0.12)
                                }
                            }
                            ComboBox {
                                Layout.preferredWidth: 100
                                model: ["text", "image", "color", "number"]
                                currentIndex: {
                                    var types = ["text", "image", "color", "number"]
                                    return Math.max(0, types.indexOf(modelData.fieldType))
                                }
                                onCurrentIndexChanged: {
                                    var types = ["text", "image", "color", "number"]
                                    var list = root.placeholders
                                    list[index].fieldType = types[currentIndex]
                                    root.placeholders = list
                                }
                            }
                        }

                        TextField {
                            Layout.fillWidth: true
                            text: modelData.defaultValue || ""
                            placeholderText: "Default value"
                            color: "#E0E0E0"; font.pixelSize: 12
                            onTextChanged: {
                                var list = root.placeholders
                                list[index].defaultValue = text
                                root.placeholders = list
                            }
                            background: Rectangle {
                                color: Qt.rgba(1, 1, 1, 0.05); radius: 4
                                border.color: Qt.rgba(1, 1, 1, 0.12)
                            }
                        }
                    }
                }
            }

            // Add placeholder button
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 8; color: "transparent"
                border.color: Qt.rgba(1, 1, 1, 0.24)
                visible: canvasNotifier && canvasNotifier.layerCount > root.placeholders.length

                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Label { text: "+"; color: Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 16 }
                    Label { text: "Add Placeholder Layer"; color: Qt.rgba(1, 1, 1, 0.54); font.pixelSize: 13 }
                }
                MouseArea { anchors.fill: parent; onClicked: internal.addPlaceholder() }
            }

            // Error
            Label {
                Layout.fillWidth: true
                visible: root.errorMessage !== ""
                text: root.errorMessage
                color: "#CF6679"; font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Cancel"; flat: true
            enabled: !root.saving
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: root.saving ? "" : "Save"
            highlighted: true; enabled: !root.saving
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            BusyIndicator {
                anchors.centerIn: parent
                running: root.saving; visible: root.saving
                implicitWidth: 20; implicitHeight: 20
            }
        }
    }

    onAccepted: internal.save()

    QtObject {
        id: internal

        function save() {
            if (root.templateName.trim() === "") {
                root.errorMessage = "Enter a template name"
                return
            }
            root.saving = true
            root.errorMessage = ""
            // Save would be triggered via C++ backend
            root.saving = false
        }

        function addPlaceholder() {
            if (!canvasNotifier) return
            var list = root.placeholders.slice()
            var assignedIds = []
            for (var j = 0; j < list.length; ++j) {
                assignedIds.push(list[j].layerId)
            }
            var layers = canvasNotifier.layers || []
            for (var i = 0; i < layers.length; ++i) {
                if (assignedIds.indexOf(layers[i].id) < 0) {
                    list.push({
                        layerId: layers[i].id,
                        layerName: layers[i].name || ("Layer " + layers[i].id),
                        key: "placeholder_" + layers[i].id,
                        label: layers[i].name || ("Layer " + layers[i].id),
                        fieldType: "text",
                        defaultValue: ""
                    })
                    root.placeholders = list
                    return
                }
            }
        }

        function removePlaceholder(idx) {
            var list = root.placeholders.slice()
            list.splice(idx, 1)
            root.placeholders = list
        }
    }
}
