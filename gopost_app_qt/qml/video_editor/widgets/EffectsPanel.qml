import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * EffectsPanel — tabbed by category, effect rows with toggle/slider, drag-to-timeline.
 *
 * Converted 1:1 from effects_panel.dart.
 */
Item {
    id: root

    readonly property var categories: [
        { id: 0, label: "COLOR & TONE",     effects: [0,1,2,3,4,5,6,7] },
        { id: 1, label: "BLUR & SHARPEN",   effects: [8,9,10] },
        { id: 2, label: "DISTORT",          effects: [11,12,13] },
        { id: 3, label: "STYLIZE",          effects: [14,15,16,17] },
    ]

    readonly property var effectNames: [
        "Brightness","Contrast","Saturation","Temperature","Tint",
        "Shadows","Highlights","Vibrance",
        "Blur","Sharpen","Noise",
        "Vignette","Distortion","Lens Flare",
        "Film Grain","Chromatic","Pixelate","Posterize"
    ]

    property int selectedClipId: timelineNotifier ? timelineNotifier.selectedClipId : -1
    property var clipEffectList: selectedClipId >= 0 && timelineNotifier && timelineNotifier.trackCount >= 0 ? timelineNotifier.clipEffects(selectedClipId) : []

    function effectValue(effectType) {
        for (var i = 0; i < clipEffectList.length; i++) {
            if (clipEffectList[i].type === effectType) return clipEffectList[i].value;
        }
        return 50;
    }
    function effectEnabled(effectType) {
        for (var i = 0; i < clipEffectList.length; i++) {
            if (clipEffectList[i].type === effectType) return clipEffectList[i].enabled;
        }
        return false;
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Info when no clip selected
        RowLayout {
            visible: selectedClipId < 0
            Layout.margins: 12
            spacing: 6
            Label { text: "\u2139"; font.pixelSize: 14; color: "#6B6B88" }
            Label { text: "Select a clip to apply effects"; font.pixelSize: 13; color: "#6B6B88" }
        }

        // Scrollable effect list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: categories.length

            delegate: Column {
                width: ListView.view.width
                property var cat: categories[index]

                // Category header
                Label {
                    text: cat.label
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: "#8888A0"
                    leftPadding: 12
                    topPadding: 12
                    bottomPadding: 4
                    font.letterSpacing: 0.5
                }

                // Effects in this category
                Repeater {
                    model: cat.effects
                    delegate: Rectangle {
                        property int effectType: modelData
                        width: parent.width
                        height: 44
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            // Toggle
                            ToolButton {
                                width: 28; height: 28
                                checkable: true
                                checked: effectEnabled(effectType)
                                icon.name: "visibility"
                                icon.width: 16; icon.height: 16
                                onToggled: timelineNotifier.toggleEffect(selectedClipId, effectType)
                            }

                            Label {
                                text: effectNames[effectType] || "Effect"
                                font.pixelSize: 13
                                color: "#E0E0F0"
                                Layout.preferredWidth: 80
                            }

                            Slider {
                                id: effectSlider
                                Layout.fillWidth: true
                                from: 0; to: 100
                                value: effectValue(effectType)
                                onMoved: timelineNotifier.updateEffect(selectedClipId, effectType, value)
                            }

                            Label {
                                text: Math.round(effectSlider.value)
                                font.pixelSize: 12
                                font.family: "monospace"
                                color: "#B0B0C8"
                                Layout.preferredWidth: 30
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }
                }
            }
        }
    }
}
