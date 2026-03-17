import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Panel for adding and editing text layers.
/// Text input, font size slider, bold/italic/underline, alignment, color picker, shadow toggle.
Rectangle {
    id: root

    height: 380
    color: "#1E1E2E"

    Rectangle {
        anchors.top: parent.top
        width: parent.width; height: 0.5
        color: Qt.rgba(1, 1, 1, 0.1)
    }

    Component.onCompleted: {
        if (textToolNotifier) textToolNotifier.loadFonts()
    }

    readonly property var presetColors: [
        { r: 1.0, g: 1.0, b: 1.0 },
        { r: 0.0, g: 0.0, b: 0.0 },
        { r: 1.0, g: 0.0, b: 0.0 },
        { r: 0.0, g: 0.8, b: 0.0 },
        { r: 0.0, g: 0.0, b: 1.0 },
        { r: 1.0, g: 1.0, b: 0.0 },
        { r: 1.0, g: 0.5, b: 0.0 },
        { r: 0.8, g: 0.0, b: 0.8 },
        { r: 0.5, g: 0.5, b: 0.5 },
        { r: 0.6, g: 0.4, b: 0.2 },
        { r: 0.0, g: 0.6, b: 0.6 },
        { r: 0.6, g: 0.2, b: 0.4 }
    ]

    Flickable {
        anchors.fill: parent
        contentHeight: contentCol.height
        clip: true

        ColumnLayout {
            id: contentCol
            anchors {
                left: parent.left; right: parent.right
                margins: 12; top: parent.top; topMargin: 12
            }
            spacing: 12

            // Text input
            TextArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                text: textToolNotifier ? textToolNotifier.text : ""
                placeholderText: "Enter text..."
                color: "white"
                font.pixelSize: 14
                wrapMode: TextEdit.Wrap

                background: Rectangle {
                    color: Qt.rgba(1, 1, 1, 0.08)
                    radius: 8
                    border.color: parent.activeFocus ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.24)
                    border.width: parent.activeFocus ? 1.5 : 1
                }

                onTextChanged: {
                    if (textToolNotifier) textToolNotifier.text = text
                }
            }

            // Font size slider
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "Font size"
                        color: Qt.rgba(1, 1, 1, 0.7); font.pixelSize: 13
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: textToolNotifier ? Math.round(textToolNotifier.fontSize) : "24"
                        color: "#6C63FF"; font.pixelSize: 12; font.weight: Font.DemiBold
                    }
                }

                Slider {
                    Layout.fillWidth: true
                    from: 8; to: 128
                    value: textToolNotifier ? textToolNotifier.fontSize : 24
                    onMoved: { if (textToolNotifier) textToolNotifier.fontSize = value }
                }
            }

            // Bold/Italic toggles
            RowLayout {
                spacing: 8

                Rectangle {
                    width: boldLabel.implicitWidth + 24; height: 36; radius: 8
                    color: internal.isBold ? Qt.rgba(0.42, 0.39, 1.0, 0.3) : Qt.rgba(1, 1, 1, 0.08)
                    border.color: internal.isBold ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.24)

                    Label {
                        id: boldLabel; anchors.centerIn: parent
                        text: "Bold"
                        color: internal.isBold ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.7)
                        font.pixelSize: 13; font.weight: Font.Medium
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: internal.toggleBold()
                    }
                }

                Rectangle {
                    width: italicLabel.implicitWidth + 24; height: 36; radius: 8
                    color: internal.isItalic ? Qt.rgba(0.42, 0.39, 1.0, 0.3) : Qt.rgba(1, 1, 1, 0.08)
                    border.color: internal.isItalic ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.24)

                    Label {
                        id: italicLabel; anchors.centerIn: parent
                        text: "Italic"
                        color: internal.isItalic ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.7)
                        font.pixelSize: 13; font.weight: Font.Medium
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: internal.toggleItalic()
                    }
                }
            }

            // Alignment buttons
            RowLayout {
                Label {
                    text: "Align"
                    color: Qt.rgba(1, 1, 1, 0.7); font.pixelSize: 13
                }
                Item { Layout.preferredWidth: 12 }

                Repeater {
                    model: [
                        { icon: "\u2190", align: 0, label: "Left" },
                        { icon: "\u2194", align: 1, label: "Center" },
                        { icon: "\u2192", align: 2, label: "Right" }
                    ]

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: textToolNotifier && textToolNotifier.alignment === modelData.align
                               ? Qt.rgba(0.42, 0.39, 1.0, 0.3)
                               : Qt.rgba(1, 1, 1, 0.08)

                        Label {
                            anchors.centerIn: parent
                            text: modelData.icon
                            font.pixelSize: 18
                            color: textToolNotifier && textToolNotifier.alignment === modelData.align
                                   ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.54)
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: { if (textToolNotifier) textToolNotifier.alignment = modelData.align }
                        }
                    }
                }
            }

            // Color picker
            Flow {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: root.presetColors

                    delegate: Rectangle {
                        width: 28; height: 28; radius: 14
                        color: Qt.rgba(modelData.r, modelData.g, modelData.b, 1)
                        border.color: internal.isColorSelected(modelData.r, modelData.g, modelData.b)
                                      ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.38)
                        border.width: internal.isColorSelected(modelData.r, modelData.g, modelData.b) ? 2.5 : 1

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (textToolNotifier) {
                                    textToolNotifier.colorR = modelData.r
                                    textToolNotifier.colorG = modelData.g
                                    textToolNotifier.colorB = modelData.b
                                }
                            }
                        }
                    }
                }
            }

            // Shadow toggle
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Shadow"
                    color: Qt.rgba(1, 1, 1, 0.7); font.pixelSize: 13
                }
                Item { Layout.fillWidth: true }
                Switch {
                    checked: textToolNotifier ? textToolNotifier.hasShadow : false
                    onToggled: { if (textToolNotifier) textToolNotifier.hasShadow = checked }
                }
            }

            // Action button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                text: textToolNotifier && textToolNotifier.isEditing && textToolNotifier.activeLayerId >= 0
                      ? "Update" : "Add Text"
                highlighted: true
                enabled: textToolNotifier && textToolNotifier.text.trim().length > 0
                         && canvasNotifier && canvasNotifier.hasCanvas
                         && canvasNotifier.canvasWidth > 0

                onClicked: {
                    if (!textToolNotifier || !canvasNotifier) return
                    var cid = canvasNotifier.canvasId
                    var maxW = canvasNotifier.canvasWidth

                    if (textToolNotifier.isEditing && textToolNotifier.activeLayerId >= 0) {
                        textToolNotifier.updateActiveTextLayer(cid, maxW)
                    } else {
                        textToolNotifier.addTextLayer(cid, maxW)
                    }
                }
            }
        }
    }

    QtObject {
        id: internal

        // FontStyle: 0=Normal, 1=Bold, 2=Italic, 3=BoldItalic
        readonly property int fontStyle: textToolNotifier ? textToolNotifier.fontStyle : 0
        readonly property bool isBold: fontStyle === 1 || fontStyle === 3
        readonly property bool isItalic: fontStyle === 2 || fontStyle === 3

        function toggleBold() {
            if (!textToolNotifier) return
            var s = fontStyle
            if (isBold) {
                textToolNotifier.fontStyle = (s === 3) ? 2 : 0
            } else {
                textToolNotifier.fontStyle = (s === 2) ? 3 : 1
            }
        }

        function toggleItalic() {
            if (!textToolNotifier) return
            var s = fontStyle
            if (isItalic) {
                textToolNotifier.fontStyle = (s === 3) ? 1 : 0
            } else {
                textToolNotifier.fontStyle = (s === 1) ? 3 : 2
            }
        }

        function isColorSelected(r, g, b) {
            if (!textToolNotifier) return false
            return Math.abs(textToolNotifier.colorR - r) < 0.01
                && Math.abs(textToolNotifier.colorG - g) < 0.01
                && Math.abs(textToolNotifier.colorB - b) < 0.01
        }
    }
}
