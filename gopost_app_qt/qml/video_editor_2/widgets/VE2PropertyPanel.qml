import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * VE2PropertyPanel — tabbed sidebar for clip inspection and editing.
 *
 * Tabs: Inspector | Effects | Color | Audio | Transform | Speed | Markers
 * All controls wired to timelineNotifier Q_INVOKABLE methods.
 * Cross-platform: uses standard Qt Quick Controls 2 components.
 */
Item {
    id: root

    readonly property int selClip: timelineNotifier ? timelineNotifier.selectedClipId : -1
    readonly property bool hasClip: selClip >= 0
    readonly property var clipMap: hasClip && timelineNotifier ? timelineNotifier.selectedClip : ({})

    Rectangle { anchors.fill: parent; color: "#0D0D1A" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab bar
        TabBar {
            id: tabBar
            Layout.fillWidth: true

            background: Rectangle {
                color: "#10102A"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#1E1E38" }
            }

            TabButton { text: "\uD83D\uDD0D"; ToolTip.text: "Inspector"; ToolTip.visible: hovered; width: implicitWidth }
            TabButton { text: "\u2728"; ToolTip.text: "Effects"; ToolTip.visible: hovered; width: implicitWidth }
            TabButton { text: "\uD83C\uDFA8"; ToolTip.text: "Color"; ToolTip.visible: hovered; width: implicitWidth }
            TabButton { text: "\uD83C\uDFB5"; ToolTip.text: "Audio"; ToolTip.visible: hovered; width: implicitWidth }
            TabButton { text: "\u2B1C"; ToolTip.text: "Transform"; ToolTip.visible: hovered; width: implicitWidth }
            TabButton { text: "\u23E9"; ToolTip.text: "Speed"; ToolTip.visible: hovered; width: implicitWidth }
            TabButton { text: "\uD83D\uDCCD"; ToolTip.text: "Markers"; ToolTip.visible: hovered; width: implicitWidth }
        }

        // No-clip placeholder
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !hasClip && tabBar.currentIndex < 6  // markers tab doesn't need a clip

            ColumnLayout {
                anchors.centerIn: parent; spacing: 8
                Label { text: "\uD83C\uDFAC"; font.pixelSize: 32; color: "#303050"; Layout.alignment: Qt.AlignHCenter }
                Label { text: "Select a clip to edit"; font.pixelSize: 13; color: "#505068"; Layout.alignment: Qt.AlignHCenter }
            }
        }

        // Tab content
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
            visible: hasClip || tabBar.currentIndex === 6

            // ---- 0: Inspector ----
            Flickable {
                contentHeight: inspCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: inspCol
                    width: parent.width - 16
                    x: 8; y: 8
                    spacing: 10

                    SectionHeader { text: "Clip Info" }

                    PropRow { label: "Name"; value: clipMap.displayName || "" }
                    PropRow { label: "Type"; value: internal.sourceTypeName(clipMap.sourceType) }
                    PropRow { label: "Duration"; value: (clipMap.duration || 0).toFixed(2) + "s" }
                    PropRow { label: "Timeline In"; value: (clipMap.timelineIn || 0).toFixed(2) + "s" }

                    SectionHeader { text: "Actions" }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Split"; onClicked: { if (timelineNotifier) timelineNotifier.splitClipAtPlayhead() } }
                        ActionBtn { text: "Duplicate"; onClicked: { if (timelineNotifier) timelineNotifier.duplicateClip(selClip) } }
                        ActionBtn { text: "Delete"; accent: "#EF5350"; onClicked: { if (timelineNotifier) timelineNotifier.removeClip(selClip) } }
                    }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Ripple Del"; onClicked: { if (timelineNotifier) timelineNotifier.rippleDelete(selClip) } }
                        ActionBtn { text: "Freeze"; onClicked: { if (timelineNotifier) timelineNotifier.freezeFrame(selClip) } }
                        ActionBtn { text: "Reverse"; onClicked: { if (timelineNotifier) timelineNotifier.reverseClip(selClip) } }
                    }

                    SectionHeader { text: "Opacity & Blend" }

                    PropSlider {
                        label: "Opacity"
                        from: 0; to: 1; value: timelineNotifier && hasClip ? timelineNotifier.clipOpacity(selClip) : 1.0
                        onUserMoved: val => { if (timelineNotifier) timelineNotifier.setClipOpacity(selClip, val) }
                    }
                }
            }

            // ---- 1: Effects ----
            Flickable {
                contentHeight: fxCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: fxCol
                    width: parent.width - 16; x: 8; y: 8
                    spacing: 8

                    SectionHeader { text: "Effects" }

                    // Effect type grid
                    GridLayout {
                        columns: 2; columnSpacing: 6; rowSpacing: 6; Layout.fillWidth: true

                        Repeater {
                            model: [
                                { id: 0, label: "Blur", icon: "\u25CE", color: "#AB47BC" },
                                { id: 1, label: "Sharpen", icon: "\u2736", color: "#FF7043" },
                                { id: 2, label: "Vignette", icon: "\u25C9", color: "#26C6DA" },
                                { id: 3, label: "Film Grain", icon: "\u2591", color: "#FFCA28" },
                                { id: 4, label: "Chromatic", icon: "\u25C8", color: "#42A5F5" },
                                { id: 5, label: "Pixelate", icon: "\u25A3", color: "#66BB6A" },
                                { id: 6, label: "Glitch", icon: "\u26A1", color: "#EF5350" },
                                { id: 7, label: "Sepia", icon: "\u263C", color: "#8D6E63" }
                            ]
                            delegate: Rectangle {
                                Layout.fillWidth: true; Layout.preferredHeight: 40; radius: 6
                                color: Qt.rgba(0.4, 0.3, 0.8, 0.12); border.color: Qt.rgba(0.4, 0.3, 0.8, 0.3)

                                RowLayout {
                                    anchors.centerIn: parent; spacing: 6
                                    Label { text: modelData.icon; font.pixelSize: 16; color: modelData.color }
                                    Label { text: modelData.label; font.pixelSize: 11; color: "#D0D0E8" }
                                }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (timelineNotifier && hasClip)
                                            timelineNotifier.addEffect(selClip, modelData.id, 0.5)
                                    }
                                }
                            }
                        }
                    }

                    SectionHeader { text: "Preset Filters" }

                    Flow {
                        Layout.fillWidth: true; spacing: 4

                        Repeater {
                            model: [
                                { id: 1, label: "Cinematic" }, { id: 2, label: "Vintage" },
                                { id: 3, label: "Warm Sunset" }, { id: 4, label: "Cool Blue" },
                                { id: 5, label: "Desaturated" }, { id: 6, label: "High Contrast" },
                                { id: 7, label: "Film Noir" }, { id: 8, label: "Dreamy" },
                                { id: 9, label: "Retro 70s" }, { id: 10, label: "Polaroid" },
                                { id: 11, label: "HDR" }, { id: 12, label: "Matte" }
                            ]
                            delegate: Rectangle {
                                width: presetLbl.implicitWidth + 16; height: 28; radius: 4
                                color: Qt.rgba(0.671, 0.294, 0.737, 0.1)
                                border.color: Qt.rgba(0.671, 0.294, 0.737, 0.3)

                                Label { id: presetLbl; anchors.centerIn: parent; text: modelData.label; font.pixelSize: 11; color: "#D0D0E8" }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applyPresetFilter(selClip, modelData.id) }
                                }
                            }
                        }
                    }

                    ActionBtn { text: "Clear All Effects"; accent: "#EF5350"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.clearEffects(selClip) } }
                }
            }

            // ---- 2: Color Grading ----
            Flickable {
                contentHeight: colorCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: colorCol
                    width: parent.width - 16; x: 8; y: 8
                    spacing: 8

                    SectionHeader { text: "Color Grading" }

                    Repeater {
                        model: [
                            { prop: "brightness", label: "Brightness", from: -1, to: 1 },
                            { prop: "contrast", label: "Contrast", from: -1, to: 1 },
                            { prop: "saturation", label: "Saturation", from: -1, to: 1 },
                            { prop: "temperature", label: "Temperature", from: -1, to: 1 },
                            { prop: "tint", label: "Tint", from: -1, to: 1 },
                            { prop: "highlights", label: "Highlights", from: -1, to: 1 },
                            { prop: "shadows", label: "Shadows", from: -1, to: 1 },
                            { prop: "vibrance", label: "Vibrance", from: -1, to: 1 }
                        ]
                        delegate: PropSlider {
                            label: modelData.label
                            from: modelData.from; to: modelData.to
                            value: {
                                if (!timelineNotifier || !hasClip) return 0
                                var cg = timelineNotifier.clipColorGrading(selClip)
                                return cg[modelData.prop] !== undefined ? cg[modelData.prop] : 0
                            }
                            onUserMoved: val => {
                                if (timelineNotifier && hasClip)
                                    timelineNotifier.setColorGradingProperty(selClip, modelData.prop, val)
                            }
                        }
                    }

                    ActionBtn { text: "Reset Color"; accent: "#EF5350"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.resetColorGrading(selClip) } }
                }
            }

            // ---- 3: Audio ----
            Flickable {
                contentHeight: audioCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: audioCol
                    width: parent.width - 16; x: 8; y: 8
                    spacing: 8

                    SectionHeader { text: "Audio Controls" }

                    PropSlider {
                        label: "Volume"
                        from: 0; to: 2
                        value: timelineNotifier ? timelineNotifier.clipVolume : 1.0
                        onUserMoved: val => { if (timelineNotifier && hasClip) timelineNotifier.setClipVolume(selClip, val) }
                    }
                    PropSlider {
                        label: "Pan"
                        from: -1; to: 1
                        value: timelineNotifier ? timelineNotifier.clipPan : 0.0
                        onUserMoved: val => { if (timelineNotifier && hasClip) timelineNotifier.setClipPan(selClip, val) }
                    }
                    PropSlider {
                        label: "Fade In"
                        from: 0; to: 5
                        value: timelineNotifier ? timelineNotifier.clipFadeIn : 0.0
                        onUserMoved: val => { if (timelineNotifier && hasClip) timelineNotifier.setClipFadeIn(selClip, val) }
                    }
                    PropSlider {
                        label: "Fade Out"
                        from: 0; to: 5
                        value: timelineNotifier ? timelineNotifier.clipFadeOut : 0.0
                        onUserMoved: val => { if (timelineNotifier && hasClip) timelineNotifier.setClipFadeOut(selClip, val) }
                    }

                    RowLayout {
                        spacing: 8
                        Switch {
                            checked: timelineNotifier ? timelineNotifier.clipIsMuted : false
                            onToggled: { if (timelineNotifier && hasClip) timelineNotifier.setClipMuted(selClip, checked) }
                        }
                        Label { text: "Mute"; font.pixelSize: 12; color: "#B0B0C8" }
                    }
                }
            }

            // ---- 4: Transform ----
            Flickable {
                contentHeight: txCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: txCol
                    width: parent.width - 16; x: 8; y: 8
                    spacing: 8

                    SectionHeader { text: "Transform" }

                    Repeater {
                        model: [
                            { prop: 0, label: "Position X", from: -1920, to: 1920 },
                            { prop: 1, label: "Position Y", from: -1080, to: 1080 },
                            { prop: 2, label: "Scale X", from: 0, to: 4 },
                            { prop: 3, label: "Scale Y", from: 0, to: 4 },
                            { prop: 4, label: "Rotation", from: -360, to: 360 },
                            { prop: 5, label: "Anchor X", from: 0, to: 1 },
                            { prop: 6, label: "Anchor Y", from: 0, to: 1 }
                        ]
                        delegate: PropSlider {
                            label: modelData.label
                            from: modelData.from; to: modelData.to
                            value: 0
                            onUserMoved: val => {
                                if (timelineNotifier && hasClip)
                                    timelineNotifier.setKeyframe(selClip, modelData.prop, val)
                            }
                        }
                    }

                    SectionHeader { text: "Ken Burns" }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Zoom In"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applyKenBurns(selClip, 1.0, 1.3) } }
                        ActionBtn { text: "Zoom Out"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applyKenBurns(selClip, 1.3, 1.0) } }
                    }
                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Pan L→R"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applyKenBurnsPan(selClip, -0.1, 0, 0.1, 0) } }
                        ActionBtn { text: "Pan R→L"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applyKenBurnsPan(selClip, 0.1, 0, -0.1, 0) } }
                    }

                    ActionBtn { text: "Reset Transform"; accent: "#EF5350"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.resetTransform(selClip) } }
                }
            }

            // ---- 5: Speed ----
            Flickable {
                contentHeight: spdCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: spdCol
                    width: parent.width - 16; x: 8; y: 8
                    spacing: 8

                    SectionHeader { text: "Speed Control" }

                    PropSlider {
                        label: "Speed"
                        from: 0.1; to: 8.0
                        value: timelineNotifier && hasClip ? timelineNotifier.clipSpeed(selClip) : 1.0
                        onUserMoved: val => { if (timelineNotifier && hasClip) timelineNotifier.setClipSpeed(selClip, val) }
                    }

                    // Presets
                    Flow {
                        Layout.fillWidth: true; spacing: 4

                        Repeater {
                            model: [0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 4.0, 8.0]
                            delegate: Rectangle {
                                width: spdLbl.implicitWidth + 16; height: 28; radius: 4
                                color: Qt.rgba(0.424, 0.388, 1.0, 0.1)
                                border.color: Qt.rgba(0.424, 0.388, 1.0, 0.3)

                                Label { id: spdLbl; anchors.centerIn: parent; text: modelData + "x"; font.pixelSize: 11; color: "#D0D0E8" }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: { if (timelineNotifier && hasClip) timelineNotifier.setClipSpeed(selClip, modelData) }
                                }
                            }
                        }
                    }

                    SectionHeader { text: "Speed Ramp Presets" }

                    Repeater {
                        model: [
                            { idx: 0, label: "Ease In" },
                            { idx: 1, label: "Ease Out" },
                            { idx: 2, label: "Ease In/Out" },
                            { idx: 3, label: "Speed Burst" }
                        ]
                        delegate: ActionBtn {
                            text: modelData.label
                            onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applySpeedRamp(selClip, modelData.idx) }
                        }
                    }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Reverse"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.reverseClip(selClip) } }
                        ActionBtn { text: "Freeze Frame"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.freezeFrame(selClip) } }
                    }
                }
            }

            // ---- 6: Markers ----
            Flickable {
                contentHeight: mkrCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: mkrCol
                    width: parent.width - 16; x: 8; y: 8
                    spacing: 8

                    SectionHeader { text: "Markers" }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Add Marker"; onClicked: { if (timelineNotifier) timelineNotifier.addMarker() } }
                        ActionBtn { text: "Add Chapter"; onClicked: { if (timelineNotifier) timelineNotifier.addMarkerWithType(1, "Chapter") } }
                    }

                    Label {
                        text: "Marker count: " + (timelineNotifier ? timelineNotifier.markerCount : 0)
                        font.pixelSize: 12; color: "#8888A0"
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(300, count * 48 + 8)
                        clip: true; spacing: 4

                        model: timelineNotifier ? timelineNotifier.markers : []

                        delegate: Rectangle {
                            readonly property var marker: modelData || ({})
                            width: ListView.view.width; height: 44; radius: 4
                            color: "#1A1A34"

                            RowLayout {
                                anchors.fill: parent; anchors.margins: 6; spacing: 6

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: marker.color || "#6C63FF"
                                }

                                ColumnLayout {
                                    spacing: 1; Layout.fillWidth: true
                                    Label { text: marker.label || "Marker"; font.pixelSize: 11; color: "#D0D0E8"; elide: Text.ElideRight; Layout.fillWidth: true }
                                    Label { text: internal.formatTimecode(marker.positionSeconds); font.pixelSize: 9; font.family: "monospace"; color: "#6B6B88" }
                                }

                                ToolButton {
                                    width: 22; height: 22
                                    contentItem: Label { text: "\u27A4"; font.pixelSize: 10; color: "#8888A0"; anchors.centerIn: parent }
                                    onClicked: { if (timelineNotifier) timelineNotifier.navigateToMarker(marker.id) }
                                    ToolTip.text: "Go to marker"; ToolTip.visible: hovered
                                }

                                ToolButton {
                                    width: 22; height: 22
                                    contentItem: Label { text: "\u2716"; font.pixelSize: 10; color: "#EF5350"; anchors.centerIn: parent }
                                    onClicked: { if (timelineNotifier) timelineNotifier.removeMarker(marker.id) }
                                    ToolTip.text: "Remove marker"; ToolTip.visible: hovered
                                }
                            }
                        }
                    }

                    SectionHeader { text: "Navigation" }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "\u25C0 Prev"; onClicked: { if (timelineNotifier) timelineNotifier.navigateToPreviousMarker() } }
                        ActionBtn { text: "Next \u25B6"; onClicked: { if (timelineNotifier) timelineNotifier.navigateToNextMarker() } }
                    }
                }
            }
        }
    }

    // ================================================================
    // Reusable components
    // ================================================================
    component SectionHeader: Rectangle {
        property string text: ""
        Layout.fillWidth: true; height: 24; radius: 2
        color: "#14142B"
        Label { anchors.verticalCenter: parent.verticalCenter; x: 6; text: parent.text; font.pixelSize: 11; font.weight: Font.DemiBold; color: "#8888A0" }
    }

    component PropRow: RowLayout {
        property string label: ""
        property string value: ""
        Layout.fillWidth: true; spacing: 4
        Label { text: parent.label; font.pixelSize: 11; color: "#6B6B88"; Layout.preferredWidth: 80 }
        Label { text: parent.value; font.pixelSize: 11; color: "#D0D0E8"; Layout.fillWidth: true; elide: Text.ElideRight }
    }

    component PropSlider: ColumnLayout {
        property string label: ""
        property real from: 0
        property real to: 1
        property real value: 0
        signal userMoved(real val)

        Layout.fillWidth: true; spacing: 2

        RowLayout {
            spacing: 4
            Label { text: parent.parent.label; font.pixelSize: 11; color: "#8888A0"; Layout.fillWidth: true }
            Label { text: sldr.value.toFixed(2); font.pixelSize: 10; font.family: "monospace"; color: "#6B6B88" }
        }

        Slider {
            id: sldr
            Layout.fillWidth: true
            from: parent.from; to: parent.to; value: parent.value
            onMoved: parent.userMoved(value)

            background: Rectangle {
                x: sldr.leftPadding; y: sldr.topPadding + sldr.availableHeight / 2 - 1.5
                width: sldr.availableWidth; height: 3; radius: 1.5; color: "#1A1A34"
                Rectangle { width: sldr.visualPosition * parent.width; height: parent.height; radius: 1.5; color: "#6C63FF" }
            }
        }
    }

    component ActionBtn: Rectangle {
        property string text: ""
        property string accent: "#6C63FF"
        signal clicked()
        Layout.fillWidth: true; height: 30; radius: 4
        color: Qt.rgba(accent === "#EF5350" ? 0.94 : 0.424, accent === "#EF5350" ? 0.32 : 0.388, accent === "#EF5350" ? 0.31 : 1.0, 0.1)
        border.color: Qt.rgba(accent === "#EF5350" ? 0.94 : 0.424, accent === "#EF5350" ? 0.32 : 0.388, accent === "#EF5350" ? 0.31 : 1.0, 0.3)
        Label { anchors.centerIn: parent; text: parent.text; font.pixelSize: 11; font.weight: Font.Medium; color: parent.accent }
        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: parent.clicked() }
    }

    // ================================================================
    // Internal helpers
    // ================================================================
    QtObject {
        id: internal

        function sourceTypeName(t) {
            switch (t) {
            case 0: return "Video"
            case 1: return "Image"
            case 2: return "Title"
            case 3: return "Color"
            case 4: return "Adjustment"
            default: return "Unknown"
            }
        }

        function formatTimecode(s) {
            if (s === undefined || s === null || isNaN(s)) return "00:00"
            var m = Math.floor(s / 60)
            var sec = Math.floor(s % 60)
            return String(m).padStart(2, '0') + ":" + String(sec).padStart(2, '0')
        }
    }
}
