import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/// VE2EditorSidebar — Icon rail (9 tabs) + Panel area.
/// Mirrors the EditorSidebar from video_editor but uses VE2 panels.
/// Tab mapping:
///   0 = Media Pool       (VE2MediaPoolPanel)
///   1 = Inspector         (VE2PropertyPanel tab 0)
///   2 = Effects           (VE2PropertyPanel tab 1)
///   3 = Color             (VE2PropertyPanel tab 2)
///   4 = Audio             (VE2PropertyPanel tab 3)
///   5 = Transform         (VE2PropertyPanel tab 4)
///   6 = Speed             (VE2PropertyPanel tab 5)
///   7 = Markers           (VE2PropertyPanel tab 6)
///   8 = Transitions       (VE2PropertyPanel tab 7)
Item {
    id: root

    readonly property real iconRailWidth: 56
    property bool panelOpen: true
    property int activePanel: 0

    readonly property var tabs: [
        { tab: 0,  icon: "\uD83C\uDFA5", label: "Media" },
        { tab: 1,  icon: "\u24D8",       label: "Inspector" },
        { tab: 2,  icon: "\u2728",       label: "Effects" },
        { tab: 3,  icon: "\uD83C\uDFA8", label: "Color" },
        { tab: 4,  icon: "\uD83C\uDFB5", label: "Audio" },
        { tab: 5,  icon: "\u2B1C",       label: "Transform" },
        { tab: 6,  icon: "\u26A1",       label: "Speed" },
        { tab: 7,  icon: "\uD83D\uDCCD", label: "Markers" },
        { tab: 8,  icon: "\uD83D\uDD17", label: "Transitions" },
        { tab: 9,  icon: "\uD83D\uDD52", label: "Versions" }
    ]

    // Sync: when toolbar/code calls timelineNotifier.setActivePanel(),
    // map the BottomPanelTab enum to sidebar tab index and open the panel.
    Connections {
        target: timelineNotifier
        function onStateChanged() {
            if (!timelineNotifier) return
            var tp = timelineNotifier.activePanel
            // BottomPanelTab enum → sidebar tab mapping
            var map = {
                1: 1,   // inspector  → Inspector
                3: 2,   // effects    → Effects
                4: 3,   // colorGrading → Color
                9: 4,   // audio      → Audio
                6: 5,   // transform  → Transform
                7: 6,   // speed      → Speed
                10: 7,  // markers    → Markers
                5: 8,   // transitions → Transitions
                11: 8   // adjustmentLayers → Transitions (fallback)
            }
            if (tp in map) {
                root.activePanel = map[tp]
                if (!root.panelOpen) root.panelOpen = true
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // --- Icon Rail ---
        Rectangle {
            Layout.preferredWidth: root.iconRailWidth
            Layout.fillHeight: true
            color: "#0E0E1C"

            Rectangle {
                anchors.right: parent.right
                width: 1; height: parent.height
                color: "#252540"
            }

            Flickable {
                anchors.fill: parent
                anchors.topMargin: 4
                contentHeight: iconColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: iconColumn
                    width: parent.width

                    Repeater {
                        model: root.tabs
                        delegate: Rectangle {
                            property bool isActive: root.activePanel === modelData.tab && root.panelOpen
                            width: root.iconRailWidth
                            height: Math.min(Math.max(44, (root.height - 8) / root.tabs.length), 64)
                            radius: 6
                            color: isActive ? "#1A1A38" : "transparent"

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 3

                                Label {
                                    text: modelData.icon
                                    font.pixelSize: parent.parent.height > 50 ? 22 : 18
                                    color: isActive ? "#6C63FF" : "#6B6B88"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Label {
                                    text: modelData.label
                                    font.pixelSize: 10
                                    font.weight: isActive ? Font.DemiBold : Font.Normal
                                    color: isActive ? "#D0D0E8" : "#6B6B88"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }

                            ToolTip.visible: hoverHandler.hovered
                            ToolTip.text: modelData.label
                            HoverHandler { id: hoverHandler }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (root.activePanel === modelData.tab) {
                                        root.panelOpen = !root.panelOpen
                                    } else {
                                        root.activePanel = modelData.tab
                                        if (!root.panelOpen) root.panelOpen = true
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- Panel Area ---
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.panelOpen
            color: "#12122A"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Panel header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "#16162E"

                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width; height: 1; color: "#252540"
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14; anchors.rightMargin: 14
                        spacing: 6

                        Label {
                            text: "\u2630"
                            font.pixelSize: 14; color: "#505068"
                        }
                        Label {
                            text: {
                                for (var i = 0; i < root.tabs.length; i++) {
                                    if (root.tabs[i].tab === root.activePanel) return root.tabs[i].label
                                }
                                return "Panel"
                            }
                            font.pixelSize: 15; font.weight: Font.DemiBold
                            color: "#E0E0F0"
                            Layout.fillWidth: true
                        }

                        ToolButton {
                            contentItem: Label {
                                text: "\u2715"
                                font.pixelSize: 14; color: "#8888A0"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            ToolTip.text: "Collapse"
                            ToolTip.visible: hovered
                            onClicked: root.panelOpen = false
                        }
                    }
                }

                // Panel content
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // Media Pool (panel 0)
                    VE2MediaPoolPanel {
                        anchors.fill: parent
                        visible: root.activePanel === 0
                    }

                    // Property panels (panels 1-7) — mapped to VE2PropertyPanel tabs 0-6
                    VE2PropertyPanel {
                        anchors.fill: parent
                        visible: root.activePanel > 0 && root.activePanel < 8
                        forceTabIndex: root.activePanel > 0 && root.activePanel < 8 ? root.activePanel - 1 : -1
                    }

                    // Transitions Library + Controls (panel 8)
                    Item {
                        id: trPanel
                        anchors.fill: parent
                        visible: root.activePanel === 8

                        readonly property int selClip: timelineNotifier ? timelineNotifier.selectedClipId : -1
                        readonly property bool hasClip: selClip >= 0
                        property int trDuration: 5   // tenths of seconds (0.5s default)
                        property int trEasing: 3     // EaseInOut default
                        property bool applyAsOut: false  // false=TransitionIn, true=TransitionOut

                        Flickable {
                            anchors.fill: parent; anchors.margins: 8
                            contentHeight: trPanelCol.implicitHeight + 20
                            clip: true; boundsBehavior: Flickable.StopAtBounds

                            ColumnLayout {
                                id: trPanelCol
                                width: parent.width; spacing: 6

                                // Header
                                Label {
                                    text: "Transitions"
                                    font.pixelSize: 14; font.weight: Font.DemiBold; color: "#D0D0E8"
                                }

                                // Status banner
                                Rectangle {
                                    Layout.fillWidth: true; height: 26; radius: 4
                                    color: trPanel.hasClip ? Qt.rgba(0.15, 0.78, 0.85, 0.1) : Qt.rgba(1, 0.79, 0.16, 0.1)
                                    border.color: trPanel.hasClip ? Qt.rgba(0.15, 0.78, 0.85, 0.3) : Qt.rgba(1, 0.79, 0.16, 0.3)
                                    Label {
                                        anchors.centerIn: parent
                                        text: trPanel.hasClip
                                            ? "Clip #" + trPanel.selClip + " selected"
                                            : "\u26A0 Select a clip first"
                                        font.pixelSize: 10; font.weight: Font.DemiBold
                                        color: trPanel.hasClip ? "#26C6DA" : "#FFCA28"
                                    }
                                }

                                // Apply as In or Out toggle
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 4
                                    Repeater {
                                        model: ["Transition In", "Transition Out"]
                                        delegate: Rectangle {
                                            Layout.fillWidth: true; height: 26; radius: 4
                                            property bool isActive: (index === 0 && !trPanel.applyAsOut) || (index === 1 && trPanel.applyAsOut)
                                            color: isActive ? Qt.rgba(0.15, 0.78, 0.85, 0.2) : "#1A1A34"
                                            border.color: isActive ? "#26C6DA" : "#303050"
                                            Label { anchors.centerIn: parent; text: modelData; font.pixelSize: 10; color: isActive ? "#26C6DA" : "#8888A0" }
                                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: trPanel.applyAsOut = (index === 1) }
                                        }
                                    }
                                }

                                // Duration + Easing (compact row)
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 4
                                    Label { text: "Duration:"; font.pixelSize: 10; color: "#8888A0" }
                                    Slider {
                                        id: trDurSlider
                                        Layout.fillWidth: true; from: 1; to: 30; value: trPanel.trDuration; stepSize: 1
                                        onMoved: trPanel.trDuration = value
                                    }
                                    Label { text: (trPanel.trDuration / 10).toFixed(1) + "s"; font.pixelSize: 10; font.family: "monospace"; color: "#B0B0C8"; Layout.preferredWidth: 28 }
                                }

                                // Easing
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 3
                                    Label { text: "Easing:"; font.pixelSize: 10; color: "#8888A0"; Layout.preferredWidth: 42 }
                                    Repeater {
                                        model: ["Linear", "In", "Out", "InOut"]
                                        delegate: Rectangle {
                                            Layout.fillWidth: true; height: 22; radius: 11
                                            property bool isActive: trPanel.trEasing === index
                                            color: isActive ? Qt.rgba(0.15, 0.78, 0.85, 0.2) : "#1A1A34"
                                            border.color: isActive ? "#26C6DA" : "#303050"
                                            Label { anchors.centerIn: parent; text: modelData; font.pixelSize: 9; color: isActive ? "#26C6DA" : "#8888A0" }
                                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: trPanel.trEasing = index }
                                        }
                                    }
                                }

                                // Separator
                                Rectangle { Layout.fillWidth: true; height: 1; color: "#1E1E38" }

                                // Transition library by category
                                Repeater {
                                    model: ["Basic", "Slide", "Wipe", "Motion", "Creative"]
                                    delegate: ColumnLayout {
                                        Layout.fillWidth: true; spacing: 2

                                        Label {
                                            text: modelData
                                            font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.5
                                            color: "#505068"; Layout.topMargin: 2
                                        }

                                        Flow {
                                            Layout.fillWidth: true; spacing: 4

                                            Repeater {
                                                property string cat: modelData
                                                model: {
                                                    if (!timelineNotifier) return []
                                                    var all = timelineNotifier.transitionLibrary()
                                                    var filtered = []
                                                    for (var i = 0; i < all.length; i++) {
                                                        if (all[i].category === cat) filtered.push(all[i])
                                                    }
                                                    return filtered
                                                }
                                                delegate: Rectangle {
                                                    width: trLbl.implicitWidth + 14; height: 26; radius: 4
                                                    color: trHov.hovered ? Qt.rgba(0.15, 0.78, 0.85, 0.2) : "#14142B"
                                                    border.color: trHov.hovered ? "#26C6DA" : "#252540"

                                                    Label {
                                                        id: trLbl; anchors.centerIn: parent
                                                        text: modelData.name; font.pixelSize: 10
                                                        color: trHov.hovered ? "#E0E0F0" : "#B0B0C8"
                                                    }
                                                    HoverHandler { id: trHov }
                                                    MouseArea {
                                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            if (!timelineNotifier) return
                                                            var selId = timelineNotifier.selectedClipId
                                                            if (selId < 0) return
                                                            var dur = trPanel.trDuration / 10.0
                                                            var eas = trPanel.trEasing
                                                            var asOut = trPanel.applyAsOut
                                                            if (asOut)
                                                                timelineNotifier.setTransitionOut(selId, modelData.type, dur, eas)
                                                            else
                                                                timelineNotifier.setTransitionIn(selId, modelData.type, dur, eas)
                                                        }
                                                    }
                                                    ToolTip.visible: trHov.hovered
                                                    ToolTip.text: modelData.name
                                                }
                                            }
                                        }
                                    }
                                }

                                // Remove section
                                Rectangle { Layout.fillWidth: true; height: 1; color: "#1E1E38"; Layout.topMargin: 4 }

                                RowLayout {
                                    Layout.fillWidth: true; spacing: 4
                                    Rectangle {
                                        Layout.fillWidth: true; height: 26; radius: 4
                                        color: Qt.rgba(0.94, 0.32, 0.31, 0.1); border.color: Qt.rgba(0.94, 0.32, 0.31, 0.3)
                                        Label { anchors.centerIn: parent; text: "Remove In"; font.pixelSize: 10; color: "#EF5350" }
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && trPanel.hasClip) timelineNotifier.removeTransitionIn(trPanel.selClip) } }
                                    }
                                    Rectangle {
                                        Layout.fillWidth: true; height: 26; radius: 4
                                        color: Qt.rgba(0.94, 0.32, 0.31, 0.1); border.color: Qt.rgba(0.94, 0.32, 0.31, 0.3)
                                        Label { anchors.centerIn: parent; text: "Remove Out"; font.pixelSize: 10; color: "#EF5350" }
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier && trPanel.hasClip) timelineNotifier.removeTransitionOut(trPanel.selClip) } }
                                    }
                                }

                                // Shortcut hint
                                Label {
                                    text: "Ctrl+T: Apply Cross Dissolve (default)"
                                    font.pixelSize: 9; color: "#505068"; Layout.topMargin: 4
                                }
                            }
                        }
                    }

                    // Version History (panel 9)
                    Item {
                        anchors.fill: parent
                        visible: root.activePanel === 9

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: 6

                            // Header with Save Version button
                            RowLayout {
                                Layout.fillWidth: true; spacing: 6
                                Label {
                                    text: "Version History"
                                    font.pixelSize: 13; font.weight: Font.DemiBold; color: "#D0D0E8"
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: timelineNotifier ? timelineNotifier.versionCount + " versions" : "0"
                                    font.pixelSize: 10; color: "#6B6B88"
                                }
                            }

                            // Save Version button
                            Rectangle {
                                Layout.fillWidth: true; height: 32; radius: 4
                                color: Qt.rgba(0.424, 0.388, 1.0, 0.15)
                                border.color: Qt.rgba(0.424, 0.388, 1.0, 0.3)

                                RowLayout {
                                    anchors.centerIn: parent; spacing: 6
                                    Label { text: "+"; font.pixelSize: 14; font.weight: Font.Bold; color: "#6C63FF" }
                                    Label { text: "Save Version"; font.pixelSize: 11; font.weight: Font.Medium; color: "#6C63FF" }
                                }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (timelineNotifier) {
                                            var label = versionNameField.text.trim()
                                            timelineNotifier.saveVersion(label || "")
                                            versionNameField.text = ""
                                        }
                                    }
                                }
                            }

                            // Version name input
                            TextField {
                                id: versionNameField
                                Layout.fillWidth: true; Layout.preferredHeight: 28
                                font.pixelSize: 11; color: "#D0D0E8"
                                placeholderText: "Version name (optional)"
                                placeholderTextColor: "#505068"
                                background: Rectangle { radius: 4; color: "#1A1A34"; border.color: "#303050" }
                                onAccepted: {
                                    if (timelineNotifier) {
                                        timelineNotifier.saveVersion(text.trim())
                                        text = ""
                                    }
                                }
                            }

                            // Auto-save interval
                            RowLayout {
                                Layout.fillWidth: true; spacing: 4
                                Label { text: "Auto-save:"; font.pixelSize: 10; color: "#6B6B88" }
                                Label {
                                    text: "60s"
                                    font.pixelSize: 10; color: "#8888A0"
                                }
                            }

                            // Separator
                            Rectangle { Layout.fillWidth: true; height: 1; color: "#1E1E38" }

                            // Version list
                            ListView {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                clip: true; spacing: 2

                                model: timelineNotifier ? timelineNotifier.getVersionHistory() : []

                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: ListView.view.width; height: 52; radius: 4
                                    color: versionHov.hovered ? "#1A1A34" : "#14142B"

                                    HoverHandler { id: versionHov }

                                    ColumnLayout {
                                        anchors.fill: parent; anchors.margins: 6; spacing: 2

                                        RowLayout {
                                            spacing: 4
                                            // Auto-save or manual indicator
                                            Rectangle {
                                                width: 6; height: 6; radius: 3
                                                color: modelData.isAutoSave ? "#FFCA28" : "#6C63FF"
                                            }
                                            Label {
                                                text: modelData.label || "Unnamed"
                                                font.pixelSize: 11; font.weight: Font.DemiBold
                                                color: "#D0D0E8"; elide: Text.ElideRight
                                                Layout.fillWidth: true
                                            }
                                        }
                                        Label {
                                            text: modelData.timestamp || ""
                                            font.pixelSize: 9; font.family: "monospace"; color: "#6B6B88"
                                        }

                                        RowLayout {
                                            spacing: 4
                                            // Restore button
                                            Rectangle {
                                                width: restoreLbl.implicitWidth + 12; height: 18; radius: 3
                                                color: Qt.rgba(0.424, 0.388, 1.0, 0.15)
                                                border.color: Qt.rgba(0.424, 0.388, 1.0, 0.3)
                                                Label {
                                                    id: restoreLbl; anchors.centerIn: parent
                                                    text: "Restore"; font.pixelSize: 9; font.weight: Font.Bold; color: "#6C63FF"
                                                }
                                                MouseArea {
                                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                    onClicked: { if (timelineNotifier) timelineNotifier.restoreVersion(modelData.index) }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Clear history
                            Rectangle {
                                Layout.fillWidth: true; height: 26; radius: 4
                                color: Qt.rgba(0.94, 0.32, 0.31, 0.1)
                                border.color: Qt.rgba(0.94, 0.32, 0.31, 0.3)
                                Label { anchors.centerIn: parent; text: "Clear History"; font.pixelSize: 10; color: "#EF5350" }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier) timelineNotifier.clearVersionHistory() } }
                            }
                        }
                    }
                }
            }
        }
    }
}
