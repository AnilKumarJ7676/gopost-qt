import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * VE2TimelineToolbar — secondary toolbar between preview and timeline.
 *
 * Left:   Split, Ripple, In/Out/Clear, Text, Adjust
 * Center: Effects, Color, Trans., Keyframe, Audio, +Track
 * Right:  Split All, Marker, Snap, Color Legend, Zoom+Autofit, Proxy, Delete
 */
Item {
    id: root
    height: 36

    property bool snapEnabled: true

    Rectangle {
        anchors.fill: parent
        color: "#0E0E1E"
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#1E1E38" }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#1E1E38" }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 6; anchors.rightMargin: 6
        spacing: 0

        // ================================================================
        // LEFT: Edit tools
        // ================================================================

        // Selection mode (V)
        TBtn {
            iconText: "\u25E2"; label: "Select"; tooltip: "Selection tool (V)"
            highlighted: timelineNotifier ? (!timelineNotifier.razorModeEnabled && timelineNotifier.trimEditMode === 0) : true
            highlightColor: "#B0B0C8"
            onClicked: {
                if (timelineNotifier) {
                    timelineNotifier.setRazorMode(false)
                    timelineNotifier.setTrimEditMode(0)
                }
            }
        }

        // Razor / Blade tool (C)
        TBtn {
            iconText: "\uD83D\uDD2A"; label: "Razor"; tooltip: "Razor tool (C) — click to cut clips. Shift+click cuts all tracks"
            highlighted: timelineNotifier ? timelineNotifier.razorModeEnabled : false
            highlightColor: "#EF5350"
            onClicked: { if (timelineNotifier) timelineNotifier.toggleRazorMode() }
        }

        TBtn {
            iconText: "\u2702"; label: "Split"; tooltip: "Split at playhead (Ctrl+B)"
            enabled: timelineNotifier && timelineNotifier.isReady && timelineNotifier.totalDuration > 0
            onClicked: timelineNotifier.splitClipAtPlayhead()
        }

        // Trim edit mode buttons (mutually exclusive, press again to deactivate)
        TBtn {
            iconText: "\u21C4"; label: "Ripple"; tooltip: "Ripple Edit (B) — trim shifts downstream clips"
            highlighted: timelineNotifier ? timelineNotifier.trimEditMode === 1 : false
            highlightColor: "#FFCA28"
            onClicked: { if (timelineNotifier) timelineNotifier.setTrimEditMode(1) }
        }
        TBtn {
            iconText: "\u21CB"; label: "Roll"; tooltip: "Roll Edit (N) — move edit point between clips"
            highlighted: timelineNotifier ? timelineNotifier.trimEditMode === 2 : false
            highlightColor: "#26C6DA"
            onClicked: { if (timelineNotifier) timelineNotifier.setTrimEditMode(2) }
        }
        TBtn {
            iconText: "\u21D4"; label: "Slip"; tooltip: "Slip Edit (Y) — slip source content inside clip"
            highlighted: timelineNotifier ? timelineNotifier.trimEditMode === 3 : false
            highlightColor: "#AB47BC"
            onClicked: { if (timelineNotifier) timelineNotifier.setTrimEditMode(3) }
        }
        TBtn {
            iconText: "\u21E0"; label: "Slide"; tooltip: "Slide Edit (U) — slide clip, neighbors adjust"
            highlighted: timelineNotifier ? timelineNotifier.trimEditMode === 4 : false
            highlightColor: "#66BB6A"
            onClicked: { if (timelineNotifier) timelineNotifier.setTrimEditMode(4) }
        }

        TSep {}

        TBtn {
            iconText: "["; label: "In"; tooltip: "Set In point (I)"
            onClicked: { if (timelineNotifier) timelineNotifier.setInPoint() }
        }
        TBtn {
            iconText: "]"; label: "Out"; tooltip: "Set Out point (O)"
            onClicked: { if (timelineNotifier) timelineNotifier.setOutPoint() }
        }
        TBtn {
            iconText: "\u2715"; label: "Clear"; tooltip: "Clear In/Out points (Alt+X)"
            onClicked: { if (timelineNotifier) timelineNotifier.clearInOutPoints() }
        }

        TSep {}

        TBtn {
            iconText: "T"; label: "Text"; tooltip: "Add text clip"
            onClicked: {
                if (!timelineNotifier) return
                timelineNotifier.addTrack(3)
                var clipId = timelineNotifier.addClip(0, 2, "", "Text", 5.0)
                if (clipId >= 0) timelineNotifier.selectClip(clipId)
            }
        }
        TBtn {
            iconText: "\u25A3"; label: "Adjust"; tooltip: "Add adjustment layer"
            onClicked: { if (timelineNotifier) timelineNotifier.createAdjustmentClip() }
        }

        TSep {}

        // ================================================================
        // CENTER: Property tools (colored icons)
        // ================================================================

        TBtn {
            iconText: "\u2728"; label: "Effects"; tooltip: "Effects panel"
            iconColor: "#AB47BC"
            onClicked: { if (timelineNotifier) timelineNotifier.setActivePanel(3) }
        }
        TBtn {
            iconText: "\uD83C\uDFA8"; label: "Color"; tooltip: "Color grading panel"
            iconColor: "#FF7043"
            onClicked: { if (timelineNotifier) timelineNotifier.setActivePanel(4) }
        }
        TBtn {
            iconText: "\u21C6"; label: "Trans."; tooltip: "Transitions panel"
            iconColor: "#26C6DA"
            onClicked: { if (timelineNotifier) timelineNotifier.setActivePanel(11) }
        }
        TBtn {
            iconText: "\u25C7"; label: "Keyframe"; tooltip: "Keyframe panel"
            iconColor: "#42A5F5"
            onClicked: { if (timelineNotifier) timelineNotifier.setActivePanel(8) }
        }
        TBtn {
            iconText: "\u266B"; label: "Audio"; tooltip: "Audio panel"
            iconColor: "#66BB6A"
            onClicked: { if (timelineNotifier) timelineNotifier.setActivePanel(9) }
        }
        TBtn {
            iconText: "+"; label: "Track"; tooltip: "Add track"
            onClicked: addTrackMenu.open()

            Menu {
                id: addTrackMenu
                MenuItem { text: "Video Track"; onTriggered: { if (timelineNotifier) timelineNotifier.addTrack(0) } }
                MenuItem { text: "Audio Track"; onTriggered: { if (timelineNotifier) timelineNotifier.addTrack(1) } }
                MenuItem { text: "Title Track"; onTriggered: { if (timelineNotifier) timelineNotifier.addTrack(2) } }
            }
        }

        Item { Layout.fillWidth: true }

        // ================================================================
        // RIGHT: Timeline tools
        // ================================================================

        TBtn {
            iconText: "\u2702\u2191"; label: "Split All"; tooltip: "Split all unlocked tracks at playhead"
            enabled: timelineNotifier && timelineNotifier.isReady && timelineNotifier.totalDuration > 0
            onClicked: { if (timelineNotifier) timelineNotifier.splitAllAtPlayhead() }
        }

        TSep {}

        TBtn {
            iconText: "\u25C6"; label: "Marker"; tooltip: "Add marker (M)"
            enabled: timelineNotifier && timelineNotifier.isReady
            onClicked: { if (timelineNotifier) timelineNotifier.addMarker() }
        }
        TBtn {
            iconText: "\u2630"; label: "Markers"; tooltip: "Markers panel"
            enabled: timelineNotifier && timelineNotifier.isReady
            onClicked: {
                if (timelineNotifier) {
                    if (timelineNotifier.activePanel === 10)
                        timelineNotifier.setActivePanel(0)
                    else
                        timelineNotifier.setActivePanel(10)
                }
            }
        }

        TSep {}

        // Snap toggle
        TBtn {
            id: snapBtn
            iconText: "\uD83E\uDDF2"; label: "Snap"
            tooltip: root.snapEnabled ? "Snap ON — click to disable" : "Snap OFF — click to enable"
            highlighted: root.snapEnabled
            highlightColor: "#6C63FF"
            onClicked: root.snapEnabled = !root.snapEnabled
        }

        // Magnetic timeline toggle (FCP-style)
        TBtn {
            iconText: "\uD83E\uDDF2"; label: "Magnetic"
            tooltip: {
                var on = timelineNotifier && timelineNotifier.magneticTimelineEnabled
                return on ? "Magnetic Timeline ON — clips auto-ripple (hold P to override)"
                          : "Magnetic Timeline OFF — free clip placement"
            }
            highlighted: timelineNotifier ? timelineNotifier.magneticTimelineEnabled : false
            highlightColor: "#FF7043"
            onClicked: { if (timelineNotifier) timelineNotifier.toggleMagneticTimeline() }
        }

        // Duplicate frame detection toggle
        TBtn {
            iconText: "\u229A"; label: "Dupes"
            tooltip: {
                var on = timelineNotifier && timelineNotifier.duplicateFrameDetectionEnabled
                return on ? "Duplicate Frame Detection ON — highlights reused footage"
                          : "Duplicate Frame Detection OFF — click to highlight reused footage"
            }
            highlighted: timelineNotifier ? timelineNotifier.duplicateFrameDetectionEnabled : false
            highlightColor: "#EF5350"
            onClicked: { if (timelineNotifier) timelineNotifier.toggleDuplicateFrameDetection() }
        }

        TSep {}

        // Color legend
        TBtn {
            iconText: "\u25CF"; label: "Legend"; tooltip: "Clip color legend"
            iconColor: "#EF5350"
            onClicked: colorLegendPopup.open()

            Popup {
                id: colorLegendPopup
                y: parent.height + 4
                width: 180; padding: 8
                background: Rectangle { radius: 8; color: "#1A1A34"; border.color: "#303050" }

                ColumnLayout {
                    spacing: 4
                    Repeater {
                        model: [
                            { clr: "#42A5F5", lbl: "Video" },
                            { clr: "#FFEE58", lbl: "Image" },
                            { clr: "#AB47BC", lbl: "Title/Text" },
                            { clr: "#66BB6A", lbl: "Color / Audio" },
                            { clr: "#78909C", lbl: "Adjustment" }
                        ]
                        delegate: RowLayout {
                            spacing: 6
                            Rectangle { width: 10; height: 10; radius: 2; color: modelData.clr }
                            Label { text: modelData.lbl; font.pixelSize: 11; color: "#D0D0E8" }
                        }
                    }
                }
            }
        }

        TSep {}

        // Zoom controls
        RowLayout {
            spacing: 1
            Label {
                text: "\u2796"; font.pixelSize: 9; color: "#6B6B88"
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (timelineNotifier) timelineNotifier.zoomOut() } }
            }
            Slider {
                id: zoomSlider
                Layout.preferredWidth: 50
                from: 0; to: 1
                readonly property real logMin: Math.log(0.01)
                readonly property real logMax: Math.log(400)
                readonly property real logRange: logMax - logMin
                function ppsToSlider(p) { return (Math.log(Math.max(0.01, p)) - logMin) / logRange }
                function sliderToPps(s) { return Math.exp(logMin + s * logRange) }

                onMoved: {
                    if (timelineNotifier) timelineNotifier.setPixelsPerSecond(sliderToPps(value))
                }
                Binding on value {
                    value: zoomSlider.ppsToSlider(timelineNotifier ? timelineNotifier.pixelsPerSecond : 80)
                    when: !zoomSlider.pressed
                }
                background: Rectangle {
                    x: zoomSlider.leftPadding; y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - 1
                    width: zoomSlider.availableWidth; height: 2; radius: 1; color: "#1A1A34"
                    Rectangle { width: zoomSlider.visualPosition * parent.width; height: parent.height; radius: 1; color: "#6C63FF" }
                }
            }
            Label {
                text: "\u2795"; font.pixelSize: 9; color: "#6B6B88"
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (timelineNotifier) timelineNotifier.zoomIn() } }
            }
            Label {
                text: timelineNotifier ? Math.round(timelineNotifier.zoomPercentage) + "%" : "100%"
                font.pixelSize: 9; font.family: "monospace"; color: "#6B6B88"
                Layout.preferredWidth: 28
            }
        }

        // Autofit
        TBtn {
            iconText: "\u2922"; label: "Fit"; tooltip: "Zoom to fit all clips"
            onClicked: {
                if (!timelineNotifier) return
                var dur = timelineNotifier.totalDuration
                if (dur <= 0) return
                var fitPps = Math.max(0.01, Math.min(400, (root.width * 0.6) / dur))
                timelineNotifier.setPixelsPerSecond(fitPps)
            }
        }

        TSep {}

        // Proxy toggle
        Rectangle {
            width: proxyRow.implicitWidth + 12; height: 24; radius: 5
            color: timelineNotifier && timelineNotifier.useProxyPlayback ? Qt.rgba(0.4, 0.73, 0.42, 0.15) : "transparent"
            border.color: timelineNotifier && timelineNotifier.useProxyPlayback ? "#66BB6A" : "#303050"
            Layout.leftMargin: 2

            RowLayout {
                id: proxyRow; anchors.centerIn: parent; spacing: 4
                Rectangle {
                    width: 6; height: 6; radius: 3
                    color: timelineNotifier && timelineNotifier.useProxyPlayback ? "#66BB6A" : "#505068"
                }
                Label {
                    text: "Proxy"; font.pixelSize: 10; font.weight: Font.DemiBold
                    color: timelineNotifier && timelineNotifier.useProxyPlayback ? "#66BB6A" : "#6B6B88"
                }
            }
            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (timelineNotifier) timelineNotifier.toggleProxyPlayback() } }
            ToolTip.visible: proxyHov2.hovered; ToolTip.text: "Toggle proxy playback"
            HoverHandler { id: proxyHov2 }
        }

        TSep {}

        // Delete
        TBtn {
            iconText: "\uD83D\uDDD1"; label: "Delete"; tooltip: "Delete selected (Del)"
            iconColor: "#EF5350"
            enabled: timelineNotifier && timelineNotifier.selectedClipId >= 0
            onClicked: timelineNotifier.removeClip(timelineNotifier.selectedClipId)
        }

        TSep {}

        // Delete All Gaps
        TBtn {
            iconText: "\u2B1C"; label: "Gaps"; tooltip: "Delete all gaps on all unlocked tracks"
            iconColor: "#CC4444"
            onClicked: { if (timelineNotifier) timelineNotifier.deleteAllGaps() }
        }

        TSep {}

        // Expand All Tracks
        TBtn {
            iconText: "\u2195"; label: "Expand"; tooltip: "Expand all tracks to full height"
            onClicked: { if (timelineNotifier) timelineNotifier.expandAllTracks() }
        }

        // Collapse All Tracks
        TBtn {
            iconText: "\u2012"; label: "Collapse"; tooltip: "Collapse all tracks to minimum height"
            onClicked: { if (timelineNotifier) timelineNotifier.collapseAllTracks() }
        }
    }

    // ================================================================
    // Reusable inline components
    // ================================================================

    component TBtn: Item {
        property string iconText: ""
        property string label: ""
        property string tooltip: ""
        property string iconColor: ""
        property bool enabled: true
        property bool highlighted: false
        property string highlightColor: ""
        signal clicked()

        width: Math.max(col.implicitWidth + 10, 32); height: 34
        Layout.leftMargin: 1; Layout.rightMargin: 1
        opacity: enabled ? 1.0 : 0.3

        Rectangle {
            anchors.fill: parent; radius: 3
            color: highlighted && highlightColor !== ""
                ? Qt.rgba(Qt.color(highlightColor).r, Qt.color(highlightColor).g, Qt.color(highlightColor).b, 0.12)
                : (hov2.hovered ? Qt.rgba(1,1,1,0.04) : "transparent")
            border.color: highlighted && highlightColor !== "" ? highlightColor : "transparent"
            border.width: highlighted ? 1 : 0
        }

        Column {
            id: col; anchors.centerIn: parent; spacing: 1
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: iconText; font.pixelSize: 13
                color: {
                    if (highlighted && highlightColor !== "") return highlightColor
                    if (iconColor !== "") return iconColor
                    return "#D0D0E8"
                }
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: label; font.pixelSize: 8
                color: {
                    if (highlighted && highlightColor !== "") return highlightColor
                    return "#7878A0"
                }
            }
        }
        MouseArea {
            anchors.fill: parent; enabled: parent.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
        ToolTip.visible: tooltip !== "" && hov2.hovered; ToolTip.text: tooltip
        HoverHandler { id: hov2 }
    }

    component TSep: Rectangle {
        width: 1; height: 20; color: "#252540"
        Layout.leftMargin: 3; Layout.rightMargin: 3
    }
}
