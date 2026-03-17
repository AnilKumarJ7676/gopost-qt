import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import GopostApp

/// Screen for customizing a video template's placeholder fields.
Page {
    id: root

    property string templateId: ""
    property bool loading: true
    property string error: ""
    property var template: null
    property var customValues: ({})

    Component.onCompleted: internal.loadTemplate()

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8; anchors.rightMargin: 8

            ToolButton {
                icon.name: "go-previous"
                onClicked: router.pop()
            }
            Label {
                text: template ? template.name : "Customize Template"
                font.pixelSize: 18
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Button {
                text: "Export"
                enabled: template !== null
                font.pixelSize: 12

                background: Rectangle {
                    radius: 6
                    color: parent.enabled ? "#6C63FF" : Qt.rgba(0.424, 0.388, 1.0, 0.3)
                }
                contentItem: Label {
                    text: "\u2913 Export"
                    color: "white"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                }

                onClicked: internal.exportTemplate()
            }
        }
    }

    // Loading state
    BusyIndicator {
        anchors.centerIn: parent
        running: root.loading
        visible: running
    }

    // Error state
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12
        visible: !root.loading && root.error.length > 0

        Label {
            text: "\u26A0"
            font.pixelSize: 48
            color: Theme.semanticError
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: root.error
            color: Theme.semanticError
            Layout.alignment: Qt.AlignHCenter
        }
    }

    // No fields state
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12
        visible: !root.loading && root.error.length === 0 && template && template.editableFields.length === 0

        Label {
            text: "\u270F\uFE0F"
            font.pixelSize: 48
            opacity: 0.5
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: "No customizable fields in this template."
            opacity: 0.6
            Layout.alignment: Qt.AlignHCenter
        }
    }

    // Field editor
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: !root.loading && root.error.length === 0 && template && template.editableFields.length > 0

        // Preview header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            color: "#20203A"

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8

                Label {
                    text: "\u25B6"
                    font.pixelSize: 48
                    opacity: 0.5
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: template ? (template.width + " x " + template.height) : ""
                    font.pixelSize: 13
                    opacity: 0.6
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: template ? (template.editableFields.length + " customizable fields") : ""
                    font.pixelSize: 12
                    opacity: 0.5
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Fields list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 16
            clip: true
            spacing: 12
            model: template ? template.editableFields : []

            delegate: Rectangle {
                width: ListView.view.width
                height: fieldCol.implicitHeight + 28
                radius: 12
                color: Theme.editorSurface

                ColumnLayout {
                    id: fieldCol
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    RowLayout {
                        spacing: 8

                        Label {
                            text: internal.iconForFieldType(model.fieldType)
                            font.pixelSize: 16
                            color: "#6C63FF"
                        }
                        Label {
                            text: model.label
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            Layout.fillWidth: true
                        }
                        ToolButton {
                            icon.name: "view-refresh"
                            icon.width: 18; icon.height: 18
                            ToolTip.text: "Reset to default"
                            onClicked: {
                                var vals = root.customValues
                                vals[model.key] = model.defaultValue || ""
                                root.customValues = vals
                            }
                        }
                    }

                    // Text field input
                    Loader {
                        Layout.fillWidth: true
                        active: model.fieldType === "text" || model.fieldType === "number"
                        sourceComponent: TextField {
                            text: root.customValues[model.key] || ""
                            placeholderText: model.fieldType === "number" ? "Enter number..." : "Enter text..."
                            inputMethodHints: model.fieldType === "number" ? Qt.ImhDigitsOnly : Qt.ImhNone
                            onTextChanged: {
                                var vals = root.customValues
                                vals[model.key] = text
                                root.customValues = vals
                            }
                        }
                    }

                    // Image/video picker button
                    Loader {
                        Layout.fillWidth: true
                        active: model.fieldType === "image"
                        sourceComponent: Button {
                            text: {
                                var path = root.customValues[model.key] || ""
                                return path.length > 0 ? path.split("/").pop() : "Choose media..."
                            }
                            icon.name: "folder-images"
                            onClicked: internal.pickMedia(model.key, model.fieldType)
                        }
                    }

                    // Color field
                    Loader {
                        Layout.fillWidth: true
                        active: model.fieldType === "color"
                        sourceComponent: RowLayout {
                            spacing: 12

                            Rectangle {
                                width: 40; height: 40
                                radius: 8
                                color: root.customValues[model.key] || "#FFFFFF"
                                border.color: "#303050"
                            }

                            TextField {
                                Layout.fillWidth: true
                                text: root.customValues[model.key] || "#FFFFFF"
                                placeholderText: "#RRGGBB"
                                onTextChanged: {
                                    var vals = root.customValues
                                    vals[model.key] = text
                                    root.customValues = vals
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    QtObject {
        id: internal

        function loadTemplate() {
            // Calls C++ context to fetch template by ID
            templateBrowser.loadTemplate(root.templateId, function(tmpl) {
                root.template = tmpl
                if (tmpl && tmpl.editableFields) {
                    var vals = {}
                    for (var i = 0; i < tmpl.editableFields.length; i++) {
                        var f = tmpl.editableFields[i]
                        vals[f.key] = f.defaultValue || ""
                    }
                    root.customValues = vals
                }
                root.loading = false
            }, function(err) {
                root.error = err
                root.loading = false
            })
        }

        function exportTemplate() {
            var params = "templateId=" + root.templateId
            router.push("/editor/video?" + params)
        }

        function pickMedia(key, fieldType) {
            fileDialog.fieldKey = key
            fileDialog.open()
        }

        function iconForFieldType(type) {
            switch (type) {
            case "text": return "\uD83D\uDCDD"
            case "image": return "\uD83D\uDDBC"
            case "color": return "\uD83C\uDFA8"
            case "number": return "#"
            default: return "\u2699"
            }
        }
    }

    FileDialog {
        id: fileDialog
        property string fieldKey: ""
        title: "Choose Media"
        fileMode: FileDialog.OpenFile
        nameFilters: [
            "Image files (*.jpg *.jpeg *.png *.webp *.gif *.bmp)",
            "Video files (*.mp4 *.mov *.avi *.mkv *.webm)",
            "All files (*)"
        ]
        onAccepted: {
            if (selectedFile.toString().length > 0) {
                var vals = root.customValues
                vals[fieldKey] = selectedFile.toString()
                root.customValues = vals
            }
        }
    }
}
