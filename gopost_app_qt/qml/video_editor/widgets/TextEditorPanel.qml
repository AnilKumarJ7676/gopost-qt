import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * TextEditorPanel — font/style/size, fill/stroke colors, alignment,
 *                   spacing, shadow, animation presets.
 *
 * Converted 1:1 from text_editor_panel.dart.
 */
Item {
    id: root

    readonly property var animationPresets: [
        { key: "none",       label: "None" },
        { key: "fadeIn",     label: "Fade In" },
        { key: "slideUp",   label: "Slide Up" },
        { key: "slideDown", label: "Slide Down" },
        { key: "slideLeft", label: "Slide Left" },
        { key: "slideRight",label: "Slide Right" },
        { key: "scaleUp",   label: "Scale Up" },
        { key: "typewriter",label: "Typewriter" },
        { key: "bounce",    label: "Bounce" },
        { key: "glitch",    label: "Glitch" },
        { key: "neon",      label: "Neon Flicker" }
    ]

    readonly property var fontFamilies: [
        "Roboto", "Inter", "Poppins", "Montserrat", "Open Sans",
        "Lato", "Oswald", "Raleway", "Playfair Display", "Bebas Neue",
        "Dancing Script", "Pacifico", "Lobster", "Permanent Marker"
    ]

    property string fontFamily: "Roboto"
    property string fontStyle: "Regular"
    property real fontSize: 48
    property bool fillEnabled: true
    property string fillColor: "#FFFFFF"
    property bool strokeEnabled: false
    property string strokeColor: "#000000"
    property real strokeWidth: 0
    property int alignment: 1 // 0=left, 1=center, 2=right
    property real tracking: 0
    property real leading: 1.2
    property string animPreset: "none"
    property bool hasShadow: false

    property int selectedClipId: timelineNotifier ? timelineNotifier.selectedClipId : -1

    // Load text from clip when selection changes
    onSelectedClipIdChanged: {
        if (selectedClipId >= 0 && timelineNotifier) {
            var td = timelineNotifier.clipTextData(selectedClipId)
            if (td && td.text !== undefined) textArea.text = td.text
        }
    }

    // Empty state: no title clip selected
    ColumnLayout {
        anchors.centerIn: parent
        visible: selectedClipId < 0
        spacing: 8
        Label { text: "T"; font.pixelSize: 38; color: "#404060"; Layout.alignment: Qt.AlignHCenter }
        Label { text: "Select a text clip to edit"; font.pixelSize: 14; color: "#6B6B88"; Layout.alignment: Qt.AlignHCenter }
    }

    // Editor content
    Flickable {
        anchors.fill: parent
        contentHeight: editorCol.implicitHeight
        clip: true
        visible: selectedClipId >= 0
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: editorCol
            width: parent.width
            spacing: 0

            // Text content
            ColumnLayout {
                Layout.margins: 12
                spacing: 6

                SectionLabel { labelText: "Text Content" }

                TextArea {
                    id: textArea
                    Layout.fillWidth: true
                    text: "Your Text"
                    font.pixelSize: 15
                    color: "#E0E0F0"
                    wrapMode: TextEdit.Wrap
                    background: Rectangle { color: "#1A1A34"; radius: 8; border.color: "#303050" }
                    leftPadding: 10; rightPadding: 10; topPadding: 10; bottomPadding: 10
                }
            }

            // Font section
            ColumnLayout {
                Layout.margins: 12
                spacing: 6

                SectionLabel { labelText: "Font" }

                // Font family dropdown
                ComboBox {
                    id: fontCombo
                    Layout.fillWidth: true
                    model: fontFamilies
                    currentIndex: 0
                    onCurrentTextChanged: fontFamily = currentText
                    background: Rectangle { color: "#1A1A34"; radius: 8; border.color: "#303050" }
                    contentItem: Label {
                        text: fontCombo.currentText
                        font.pixelSize: 14; color: "#E0E0F0"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 10
                    }
                }

                RowLayout {
                    spacing: 8

                    // Font style
                    ComboBox {
                        id: styleCombo
                        Layout.fillWidth: true
                        model: ["Regular", "Bold", "Italic", "Bold Italic"]
                        currentIndex: 0
                        onCurrentTextChanged: fontStyle = currentText
                        background: Rectangle { color: "#1A1A34"; radius: 8; border.color: "#303050" }
                        contentItem: Label {
                            text: styleCombo.currentText
                            font.pixelSize: 14; color: "#E0E0F0"
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 10
                        }
                    }

                    // Font size
                    TextField {
                        Layout.preferredWidth: 70
                        text: Math.round(fontSize).toString()
                        font.pixelSize: 14
                        color: "#E0E0F0"
                        horizontalAlignment: Text.AlignHCenter
                        inputMethodHints: Qt.ImhDigitsOnly
                        background: Rectangle { color: "#1A1A34"; radius: 8; border.color: "#303050" }
                        onAccepted: {
                            var sz = parseFloat(text);
                            if (sz >= 8 && sz <= 300) fontSize = sz;
                        }
                        Label {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: "px"
                            font.pixelSize: 12; color: "#6B6B88"
                        }
                    }
                }
            }

            // Alignment
            ColumnLayout {
                Layout.margins: 12
                spacing: 6

                SectionLabel { labelText: "Alignment" }

                RowLayout {
                    spacing: 0
                    Repeater {
                        model: [
                            { icon: "\u2190", align: 0 }, // left
                            { icon: "\u2194", align: 1 }, // center
                            { icon: "\u2192", align: 2 }  // right
                        ]
                        delegate: Rectangle {
                            property bool isActive: alignment === modelData.align
                            Layout.fillWidth: true
                            height: 34
                            radius: 6
                            color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "#1A1A34"
                            border.color: isActive ? "#6C63FF" : "#303050"
                            Layout.leftMargin: 2; Layout.rightMargin: 2

                            Label {
                                anchors.centerIn: parent
                                text: modelData.icon
                                font.pixelSize: 18
                                color: isActive ? "#6C63FF" : "#6B6B88"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: alignment = modelData.align
                            }
                        }
                    }
                }
            }

            // Colors
            ColumnLayout {
                Layout.margins: 12
                spacing: 6

                SectionLabel { labelText: "Colors" }

                // Fill
                RowLayout {
                    spacing: 4
                    CheckBox {
                        id: fillCheck
                        checked: fillEnabled
                        onCheckedChanged: fillEnabled = checked
                    }
                    Label { text: "Fill"; font.pixelSize: 13; color: "#8888A0" }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        width: 24; height: 24; radius: 4
                        color: fillColor
                        border.color: "#505070"
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: colorPickerDialog.openFor("fill")
                        }
                    }
                }

                // Stroke
                RowLayout {
                    spacing: 4
                    CheckBox {
                        id: strokeCheck
                        checked: strokeEnabled
                        onCheckedChanged: strokeEnabled = checked
                    }
                    Label { text: "Stroke"; font.pixelSize: 13; color: "#8888A0" }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        width: 24; height: 24; radius: 4
                        color: strokeColor
                        border.color: "#505070"
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: colorPickerDialog.openFor("stroke")
                        }
                    }
                }

                // Stroke width slider (visible when stroke enabled)
                RowLayout {
                    visible: strokeEnabled
                    Label { text: "Width"; font.pixelSize: 12; color: "#8888A0"; Layout.preferredWidth: 60 }
                    Slider {
                        Layout.fillWidth: true
                        from: 0; to: 10; value: strokeWidth
                        onMoved: strokeWidth = value
                    }
                    Label {
                        text: strokeWidth.toFixed(1)
                        font.pixelSize: 12; font.family: "monospace"; color: "#B0B0C8"
                        Layout.preferredWidth: 36
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            // Spacing
            ColumnLayout {
                Layout.margins: 12
                spacing: 6

                SectionLabel { labelText: "Spacing" }

                SliderRow { label: "Tracking"; value: tracking; minVal: -50; maxVal: 100; onValueChanged: tracking = value }
                SliderRow { label: "Line Height"; value: leading; minVal: 0.5; maxVal: 3.0; onValueChanged: leading = value }
            }

            // Shadow
            ColumnLayout {
                Layout.margins: 12
                spacing: 6

                SectionLabel { labelText: "Shadow" }

                RowLayout {
                    Label { text: "Drop Shadow"; font.pixelSize: 13; color: "#8888A0" }
                    Item { Layout.fillWidth: true }
                    Switch {
                        checked: hasShadow
                        onCheckedChanged: hasShadow = checked
                    }
                }
            }

            // Animation
            ColumnLayout {
                Layout.margins: 12
                spacing: 6

                SectionLabel { labelText: "Animation" }

                Flow {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: animationPresets
                        delegate: Rectangle {
                            property bool isActive: animPreset === modelData.key
                            width: animLabel.implicitWidth + 20
                            height: 28
                            radius: 6
                            color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "#1A1A34"
                            border.color: isActive ? "#6C63FF" : "#303050"

                            Label {
                                id: animLabel
                                anchors.centerIn: parent
                                text: modelData.label
                                font.pixelSize: 12
                                font.weight: isActive ? Font.DemiBold : Font.Normal
                                color: isActive ? "#6C63FF" : "#B0B0C8"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: animPreset = modelData.key
                            }
                        }
                    }
                }
            }

            // Apply button
            Button {
                Layout.margins: 12
                Layout.fillWidth: true
                text: "Apply Text"
                implicitHeight: 38
                background: Rectangle { color: "#6C63FF"; radius: 8 }
                contentItem: RowLayout {
                    spacing: 6
                    Item { Layout.fillWidth: true }
                    Label { text: "\u2713"; font.pixelSize: 16; color: "white" }
                    Label { text: "Apply Text"; font.pixelSize: 14; color: "white" }
                    Item { Layout.fillWidth: true }
                }
                onClicked: {
                    if (selectedClipId >= 0 && timelineNotifier) {
                        timelineNotifier.updateClipDisplayName(selectedClipId, textArea.text)
                        timelineNotifier.setClipOpacity(selectedClipId, fillEnabled ? 1.0 : 0.0)
                    }
                }
            }

            Item { height: 16 }
        }
    }

    // Color picker dialog (simple grid)
    Dialog {
        id: colorPickerDialog
        title: "Pick Color"
        modal: true
        width: 240

        property string targetField: "" // "fill" or "stroke"

        function openFor(field) {
            targetField = field;
            open();
        }

        readonly property var swatchColors: [
            "#FFFFFF", "#000000", "#F44336", "#E91E63",
            "#9C27B0", "#673AB7", "#3F51B5", "#2196F3",
            "#00BCD4", "#009688", "#4CAF50", "#8BC34A",
            "#CDDC39", "#FFEB3B", "#FFC107", "#FF9800",
            "#FF5722", "#795548", "#9E9E9E", "#607D8B"
        ]

        background: Rectangle { color: "#1A1A34"; radius: 8 }

        contentItem: Flow {
            width: 200
            spacing: 8

            Repeater {
                model: colorPickerDialog.swatchColors
                delegate: Rectangle {
                    width: 32; height: 32; radius: 4
                    color: modelData
                    border.color: (colorPickerDialog.targetField === "fill" && fillColor === modelData) ||
                                  (colorPickerDialog.targetField === "stroke" && strokeColor === modelData)
                                  ? "#6C63FF" : "transparent"
                    border.width: 2

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (colorPickerDialog.targetField === "fill")
                                fillColor = modelData;
                            else
                                strokeColor = modelData;
                            colorPickerDialog.close();
                        }
                    }
                }
            }
        }
    }

    // Reusable section label
    component SectionLabel: Label {
        property string labelText: ""
        text: labelText
        font.pixelSize: 13
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        color: "#8888A0"
    }

    // Reusable slider row
    component SliderRow: RowLayout {
        property string label: ""
        property real value: 0
        property real minVal: 0
        property real maxVal: 100

        Label { text: label; font.pixelSize: 12; color: "#8888A0"; Layout.preferredWidth: 60 }
        Slider {
            Layout.fillWidth: true
            from: minVal; to: maxVal; value: parent.value
            onMoved: parent.value = value
        }
        Label {
            text: parent.value.toFixed(1)
            font.pixelSize: 12; font.family: "monospace"; color: "#B0B0C8"
            Layout.preferredWidth: 36
            horizontalAlignment: Text.AlignRight
        }
    }
}
