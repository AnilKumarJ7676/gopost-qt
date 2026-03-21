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

    // When >= 0, controls the active tab externally and hides the internal tab bar.
    // Used by VE2EditorSidebar to drive tab selection from the icon rail.
    property int forceTabIndex: -1

    readonly property int selClip: timelineNotifier ? timelineNotifier.selectedClipId : -1
    readonly property bool hasClip: selClip >= 0
    readonly property var clipMap: hasClip && timelineNotifier ? timelineNotifier.selectedClip : ({})
    readonly property int effectiveTab: forceTabIndex >= 0 ? forceTabIndex : (tabBar ? tabBar.currentIndex : 0)

    // Generation counter — bumped on every clip/track mutation. Used to force
    // re-evaluation of Q_INVOKABLE property reads (clipEffects, clipSpeed, etc.)
    readonly property int _gen: timelineNotifier ? timelineNotifier.trackGeneration : 0

    Rectangle { anchors.fill: parent; color: "#0D0D1A" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab bar (hidden when controlled externally by VE2EditorSidebar)
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            visible: root.forceTabIndex < 0

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
            TabButton { text: "\uD83D\uDD17"; ToolTip.text: "Transitions"; ToolTip.visible: hovered; width: implicitWidth }
        }

        // No-clip placeholder
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !hasClip && effectiveTab !== 6  // markers tab doesn't need a clip

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
            currentIndex: effectiveTab
            visible: hasClip || effectiveTab === 6  // markers don't need clip

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

                    // Editable clip name
                    RowLayout {
                        Layout.fillWidth: true; spacing: 4
                        Label { text: "Name"; font.pixelSize: 11; color: "#6B6B88"; Layout.preferredWidth: 80 }
                        TextField {
                            Layout.fillWidth: true; Layout.preferredHeight: 26
                            text: clipMap.displayName || ""
                            font.pixelSize: 11; color: "#D0D0E8"
                            placeholderText: "Clip name"
                            placeholderTextColor: "#505068"
                            background: Rectangle { radius: 4; color: "#1A1A34"; border.color: "#303050" }
                            onEditingFinished: {
                                if (timelineNotifier && hasClip && text.trim().length > 0)
                                    timelineNotifier.updateClipDisplayName(selClip, text.trim())
                            }
                        }
                    }

                    PropRow { label: "Type"; value: internal.sourceTypeName(clipMap.sourceType) }
                    PropRow { label: "Duration"; value: (clipMap.duration || 0).toFixed(2) + "s" }
                    PropRow { label: "Timeline In"; value: (clipMap.timelineIn || 0).toFixed(2) + "s" }

                    // Clip color tag
                    SectionHeader { text: "Color Tag" }

                    Flow {
                        Layout.fillWidth: true; spacing: 4

                        Repeater {
                            model: timelineNotifier ? timelineNotifier.clipColorPresetCount() : 0
                            delegate: Rectangle {
                                width: 24; height: 24; radius: 4
                                color: timelineNotifier ? timelineNotifier.clipColorPresetHex(index) : "#808080"
                                border.color: Qt.darker(color, 1.3); border.width: 1

                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (timelineNotifier && hasClip)
                                            timelineNotifier.setClipColorTag(selClip, timelineNotifier.clipColorPresetHex(index))
                                    }
                                }
                                ToolTip.visible: colorTagHov.hovered
                                ToolTip.text: timelineNotifier ? timelineNotifier.clipColorPresetName(index) : ""
                                HoverHandler { id: colorTagHov }
                            }
                        }

                        // Clear color tag
                        Rectangle {
                            width: 24; height: 24; radius: 4
                            color: "transparent"; border.color: "#353550"; border.width: 1

                            Label { anchors.centerIn: parent; text: "\u2715"; font.pixelSize: 10; color: "#6B6B88" }
                            MouseArea {
                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: { if (timelineNotifier && hasClip) timelineNotifier.clearClipColorTag(selClip) }
                            }
                            ToolTip.visible: clearTagHov.hovered; ToolTip.text: "Clear color tag"
                            HoverHandler { id: clearTagHov }
                        }
                    }

                    // Custom label
                    RowLayout {
                        Layout.fillWidth: true; spacing: 4
                        Label { text: "Label"; font.pixelSize: 11; color: "#6B6B88"; Layout.preferredWidth: 80 }
                        TextField {
                            Layout.fillWidth: true; Layout.preferredHeight: 26
                            text: clipMap.customLabel || ""
                            font.pixelSize: 11; color: "#D0D0E8"
                            placeholderText: "Custom label"
                            placeholderTextColor: "#505068"
                            background: Rectangle { radius: 4; color: "#1A1A34"; border.color: "#303050" }
                            onEditingFinished: {
                                if (timelineNotifier && hasClip)
                                    timelineNotifier.setClipCustomLabel(selClip, text)
                            }
                        }
                    }

                    SectionHeader { text: "Actions" }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Split"; onClicked: { if (timelineNotifier) timelineNotifier.splitClipAtPlayhead() } }
                        ActionBtn { text: "Split All"; onClicked: { if (timelineNotifier) timelineNotifier.splitAllAtPlayhead() } }
                        ActionBtn { text: "Duplicate"; onClicked: { if (timelineNotifier) timelineNotifier.duplicateClip(selClip) } }
                    }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Delete"; accent: "#EF5350"; onClicked: { if (timelineNotifier) timelineNotifier.removeClip(selClip) } }
                        ActionBtn { text: "Ripple Del"; accent: "#EF5350"; onClicked: { if (timelineNotifier) timelineNotifier.rippleDelete(selClip) } }
                    }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Freeze"; onClicked: { if (timelineNotifier) timelineNotifier.freezeFrame(selClip) } }
                        ActionBtn { text: "Reverse"; onClicked: { if (timelineNotifier) timelineNotifier.reverseClip(selClip) } }
                        ActionBtn { text: "Rate Stretch"; onClicked: {
                            if (timelineNotifier && hasClip) {
                                var dur = clipMap.duration || 5.0
                                timelineNotifier.rateStretch(selClip, dur * 1.5)
                            }
                        }}
                    }

                    // Clip linking
                    SectionHeader { text: "Linking" }

                    RowLayout {
                        spacing: 6
                        Label {
                            text: {
                                var g = _gen
                                return timelineNotifier && hasClip && timelineNotifier.isClipLinked(selClip)
                                    ? "Linked to clip #" + timelineNotifier.linkedClipId(selClip)
                                    : "Not linked"
                            }
                            font.pixelSize: 11; color: "#8888A0"; Layout.fillWidth: true
                        }
                        Rectangle {
                            visible: { var g = _gen; return timelineNotifier && hasClip && timelineNotifier.isClipLinked(selClip) }
                            width: unlinkLbl.implicitWidth + 12; height: 24; radius: 4
                            color: Qt.rgba(0.94, 0.32, 0.31, 0.1); border.color: Qt.rgba(0.94, 0.32, 0.31, 0.3)
                            Label { id: unlinkLbl; anchors.centerIn: parent; text: "Unlink"; font.pixelSize: 10; color: "#EF5350" }
                            MouseArea {
                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: { if (timelineNotifier && hasClip) timelineNotifier.unlinkClip(selClip) }
                            }
                        }
                    }

                    // Proxy
                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Generate Proxy"; onClicked: {
                            if (timelineNotifier && hasClip) timelineNotifier.generateProxyForClip(selClip)
                        }}
                    }

                    SectionHeader { text: "Opacity & Blend" }

                    PropSlider {
                        label: "Opacity"
                        from: 0; to: 1; value: { var g = _gen; return timelineNotifier && hasClip ? timelineNotifier.clipOpacity(selClip) : 1.0 }
                        onUserMoved: val => { if (timelineNotifier) timelineNotifier.setClipOpacity(selClip, val) }
                    }

                    // Blend mode selector
                    RowLayout {
                        Layout.fillWidth: true; spacing: 4
                        Label { text: "Blend"; font.pixelSize: 11; color: "#6B6B88"; Layout.preferredWidth: 80 }
                        ComboBox {
                            Layout.fillWidth: true; Layout.preferredHeight: 28
                            model: ["Normal", "Multiply", "Screen", "Overlay", "Darken",
                                    "Lighten", "Color Dodge", "Color Burn", "Hard Light",
                                    "Soft Light", "Difference", "Exclusion", "Hue",
                                    "Saturation", "Color", "Luminosity"]
                            currentIndex: 0
                            font.pixelSize: 11
                            onActivated: index => {
                                if (timelineNotifier && hasClip)
                                    timelineNotifier.setClipBlendMode(selClip, index)
                            }
                        }
                    }
                }
            }

            // ---- 1: Effects ----
            Flickable {
                id: fxFlickable
                contentHeight: fxCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                // Live effect state — bound to clipEffectsList Q_PROPERTY (NOTIFY selectionChanged)
                // Falls back to _gen-based refresh for track mutations
                property var fxList: {
                    if (!hasClip || !timelineNotifier) return []
                    var g = _gen  // also depend on trackGeneration for add/remove/toggle
                    return timelineNotifier.clipEffectsList
                }

                function fxValue(effectType) {
                    for (var i = 0; i < fxList.length; i++) {
                        if (fxList[i].type === effectType) return fxList[i].value
                    }
                    return 50
                }
                function fxEnabled(effectType) {
                    for (var i = 0; i < fxList.length; i++) {
                        if (fxList[i].type === effectType) return fxList[i].enabled
                    }
                    return false
                }
                function fxActive(effectType) {
                    for (var i = 0; i < fxList.length; i++) {
                        if (fxList[i].type === effectType) return true
                    }
                    return false
                }
                function activeCount() {
                    var n = 0
                    for (var i = 0; i < fxList.length; i++) {
                        if (fxList[i].enabled) n++
                    }
                    return n
                }

                // All 22 effects by category, matching C++ EffectType enum + effectTypeInfo ranges
                readonly property var fxCategories: [
                    { label: "COLOR & TONE", color: "#FF7043", effects: [
                        { type: 0,  name: "Brightness",  min: -100, max: 100, def: 0 },
                        { type: 1,  name: "Contrast",    min: -100, max: 100, def: 0 },
                        { type: 2,  name: "Saturation",  min: -100, max: 100, def: 0 },
                        { type: 3,  name: "Exposure",    min: -2.0, max: 2.0, def: 0 },
                        { type: 4,  name: "Temperature", min: -100, max: 100, def: 0 },
                        { type: 5,  name: "Tint",        min: -100, max: 100, def: 0 },
                        { type: 6,  name: "Highlights",  min: -100, max: 100, def: 0 },
                        { type: 7,  name: "Shadows",     min: -100, max: 100, def: 0 },
                        { type: 8,  name: "Vibrance",    min: -100, max: 100, def: 0 },
                        { type: 9,  name: "Hue Rotate",  min: -180, max: 180, def: 0 }
                    ]},
                    { label: "BLUR & SHARPEN", color: "#26C6DA", effects: [
                        { type: 10, name: "Gaussian Blur", min: 0, max: 100, def: 25 },
                        { type: 11, name: "Radial Blur",   min: 0, max: 100, def: 25 },
                        { type: 12, name: "Tilt Shift",    min: 0, max: 100, def: 50 },
                        { type: 13, name: "Sharpen",       min: 0, max: 100, def: 50 }
                    ]},
                    { label: "DISTORT", color: "#EF5350", effects: [
                        { type: 14, name: "Pixelate",   min: 1,  max: 50,  def: 8 },
                        { type: 15, name: "Glitch",     min: 0,  max: 100, def: 30 },
                        { type: 16, name: "Chromatic",  min: 0,  max: 100, def: 25 }
                    ]},
                    { label: "STYLIZE", color: "#AB47BC", effects: [
                        { type: 17, name: "Vignette",   min: 0, max: 100, def: 40 },
                        { type: 18, name: "Film Grain",  min: 0, max: 100, def: 30 },
                        { type: 19, name: "Sepia",      min: 0, max: 100, def: 50 },
                        { type: 20, name: "Invert",     min: 0, max: 100, def: 100 },
                        { type: 21, name: "Posterize",  min: 2, max: 16,  def: 8 }
                    ]}
                ]

                ColumnLayout {
                    id: fxCol
                    width: parent.width - 16; x: 8; y: 8
                    spacing: 6

                    // Active effects summary + copy/paste
                    Rectangle {
                        Layout.fillWidth: true; height: 30; radius: 4
                        color: "#14142B"

                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                            Label {
                                text: "\u2728"
                                font.pixelSize: 14; color: "#6C63FF"
                            }
                            Label {
                                property int cnt: {
                                    var n = 0; var list = fxFlickable.fxList
                                    for (var i = 0; i < list.length; i++) if (list[i].enabled) n++
                                    return n
                                }
                                text: "Active Effects: " + cnt
                                font.pixelSize: 11; font.weight: Font.DemiBold; color: "#8888A0"
                                Layout.fillWidth: true
                            }
                            // Copy effects
                            Rectangle {
                                visible: fxFlickable.fxList.length > 0
                                width: copyLbl.implicitWidth + 12; height: 22; radius: 4
                                color: Qt.rgba(0.424, 0.388, 1.0, 0.15)
                                border.color: Qt.rgba(0.424, 0.388, 1.0, 0.3)
                                Label { id: copyLbl; anchors.centerIn: parent; text: "Copy"; font.pixelSize: 10; color: "#6C63FF" }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.copyEffects(selClip) } }
                            }
                            // Paste effects
                            Rectangle {
                                visible: timelineNotifier ? timelineNotifier.hasClipboardEffects() : false
                                width: pasteLbl.implicitWidth + 12; height: 22; radius: 4
                                color: Qt.rgba(0.424, 0.388, 1.0, 0.15)
                                border.color: Qt.rgba(0.424, 0.388, 1.0, 0.3)
                                Label { id: pasteLbl; anchors.centerIn: parent; text: "Paste"; font.pixelSize: 10; color: "#6C63FF" }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.pasteEffects(selClip) } }
                            }
                            // Clear all
                            Rectangle {
                                visible: fxFlickable.fxList.length > 0
                                width: clearLbl.implicitWidth + 12; height: 22; radius: 4
                                color: Qt.rgba(0.94, 0.32, 0.31, 0.15)
                                border.color: Qt.rgba(0.94, 0.32, 0.31, 0.3)
                                Label { id: clearLbl; anchors.centerIn: parent; text: "Clear All"; font.pixelSize: 10; color: "#EF5350" }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.clearEffects(selClip) } }
                            }
                        }
                    }

                    // Effects Stack — shows active effects in processing order with reorder controls
                    SectionHeader { text: "Effects Stack (Processing Order)"; visible: fxFlickable.fxList.length > 0 }

                    Repeater {
                        model: fxFlickable.fxList.length > 0 ? fxFlickable.fxList : []
                        delegate: Rectangle {
                            Layout.fillWidth: true; height: 28; radius: 4
                            color: modelData.enabled ? Qt.rgba(0.424, 0.388, 1.0, 0.06) : Qt.rgba(0.3, 0.3, 0.3, 0.06)
                            border.color: modelData.enabled ? "#6C63FF" : "#303050"
                            border.width: 0.5

                            property int fxIndex: index
                            property var fxNames: ["Brightness","Contrast","Saturation","Exposure","Temperature","Tint","Highlights","Shadows","Vibrance","Hue Rotate","Gaussian Blur","Radial Blur","Tilt Shift","Sharpen","Pixelate","Glitch","Chromatic","Vignette","Film Grain","Sepia","Invert","Posterize"]

                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 3

                                // Stack index
                                Label {
                                    text: (fxIndex + 1) + "."
                                    font.pixelSize: 9; font.family: "monospace"; color: "#505068"
                                    Layout.preferredWidth: 16
                                }

                                // Enable/disable toggle
                                Rectangle {
                                    width: 16; height: 16; radius: 3
                                    color: modelData.enabled ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "transparent"
                                    border.color: modelData.enabled ? "#6C63FF" : "#404060"
                                    Label { anchors.centerIn: parent; text: modelData.enabled ? "\u2713" : "\u2012"; font.pixelSize: 10; color: modelData.enabled ? "#6C63FF" : "#505068" }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.toggleEffect(selClip, modelData.type) } }
                                }

                                // Effect name
                                Label {
                                    text: modelData.type < fxNames.length ? fxNames[modelData.type] : "Effect"
                                    font.pixelSize: 10; color: modelData.enabled ? "#D0D0E8" : "#6B6B88"
                                    Layout.fillWidth: true; elide: Text.ElideRight
                                }

                                // Value readout
                                Label {
                                    text: Math.round(modelData.value)
                                    font.pixelSize: 9; font.family: "monospace"; color: "#8888A0"
                                    Layout.preferredWidth: 24; horizontalAlignment: Text.AlignRight
                                }

                                // Move up
                                Rectangle {
                                    visible: fxIndex > 0
                                    width: 16; height: 16; radius: 3; color: "transparent"; border.color: "#353550"
                                    Label { anchors.centerIn: parent; text: "\u25B2"; font.pixelSize: 8; color: "#8888A0" }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.reorderEffect(selClip, fxIndex, fxIndex - 1) } }
                                    ToolTip.visible: upHov.hovered; ToolTip.text: "Move up in stack"
                                    HoverHandler { id: upHov }
                                }

                                // Move down
                                Rectangle {
                                    visible: fxIndex < fxFlickable.fxList.length - 1
                                    width: 16; height: 16; radius: 3; color: "transparent"; border.color: "#353550"
                                    Label { anchors.centerIn: parent; text: "\u25BC"; font.pixelSize: 8; color: "#8888A0" }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.reorderEffect(selClip, fxIndex, fxIndex + 1) } }
                                    ToolTip.visible: downHov.hovered; ToolTip.text: "Move down in stack"
                                    HoverHandler { id: downHov }
                                }

                                // Delete
                                Rectangle {
                                    width: 16; height: 16; radius: 3; color: "transparent"
                                    Label { anchors.centerIn: parent; text: "\u2715"; font.pixelSize: 8; color: "#EF5350" }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.removeEffect(selClip, modelData.type) } }
                                }
                            }
                        }
                    }

                    // Effect categories + rows
                    Repeater {
                        model: fxFlickable.fxCategories

                        delegate: Column {
                            Layout.fillWidth: true
                            spacing: 2

                            property var cat: modelData

                            // Category header
                            Rectangle {
                                width: parent.width; height: 26; radius: 2
                                color: "#14142B"

                                RowLayout {
                                    anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 4
                                    Rectangle { width: 3; height: 14; radius: 1; color: cat.color }
                                    Label {
                                        text: cat.label
                                        font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.5
                                        color: "#8888A0"; Layout.fillWidth: true
                                    }
                                }
                            }

                            // Effect rows in this category
                            Repeater {
                                model: cat.effects

                                delegate: Rectangle {
                                    width: parent.width; height: 38; radius: 4
                                    color: fxIsActive ? Qt.rgba(0.424, 0.388, 1.0, 0.06) : "transparent"

                                    property int fxType: modelData.type
                                    // Inline fxList reads so QML tracks the dependency
                                    property bool fxIsActive: {
                                        var list = fxFlickable.fxList
                                        for (var i = 0; i < list.length; i++)
                                            if (list[i].type === fxType) return true
                                        return false
                                    }
                                    property bool fxIsEnabled: {
                                        var list = fxFlickable.fxList
                                        for (var i = 0; i < list.length; i++)
                                            if (list[i].type === fxType) return list[i].enabled
                                        return false
                                    }
                                    property real fxVal: {
                                        var list = fxFlickable.fxList
                                        for (var i = 0; i < list.length; i++)
                                            if (list[i].type === fxType) return list[i].value
                                        return modelData.def
                                    }
                                    // Re-sync slider when fxVal changes externally (binding break fix)
                                    onFxValChanged: if (fxIsActive) fxSlider.value = fxVal
                                    onFxIsActiveChanged: fxSlider.value = fxIsActive ? fxVal : modelData.min

                                    RowLayout {
                                        anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 4

                                        // Toggle / Add button
                                        Rectangle {
                                            width: 24; height: 24; radius: 4
                                            color: fxIsActive
                                                ? (fxIsEnabled ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : Qt.rgba(0.5, 0.5, 0.5, 0.15))
                                                : "transparent"
                                            border.color: fxIsActive
                                                ? (fxIsEnabled ? "#6C63FF" : "#505070")
                                                : "#353550"
                                            border.width: 1

                                            Label {
                                                anchors.centerIn: parent
                                                text: fxIsActive ? (fxIsEnabled ? "\u2713" : "\u2012") : "+"
                                                font.pixelSize: fxIsActive ? 12 : 14
                                                font.weight: Font.Bold
                                                color: fxIsActive
                                                    ? (fxIsEnabled ? "#6C63FF" : "#505070")
                                                    : "#6B6B88"
                                            }

                                            MouseArea {
                                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    if (!timelineNotifier || !hasClip) return
                                                    if (fxIsActive)
                                                        timelineNotifier.toggleEffect(selClip, fxType)
                                                    else
                                                        timelineNotifier.addEffect(selClip, fxType, modelData.def)
                                                }
                                            }
                                            ToolTip.visible: toggleHov.hovered
                                            ToolTip.text: fxIsActive ? (fxIsEnabled ? "Disable" : "Enable") : "Add effect"
                                            HoverHandler { id: toggleHov }
                                        }

                                        // Effect name
                                        Label {
                                            text: modelData.name
                                            font.pixelSize: 11
                                            color: fxIsActive && fxIsEnabled ? "#E0E0F0" : "#6B6B88"
                                            Layout.preferredWidth: 72
                                            elide: Text.ElideRight
                                        }

                                        // Intensity slider
                                        Slider {
                                            id: fxSlider
                                            Layout.fillWidth: true
                                            from: modelData.min; to: modelData.max
                                            value: fxIsActive ? fxVal : modelData.min
                                            enabled: fxIsActive && fxIsEnabled
                                            stepSize: Math.abs(modelData.max - modelData.min) > 10 ? 1 : 0.01

                                            onMoved: {
                                                if (timelineNotifier && hasClip)
                                                    timelineNotifier.updateEffect(selClip, fxType, value)
                                            }

                                            background: Rectangle {
                                                x: fxSlider.leftPadding
                                                y: fxSlider.topPadding + fxSlider.availableHeight / 2 - 1.5
                                                width: fxSlider.availableWidth; height: 3; radius: 1.5
                                                color: fxSlider.enabled ? "#1A1A34" : "#0E0E1C"
                                                Rectangle {
                                                    width: fxSlider.visualPosition * parent.width
                                                    height: parent.height; radius: 1.5
                                                    color: fxSlider.enabled ? "#6C63FF" : "#303050"
                                                }
                                            }
                                            handle: Rectangle {
                                                x: fxSlider.leftPadding + fxSlider.visualPosition * (fxSlider.availableWidth - width)
                                                y: fxSlider.topPadding + fxSlider.availableHeight / 2 - height / 2
                                                width: 12; height: 12; radius: 6
                                                color: fxSlider.pressed ? "#8A83FF" : (fxSlider.enabled ? "#6C63FF" : "#404060")
                                                border.color: fxSlider.pressed ? "#AEA8FF" : (fxSlider.enabled ? "#8A83FF" : "#505070")
                                                border.width: 1
                                            }
                                        }

                                        // Value readout
                                        Label {
                                            text: fxIsActive ? Math.round(fxVal) : "--"
                                            font.pixelSize: 10; font.family: "monospace"
                                            color: fxIsActive ? "#B0B0C8" : "#505068"
                                            Layout.preferredWidth: 26
                                            horizontalAlignment: Text.AlignRight
                                        }

                                        // Remove button
                                        Rectangle {
                                            visible: fxIsActive
                                            width: 20; height: 20; radius: 4
                                            color: rmHov.hovered ? Qt.rgba(0.94, 0.32, 0.31, 0.2) : "transparent"

                                            Label {
                                                anchors.centerIn: parent
                                                text: "\u2715"; font.pixelSize: 9; color: "#EF5350"
                                            }
                                            MouseArea {
                                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    if (timelineNotifier && hasClip)
                                                        timelineNotifier.removeEffect(selClip, fxType)
                                                }
                                            }
                                            ToolTip.visible: rmHov.hovered; ToolTip.text: "Remove effect"
                                            HoverHandler { id: rmHov }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ---- Preset Filters ----
                    SectionHeader { text: "Preset Filters" }

                    // Preset filter categories
                    Label {
                        text: "LIGHTING & SCENE"
                        font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.5
                        color: "#505068"; Layout.topMargin: 2
                    }
                    Flow {
                        Layout.fillWidth: true; spacing: 4
                        Repeater {
                            model: [
                                { id: 1,  label: "Natural" },     { id: 2,  label: "Daylight" },
                                { id: 3,  label: "Golden Hour" }, { id: 4,  label: "Overcast" },
                                { id: 7,  label: "Studio" }
                            ]
                            delegate: PresetChip {}
                        }
                    }

                    Label {
                        text: "PORTRAIT"
                        font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.5
                        color: "#505068"; Layout.topMargin: 2
                    }
                    Flow {
                        Layout.fillWidth: true; spacing: 4
                        Repeater {
                            model: [
                                { id: 5,  label: "Portrait" },      { id: 6,  label: "Soft Skin" },
                                { id: 8,  label: "Warm Portrait" }
                            ]
                            delegate: PresetChip {}
                        }
                    }

                    Label {
                        text: "FILM & VINTAGE"
                        font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.5
                        color: "#505068"; Layout.topMargin: 2
                    }
                    Flow {
                        Layout.fillWidth: true; spacing: 4
                        Repeater {
                            model: [
                                { id: 9,  label: "Vintage" },     { id: 10, label: "Polaroid" },
                                { id: 11, label: "Kodachrome" },   { id: 12, label: "Retro" },
                                { id: 31, label: "Retro 70s" }
                            ]
                            delegate: PresetChip {}
                        }
                    }

                    Label {
                        text: "CINEMATIC"
                        font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.5
                        color: "#505068"; Layout.topMargin: 2
                    }
                    Flow {
                        Layout.fillWidth: true; spacing: 4
                        Repeater {
                            model: [
                                { id: 13, label: "Cinematic" },    { id: 14, label: "Teal & Orange" },
                                { id: 15, label: "Noir" },         { id: 25, label: "Film Noir" },
                                { id: 29, label: "Moody" }
                            ]
                            delegate: PresetChip {}
                        }
                    }

                    Label {
                        text: "BLACK & WHITE"
                        font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.5
                        color: "#505068"; Layout.topMargin: 2
                    }
                    Flow {
                        Layout.fillWidth: true; spacing: 4
                        Repeater {
                            model: [
                                { id: 16, label: "Desaturated" },  { id: 17, label: "B&W Classic" },
                                { id: 18, label: "B&W High" },     { id: 19, label: "B&W Selenium" },
                                { id: 20, label: "B&W Infrared" }
                            ]
                            delegate: PresetChip {}
                        }
                    }

                    Label {
                        text: "CREATIVE"
                        font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.5
                        color: "#505068"; Layout.topMargin: 2
                    }
                    Flow {
                        Layout.fillWidth: true; spacing: 4
                        Repeater {
                            model: [
                                { id: 21, label: "Warm Sunset" },  { id: 22, label: "Cool Blue" },
                                { id: 23, label: "High Contrast" },{ id: 24, label: "Soft Pastel" },
                                { id: 26, label: "Bleach Bypass" },{ id: 27, label: "Cross Process" },
                                { id: 28, label: "Orange Teal" },  { id: 30, label: "Dreamy" },
                                { id: 32, label: "HDR" },          { id: 33, label: "Matte" },
                                { id: 34, label: "Cyberpunk" },    { id: 35, label: "Golden" },
                                { id: 36, label: "Arctic" }
                            ]
                            delegate: PresetChip {}
                        }
                    }
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
                            { prop: "brightness", label: "Brightness", from: -100, to: 100 },
                            { prop: "contrast", label: "Contrast", from: -100, to: 100 },
                            { prop: "saturation", label: "Saturation", from: -100, to: 100 },
                            { prop: "exposure", label: "Exposure", from: -100, to: 100 },
                            { prop: "temperature", label: "Temperature", from: -100, to: 100 },
                            { prop: "tint", label: "Tint", from: -100, to: 100 },
                            { prop: "highlights", label: "Highlights", from: -100, to: 100 },
                            { prop: "shadows", label: "Shadows", from: -100, to: 100 },
                            { prop: "vibrance", label: "Vibrance", from: -100, to: 100 },
                            { prop: "hue", label: "Hue", from: -180, to: 180 },
                            { prop: "fade", label: "Fade", from: 0, to: 100 },
                            { prop: "vignette", label: "Vignette", from: 0, to: 100 }
                        ]
                        delegate: PropSlider {
                            label: modelData.label
                            from: modelData.from; to: modelData.to
                            value: {
                                if (!timelineNotifier || !hasClip) return 0
                                var cg = timelineNotifier.clipColorGradingMap
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

                    // Keyframe management per property
                    SectionHeader { text: "Keyframes" }

                    Repeater {
                        model: [
                            { prop: 0, label: "Pos X" }, { prop: 1, label: "Pos Y" },
                            { prop: 2, label: "Scale" }, { prop: 3, label: "Scale Y" },
                            { prop: 4, label: "Rotation" }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true; spacing: 4

                            Label {
                                text: modelData.label; font.pixelSize: 10; color: "#8888A0"
                                Layout.preferredWidth: 52
                            }

                            // Keyframe count
                            Label {
                                property var kfTimes: { var g = _gen; return timelineNotifier && hasClip ? timelineNotifier.clipKeyframeTimes(selClip, modelData.prop) : [] }
                                text: kfTimes.length + " kf"
                                font.pixelSize: 9; font.family: "monospace"; color: "#6B6B88"
                                Layout.preferredWidth: 30
                            }

                            // Add keyframe at current position
                            Rectangle {
                                width: 22; height: 22; radius: 4
                                color: Qt.rgba(0.424, 0.388, 1.0, 0.15); border.color: "#6C63FF"
                                Label { anchors.centerIn: parent; text: "+"; font.pixelSize: 12; font.weight: Font.Bold; color: "#6C63FF" }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (timelineNotifier && hasClip) {
                                            var t = timelineNotifier.position - (clipMap.timelineIn || 0)
                                            timelineNotifier.addKeyframe(selClip, modelData.prop, Math.max(0, t), 0)
                                        }
                                    }
                                }
                                ToolTip.visible: addKfHov.hovered; ToolTip.text: "Add keyframe at playhead"
                                HoverHandler { id: addKfHov }
                            }

                            // Remove keyframe at current position
                            Rectangle {
                                width: 22; height: 22; radius: 4
                                color: Qt.rgba(0.94, 0.32, 0.31, 0.1); border.color: "#EF5350"
                                Label { anchors.centerIn: parent; text: "\u2212"; font.pixelSize: 12; font.weight: Font.Bold; color: "#EF5350" }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (timelineNotifier && hasClip) {
                                            var t = timelineNotifier.position - (clipMap.timelineIn || 0)
                                            timelineNotifier.removeKeyframe(selClip, modelData.prop, Math.max(0, t))
                                        }
                                    }
                                }
                                ToolTip.visible: rmKfHov.hovered; ToolTip.text: "Remove keyframe at playhead"
                                HoverHandler { id: rmKfHov }
                            }

                            // Clear all keyframes for this property
                            Rectangle {
                                width: 22; height: 22; radius: 4
                                color: "transparent"; border.color: "#353550"
                                Label { anchors.centerIn: parent; text: "\u2715"; font.pixelSize: 9; color: "#6B6B88" }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: { if (timelineNotifier && hasClip) timelineNotifier.clearKeyframes(selClip, modelData.prop) }
                                }
                                ToolTip.visible: clrKfHov.hovered; ToolTip.text: "Clear all keyframes"
                                HoverHandler { id: clrKfHov }
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
                        ActionBtn { text: "Pan L\u2192R"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applyKenBurnsPan(selClip, -0.1, 0, 0.1, 0) } }
                        ActionBtn { text: "Pan R\u2192L"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applyKenBurnsPan(selClip, 0.1, 0, -0.1, 0) } }
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
                        value: { var g = _gen; return timelineNotifier && hasClip ? timelineNotifier.clipSpeed(selClip) : 1.0 }
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

                    // Speed ramp point editor
                    SectionHeader { text: "Speed Ramp Points" }

                    Label {
                        property var rampPts: { var g = _gen; return timelineNotifier && hasClip ? timelineNotifier.clipSpeedRampPoints(selClip) : [] }
                        text: rampPts.length + " ramp point(s)"
                        font.pixelSize: 11; color: "#8888A0"
                    }

                    RowLayout {
                        spacing: 6
                        ActionBtn {
                            text: "+ Add Point"
                            onClicked: {
                                if (timelineNotifier && hasClip) {
                                    var clipIn = clipMap.timelineIn || 0
                                    var clipDur = clipMap.duration || 1
                                    var normT = Math.max(0, Math.min(1, (timelineNotifier.position - clipIn) / clipDur))
                                    timelineNotifier.addSpeedRampPoint(selClip, normT, 1.0)
                                }
                            }
                        }
                        ActionBtn {
                            text: "\u2212 Remove Point"
                            accent: "#EF5350"
                            onClicked: {
                                if (timelineNotifier && hasClip) {
                                    var clipIn = clipMap.timelineIn || 0
                                    var clipDur = clipMap.duration || 1
                                    var normT = (timelineNotifier.position - clipIn) / clipDur
                                    timelineNotifier.removeSpeedRampPoint(selClip, normT)
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Reverse"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.reverseClip(selClip) } }
                        ActionBtn { text: "Freeze Frame"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.freezeFrame(selClip) } }
                    }

                    // Reverse status
                    RowLayout {
                        spacing: 8
                        Switch {
                            checked: { var g = _gen; return timelineNotifier && hasClip ? timelineNotifier.clipIsReversed(selClip) : false }
                            onToggled: { if (timelineNotifier && hasClip) timelineNotifier.reverseClip(selClip) }
                        }
                        Label { text: "Reverse Playback"; font.pixelSize: 12; color: "#B0B0C8" }
                    }

                    ActionBtn {
                        text: "Clear Speed Ramp"
                        accent: "#EF5350"
                        enabled: { var g = _gen; return timelineNotifier && hasClip ? timelineNotifier.clipHasSpeedRamp(selClip) : false }
                        onClicked: { if (timelineNotifier && hasClip) timelineNotifier.clearSpeedRamp(selClip) }
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

                    SectionHeader { text: "Add Markers" }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "+ At Playhead (M)"; onClicked: { if (timelineNotifier) timelineNotifier.addMarker() } }
                    }

                    // Typed markers
                    RowLayout {
                        spacing: 4
                        ActionBtn { text: "Chapter"; onClicked: { if (timelineNotifier) timelineNotifier.addMarkerWithType(0, "Chapter") } }
                        ActionBtn { text: "Comment"; onClicked: { if (timelineNotifier) timelineNotifier.addMarkerWithType(1, "Comment") } }
                        ActionBtn { text: "ToDo"; onClicked: { if (timelineNotifier) timelineNotifier.addMarkerWithType(2, "ToDo") } }
                        ActionBtn { text: "Sync"; onClicked: { if (timelineNotifier) timelineNotifier.addMarkerWithType(3, "Sync") } }
                    }

                    // Position-based marker
                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "+ At Position..."; onClicked: {
                            if (timelineNotifier) {
                                var pos = timelineNotifier.position
                                timelineNotifier.addMarkerAtPosition(pos, 0, "Marker @" + pos.toFixed(1) + "s", "")
                            }
                        }}
                    }

                    // Clip marker
                    RowLayout {
                        spacing: 6
                        ActionBtn {
                            text: "+ Clip Marker"
                            enabled: hasClip
                            onClicked: { if (timelineNotifier && hasClip) timelineNotifier.addClipMarker(selClip, 1, "Clip note") }
                        }
                        ActionBtn {
                            text: "+ Region"
                            onClicked: {
                                if (timelineNotifier) {
                                    var start = timelineNotifier.hasInPoint ? timelineNotifier.inPoint : timelineNotifier.position
                                    var end = timelineNotifier.hasOutPoint ? timelineNotifier.outPoint : start + 2.0
                                    timelineNotifier.addMarkerRegion(start, end, 0, "Region")
                                }
                            }
                        }
                    }

                    SectionHeader { text: "Marker List (" + (timelineNotifier ? timelineNotifier.markerCount : 0) + ")" }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(300, count * 48 + 8)
                        clip: true; spacing: 4

                        model: timelineNotifier ? timelineNotifier.markers : []

                        delegate: Rectangle {
                            readonly property var marker: modelData || ({})
                            readonly property int mType: marker.type !== undefined ? marker.type : 0
                            width: ListView.view.width; height: 44; radius: 4
                            color: "#1A1A34"

                            RowLayout {
                                anchors.fill: parent; anchors.margins: 6; spacing: 6

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: marker.color || "#4CAF50"
                                }

                                ColumnLayout {
                                    spacing: 1; Layout.fillWidth: true
                                    RowLayout {
                                        spacing: 4
                                        Label {
                                            text: ["CH", "CMT", "TODO", "SYNC"][Math.min(mType, 3)]
                                            font.pixelSize: 8; font.weight: Font.Bold
                                            color: marker.color || "#4CAF50"
                                        }
                                        Label { text: marker.label || "Marker"; font.pixelSize: 11; color: "#D0D0E8"; elide: Text.ElideRight; Layout.fillWidth: true }
                                    }
                                    Label {
                                        text: {
                                            var pos = internal.formatTimecode(marker.position || 0)
                                            if (marker.isRegion) pos += " - " + internal.formatTimecode(marker.endPosition || 0)
                                            return pos
                                        }
                                        font.pixelSize: 9; font.family: "monospace"; color: "#6B6B88"
                                    }
                                }

                                ToolButton {
                                    width: 22; height: 22
                                    contentItem: Label { text: "\u27A4"; font.pixelSize: 10; color: "#8888A0"; anchors.centerIn: parent }
                                    onClicked: { if (timelineNotifier) timelineNotifier.navigateToMarker(marker.id) }
                                    ToolTip.text: "Go to marker"; ToolTip.visible: hovered
                                }

                                // Edit marker
                                ToolButton {
                                    width: 22; height: 22
                                    contentItem: Label { text: "\u270E"; font.pixelSize: 10; color: "#6C63FF"; anchors.centerIn: parent }
                                    onClicked: {
                                        markerEditId = marker.id
                                        markerEditLabel.text = marker.label || ""
                                        markerEditNotes.text = marker.notes || ""
                                        markerEditColor.text = marker.color || "#4CAF50"
                                    }
                                    ToolTip.text: "Edit marker"; ToolTip.visible: hovered
                                }

                                // Move to playhead
                                ToolButton {
                                    width: 22; height: 22
                                    contentItem: Label { text: "\u21E5"; font.pixelSize: 10; color: "#FFCA28"; anchors.centerIn: parent }
                                    onClicked: {
                                        if (timelineNotifier)
                                            timelineNotifier.updateMarkerPosition(marker.id, timelineNotifier.position)
                                    }
                                    ToolTip.text: "Move to playhead"; ToolTip.visible: hovered
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

                    // Marker editing section
                    SectionHeader { text: "Edit Marker" }

                    property int markerEditId: -1

                    RowLayout {
                        Layout.fillWidth: true; spacing: 4
                        Label { text: "Label"; font.pixelSize: 11; color: "#6B6B88"; Layout.preferredWidth: 50 }
                        TextField {
                            id: markerEditLabel
                            Layout.fillWidth: true; Layout.preferredHeight: 26
                            font.pixelSize: 11; color: "#D0D0E8"
                            placeholderText: "Select a marker to edit"
                            placeholderTextColor: "#505068"
                            background: Rectangle { radius: 4; color: "#1A1A34"; border.color: "#303050" }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 4
                        Label { text: "Notes"; font.pixelSize: 11; color: "#6B6B88"; Layout.preferredWidth: 50 }
                        TextField {
                            id: markerEditNotes
                            Layout.fillWidth: true; Layout.preferredHeight: 26
                            font.pixelSize: 11; color: "#D0D0E8"
                            placeholderText: "Notes..."
                            placeholderTextColor: "#505068"
                            background: Rectangle { radius: 4; color: "#1A1A34"; border.color: "#303050" }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 4
                        Label { text: "Color"; font.pixelSize: 11; color: "#6B6B88"; Layout.preferredWidth: 50 }
                        TextField {
                            id: markerEditColor
                            Layout.fillWidth: true; Layout.preferredHeight: 26
                            font.pixelSize: 11; color: "#D0D0E8"
                            placeholderText: "#4CAF50"
                            placeholderTextColor: "#505068"
                            background: Rectangle { radius: 4; color: "#1A1A34"; border.color: "#303050" }
                        }
                        // Color preview
                        Rectangle {
                            width: 26; height: 26; radius: 4
                            color: markerEditColor.text || "#4CAF50"
                            border.color: Qt.darker(color, 1.3)
                        }
                    }

                    ActionBtn {
                        text: "Save Marker Changes"
                        enabled: mkrCol.markerEditId >= 0
                        onClicked: {
                            if (timelineNotifier && mkrCol.markerEditId >= 0) {
                                timelineNotifier.updateMarkerFull(
                                    mkrCol.markerEditId,
                                    markerEditLabel.text,
                                    markerEditNotes.text,
                                    markerEditColor.text,
                                    0  // keep current type
                                )
                                mkrCol.markerEditId = -1
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

            // ---- 7: Transitions ----
            Flickable {
                contentHeight: trCol.implicitHeight + 20
                clip: true; boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: trCol
                    width: parent.width - 16; x: 8; y: 8
                    spacing: 8

                    SectionHeader { text: "Transition In" }

                    GridLayout {
                        columns: 4; columnSpacing: 4; rowSpacing: 4
                        Layout.fillWidth: true

                        Repeater {
                            model: [
                                {name: "None", type: 0},
                                {name: "Fade", type: 1},
                                {name: "Dissolve", type: 2},
                                {name: "Slide L", type: 3},
                                {name: "Slide R", type: 4},
                                {name: "Slide Up", type: 5},
                                {name: "Slide Dn", type: 6},
                                {name: "Wipe L", type: 7},
                                {name: "Wipe R", type: 8},
                                {name: "Wipe Up", type: 9},
                                {name: "Wipe Dn", type: 10},
                                {name: "Zoom", type: 11}
                            ]
                            delegate: Rectangle {
                                Layout.fillWidth: true; height: 40; radius: 4
                                color: "#1A1A34"; border.color: "#303050"

                                Label {
                                    anchors.centerIn: parent
                                    text: modelData.name; font.pixelSize: 10; color: "#B0B0C8"
                                }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (!timelineNotifier || !hasClip) return
                                        if (modelData.type === 0)
                                            timelineNotifier.removeTransitionIn(selClip)
                                        else
                                            timelineNotifier.setTransitionIn(selClip, modelData.type,
                                                trDurationSlider.value, trEasingIndex)
                                    }
                                }
                            }
                        }
                    }

                    SectionHeader { text: "Transition Out" }

                    GridLayout {
                        columns: 4; columnSpacing: 4; rowSpacing: 4
                        Layout.fillWidth: true

                        Repeater {
                            model: [
                                {name: "None", type: 0},
                                {name: "Fade", type: 1},
                                {name: "Dissolve", type: 2},
                                {name: "Slide L", type: 3},
                                {name: "Slide R", type: 4},
                                {name: "Slide Up", type: 5},
                                {name: "Slide Dn", type: 6},
                                {name: "Wipe L", type: 7},
                                {name: "Wipe R", type: 8},
                                {name: "Wipe Up", type: 9},
                                {name: "Wipe Dn", type: 10},
                                {name: "Zoom", type: 11}
                            ]
                            delegate: Rectangle {
                                Layout.fillWidth: true; height: 40; radius: 4
                                color: "#1A1A34"; border.color: "#303050"

                                Label {
                                    anchors.centerIn: parent
                                    text: modelData.name; font.pixelSize: 10; color: "#B0B0C8"
                                }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (!timelineNotifier || !hasClip) return
                                        if (modelData.type === 0)
                                            timelineNotifier.removeTransitionOut(selClip)
                                        else
                                            timelineNotifier.setTransitionOut(selClip, modelData.type,
                                                trDurationSlider.value, trEasingIndex)
                                    }
                                }
                            }
                        }
                    }

                    SectionHeader { text: "Duration" }

                    RowLayout {
                        spacing: 6; Layout.fillWidth: true
                        Label { text: trDurationSlider.value.toFixed(1) + "s"; font.pixelSize: 11; color: "#8888A0"; Layout.preferredWidth: 36 }
                        Slider {
                            id: trDurationSlider
                            Layout.fillWidth: true
                            from: 0.1; to: 3.0; value: 0.5; stepSize: 0.1
                            background: Rectangle {
                                x: trDurationSlider.leftPadding; y: trDurationSlider.topPadding + trDurationSlider.availableHeight / 2 - 1.5
                                width: trDurationSlider.availableWidth; height: 3; radius: 1.5; color: "#1A1A34"
                                Rectangle { width: trDurationSlider.visualPosition * parent.width; height: parent.height; radius: 1.5; color: "#6C63FF" }
                            }
                            handle: Rectangle {
                                x: trDurationSlider.leftPadding + trDurationSlider.visualPosition * (trDurationSlider.availableWidth - width)
                                y: trDurationSlider.topPadding + trDurationSlider.availableHeight / 2 - height / 2
                                width: 14; height: 14; radius: 7
                                color: trDurationSlider.pressed ? "#8A83FF" : "#6C63FF"
                                border.color: trDurationSlider.pressed ? "#AEA8FF" : "#8A83FF"
                                border.width: 1
                            }
                        }
                    }

                    SectionHeader { text: "Easing" }

                    property int trEasingIndex: 3  // default EaseInOut

                    Flow {
                        Layout.fillWidth: true; spacing: 4

                        Repeater {
                            model: ["Linear", "Ease In", "Ease Out", "Ease InOut"]
                            delegate: Rectangle {
                                property bool isActive: trCol.trEasingIndex === index
                                width: easLbl.implicitWidth + 14; height: 26; radius: 13
                                color: isActive ? Qt.rgba(0.424, 0.388, 1.0, 0.2) : "#1A1A34"
                                border.color: isActive ? "#6C63FF" : "#303050"

                                Label {
                                    id: easLbl; anchors.centerIn: parent
                                    text: modelData; font.pixelSize: 10
                                    color: isActive ? "#6C63FF" : "#B0B0C8"
                                }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: trCol.trEasingIndex = index
                                }
                            }
                        }
                    }

                    SectionHeader { text: "Remove" }

                    RowLayout {
                        spacing: 6
                        ActionBtn { text: "Remove In"; accent: "#EF5350"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.removeTransitionIn(selClip) } }
                        ActionBtn { text: "Remove Out"; accent: "#EF5350"; onClicked: { if (timelineNotifier && hasClip) timelineNotifier.removeTransitionOut(selClip) } }
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

        // Re-sync slider when value changes externally (fixes binding break after user drag)
        onValueChanged: sldr.value = value

        Layout.fillWidth: true; spacing: 2

        RowLayout {
            spacing: 4
            Label { text: parent.parent.label; font.pixelSize: 11; color: "#8888A0"; Layout.fillWidth: true }
            Label {
                text: {
                    var absRange = Math.abs(parent.parent.to - parent.parent.from)
                    return absRange > 10 ? Math.round(sldr.value).toString() : sldr.value.toFixed(2)
                }
                font.pixelSize: 10; font.family: "monospace"; color: "#6B6B88"
            }
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
            handle: Rectangle {
                x: sldr.leftPadding + sldr.visualPosition * (sldr.availableWidth - width)
                y: sldr.topPadding + sldr.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7
                color: sldr.pressed ? "#8A83FF" : (sldr.enabled ? "#6C63FF" : "#404060")
                border.color: sldr.pressed ? "#AEA8FF" : (sldr.enabled ? "#8A83FF" : "#505070")
                border.width: 1
            }
        }
    }

    component PresetChip: Rectangle {
        width: pcLbl.implicitWidth + 16; height: 26; radius: 4
        color: Qt.rgba(0.671, 0.294, 0.737, 0.1)
        border.color: Qt.rgba(0.671, 0.294, 0.737, 0.3)

        Label { id: pcLbl; anchors.centerIn: parent; text: modelData.label; font.pixelSize: 10; color: "#D0D0E8" }
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
            onClicked: { if (timelineNotifier && hasClip) timelineNotifier.applyPresetFilter(selClip, modelData.id) }
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
