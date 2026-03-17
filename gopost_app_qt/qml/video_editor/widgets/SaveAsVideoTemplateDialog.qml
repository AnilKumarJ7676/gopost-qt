import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Dialog for saving a video project as a reusable template.
/// Converted from save_as_video_template_dialog.dart.
Dialog {
    id: root

    title: "Save as Video Template"
    modal: true
    standardButtons: Dialog.Cancel | Dialog.Save
    width: 480
    implicitHeight: contentCol.implicitHeight + 120

    property bool saving: false
    property string error: ""

    background: Rectangle {
        radius: 12
        color: "#1A1A34"
        border.color: "#303050"
    }

    header: Rectangle {
        height: 48
        color: "#16162E"
        radius: 12

        // Flatten bottom corners
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width; height: 12
            color: "#16162E"
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width; height: 1
            color: "#252540"
        }

        Label {
            anchors.centerIn: parent
            text: "Save as Video Template"
            font.pixelSize: 16; font.weight: Font.DemiBold
            color: "#E0E0F0"
        }
    }

    contentItem: ColumnLayout {
        id: contentCol
        spacing: 12

        // Name
        Label { text: "Template name *"; font.pixelSize: 13; color: "#8888A0" }
        TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: "My video template"
            font.pixelSize: 14; color: "#E0E0F0"
            enabled: !root.saving

            background: Rectangle {
                radius: 6; color: "#12122A"
                border.color: "#303050"
            }
        }

        // Description
        Label { text: "Description"; font.pixelSize: 13; color: "#8888A0" }
        TextField {
            id: descField
            Layout.fillWidth: true
            placeholderText: "What this template is for"
            font.pixelSize: 14; color: "#E0E0F0"
            enabled: !root.saving

            background: Rectangle {
                radius: 6; color: "#12122A"
                border.color: "#303050"
            }
        }

        // Category
        Label { text: "Category"; font.pixelSize: 13; color: "#8888A0" }
        ComboBox {
            id: categoryCombo
            Layout.fillWidth: true
            model: templateBrowser ? templateBrowser.categories : []
            textRole: "name"
            valueRole: "id"
            enabled: !root.saving
        }

        // Tags
        Label { text: "Tags (comma-separated)"; font.pixelSize: 13; color: "#8888A0" }
        TextField {
            id: tagsField
            Layout.fillWidth: true
            placeholderText: "social, promo, reel"
            font.pixelSize: 14; color: "#E0E0F0"
            enabled: !root.saving

            background: Rectangle {
                radius: 6; color: "#12122A"
                border.color: "#303050"
            }
        }

        Item { height: 8 }

        // Placeholder Clips header
        Label {
            text: "Placeholder Clips"
            font.pixelSize: 14; font.weight: Font.DemiBold
            color: "#E0E0F0"
        }
        Label {
            text: "Mark clips that users can replace when customizing this template."
            font.pixelSize: 12; color: "#6B6B88"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Placeholder list
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(count * 52, 200)
            clip: true
            spacing: 4

            model: timelineNotifier ? timelineNotifier.templatePlaceholders : []

            delegate: Rectangle {
                width: ListView.view.width
                height: 48
                radius: 8
                color: "#12122A"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Label { text: "\uD83C\uDFA5"; font.pixelSize: 16; color: "#6B6B88" }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Label {
                            text: model.displayName || ("Clip " + model.clipId)
                            font.pixelSize: 13; font.weight: Font.Medium; color: "#E0E0F0"
                            elide: Text.ElideRight; Layout.fillWidth: true
                        }
                        Label {
                            text: (model.fieldType || "media") + " placeholder"
                            font.pixelSize: 11; color: "#6B6B88"
                        }
                    }

                    ToolButton {
                        icon.name: "window-close"
                        icon.width: 16; icon.height: 16; icon.color: "#6B6B88"
                        onClicked: timelineNotifier.removePlaceholder(index)
                    }
                }
            }
        }

        // Add placeholder button
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            radius: 8
            color: "transparent"
            border.color: "#303050"
            visible: timelineNotifier && timelineNotifier.hasUnassignedClips

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Label { text: "+"; font.pixelSize: 18; color: "#6B6B88" }
                Label { text: "Add Placeholder Clip"; font.pixelSize: 13; color: "#6B6B88" }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: timelineNotifier.showPlaceholderPicker()
            }
        }

        // Error
        Label {
            visible: root.error.length > 0
            text: root.error
            font.pixelSize: 12
            color: Theme.semanticError
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Busy indicator
        BusyIndicator {
            visible: root.saving
            running: root.saving
            Layout.alignment: Qt.AlignHCenter
        }
    }

    onAccepted: {
        if (nameField.text.trim().length === 0) {
            root.error = "Enter a template name"
            return
        }
        root.saving = true
        root.error = ""
        templateBrowser.saveVideoTemplate(
            nameField.text.trim(),
            descField.text.trim(),
            categoryCombo.currentValue || "",
            tagsField.text.trim()
        )
    }

    Connections {
        target: templateBrowser
        function onTemplateSaved() { root.saving = false; root.close() }
        function onTemplateSaveError(msg) { root.saving = false; root.error = msg }
    }
}
