import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * VE2TimelinePanel — ruler + track lanes + clips + playhead + drag-drop.
 *
 * Layout:
 *   ┌──────────────────────────────────────────────────┐
 *   │ [header 100px] │ [TimeRuler ───────────────────] │  row 0
 *   ├──────────────────────────────────────────────────┤
 *   │ [Track headers] │ [Clip lanes (scrollable) ────] │  row 1
 *   ├──────────────────────────────────────────────────┤
 *   │ [zoom ─ scrollbar ─────────────────────────────] │  row 2
 *   └──────────────────────────────────────────────────┘
 *
 * Cross-platform: uses Flickable + PinchArea for touch zoom on mobile.
 */
Item {
    id: root

    readonly property real tabBarHeight: timelineNotifier && timelineNotifier.timelineTabCount > 1 ? 28 : 0
    readonly property real rulerHeight: 32
    readonly property real footerHeight: 28
    readonly property real headerWidth: 100
    readonly property real defaultTrackHeight: 64

    readonly property real pps: timelineNotifier ? timelineNotifier.pixelsPerSecond : 100
    // Dynamic timeline duration: always 10s beyond last clip, minimum 30s
    readonly property real totalDur: timelineNotifier && timelineNotifier.isReady
                                     ? Math.max(timelineNotifier.totalDuration + 10, 30) : 30

    clip: true

    // ---- Dynamic new-track preview state ----
    property bool _showNewTrackAbove: false
    property bool _showNewTrackBelow: false
    property bool _anyClipDragging: false

    // ---- Razor cut animation state ----
    property real _razorCutX: 0
    property real _razorCutTrackY: 0
    property bool _razorCutVisible: false

    // ---- JKL key state tracking for combo detection ----
    property bool _jKeyHeld: false
    property bool _kKeyHeld: false
    property bool _lKeyHeld: false

    Timer {
        id: jkComboTimer
        interval: 80; repeat: true
        running: root._kKeyHeld && root._jKeyHeld && !root._lKeyHeld
        onTriggered: { if (timelineNotifier) timelineNotifier.stepBackward() }
    }
    Timer {
        id: klComboTimer
        interval: 80; repeat: true
        running: root._kKeyHeld && root._lKeyHeld && !root._jKeyHeld
        onTriggered: { if (timelineNotifier) timelineNotifier.stepForward() }
    }

    // ---- Global snap state ----
    property bool snapEnabled: true
    property real _snapGuideX: -1         // viewport x of active snap line (-1=hidden)
    property bool _snapGuideVisible: false

    // ---- Drag reorder insertion indicator ----
    property real _insertIndicatorX: -1   // viewport x of insertion line (-1=hidden)
    property real _insertIndicatorTime: -1 // time of insertion point
    property bool _insertIndicatorVisible: false

    // ---- Feature 13: Multi-select lasso state ----
    property bool _lassoActive: false
    property real _lassoStartX: 0
    property real _lassoStartY: 0
    property real _lassoEndX: 0
    property real _lassoEndY: 0

    // ---- Feature 12: Split animation ----
    property real _splitLineX: -1
    property bool _splitLineVisible: false

    // ---- Stacked timelines ----
    property real stackedDividerRatio: 0.6
    readonly property bool isStacked: timelineNotifier && timelineNotifier.stackedView
    readonly property real secondaryPps: isStacked && timelineNotifier
        ? timelineNotifier.secondaryPixelsPerSecond : 80
    readonly property real secondaryTotalDur: isStacked && timelineNotifier
        ? Math.max(timelineNotifier.secondaryTotalDuration + 10, 30) : 30
    property bool _crossTimelineDragActive: false
    property int _crossTimelineDragTrack: -1
    property real _crossTimelineDragTime: 0

    // ---- Auto-fit on import ----
    property bool autoFitOnImport: true

    // ---- Cut-point transition preview ----
    property bool _cutPreviewVisible: false
    property real _cutPreviewX: 0
    property real _cutPreviewY: 0
    property string _cutPreviewSourceA: ""
    property string _cutPreviewSourceB: ""
    property real _cutPreviewTimeA: 0
    property real _cutPreviewTimeB: 0
    property string _cutPreviewNameA: ""
    property string _cutPreviewNameB: ""
    property int _cutPreviewTransType: 0
    property real _cutPreviewTransDur: 0

    Timer {
        id: cutPreviewTimer
        interval: 500; repeat: false
        onTriggered: root._cutPreviewVisible = true
    }

    // Reset position override when focus is lost
    onActiveFocusChanged: {
        if (!activeFocus && timelineNotifier && timelineNotifier.positionOverrideActive)
            timelineNotifier.setPositionOverride(false)
    }

    Component.onCompleted: console.log("[VE2Timeline] created, size:", width, "x", height)

    // Auto-remove empty dynamically-created tracks after 2 seconds of idle
    Timer {
        id: emptyTrackCleanupTimer
        interval: 2000; repeat: false
        onTriggered: {
            if (!timelineNotifier || !timelineNotifier.isReady) return
            var tracks = timelineNotifier.tracks
            if (!tracks) return
            // Remove tracks from bottom up (to avoid index shifting issues)
            // Only remove tracks with 0 clips and index > 2 (keep the 3 default tracks)
            for (var i = tracks.length - 1; i >= 3; i--) {
                if (timelineNotifier.clipCountForTrack(i) === 0) {
                    timelineNotifier.removeTrack(i)
                }
            }
        }
    }

    Rectangle { anchors.fill: parent; color: "#0A0A18" }

    // ================================================================
    // Row 0: Ruler bar
    // ================================================================
    // Timeline tab bar (only shown when multiple tabs exist)
    // ================================================================
    Rectangle {
        id: tabBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: tabBarHeight
        color: "#08081A"
        visible: tabBarHeight > 0
        z: 101

        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#1E1E38" }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 4; anchors.rightMargin: 4
            spacing: 1

            // Breadcrumb path
            Label {
                text: timelineNotifier ? timelineNotifier.breadcrumbPath : "Project"
                font.pixelSize: 9; color: "#6B6B88"
                Layout.leftMargin: 4
                visible: timelineNotifier && timelineNotifier.breadcrumbPath.indexOf(">") >= 0
            }

            Item { Layout.preferredWidth: 8; visible: timelineNotifier && timelineNotifier.breadcrumbPath.indexOf(">") >= 0 }

            // Tab buttons
            Repeater {
                model: timelineNotifier ? timelineNotifier.timelineTabs : []
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    readonly property bool isActive: modelData.isActive
                    Layout.preferredWidth: tabLbl.implicitWidth + 24 + (closeBtn.visible ? 16 : 0)
                    Layout.preferredHeight: 22
                    radius: 4
                    color: isActive ? "#1E1E38" : (tabHov.hovered ? Qt.rgba(1,1,1,0.04) : "transparent")
                    border.color: isActive ? "#6C63FF" : "transparent"
                    border.width: isActive ? 1 : 0

                    RowLayout {
                        anchors.centerIn: parent; spacing: 2
                        Label {
                            id: tabLbl
                            text: modelData.label || ("Tab " + (index + 1))
                            font.pixelSize: 10; font.weight: isActive ? Font.Bold : Font.Normal
                            color: isActive ? "#E0E0F0" : "#8888A0"
                        }
                        // Close button (not on last tab or if it's the only one)
                        Label {
                            id: closeBtn
                            visible: timelineNotifier && timelineNotifier.timelineTabCount > 1
                            text: "\u2715"; font.pixelSize: 8; color: "#6B6B88"
                            MouseArea {
                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: { if (timelineNotifier) timelineNotifier.removeTimelineTab(index) }
                            }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent; z: -1; cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton
                        onClicked: { if (timelineNotifier) timelineNotifier.switchTimelineTab(index) }
                    }
                    HoverHandler { id: tabHov }
                }
            }

            // "+" button to add new tab
            Rectangle {
                Layout.preferredWidth: 22; Layout.preferredHeight: 22; radius: 4
                color: addTabHov.hovered ? Qt.rgba(1,1,1,0.06) : "transparent"
                Label {
                    anchors.centerIn: parent; text: "+"; font.pixelSize: 14; color: "#6B6B88"
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (timelineNotifier) timelineNotifier.addTimelineTab() }
                }
                HoverHandler { id: addTabHov }
                ToolTip.visible: addTabHov.hovered; ToolTip.text: "New timeline tab"
            }

            Item { Layout.fillWidth: true }

            // Stacked view toggle
            Rectangle {
                Layout.preferredWidth: stackLbl.implicitWidth + 12; Layout.preferredHeight: 22; radius: 4
                visible: timelineNotifier && timelineNotifier.timelineTabCount > 1
                color: timelineNotifier && timelineNotifier.stackedView
                    ? Qt.rgba(0.42, 0.39, 1.0, 0.15) : (stackHov.hovered ? Qt.rgba(1,1,1,0.04) : "transparent")
                border.color: timelineNotifier && timelineNotifier.stackedView ? "#6C63FF" : "transparent"
                Label {
                    id: stackLbl; anchors.centerIn: parent
                    text: "\u2261 Stacked"
                    font.pixelSize: 9; font.weight: Font.DemiBold
                    color: timelineNotifier && timelineNotifier.stackedView ? "#6C63FF" : "#6B6B88"
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (timelineNotifier) timelineNotifier.setStackedView(!timelineNotifier.stackedView) }
                }
                HoverHandler { id: stackHov }
                ToolTip.visible: stackHov.hovered; ToolTip.text: "Toggle stacked timeline view"
            }

            // Back button for compound clips
            Rectangle {
                Layout.preferredWidth: backLbl.implicitWidth + 12; Layout.preferredHeight: 22; radius: 4
                visible: timelineNotifier && timelineNotifier.timelineTabCount > 0
                    && timelineNotifier.timelineTabs.length > 0
                    && timelineNotifier.activeTimelineIndex >= 0
                    && timelineNotifier.activeTimelineIndex < timelineNotifier.timelineTabs.length
                    && timelineNotifier.timelineTabs[timelineNotifier.activeTimelineIndex].isCompound
                color: backHov.hovered ? Qt.rgba(1,1,1,0.06) : "transparent"
                Label {
                    id: backLbl; anchors.centerIn: parent
                    text: "\u2190 Back"
                    font.pixelSize: 9; font.weight: Font.DemiBold; color: "#8888A0"
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (timelineNotifier) timelineNotifier.closeCompoundClip() }
                }
                HoverHandler { id: backHov }
            }
        }
    }

    // ================================================================
    Row {
        id: rulerRow
        anchors.top: tabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: rulerHeight

        // Header spacer
        Rectangle {
            width: headerWidth; height: parent.height
            color: "#10102A"
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: "#1E1E38" }
        }

        // Ruler viewport
        Item {
            width: parent.width - headerWidth
            height: parent.height
            clip: true

            Canvas {
                id: rulerCanvas
                width: parent.width     // viewport-sized only — not content-sized
                height: parent.height

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.fillStyle = "#10102A"
                    ctx.fillRect(0, 0, width, height)
                    ctx.fillStyle = "#1E1E38"
                    ctx.fillRect(0, height - 1, width, 1)

                    var interval = internal.bestInterval(pps)
                    if (interval <= 0) return

                    // Scroll offset — draw in viewport coordinates
                    var scrollX = trackFlick.contentX
                    var visStart = Math.max(0, scrollX - interval * pps)
                    var visEnd = scrollX + width + interval * pps
                    var firstTick = Math.floor(visStart / (interval * pps))
                    var lastTick = Math.ceil(visEnd / (interval * pps))

                    // In/Out range
                    if (timelineNotifier && timelineNotifier.hasInPoint && timelineNotifier.hasOutPoint) {
                        var rs = timelineNotifier.inPoint * pps - scrollX
                        var re = timelineNotifier.outPoint * pps - scrollX
                        if (re > rs) {
                            ctx.fillStyle = Qt.rgba(0.424, 0.388, 1.0, 0.25)
                            ctx.fillRect(rs, height - 3, re - rs, 3)
                        }
                    }

                    ctx.font = "10px monospace"
                    for (var i = firstTick; i <= lastTick; i++) {
                        var t = i * interval
                        if (t < 0 || t > totalDur) continue
                        var xp = t * pps - scrollX   // viewport-relative x

                        ctx.strokeStyle = "#353550"
                        ctx.lineWidth = 1
                        ctx.beginPath()
                        ctx.moveTo(xp, height - 10)
                        ctx.lineTo(xp, height - 1)
                        ctx.stroke()

                        ctx.fillStyle = "#7070A0"
                        ctx.fillText(internal.formatTime(t), xp + 3, 12)

                        ctx.strokeStyle = "#252540"
                        var sub = interval / 4
                        for (var j = 1; j < 4; j++) {
                            var st = t + j * sub
                            if (st > totalDur) break
                            var sx = st * pps - scrollX
                            ctx.beginPath()
                            ctx.moveTo(sx, height - 5)
                            ctx.lineTo(sx, height - 1)
                            ctx.stroke()
                        }
                    }
                }

                // Click to seek (left-click only)
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: mouse => {
                        if (timelineNotifier) {
                            var time = Math.max(0, Math.min((mouse.x + trackFlick.contentX) / pps, totalDur))
                            timelineNotifier.scrubTo(time)
                        }
                    }
                    onPositionChanged: mouse => {
                        if (pressed && timelineNotifier) {
                            var time = Math.max(0, Math.min((mouse.x + trackFlick.contentX) / pps, totalDur))
                            timelineNotifier.scrubTo(time)
                        }
                    }
                }
            }

            // ---- Marker region spans on ruler ----
            Repeater {
                model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.markers : []
                delegate: Rectangle {
                    required property var modelData
                    visible: modelData.isRegion === true
                    readonly property real startX: (modelData.position || 0) * pps - trackFlick.contentX
                    readonly property real endX: (modelData.endPosition || 0) * pps - trackFlick.contentX
                    x: startX
                    y: 0
                    width: Math.max(0, endX - startX)
                    height: parent.height
                    color: Qt.rgba(
                        parseInt((modelData.color || "#4CAF50").substr(1,2), 16) / 255,
                        parseInt((modelData.color || "#4CAF50").substr(3,2), 16) / 255,
                        parseInt((modelData.color || "#4CAF50").substr(5,2), 16) / 255,
                        0.15)
                    border.color: modelData.color || "#4CAF50"
                    border.width: 0.5

                    Label {
                        anchors.centerIn: parent
                        text: modelData.label || ""
                        font.pixelSize: 8; font.weight: Font.Medium
                        color: modelData.color || "#4CAF50"
                        opacity: 0.8
                        visible: parent.width > 40
                        elide: Text.ElideRight
                        width: Math.min(implicitWidth, parent.width - 8)
                    }
                }
            }

            // ---- Marker triangles on ruler ----
            Repeater {
                model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.markers : []
                delegate: Item {
                    required property var modelData
                    readonly property real markerX: (modelData.position || 0) * pps - trackFlick.contentX
                    readonly property string markerColor: modelData.color || "#4CAF50"
                    readonly property int markerId: modelData.id || 0
                    readonly property string markerLabel: modelData.label || ""
                    readonly property int markerType: modelData.type !== undefined ? modelData.type : 0

                    x: markerX - 6
                    y: 0
                    width: 12
                    height: parent.height
                    visible: markerX > -12 && markerX < parent.width + 12
                    z: 5

                    // Triangle marker on ruler
                    Canvas {
                        id: markerTriangle
                        width: 12; height: 10
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.fillStyle = markerColor
                            ctx.beginPath()
                            ctx.moveTo(0, height)
                            ctx.lineTo(6, 0)
                            ctx.lineTo(12, height)
                            ctx.closePath()
                            ctx.fill()
                        }
                    }

                    // Marker type icon
                    Label {
                        anchors.top: parent.top
                        anchors.topMargin: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: {
                            var icons = ["\u25C6", "\u25AC", "\u2713", "\u25CE"]  // Chapter, Comment, Todo, Sync
                            return icons[Math.min(markerType, icons.length - 1)]
                        }
                        font.pixelSize: 7
                        color: markerColor
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                markerContextMenu.targetMarkerId = markerId
                                markerContextMenu.popup()
                            } else {
                                if (timelineNotifier) timelineNotifier.navigateToMarker(markerId)
                            }
                        }
                        onDoubleClicked: {
                            markerEditDialog.openForMarker(markerId)
                        }

                        ToolTip.visible: containsMouse
                        ToolTip.delay: 300
                        ToolTip.text: {
                            var typeNames = ["Chapter", "Comment", "ToDo", "Sync"]
                            var t = typeNames[Math.min(markerType, 3)]
                            var pos = internal.formatTimecodeFF(modelData.position || 0)
                            return "[" + t + "] " + markerLabel + " @ " + pos
                        }
                    }
                }
            }

            // Ruler click — right-click to place marker, left-click to seek
            MouseArea {
                id: rulerClickArea
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                z: -1
                onClicked: function(mouse) {
                    if (mouse.button === Qt.RightButton) {
                        var time = Math.max(0, Math.min((mouse.x + trackFlick.contentX) / pps, totalDur))
                        rulerContextMenu._clickTime = time
                        rulerContextMenu.popup()
                    }
                }
            }

            // Throttled repaint — avoid repainting ruler every frame during playback
            Timer {
                id: rulerRepaintTimer
                interval: 100; repeat: false
                onTriggered: rulerCanvas.requestPaint()
            }
            Connections {
                target: timelineNotifier
                function onPlaybackChanged() { if (!rulerRepaintTimer.running) rulerRepaintTimer.start() }
                function onStateChanged() { if (!rulerRepaintTimer.running) rulerRepaintTimer.start() }
            }
            // Repaint ruler when zoom, scroll, or size changes
            Connections {
                target: root
                function onPpsChanged() { rulerCanvas.requestPaint() }
            }
            Connections {
                target: trackFlick
                function onContentXChanged() { rulerCanvas.requestPaint() }
            }
            Component.onCompleted: rulerCanvas.requestPaint()
        }
    }

    // ================================================================
    // Row 1: Track area (fixed headers + scrollable clip lanes)
    // ================================================================
    Row {
        id: trackRow
        anchors.top: rulerRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: isStacked ? stackedDivider.top : footer.top

        // --- Fixed track headers ---
        Flickable {
            id: headerFlick
            width: headerWidth
            height: parent.height
            contentHeight: trackFlick.contentHeight
            contentY: trackFlick.contentY
            interactive: false
            clip: true

            Column {
                width: headerWidth

                Repeater {
                    model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.trackCount : 0
                    delegate: Rectangle {
                        id: trackHeaderDelegate
                        required property int index
                        width: headerWidth
                        // Feature 15: hidden tracks collapse to thin bar
                        readonly property var tInfo: timelineNotifier && timelineNotifier.trackGeneration >= 0
                                                        ? timelineNotifier.trackInfo(index) : ({})
                        readonly property bool trackVisible: tInfo.isVisible !== undefined ? tInfo.isVisible : true
                        readonly property bool trackLocked: tInfo.isLocked !== undefined ? tInfo.isLocked : false
                        readonly property bool trackMuted: tInfo.isMuted !== undefined ? tInfo.isMuted : false
                        readonly property bool trackSoloed: tInfo.isSolo !== undefined ? tInfo.isSolo : false
                        readonly property string trackLabel: tInfo.label || ("Track " + (index + 1))
                        readonly property string trackColor: tInfo.color || ""
                        readonly property double perTrackHeight: tInfo.trackHeight !== undefined ? tInfo.trackHeight : defaultTrackHeight

                        height: trackVisible ? perTrackHeight : 12
                        color: trackSoloed ? "#1A1608" : (trackLocked ? "#0E0A1A" : "#10102A")
                        border.color: trackSoloed ? "#FFCA28" : "#1E1E38"
                        border.width: trackSoloed ? 1 : 0.5

                        Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }

                        // Resize handle at bottom edge of header
                        Rectangle {
                            anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
                            height: 4; z: 10
                            color: resizeHandleMa.containsMouse || resizeHandleMa.pressed ? "#6C63FF" : "transparent"
                            visible: trackVisible

                            MouseArea {
                                id: resizeHandleMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.SizeVerCursor
                                property real startY: 0
                                property real startHeight: 0

                                onPressed: function(mouse) {
                                    startY = mapToItem(null, 0, mouse.y).y
                                    startHeight = perTrackHeight
                                }
                                onPositionChanged: function(mouse) {
                                    if (pressed && timelineNotifier) {
                                        var globalY = mapToItem(null, 0, mouse.y).y
                                        var newHeight = Math.max(28, Math.min(200, startHeight + (globalY - startY)))
                                        timelineNotifier.setTrackHeight(trackHeaderDelegate.index, newHeight)
                                    }
                                }
                                onDoubleClicked: {
                                    // Toggle between collapsed (28) and expanded (120)
                                    if (timelineNotifier) {
                                        var target = perTrackHeight <= 40 ? 120 : 28
                                        timelineNotifier.setTrackHeight(trackHeaderDelegate.index, target)
                                    }
                                }
                            }
                            ToolTip.visible: resizeHandleMa.containsMouse && !resizeHandleMa.pressed
                            ToolTip.text: "Drag to resize track, double-click to toggle"
                        }

                        // Track color strip on left edge
                        Rectangle {
                            visible: trackColor !== "" && trackVisible
                            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                            width: 3; color: trackColor; z: 2
                        }

                        // Right-click for track color menu
                        MouseArea {
                            anchors.fill: parent; z: -1
                            acceptedButtons: Qt.RightButton
                            onClicked: function(mouse) {
                                trackColorMenu._trackIdx = trackHeaderDelegate.index
                                trackColorMenu.popup()
                            }
                        }

                        // Collapsed hidden track bar
                        Rectangle {
                            anchors.fill: parent
                            visible: !trackVisible
                            color: "#08081A"
                            Label {
                                anchors.centerIn: parent
                                text: trackLabel + " (hidden)"
                                font.pixelSize: 8; color: "#404060"
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: { if (timelineNotifier) timelineNotifier.toggleTrackVisibility(index) }
                                cursorShape: Qt.PointingHandCursor
                            }
                        }

                        // Full track header (visible tracks)
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 2
                            visible: trackVisible

                            RowLayout {
                                spacing: 3

                                // Track type icon
                                Label {
                                    text: {
                                        var t = tInfo.type !== undefined ? tInfo.type : 0
                                        var icons = ["\uD83C\uDFA5", "\uD83C\uDFB5", "\uD83D\uDCDD", "\u2728", "\uD83D\uDCCC"]
                                        return icons[Math.min(t, icons.length - 1)]
                                    }
                                    font.pixelSize: 11
                                }

                                // Magnetic primary indicator
                                Label {
                                    visible: tInfo.isMagneticPrimary === true
                                        && timelineNotifier && timelineNotifier.magneticTimelineEnabled
                                    text: "\u25C9"
                                    font.pixelSize: 9
                                    color: "#FF7043"
                                }

                                // Editable track name (Feature 15)
                                Label {
                                    id: trackNameLabel
                                    text: trackLabel
                                    font.pixelSize: 10; font.weight: Font.Medium
                                    color: trackLocked ? "#6B6B88" : "#C0C0D8"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                    visible: !trackNameEditor.visible

                                    MouseArea {
                                        anchors.fill: parent
                                        onDoubleClicked: {
                                            trackNameEditor.text = trackLabel
                                            trackNameEditor.visible = true
                                            trackNameEditor.forceActiveFocus()
                                            trackNameEditor.selectAll()
                                        }
                                    }
                                }
                                TextField {
                                    id: trackNameEditor
                                    visible: false
                                    font.pixelSize: 10
                                    color: "#E0E0F0"
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 18
                                    background: Rectangle { radius: 2; color: "#1A1A34"; border.color: "#6C63FF" }
                                    onAccepted: {
                                        if (timelineNotifier && text.trim() !== "")
                                            timelineNotifier.renameTrack(trackHeaderDelegate.index, text.trim())
                                        visible = false
                                    }
                                    onFocusChanged: { if (!focus) visible = false }
                                    Keys.onEscapePressed: visible = false
                                }
                            }

                            RowLayout {
                                spacing: 1

                                // Eye icon — visibility toggle (Feature 15)
                                Rectangle {
                                    width: 22; height: 18; radius: 3
                                    color: trackVisible ? Qt.rgba(0.4, 0.73, 0.42, 0.15) : Qt.rgba(0.5, 0.5, 0.5, 0.1)
                                    border.color: trackVisible ? "#66BB6A" : "#404060"
                                    border.width: 0.5
                                    Label {
                                        anchors.centerIn: parent
                                        text: trackVisible ? "\uD83D\uDC41" : "\u2014"
                                        font.pixelSize: 10; color: trackVisible ? "#66BB6A" : "#505068"
                                    }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: { if (timelineNotifier) timelineNotifier.toggleTrackVisibility(trackHeaderDelegate.index) }
                                    }
                                    ToolTip.text: trackVisible ? "Hide track" : "Show track"
                                    ToolTip.visible: visHov.hovered
                                    HoverHandler { id: visHov }
                                }

                                // Lock icon — lock toggle (Feature 15)
                                Rectangle {
                                    width: 22; height: 18; radius: 3
                                    color: trackLocked ? Qt.rgba(0.94, 0.33, 0.31, 0.15) : Qt.rgba(0.5, 0.5, 0.5, 0.1)
                                    border.color: trackLocked ? "#EF5350" : "#404060"
                                    border.width: 0.5
                                    Label {
                                        anchors.centerIn: parent
                                        text: trackLocked ? "\uD83D\uDD12" : "\uD83D\uDD13"
                                        font.pixelSize: 10; color: trackLocked ? "#EF5350" : "#505068"
                                    }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: { if (timelineNotifier) timelineNotifier.toggleTrackLock(trackHeaderDelegate.index) }
                                    }
                                    ToolTip.text: trackLocked ? "Unlock track" : "Lock track"
                                    ToolTip.visible: lockHov.hovered
                                    HoverHandler { id: lockHov }
                                }

                                // Mute toggle
                                Rectangle {
                                    width: 22; height: 18; radius: 3
                                    color: trackMuted ? Qt.rgba(1, 0.79, 0.16, 0.15) : Qt.rgba(0.5, 0.5, 0.5, 0.1)
                                    border.color: trackMuted ? "#FFCA28" : "#404060"
                                    border.width: 0.5
                                    Label {
                                        anchors.centerIn: parent
                                        text: trackMuted ? "\uD83D\uDD07" : "\uD83D\uDD0A"
                                        font.pixelSize: 10; color: trackMuted ? "#FFCA28" : "#505068"
                                    }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: { if (timelineNotifier) timelineNotifier.toggleTrackMute(trackHeaderDelegate.index) }
                                    }
                                    ToolTip.text: trackMuted ? "Unmute track" : "Mute track"
                                    ToolTip.visible: muteHov.hovered
                                    HoverHandler { id: muteHov }
                                }

                                // Solo button
                                Rectangle {
                                    id: soloBtn
                                    property bool isSoloed: tInfo.isSolo !== undefined ? tInfo.isSolo : false
                                    width: 18; height: 18; radius: 3
                                    color: soloBtn.isSoloed ? Qt.rgba(1, 0.79, 0.16, 0.15) : "transparent"
                                    border.color: soloBtn.isSoloed ? "#FFCA28" : "#353550"
                                    border.width: 0.5
                                    Label {
                                        anchors.centerIn: parent
                                        text: "S"
                                        font.pixelSize: 9; font.weight: Font.Bold
                                        color: soloBtn.isSoloed ? "#FFCA28" : "#505068"
                                    }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: { if (timelineNotifier) timelineNotifier.toggleTrackSolo(trackHeaderDelegate.index) }
                                    }
                                    ToolTip.text: soloBtn.isSoloed ? "Unsolo track (Alt+S)" : "Solo track (Alt+S)"
                                    ToolTip.visible: soloHov.hovered
                                    HoverHandler { id: soloHov }
                                }
                            }

                            // Track volume/pan (compact inline sliders — shown on tall tracks)
                            Row {
                                x: 4; width: headerWidth - 12
                                y: trackHeaderDelegate.height - 18
                                spacing: 2
                                visible: trackHeaderDelegate.height > 80

                                Label { text: "V"; font.pixelSize: 8; color: "#505068"; width: 10 }
                                Slider {
                                    id: trackVolSlider
                                    width: (parent.width - 24) / 2; height: 14
                                    from: 0; to: 2; value: tInfo.volume !== undefined ? tInfo.volume : 1.0
                                    onMoved: { if (timelineNotifier) timelineNotifier.setTrackVolume(trackHeaderDelegate.index, value) }
                                    background: Rectangle {
                                        x: trackVolSlider.leftPadding; y: trackVolSlider.topPadding + trackVolSlider.availableHeight / 2 - 1
                                        width: trackVolSlider.availableWidth; height: 2; radius: 1; color: "#1A1A34"
                                        Rectangle { width: trackVolSlider.visualPosition * parent.width; height: parent.height; radius: 1; color: "#6C63FF" }
                                    }
                                }
                                Label { text: "P"; font.pixelSize: 8; color: "#505068"; width: 10 }
                                Slider {
                                    id: trackPanSlider
                                    width: (parent.width - 24) / 2; height: 14
                                    from: -1; to: 1; value: tInfo.pan !== undefined ? tInfo.pan : 0.0
                                    onMoved: { if (timelineNotifier) timelineNotifier.setTrackPan(trackHeaderDelegate.index, value) }
                                    background: Rectangle {
                                        x: trackPanSlider.leftPadding; y: trackPanSlider.topPadding + trackPanSlider.availableHeight / 2 - 1
                                        width: trackPanSlider.availableWidth; height: 2; radius: 1; color: "#1A1A34"
                                        Rectangle { width: trackPanSlider.visualPosition * parent.width; height: parent.height; radius: 1; color: "#26C6DA" }
                                    }
                                }
                            }
                        }

                        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: "#1E1E38" }
                    }
                }
            }
        }

        // --- Scrollable clip lanes ---
        Flickable {
            id: trackFlick
            width: parent.width - headerWidth
            height: parent.height
            clip: true
            contentWidth: Math.max(width, totalDur * pps)
            contentHeight: lanesColumn.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.AutoFlickIfNeeded

            property bool externalScrollUpdate: false

            onContentXChanged: {
                if (!externalScrollUpdate && timelineNotifier)
                    timelineNotifier.setScrollOffset(contentX)
            }

            // Sync scroll offset from C++ (e.g. after zoom recalculates anchor)
            Connections {
                target: timelineNotifier
                function onZoomChanged() {
                    if (!trackFlick.moving && !trackFlick.dragging) {
                        var target = timelineNotifier.scrollOffset
                        if (Math.abs(trackFlick.contentX - target) > 1) {
                            trackFlick.externalScrollUpdate = true
                            trackFlick.contentX = target
                            trackFlick.externalScrollUpdate = false
                        }
                    }
                }
            }

            Column {
                id: lanesColumn
                width: trackFlick.contentWidth

                // ---- "New track above" drop preview zone ----
                Rectangle {
                    id: newTrackAboveZone
                    width: lanesColumn.width
                    height: root._showNewTrackAbove ? defaultTrackHeight : 0
                    visible: root._showNewTrackAbove
                    color: Qt.rgba(0.424, 0.388, 1.0, 0.08)
                    border.color: "#6C63FF"; border.width: 1.5; radius: 2
                    Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }

                    RowLayout {
                        anchors.centerIn: parent; spacing: 6
                        Label { text: "\u2795"; font.pixelSize: 14; color: "#6C63FF" }
                        Label { text: "New Video Track"; font.pixelSize: 11; color: "#6C63FF"; font.weight: Font.DemiBold }
                    }
                }

                Repeater {
                    model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.trackCount : 0
                    delegate: Item {
                        id: trackLane
                        required property int index
                        property int trackIdx: index  // stable alias for child access
                        width: lanesColumn.width
                        // Feature 15: match header collapsed height for hidden tracks
                        // trackCount dependency ensures re-eval on tracksChanged
                        // trackGeneration ensures re-eval on any track/clip change
                        readonly property var laneInfo: timelineNotifier && timelineNotifier.trackGeneration >= 0
                                                        ? timelineNotifier.trackInfo(index) : ({})
                        readonly property bool laneVisible: laneInfo.isVisible !== undefined ? laneInfo.isVisible : true
                        readonly property bool laneLocked: laneInfo.isLocked !== undefined ? laneInfo.isLocked : false
                        readonly property string laneColor: laneInfo.color || ""
                        readonly property double laneTrackHeight: laneInfo.trackHeight !== undefined ? laneInfo.trackHeight : defaultTrackHeight
                        height: laneVisible ? laneTrackHeight : 12

                        Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }

                        // Lane background
                        Rectangle {
                            anchors.fill: parent
                            color: trackLane.laneLocked
                                ? (trackLane.index % 2 === 0 ? "#0A081C" : "#0C0A1E")
                                : (trackLane.index % 2 === 0 ? "#0C0C20" : "#0E0E24")

                            // Track color tint overlay
                            Rectangle {
                                anchors.fill: parent; visible: laneColor !== ""
                                color: laneColor !== "" ? laneColor : "transparent"
                                opacity: 0.06
                            }

                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 0.5; color: "#1E1E38" }

                            // Feature 13: Empty space click handler for deselect + lasso
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                z: -1  // Below clips

                                property real dragOriginX: 0
                                property real dragOriginY: 0
                                property bool isDragging: false

                                onPressed: function(mouse) {
                                    dragOriginX = mouse.x
                                    dragOriginY = mouse.y
                                    isDragging = false
                                }
                                onPositionChanged: function(mouse) {
                                    if (!pressed) return
                                    var dx = Math.abs(mouse.x - dragOriginX)
                                    var dy = Math.abs(mouse.y - dragOriginY)
                                    if (!isDragging && (dx > 5 || dy > 5)) {
                                        isDragging = true
                                        root._lassoActive = true
                                        // Convert to root coordinates
                                        var startPt = mapToItem(root, dragOriginX, dragOriginY)
                                        root._lassoStartX = startPt.x
                                        root._lassoStartY = startPt.y
                                    }
                                    if (isDragging) {
                                        var curPt = mapToItem(root, mouse.x, mouse.y)
                                        root._lassoEndX = curPt.x
                                        root._lassoEndY = curPt.y
                                    }
                                }
                                onReleased: function(mouse) {
                                    if (isDragging && timelineNotifier) {
                                        internal.performLassoSelect()
                                    } else {
                                        // Click on empty space — deselect all
                                        if (timelineNotifier) timelineNotifier.deselectAll()
                                    }
                                    isDragging = false
                                    root._lassoActive = false
                                }
                            }
                        }

                        // Clips
                        Repeater {
                            model: timelineNotifier && timelineNotifier.trackCount >= 0 && laneVisible
                                   ? timelineNotifier.clipCountForTrack(trackLane.trackIdx) : 0
                            delegate: VE2ClipItem {
                                required property int index
                                clipIndex: index
                                trackIndex: trackLane.trackIdx
                                y: 3
                                height: trackLane.height - 6
                            }
                        }

                        // Gap overlays — highlight empty gaps between clips
                        Repeater {
                            id: gapRepeater
                            model: {
                                var g = timelineNotifier ? timelineNotifier.trackGeneration : 0
                                if (!timelineNotifier || !laneVisible || trackLane.laneLocked) return []
                                return timelineNotifier.detectGaps(trackLane.trackIdx)
                            }
                            delegate: Rectangle {
                                readonly property var gapData: modelData || ({})
                                readonly property double gapStart: gapData.start || 0
                                readonly property double gapEnd: gapData.end || 0
                                readonly property double gapDuration: gapData.duration || 0

                                x: gapStart * pps
                                width: Math.max(2, gapDuration * pps)
                                y: 1; height: trackLane.height - 2
                                z: 5
                                color: Qt.rgba(0.6, 0.15, 0.15, 0.25)
                                border.color: Qt.rgba(0.8, 0.2, 0.2, 0.4)
                                border.width: 0.5
                                radius: 2

                                // Dashed center line
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: parent.width - 4; height: 1
                                    color: Qt.rgba(0.8, 0.2, 0.2, 0.3)
                                    visible: parent.width > 10
                                }

                                // Gap duration label
                                Label {
                                    anchors.centerIn: parent
                                    visible: parent.width > 40
                                    text: gapDuration.toFixed(1) + "s"
                                    font.pixelSize: 9; font.weight: Font.DemiBold
                                    color: "#CC4444"; opacity: 0.8
                                }

                                // Hover tooltip with gap duration
                                ToolTip.visible: gapHov.hovered
                                ToolTip.text: "Gap: " + gapDuration.toFixed(2) + "s"
                                HoverHandler { id: gapHov }

                                // Right-click for gap context menu
                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.RightButton | Qt.LeftButton
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: function(mouse) {
                                        if (mouse.button === Qt.RightButton) {
                                            gapContextMenu._trackIdx = trackLane.trackIdx
                                            gapContextMenu._gapStart = gapStart
                                            gapContextMenu._gapDuration = gapDuration
                                            gapContextMenu.popup()
                                        }
                                    }
                                }
                            }
                        }

                        // Feature 15: Lock overlay — shows "not-allowed" cursor + reduced opacity
                        Rectangle {
                            anchors.fill: parent
                            visible: trackLane.laneLocked && trackLane.laneVisible
                            color: Qt.rgba(0.06, 0.04, 0.12, 0.4)
                            z: 10

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.ForbiddenCursor
                                // Consume clicks so locked clips can't be interacted with
                                onPressed: mouse => mouse.accepted = true
                            }

                            // Lock icon in center
                            Label {
                                anchors.centerIn: parent
                                text: "\uD83D\uDD12"
                                font.pixelSize: 16; opacity: 0.3
                                visible: parent.width > 50
                            }
                        }

                        // Per-track drop zone with type-affinity validation
                        DropArea {
                            id: trackDropArea
                            anchors.fill: parent
                            keys: ["application/x-gopost-media"]

                            // Determine source type from drag payload for visual feedback
                            function dragSourceType(drag) {
                                if (drag.source && drag.source.assetData && drag.source.assetData.type === "media") {
                                    var mt = drag.source.assetData.mediaType
                                    if (mt === "image") return 1
                                    if (mt === "audio") return 5
                                    return 0  // video
                                }
                                return -1  // unknown
                            }
                            property int _hoveredSrcType: -1
                            property bool _isCompatible: _hoveredSrcType >= 0 && timelineNotifier
                                                          ? timelineNotifier.isTrackCompatible(trackLane.trackIdx, _hoveredSrcType)
                                                          : true

                            onEntered: drag => { _hoveredSrcType = dragSourceType(drag) }
                            onExited: {
                                _hoveredSrcType = -1
                                root._insertIndicatorVisible = false
                            }
                            onPositionChanged: function(drag) {
                                if (!timelineNotifier) return
                                var timelineX = drag.x + trackFlick.contentX
                                var dropTime = Math.max(0, timelineX / pps)
                                var snapResult = internal.snapToNearest(dropTime, -1)
                                if (snapResult) dropTime = snapResult.time
                                root._insertIndicatorTime = dropTime
                                root._insertIndicatorX = headerWidth + dropTime * pps - trackFlick.contentX
                                root._insertIndicatorVisible = true
                            }

                            // Visual feedback: green highlight = compatible, red dim = incompatible
                            Rectangle {
                                anchors.fill: parent
                                visible: trackDropArea.containsDrag
                                color: trackDropArea._isCompatible
                                    ? Qt.rgba(0.424, 0.388, 1.0, 0.08)
                                    : Qt.rgba(0.94, 0.32, 0.31, 0.08)
                                border.color: trackDropArea._isCompatible ? "#6C63FF" : "#EF5350"
                                border.width: 1.5
                                radius: 2
                            }
                            // Forbidden overlay for incompatible / locked tracks
                            Label {
                                anchors.centerIn: parent
                                visible: trackDropArea.containsDrag && !trackDropArea._isCompatible
                                text: trackLane.isLocked ? "\uD83D\uDD12 Locked" : "\u26D4 Incompatible"
                                font.pixelSize: 11; font.weight: Font.Bold
                                color: "#EF5350"; opacity: 0.7
                            }

                            onDropped: drop => {
                                _hoveredSrcType = -1
                                if (!timelineNotifier || !timelineNotifier.isReady) {
                                    console.warn("[VE2Timeline] track-drop rejected: timeline not ready")
                                    return
                                }

                                // Read data from drag source or MIME text
                                var data
                                if (drop.source && drop.source.assetData && drop.source.assetData.type === "media") {
                                    data = drop.source.assetData
                                } else if (drop.hasText) {
                                    try { data = JSON.parse(drop.text) } catch (e) {
                                        console.warn("[VE2Timeline] track-drop: invalid JSON:", e)
                                        return
                                    }
                                }
                                if (!data || data.type !== "media") return

                                // Support multi-item drag: auto-route by media type
                                var items = data.assets && data.assets.length > 0 ? data.assets : [data]
                                // Batch size guardrail
                                if (items.length > 50) {
                                    if (mediaPoolNotifier) mediaPoolNotifier.validateBatchSize(items.length)
                                    drop.accept(); return
                                }
                                root._insertIndicatorVisible = false
                                var videoClips = [], audioClips = []
                                for (var i = 0; i < items.length; ++i) {
                                    var item = items[i]
                                    var sourceType = 0
                                    if (item.mediaType === "image") sourceType = 1
                                    else if (item.mediaType === "audio") sourceType = 5
                                    var sourcePath = item.sourcePath || ""
                                    if (sourcePath === "") continue
                                    var desc = {
                                        sourceType: sourceType,
                                        sourcePath: sourcePath,
                                        displayName: item.displayName || "Untitled",
                                        duration: (item.duration && item.duration > 0) ? item.duration : 5.0
                                    }
                                    if (sourceType === 5) audioClips.push(desc)
                                    else videoClips.push(desc)
                                }
                                // Use a single addClipsBatch call — addClipsBatch already
                                // auto-routes clips to compatible tracks, so we don't need
                                // to split video/audio and call twice (which triggers two
                                // full state mutations + two QML binding cascades).
                                var allClips = videoClips.concat(audioClips)
                                var allDroppedIds = []
                                if (allClips.length > 0) {
                                    // Pre-set zoom BEFORE addClipsBatch so that when
                                    // tracksChanged fires, clips render at the fitted zoom
                                    // instead of 100% zoom (which causes a 10-20s freeze).
                                    var totalAddedDuration = 0
                                    for (var j = 0; j < allClips.length; ++j)
                                        totalAddedDuration += allClips[j].duration
                                    internal.preComputeFitZoom(totalAddedDuration)
                                    allDroppedIds = timelineNotifier.addClipsBatch(trackLane.trackIdx, allClips)
                                }
                                // Defer selection + scroll centering to next tick
                                if (allDroppedIds.length > 0) {
                                    var capturedIds = allDroppedIds.slice()
                                    Qt.callLater(function() {
                                        timelineNotifier.selectClip(capturedIds[capturedIds.length - 1])
                                        internal.autoFitToClips(capturedIds)
                                    })
                                }
                                drop.accept()
                            }
                        }
                    }
                }

                // ---- "New track below" drop preview zone ----
                Rectangle {
                    id: newTrackBelowZone
                    width: lanesColumn.width
                    height: root._showNewTrackBelow ? defaultTrackHeight : 0
                    visible: root._showNewTrackBelow
                    color: Qt.rgba(0.424, 0.388, 1.0, 0.08)
                    border.color: "#6C63FF"; border.width: 1.5; radius: 2
                    Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }

                    RowLayout {
                        anchors.centerIn: parent; spacing: 6
                        Label { text: "\u2795"; font.pixelSize: 14; color: "#6C63FF" }
                        Label { text: "New Track"; font.pixelSize: 11; color: "#6C63FF"; font.weight: Font.DemiBold }
                    }
                }
            }

            // Pinch zoom (mobile/tablet)
            PinchArea {
                anchors.fill: parent
                onPinchUpdated: pinch => {
                    var factor = pinch.scale / pinch.previousScale
                    if (timelineNotifier)
                        timelineNotifier.zoomAtPosition(factor, pinch.center.x + trackFlick.contentX)
                }
            }

            // Mouse wheel zoom (Ctrl+scroll)
            WheelHandler {
                acceptedModifiers: Qt.ControlModifier
                onWheel: event => {
                    if (!timelineNotifier) return
                    var factor = event.angleDelta.y > 0 ? 1.5 : (1.0 / 1.5)
                    var anchorX = event.x + trackFlick.contentX
                    timelineNotifier.zoomAtPosition(factor, anchorX)
                    event.accepted = true
                }
            }

            // Alt+Scroll: pan timeline horizontally
            WheelHandler {
                acceptedModifiers: Qt.AltModifier
                onWheel: event => {
                    var delta = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
                    trackFlick.contentX = Math.max(0, Math.min(
                        trackFlick.contentWidth - trackFlick.width,
                        trackFlick.contentX - delta))
                    event.accepted = true
                }
            }
        }
    }

    // ================================================================
    // In/Out point markers
    // ================================================================
    Rectangle {
        visible: timelineNotifier ? timelineNotifier.hasInPoint : false
        anchors.top: rulerRow.top
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        x: headerWidth + (timelineNotifier ? timelineNotifier.inPoint * pps : 0) - trackFlick.contentX
        width: 2; z: 90; color: "#66BB6A"; opacity: 0.8
        Rectangle {
            width: 12; height: 12; x: -6; y: 0; radius: 2; color: "#66BB6A"
            Label { anchors.centerIn: parent; text: "I"; font.pixelSize: 8; font.weight: Font.Bold; color: "white" }
        }
    }
    Rectangle {
        visible: timelineNotifier ? timelineNotifier.hasOutPoint : false
        anchors.top: rulerRow.top
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        x: headerWidth + (timelineNotifier ? timelineNotifier.outPoint * pps : 0) - trackFlick.contentX
        width: 2; z: 90; color: "#EF5350"; opacity: 0.8
        Rectangle {
            width: 12; height: 12; x: -6; y: 0; radius: 2; color: "#EF5350"
            Label { anchors.centerIn: parent; text: "O"; font.pixelSize: 8; font.weight: Font.Bold; color: "white" }
        }
    }
    // Shaded In-Out region
    Rectangle {
        visible: timelineNotifier ? (timelineNotifier.hasInPoint && timelineNotifier.hasOutPoint) : false
        anchors.top: rulerRow.bottom
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        x: headerWidth + (timelineNotifier ? timelineNotifier.inPoint * pps : 0) - trackFlick.contentX
        width: timelineNotifier ? (timelineNotifier.outPoint - timelineNotifier.inPoint) * pps : 0
        z: 80; color: Qt.rgba(0.424, 0.388, 1.0, 0.06)
        border.color: Qt.rgba(0.424, 0.388, 1.0, 0.15); border.width: 1
    }

    // ================================================================
    // ================================================================
    // Marker vertical lines spanning track area
    // ================================================================
    Repeater {
        model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.markers : []
        delegate: Rectangle {
            required property var modelData
            readonly property real markerX: headerWidth + (modelData.position || 0) * pps - trackFlick.contentX
            readonly property string markerColor: modelData.color || "#4CAF50"
            visible: markerX > headerWidth - 5 && markerX < root.width + 5
            anchors.top: rulerRow.bottom
            anchors.bottom: isStacked ? stackedDivider.top : footer.top
            x: markerX
            width: 1; z: 85
            color: markerColor; opacity: 0.35

            // Region span overlay in track area
            Rectangle {
                visible: modelData.isRegion === true
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                x: 0
                width: Math.max(0, ((modelData.endPosition || 0) - (modelData.position || 0)) * pps)
                color: Qt.rgba(
                    parseInt((markerColor).substr(1,2), 16) / 255,
                    parseInt((markerColor).substr(3,2), 16) / 255,
                    parseInt((markerColor).substr(5,2), 16) / 255,
                    0.06)
                border.color: markerColor; border.width: 0.5; opacity: 0.5
            }
        }
    }

    // Global snap guide line (yellow, spans ruler to footer)
    // ================================================================
    Rectangle {
        id: snapGuideLine
        visible: root._snapGuideVisible && root._snapGuideX >= 0
        anchors.top: rulerRow.top
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        x: root._snapGuideX
        width: 1; z: 95
        color: "#FFCA28"; opacity: 0.9

        // Small diamond indicator on the ruler
        Rectangle {
            width: 8; height: 8; rotation: 45; x: -4; y: rulerHeight - 10
            color: "#FFCA28"
        }
    }

    // ================================================================
    // Position override indicator (shown when P key is held)
    // ================================================================
    Rectangle {
        visible: timelineNotifier && timelineNotifier.positionOverrideActive
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: rulerHeight + 2
        anchors.rightMargin: 4
        width: 108; height: 20; radius: 4; z: 100
        color: Qt.rgba(1, 0.44, 0.26, 0.2)
        border.color: "#FF7043"
        Label {
            anchors.centerIn: parent
            text: "P  Free Position"
            font.pixelSize: 10; font.weight: Font.DemiBold
            color: "#FF7043"
        }
    }

    // ================================================================
    // Razor mode indicator badge (top-right, red)
    // ================================================================
    Rectangle {
        visible: timelineNotifier && timelineNotifier.razorModeEnabled
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: rulerHeight + 24
        anchors.rightMargin: 4
        width: razorLbl.implicitWidth + 14; height: 20; radius: 4; z: 100
        color: Qt.rgba(0.94, 0.33, 0.31, 0.2)
        border.color: "#EF5350"
        Label {
            id: razorLbl; anchors.centerIn: parent
            text: "\uD83D\uDD2A RAZOR (C)"
            font.pixelSize: 10; font.weight: Font.DemiBold
            color: "#EF5350"
        }
    }

    // ================================================================
    // Razor cut animation line (flashes red at cut point)
    // ================================================================
    Rectangle {
        id: razorCutLine
        visible: root._razorCutVisible
        x: root._razorCutX + headerWidth - trackFlick.contentX
        anchors.top: rulerRow.bottom
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        width: 2; z: 96
        color: "#EF5350"
        opacity: root._razorCutVisible ? 1.0 : 0.0

        Behavior on opacity { NumberAnimation { duration: 300 } }

        // Diamond indicator at top
        Rectangle {
            width: 8; height: 8; rotation: 45; x: -3; y: 2
            color: "#EF5350"
        }
    }
    Timer {
        id: razorCutTimer
        interval: 400; repeat: false
        onTriggered: root._razorCutVisible = false
    }

    // ================================================================
    // Insertion indicator line (blue, for drag-to-reorder)
    // ================================================================
    Rectangle {
        id: insertIndicatorLine
        visible: root._insertIndicatorVisible && root._insertIndicatorX >= 0
        anchors.top: rulerRow.bottom
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        x: root._insertIndicatorX
        width: 3; z: 96; radius: 1.5
        color: "#448AFF"

        // Arrow head
        Canvas {
            width: 12; height: 8; x: -4.5; y: -8
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "#448AFF"
                ctx.beginPath()
                ctx.moveTo(0, 0); ctx.lineTo(12, 0); ctx.lineTo(6, 8)
                ctx.closePath(); ctx.fill()
            }
        }
    }

    // ================================================================
    // Playhead — draggable vertical line with triangular handle
    // ================================================================
    Item {
        id: playheadItem
        anchors.top: parent.top
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        x: playheadMa.scrubbing
            ? headerWidth + playheadMa.scrubTime * pps - trackFlick.contentX
            : headerWidth + (timelineNotifier ? timelineNotifier.position * pps : 0) - trackFlick.contentX
        width: 1; z: 100
        visible: x >= headerWidth - 20 && x <= root.width + 20
        clip: false

        // Triangular handle on the ruler (red, larger for easy grabbing)
        Canvas {
            id: playheadHandle
            width: 16; height: 12; x: -8; y: 0; z: 2
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = playheadMa.scrubbing ? "#FF5252" : "#EF5350"
                ctx.beginPath()
                ctx.moveTo(0, 0); ctx.lineTo(16, 0); ctx.lineTo(8, 12)
                ctx.closePath(); ctx.fill()
            }
            Connections {
                target: playheadMa
                function onScrubbingChanged() { playheadHandle.requestPaint() }
            }
        }

        // Vertical red line spanning all tracks
        Rectangle {
            width: playheadMa.scrubbing ? 2 : 1
            anchors.top: parent.top; anchors.topMargin: 12
            anchors.bottom: parent.bottom
            x: playheadMa.scrubbing ? -0.5 : 0
            color: "#EF5350"
            opacity: playheadMa.scrubbing ? 1.0 : 0.85
        }

        // Timecode tooltip shown while scrubbing
        Rectangle {
            visible: playheadMa.scrubbing
            width: timecodeLabel.implicitWidth + 12
            height: 20; radius: 4; x: 6; y: 14
            color: Qt.rgba(0, 0, 0, 0.85)
            border.color: "#EF5350"; border.width: 1

            Label {
                id: timecodeLabel
                anchors.centerIn: parent
                font.pixelSize: 10; font.family: "monospace"; font.weight: Font.Bold
                color: "#FFFFFF"
                text: {
                    var s = playheadMa.scrubTime
                    var mins = Math.floor(s / 60)
                    var secs = Math.floor(s % 60)
                    var frames = Math.floor((s - Math.floor(s)) * 30)
                    return String(mins).padStart(2, '0') + ":" +
                           String(secs).padStart(2, '0') + ":" +
                           String(frames).padStart(2, '0')
                }
            }
        }

        // Shuttle speed indicator (shown near playhead when shuttling J/K/L)
        Rectangle {
            id: shuttleIndicator
            visible: timelineNotifier && timelineNotifier.shuttleSpeedDisplay !== ""
            width: shuttleSpeedLbl.implicitWidth + 12
            height: 18; radius: 4; x: 10; y: rulerHeight + 4
            color: {
                if (!timelineNotifier) return Qt.rgba(0, 0, 0, 0.85)
                var spd = timelineNotifier.shuttleSpeedDisplay
                if (spd.indexOf("<") >= 0) return Qt.rgba(0.2, 0.4, 0.9, 0.85) // reverse = blue
                return Qt.rgba(0.2, 0.7, 0.3, 0.85) // forward = green
            }
            border.color: "#FFFFFF"; border.width: 0.5

            Label {
                id: shuttleSpeedLbl
                anchors.centerIn: parent
                text: timelineNotifier ? timelineNotifier.shuttleSpeedDisplay : ""
                font.pixelSize: 10; font.family: "monospace"; font.weight: Font.Bold
                color: "#FFFFFF"
            }
        }

        // Drag handle — wide invisible grab area
        MouseArea {
            id: playheadMa
            width: 24; height: rulerHeight + 10; x: -12; y: -2
            cursorShape: Qt.SizeHorCursor
            hoverEnabled: true

            property bool scrubbing: false
            property real scrubTime: 0
            property real dragStartMouseX: 0

            onPressed: function(mouse) {
                scrubbing = true
                scrubTime = timelineNotifier ? timelineNotifier.position : 0
                dragStartMouseX = mapToItem(root, mouse.x, 0).x
            }
            onPositionChanged: function(mouse) {
                if (!scrubbing) return
                var rootPos = mapToItem(root, mouse.x, 0).x
                var contentX = trackFlick.contentX
                var time = Math.max(0, (rootPos - headerWidth + contentX) / pps)
                time = Math.min(time, totalDur)
                scrubTime = time
                if (timelineNotifier) timelineNotifier.scrubTo(time)
            }
            onReleased: {
                if (scrubbing && timelineNotifier) {
                    timelineNotifier.scrubTo(scrubTime)
                }
                scrubbing = false
            }
        }
    }

    // ================================================================
    // Cut-Point Transition Preview Popup
    // ================================================================
    Rectangle {
        id: cutPreviewPopup
        visible: root._cutPreviewVisible
        z: 200
        width: 280; height: 140
        radius: 8
        color: "#1A1A30"
        border.color: "#3A3A5A"; border.width: 1

        // Position above the cut point, clamped to root bounds
        x: Math.max(4, Math.min(root.width - width - 4, root._cutPreviewX - width / 2))
        y: Math.max(4, root._cutPreviewY - height - 16)

        // Drop shadow
        Rectangle {
            anchors.fill: parent; anchors.margins: -2
            radius: 10; color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.4); border.width: 3
            z: -1
        }

        readonly property var transitionNames: [
            "None", "Fade", "Dissolve", "Slide Left", "Slide Right", "Slide Up", "Slide Down",
            "Wipe Left", "Wipe Right", "Wipe Up", "Wipe Down", "Zoom", "Push", "Reveal",
            "Iris", "Clock Wipe", "Blur", "Glitch", "Morph", "Flash", "Spin"
        ]

        Column {
            anchors.fill: parent; anchors.margins: 6; spacing: 4

            // Clip name headers
            Row {
                width: parent.width; spacing: 4
                Label {
                    width: (parent.width - 4) / 2
                    text: root._cutPreviewNameA
                    font.pixelSize: 9; font.weight: Font.DemiBold; color: "#A0A0C0"
                    elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter
                }
                Label {
                    width: (parent.width - 4) / 2
                    text: root._cutPreviewNameB
                    font.pixelSize: 9; font.weight: Font.DemiBold; color: "#A0A0C0"
                    elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter
                }
            }

            // Side-by-side thumbnails
            Row {
                width: parent.width; spacing: 4; height: 76

                Rectangle {
                    width: (parent.width - 4) / 2; height: 76; radius: 4
                    color: "#0D0D1A"; clip: true
                    Image {
                        anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                        source: root._cutPreviewSourceA !== ""
                            ? "image://videothumbnail/" + Qt.btoa(root._cutPreviewSourceA)
                              + "@" + root._cutPreviewTimeA.toFixed(1) + "@68"
                            : ""
                        asynchronous: true
                    }
                    // "A" badge
                    Rectangle {
                        anchors.left: parent.left; anchors.top: parent.top
                        anchors.margins: 3; width: 16; height: 16; radius: 8
                        color: Qt.rgba(0, 0, 0, 0.6)
                        Label { anchors.centerIn: parent; text: "A"; font.pixelSize: 9; font.weight: Font.Bold; color: "#E0E0F0" }
                    }
                }

                Rectangle {
                    width: (parent.width - 4) / 2; height: 76; radius: 4
                    color: "#0D0D1A"; clip: true
                    Image {
                        anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                        source: root._cutPreviewSourceB !== ""
                            ? "image://videothumbnail/" + Qt.btoa(root._cutPreviewSourceB)
                              + "@" + root._cutPreviewTimeB.toFixed(1) + "@68"
                            : ""
                        asynchronous: true
                    }
                    // "B" badge
                    Rectangle {
                        anchors.right: parent.right; anchors.top: parent.top
                        anchors.margins: 3; width: 16; height: 16; radius: 8
                        color: Qt.rgba(0, 0, 0, 0.6)
                        Label { anchors.centerIn: parent; text: "B"; font.pixelSize: 9; font.weight: Font.Bold; color: "#E0E0F0" }
                    }
                }
            }

            // Transition info bar
            Rectangle {
                width: parent.width; height: 22; radius: 4
                color: root._cutPreviewTransType > 0 ? Qt.rgba(0.15, 0.78, 0.85, 0.12) : Qt.rgba(1, 1, 1, 0.04)
                border.color: root._cutPreviewTransType > 0 ? "#26C6DA" : "#2A2A50"
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: {
                        if (root._cutPreviewTransType > 0) {
                            var name = cutPreviewPopup.transitionNames[Math.min(root._cutPreviewTransType,
                                cutPreviewPopup.transitionNames.length - 1)]
                            return "\u2194 " + name + " " + root._cutPreviewTransDur.toFixed(1) + "s"
                        }
                        return "\u2702 Hard Cut"
                    }
                    font.pixelSize: 10; font.weight: Font.DemiBold
                    color: root._cutPreviewTransType > 0 ? "#26C6DA" : "#8888A0"
                }
            }
        }
    }

    // ================================================================
    // Stacked View: Divider + Secondary Mini-Timeline
    // ================================================================
    Rectangle {
        id: stackedDivider
        visible: isStacked
        anchors.left: parent.left; anchors.right: parent.right
        y: tabBarHeight + rulerHeight + (root.height - tabBarHeight - rulerHeight - footerHeight) * stackedDividerRatio
        height: 6; color: dividerMa.containsMouse || dividerMa.pressed ? "#6C63FF" : "#1E1E38"
        z: 102

        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#2A2A50" }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#2A2A50" }

        MouseArea {
            id: dividerMa
            anchors.fill: parent; anchors.margins: -3
            hoverEnabled: true; cursorShape: Qt.SplitVCursor
            property real pressY: 0
            property real pressRatio: 0
            onPressed: function(mouse) { pressY = mapToItem(root, 0, mouse.y).y; pressRatio = stackedDividerRatio }
            onPositionChanged: function(mouse) {
                if (!pressed) return
                var globalY = mapToItem(root, 0, mouse.y).y
                var dy = globalY - pressY
                var available = root.height - tabBarHeight - rulerHeight - footerHeight
                if (available > 0)
                    stackedDividerRatio = Math.max(0.25, Math.min(0.75, pressRatio + dy / available))
            }
        }
    }

    Item {
        id: secondarySection
        visible: isStacked
        anchors.top: stackedDivider.bottom
        anchors.left: parent.left; anchors.right: parent.right
        anchors.bottom: footer.top
        clip: true

        readonly property real secHeaderH: 24
        readonly property real secRulerH: 20
        readonly property real secTrackH: 48
        readonly property real secLabelW: 60

        // Header bar
        Rectangle {
            id: secHeader
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            height: secondarySection.secHeaderH; color: "#0D0D20"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#1E1E38" }

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6

                Label {
                    text: timelineNotifier ? timelineNotifier.secondaryTabLabel : ""
                    font.pixelSize: 10; font.weight: Font.DemiBold; color: "#A0A0C0"
                    elide: Text.ElideRight; Layout.fillWidth: true
                }

                // Swap button
                Rectangle {
                    Layout.preferredWidth: swapLbl.implicitWidth + 12; Layout.preferredHeight: 18; radius: 3
                    color: swapHov.hovered ? Qt.rgba(1,1,1,0.08) : "transparent"
                    border.color: "#3A3A5A"; border.width: 1
                    Label {
                        id: swapLbl; anchors.centerIn: parent
                        text: "\u21C5 Swap"; font.pixelSize: 9; color: "#8888A0"
                    }
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: { if (timelineNotifier) timelineNotifier.swapActiveAndSecondary() }
                    }
                    HoverHandler { id: swapHov }
                    ToolTip.visible: swapHov.hovered; ToolTip.text: "Swap active and secondary timelines"
                }
            }
        }

        // Mini ruler
        Canvas {
            id: secRuler
            anchors.top: secHeader.bottom; anchors.left: parent.left; anchors.right: parent.right
            height: secondarySection.secRulerH
            readonly property real scrollX: secFlick.contentX

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "#08081A"
                ctx.fillRect(0, 0, width, height)

                var sPps = secondaryPps
                if (sPps <= 0) return

                // Determine tick spacing
                var minPx = 60
                var rawInterval = minPx / sPps
                var intervals = [0.1, 0.25, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600]
                var interval = intervals[intervals.length - 1]
                for (var k = 0; k < intervals.length; k++) {
                    if (intervals[k] * sPps >= minPx) { interval = intervals[k]; break }
                }

                var startTime = Math.floor(scrollX / sPps / interval) * interval
                var endTime = (scrollX + width) / sPps + interval

                ctx.strokeStyle = "#2A2A50"
                ctx.fillStyle = "#6B6B88"
                ctx.font = "9px monospace"
                ctx.lineWidth = 1

                for (var t = startTime; t <= endTime; t += interval) {
                    var x = secondarySection.secLabelW + t * sPps - scrollX
                    if (x < secondarySection.secLabelW - 5 || x > width + 5) continue
                    ctx.beginPath()
                    ctx.moveTo(x, height - 8)
                    ctx.lineTo(x, height)
                    ctx.stroke()

                    var mins = Math.floor(t / 60)
                    var secs = Math.floor(t % 60)
                    var label = mins + ":" + (secs < 10 ? "0" : "") + secs
                    ctx.fillText(label, x + 3, height - 2)
                }
            }

            onScrollXChanged: requestPaint()
            onWidthChanged: requestPaint()
            Connections {
                target: root
                function onSecondaryPpsChanged() { secRuler.requestPaint() }
            }
        }

        // Track lanes
        Flickable {
            id: secFlick
            anchors.top: secRuler.bottom; anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            contentWidth: Math.max(width, secondaryTotalDur * secondaryPps + secondarySection.secLabelW)
            contentHeight: secTrackCol.height
            clip: true
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: secTrackCol

                Repeater {
                    model: isStacked && timelineNotifier ? timelineNotifier.secondaryTracks : []
                    delegate: Item {
                        required property var modelData
                        required property int index
                        width: secFlick.contentWidth
                        height: secondarySection.secTrackH

                        // Track label
                        Rectangle {
                            id: secTrackLabel
                            width: secondarySection.secLabelW; height: parent.height
                            color: index % 2 === 0 ? "#0D0D1A" : "#0F0F1E"
                            border.color: "#1E1E38"; border.width: 0
                            z: 2

                            Label {
                                anchors.centerIn: parent
                                text: modelData.label || ("Track " + (index + 1))
                                font.pixelSize: 9; color: "#6B6B88"
                                elide: Text.ElideRight; width: parent.width - 8
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 0.5; color: "#1E1E38" }
                        }

                        // Track background
                        Rectangle {
                            anchors.left: secTrackLabel.right; anchors.right: parent.right
                            height: parent.height
                            color: index % 2 === 0 ? Qt.rgba(0.04, 0.04, 0.08, 1) : Qt.rgba(0.05, 0.05, 0.10, 1)
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 0.5; color: "#1E1E38" }

                            // Clips
                            Repeater {
                                model: modelData.clips || []
                                delegate: Rectangle {
                                    required property var modelData
                                    x: modelData.timelineIn * secondaryPps
                                    width: Math.max(4, modelData.duration * secondaryPps)
                                    y: 3; height: secondarySection.secTrackH - 6
                                    radius: 3

                                    readonly property int sType: modelData.sourceType || 0
                                    readonly property color typeColor: {
                                        var colors = ["#4A90D9", "#43A047", "#FFCA28", "#E040FB", "#FF7043", "#43A047"]
                                        return colors[Math.min(sType, colors.length - 1)]
                                    }

                                    color: Qt.rgba(typeColor.r, typeColor.g, typeColor.b, 0.35)
                                    border.color: Qt.rgba(typeColor.r, typeColor.g, typeColor.b, 0.7)
                                    border.width: 1

                                    Label {
                                        anchors.centerIn: parent; width: parent.width - 6
                                        text: parent.parent.modelData ? (parent.modelData.displayName || "") : ""
                                        font.pixelSize: 8; color: "#C0C0D0"
                                        elide: Text.ElideRight
                                        horizontalAlignment: Text.AlignHCenter
                                        visible: parent.width > 30
                                    }
                                }
                            }
                        }

                        // Click to swap
                        MouseArea {
                            anchors.fill: parent; z: 1
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { if (timelineNotifier) timelineNotifier.swapActiveAndSecondary() }
                        }
                    }
                }
            }

            // Secondary playhead
            Rectangle {
                x: secondarySection.secLabelW + (timelineNotifier ? timelineNotifier.secondaryPosition : 0) * secondaryPps - secFlick.contentX
                width: 1; height: secFlick.contentHeight
                color: "#6C63FF"; z: 10
                visible: isStacked
            }
        }

        // Cross-timeline drag ghost
        Rectangle {
            visible: root._crossTimelineDragActive
            x: secondarySection.secLabelW + root._crossTimelineDragTime * secondaryPps - secFlick.contentX
            y: secondarySection.secHeaderH + secondarySection.secRulerH + root._crossTimelineDragTrack * secondarySection.secTrackH + 3
            width: 80; height: secondarySection.secTrackH - 6
            radius: 3; z: 20
            color: Qt.rgba(0.42, 0.39, 1.0, 0.3); border.color: "#6C63FF"; border.width: 2
            Label { anchors.centerIn: parent; text: "Drop here"; font.pixelSize: 9; color: "#E0E0F0" }
        }
    }

    // ================================================================
    // Row 2: Footer with scrollbar
    // ================================================================
    Rectangle {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: footerHeight
        color: "#0D0D1A"
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#1E1E38" }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 6; anchors.rightMargin: 6
            spacing: 4

            // Scrollbar
            ScrollBar {
                id: hScrollBar
                Layout.fillWidth: true
                Layout.preferredHeight: 14
                orientation: Qt.Horizontal
                policy: ScrollBar.AlwaysOn
                size: trackFlick.width / Math.max(1, trackFlick.contentWidth)

                onPositionChanged: {
                    if (active) trackFlick.contentX = position * trackFlick.contentWidth
                }

                Binding {
                    target: hScrollBar
                    property: "position"
                    value: trackFlick.contentX / Math.max(1, trackFlick.contentWidth)
                    when: !hScrollBar.active
                }
            }
        }
    }

    // ================================================================
    // Fallback drop area (full panel, z:-1)
    // ================================================================
    DropArea {
        id: fallbackDropArea
        anchors.top: rulerRow.bottom
        anchors.left: parent.left; anchors.leftMargin: headerWidth
        anchors.right: parent.right
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        z: -1
        keys: ["application/x-gopost-media"]

        Rectangle {
            anchors.fill: parent
            color: parent.containsDrag ? Qt.rgba(0.424, 0.388, 1.0, 0.06) : "transparent"
            border.color: parent.containsDrag ? "#6C63FF" : "transparent"
            border.width: parent.containsDrag ? 1.5 : 0
            radius: 4
        }

        onDropped: drop => {
            if (!timelineNotifier || !timelineNotifier.isReady) {
                console.warn("[VE2Timeline] fallback-drop rejected: timeline not ready")
                return
            }

            // Read data from drag source or MIME text
            var data
            if (drop.source && drop.source.assetData && drop.source.assetData.type === "media") {
                data = drop.source.assetData
            } else if (drop.hasText) {
                try { data = JSON.parse(drop.text) } catch (e) {
                    console.warn("[VE2Timeline] fallback-drop: invalid JSON:", e)
                    return
                }
            }
            if (!data || data.type !== "media") return

            // Support multi-item drag: batch insert
            var items = data.assets && data.assets.length > 0 ? data.assets : [data]
            // Batch size guardrail
            if (items.length > 50) {
                if (mediaPoolNotifier) mediaPoolNotifier.validateBatchSize(items.length)
                drop.accept(); return
            }
            // Group by compatible track for batch insert
            var videoClips = [], audioClips = []
            for (var i = 0; i < items.length; ++i) {
                var item = items[i]
                var sourceType = 0
                if (item.mediaType === "image") sourceType = 1
                else if (item.mediaType === "audio") sourceType = 5
                var sourcePath = item.sourcePath || ""
                if (sourcePath === "") continue
                var desc = {
                    sourceType: sourceType,
                    sourcePath: sourcePath,
                    displayName: item.displayName || "Untitled",
                    duration: (item.duration && item.duration > 0) ? item.duration : 5.0
                }
                if (sourceType === 5) audioClips.push(desc)
                else videoClips.push(desc)
            }
            // Single batch call — addClipsBatch auto-routes by type
            var allClips = videoClips.concat(audioClips)
            var allDroppedIds = []
            if (allClips.length > 0) {
                // Pre-set zoom BEFORE addClipsBatch so clips render at fitted zoom
                var totalAddedDuration = 0
                for (var j = 0; j < allClips.length; ++j)
                    totalAddedDuration += allClips[j].duration
                internal.preComputeFitZoom(totalAddedDuration)
                var defaultTrack = timelineNotifier.findCompatibleTrack(0, 0)
                allDroppedIds = timelineNotifier.addClipsBatch(defaultTrack, allClips)
            }
            // Defer selection + scroll centering to next tick
            if (allDroppedIds.length > 0) {
                var capturedIds = allDroppedIds.slice()
                Qt.callLater(function() {
                    timelineNotifier.selectClip(capturedIds[capturedIds.length - 1])
                    internal.autoFitToClips(capturedIds)
                })
            }
            drop.accept()
        }
    }

    // ================================================================
    // Empty state
    // ================================================================
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 8
        visible: timelineNotifier ? !timelineNotifier.isReady : true
        z: 50

        Label { text: "\uD83C\uDFAC"; font.pixelSize: 36; color: "#303050"; Layout.alignment: Qt.AlignHCenter }
        Label { text: "Drop media here to start editing"; font.pixelSize: 13; color: "#505068"; Layout.alignment: Qt.AlignHCenter }
    }

    // ================================================================
    // Inline clip component
    // ================================================================
    component VE2ClipItem: Item {
        id: clipRoot
        required property int clipIndex
        required property int trackIndex

        // trackGeneration changes on every clip/track modification (trim, move, split, etc.)
        // This forces clipInfo to re-evaluate when any clip data changes.
        readonly property var clipInfo: timelineNotifier && timelineNotifier.trackGeneration >= 0
                                        ? timelineNotifier.clipData(trackIndex, clipIndex) : ({})
        readonly property int clipId: clipInfo.clipId !== undefined ? clipInfo.clipId : -1
        readonly property string displayName: clipInfo.displayName || ""
        readonly property real timelineIn: clipInfo.timelineIn !== undefined ? clipInfo.timelineIn : 0
        readonly property real duration: clipInfo.duration !== undefined ? clipInfo.duration : 1
        readonly property int sourceType: clipInfo.sourceType !== undefined ? clipInfo.sourceType : 0
        // Use selectedClipCount (a Q_PROPERTY with NOTIFY selectionChanged) as a
        // dependency trigger so this binding re-evaluates when selection changes.
        readonly property bool isSelected: clipId >= 0 && timelineNotifier
                                           && timelineNotifier.selectedClipCount >= 0
                                           && timelineNotifier.isClipSelected(clipId)
        readonly property bool isPrimarySelected: clipId >= 0
                                           && (timelineNotifier ? timelineNotifier.selectedClipId === clipId : false)
        // Feature 10: linked clip info
        readonly property int linkedId: clipInfo.linkedClipId !== undefined ? clipInfo.linkedClipId : -1
        readonly property real speed: clipInfo.speed !== undefined ? clipInfo.speed : 1.0
        readonly property bool isReversed: clipInfo.isReversed !== undefined ? clipInfo.isReversed : false
        readonly property bool hasSpeedRamp: clipInfo.hasSpeedRamp !== undefined ? clipInfo.hasSpeedRamp : false
        readonly property string sourcePath: clipInfo.sourcePath || ""
        readonly property string colorTag: clipInfo.colorTag || ""
        readonly property string customLabel: clipInfo.customLabel || ""
        readonly property bool isFreezeFrame: clipInfo.isFreezeFrame !== undefined ? clipInfo.isFreezeFrame : false
        readonly property bool hasEffects: clipInfo.hasEffects !== undefined ? clipInfo.hasEffects : false
        readonly property int effectCount: clipInfo.effectCount !== undefined ? clipInfo.effectCount : 0
        readonly property bool hasColorGrading: clipInfo.hasColorGrading !== undefined ? clipInfo.hasColorGrading : false
        readonly property bool hasTransition: clipInfo.hasTransition !== undefined ? clipInfo.hasTransition : false
        readonly property int transitionInType: clipInfo.transitionInType !== undefined ? clipInfo.transitionInType : 0
        readonly property double transitionInDur: clipInfo.transitionInDur !== undefined ? clipInfo.transitionInDur : 0
        readonly property int transitionOutType: clipInfo.transitionOutType !== undefined ? clipInfo.transitionOutType : 0
        readonly property double transitionOutDur: clipInfo.transitionOutDur !== undefined ? clipInfo.transitionOutDur : 0
        readonly property int connectedToPrimaryClipId: clipInfo.connectedToPrimaryClipId !== undefined ? clipInfo.connectedToPrimaryClipId : -1
        readonly property bool isConnectedClip: connectedToPrimaryClipId >= 0
        readonly property real sourceDuration: clipInfo.sourceDuration !== undefined ? clipInfo.sourceDuration : 0
        readonly property real sourceIn: clipInfo.sourceIn !== undefined ? clipInfo.sourceIn : 0
        readonly property real sourceOut: clipInfo.sourceOut !== undefined ? clipInfo.sourceOut : 0
        readonly property bool isDisabled: clipInfo.isDisabled !== undefined ? clipInfo.isDisabled : false

        x: timelineIn * pps
        width: Math.max(4, duration * pps)

        // Smooth animation for ripple shifts (200ms ease-out)
        // Disabled during drag, trim, AND zoom changes to keep thumbnails in sync
        property real _lastPps: pps
        property bool _ppsChanging: false
        on_LastPpsChanged: { _ppsChanging = true; _ppsResetTimer.restart() }
        Timer { id: _ppsResetTimer; interval: 50; onTriggered: clipRoot._ppsChanging = false }

        Behavior on x {
            enabled: !clipBodyMa.dragActive && !trimLeftMa.pressed && !trimRightMa.pressed && !clipRoot._ppsChanging
            NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
        }
        Behavior on width {
            enabled: !clipBodyMa.dragActive && !trimLeftMa.pressed && !trimRightMa.pressed && !clipRoot._ppsChanging
            NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
        }

        readonly property color clipColor: {
            switch (sourceType) {
            case 0: return "#26C6DA"    // video
            case 1: return "#FF7043"    // image
            case 2: return "#AB47BC"    // title
            case 3: return "#66BB6A"    // color
            case 4: return "#42A5F5"    // adjustment
            case 5: return "#FFCA28"    // audio (yellow/amber)
            default: return "#6C63FF"
            }
        }

        // True when the playhead is within this clip's time range
        readonly property real playheadPos: timelineNotifier ? timelineNotifier.position : 0
        readonly property bool playheadInRange:
            playheadPos >= timelineIn - 0.001 &&
            playheadPos < (timelineIn + duration) + 0.001

        // Encoded path for thumbnail provider
        readonly property string pathB64: sourcePath !== "" ? Qt.btoa(sourcePath) : ""

        // ---- Staggered thumbnail readiness ----
        // Delay thumbnail loading after clip creation to avoid 100+ simultaneous
        // ffmpeg spawns when a batch of clips is added at once.
        // Stagger by clipIndex so clips appear progressively.
        property bool _thumbReady: false
        Timer {
            id: _thumbDelayTimer
            interval: 80 + clipRoot.clipIndex * 60  // 80ms base + 60ms per clip
            repeat: false
            running: true
            onTriggered: clipRoot._thumbReady = true
        }

        // ---- Viewport-aware thumbnail calculation ----
        // Only create thumbnails for the visible portion of the clip (virtual scrolling).
        // Quantized to grid cells to avoid recreating items on every scroll pixel.
        // Adaptive thumbnail cell width: wider cells at high zoom to limit total count
        readonly property real thumbCellWidth: pps > 200 ? 160 : (pps > 100 ? 120 : 80)
        readonly property real clipW: Math.max(1, clipRoot.width)

        // Raw visible range of this clip in LOCAL coordinates (0 = clip left edge)
        readonly property real _rawVisLeft:  Math.max(0, trackFlick.contentX - clipRoot.x)
        readonly property real _rawVisRight: Math.min(clipW, trackFlick.contentX + trackFlick.width - clipRoot.x)
        readonly property bool _isVisible: _rawVisRight > 0 && _rawVisLeft < clipW

        // Quantize to grid boundaries with 2-cell buffer, clamped to clip bounds
        readonly property real clipVisLeft:  _isVisible ? Math.max(0, Math.floor(_rawVisLeft / thumbCellWidth - 2) * thumbCellWidth) : 0
        readonly property real clipVisRight: _isVisible ? Math.min(clipW, Math.ceil(_rawVisRight / thumbCellWidth + 2) * thumbCellWidth) : 0
        readonly property real clipVisWidth: Math.max(0, clipVisRight - clipVisLeft)

        // Number of thumbnails to fill the quantized visible range (capped at 30 for performance)
        // Gate on _thumbReady to stagger ffmpeg spawns across multiple event loop ticks
        readonly property int thumbCount: _thumbReady && _isVisible && (sourceType === 0 || sourceType === 1) && pathB64 !== "" && clipVisWidth > 0
                                           ? Math.min(30, Math.max(1, Math.round(clipVisWidth / thumbCellWidth)))
                                           : 0
        // Time range mapped from quantized pixel range, clamped to [0, duration]
        readonly property real visStartTime: Math.max(0, Math.min(duration, duration * (clipVisLeft / clipW)))
        readonly property real visEndTime:   Math.max(0, Math.min(duration, duration * (clipVisRight / clipW)))
        // Pixel offset for the thumbnail row
        readonly property real thumbRowX: clipVisLeft

        Component.onCompleted: {
            console.log("[VE2Clip] created — track:", trackIndex, "clip:", clipIndex,
                        "x:", x, "width:", width, "height:", height,
                        "name:", displayName, "dur:", duration, "path:", sourcePath.substring(sourcePath.length - 30))
        }

        // Clip body
        Rectangle {
            id: clipBody
            anchors.fill: parent
            radius: 4
            color: "#111128"  // dark base — thumbnails overlay this
            border.color: isPrimarySelected ? "#FFFFFF" : (isSelected ? "#448AFF" : clipColor)
            border.width: isSelected ? 2 : 1
            clip: true

            // Waveform bar height: dedicated bottom section for audio waveform
            readonly property bool showWaveBar: timelineNotifier
                && timelineNotifier.waveformEnabled && pathB64 !== ""
                && (sourceType === 0 || sourceType === 1 || sourceType === 5)
            readonly property real waveBarHeight: showWaveBar
                ? (sourceType === 5 ? parent.height : Math.max(16, parent.height * 0.3))
                : 0
            readonly property real thumbAreaHeight: parent.height - waveBarHeight

            // ---- Shimmer loading placeholder (visible while thumbnails are loading) ----
            Rectangle {
                id: shimmerPlaceholder
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: clipBody.thumbAreaHeight
                radius: 2
                visible: (sourceType === 0 || sourceType === 1) && !_thumbReady
                color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.10)
                clip: true

                // Animated shimmer gradient sweeping left to right
                Rectangle {
                    id: shimmerBar
                    width: parent.width * 0.4
                    height: parent.height
                    radius: 2
                    opacity: 0.4
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.5; color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.25) }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                    SequentialAnimation on x {
                        loops: Animation.Infinite
                        running: shimmerPlaceholder.visible
                        NumberAnimation {
                            from: -shimmerBar.width
                            to: shimmerPlaceholder.width
                            duration: 1200
                            easing.type: Easing.InOutQuad
                        }
                        PauseAnimation { duration: 200 }
                    }
                }

                // Clip name overlay on shimmer
                Label {
                    anchors.centerIn: parent
                    text: clipRoot.displayName
                    font.pixelSize: 9
                    color: Qt.rgba(1, 1, 1, 0.5)
                    elide: Text.ElideRight
                    width: parent.width - 8
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // ---- Video thumbnail strip (top section) ----
            Row {
                id: thumbRow
                x: thumbRowX
                width: thumbCount * thumbCellWidth
                height: clipBody.thumbAreaHeight
                visible: thumbCount > 0

                Repeater {
                    model: thumbCount
                    delegate: Item {
                        required property int index
                        width: thumbCellWidth
                        height: clipBody.thumbAreaHeight

                        Image {
                            id: thumbImg
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectCrop
                            property real timeSec: clipRoot.duration > 0
                                ? (thumbCount > 1
                                    ? visStartTime + (index / (thumbCount - 1)) * (visEndTime - visStartTime)
                                    : (visStartTime + visEndTime) / 2)
                                : 0
                            source: pathB64 !== ""
                                ? "image://videothumbnail/" + pathB64 + "@" + timeSec.toFixed(1)
                                  + "@" + Math.round(height)
                                : ""
                            cache: true
                            asynchronous: true

                            // Fade in when thumbnail loads
                            opacity: status === Image.Ready ? 1.0 : 0.0
                            Behavior on opacity {
                                NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                            }
                        }

                        // Placeholder while loading
                        Rectangle {
                            anchors.fill: parent
                            visible: thumbImg.status !== Image.Ready
                            color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.10)
                        }
                    }
                }
            }

            // ---- Non-video clip fill (title, color, adjustment) ----
            Rectangle {
                anchors.fill: parent
                visible: sourceType >= 2 || (sourceType <= 1 && pathB64 === "")
                radius: 4
                color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.2)
            }

            // ---- Audio waveform bar (dedicated bottom section, no overlap) ----
            Rectangle {
                id: waveBar
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: clipBody.waveBarHeight
                visible: clipBody.showWaveBar && clipRoot.width > 10 && _isVisible
                color: "#0A0A1A"  // dark background for contrast
                // Subtle top separator line
                Rectangle {
                    anchors.top: parent.top; width: parent.width; height: 1
                    color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.3)
                    visible: sourceType !== 5  // no separator for audio-only clips
                }

                // Waveform image inside the bar — debounced to avoid lag during zoom
                Image {
                    id: waveformImage

                    readonly property bool useFullClip: clipRoot.width <= 2000
                    readonly property real wfX: useFullClip ? 0 : _rawVisLeft
                    readonly property real wfWidth: useFullClip
                        ? clipRoot.width
                        : Math.max(10, _rawVisRight - _rawVisLeft)
                    readonly property real wfStartTime: useFullClip
                        ? 0 : Math.max(0, duration * (_rawVisLeft / clipW))
                    readonly property real wfEndTime: useFullClip
                        ? duration : Math.min(duration, duration * (_rawVisRight / clipW))
                    readonly property int wfPixelW: Math.round(Math.min(2000, Math.max(10, wfWidth)))

                    // Debounced source: only update after 300ms idle to avoid
                    // flooding ffmpeg during rapid zoom/scroll
                    property string _pendingSource: ""
                    property string _activeSource: ""

                    x: wfX
                    y: 0
                    width: wfWidth
                    height: parent.height
                    fillMode: Image.Stretch
                    opacity: 1.0
                    asynchronous: true
                    cache: true  // cache OK since debounce prevents rapid URL changes

                    source: _activeSource

                    // Compute pending source (changes rapidly during zoom)
                    onWfPixelWChanged: _wfDebounce.restart()
                    onWfStartTimeChanged: _wfDebounce.restart()
                    onWfEndTimeChanged: _wfDebounce.restart()

                    function computeSource() {
                        if (!waveBar.visible || wfWidth <= 0) return ""
                        // Skip during playback to avoid lag
                        if (timelineNotifier && timelineNotifier.isPlaying) return _activeSource
                        return "image://audiowaveform/" + pathB64
                            + "@" + wfStartTime.toFixed(2)
                            + "@" + wfEndTime.toFixed(2)
                            + "@" + wfPixelW
                            + "@" + Math.round(Math.min(100, waveBar.height))
                            + "@" + (timelineNotifier.waveformStereo ? "2" : "1")
                    }

                    Timer {
                        id: _wfDebounce
                        interval: 300; repeat: false
                        onTriggered: waveformImage._activeSource = waveformImage.computeSource()
                    }

                    // Initial load
                    Component.onCompleted: {
                        _activeSource = Qt.binding(function() {
                            if (!waveBar.visible || wfWidth <= 0) return ""
                            // Skip rapid updates — debounce handles zoom changes
                            return _activeSource
                        })
                        // Trigger first load after a short delay
                        _wfDebounce.start()
                    }

                    // Reload when playback stops (in case waveform was skipped during play)
                    Connections {
                        target: timelineNotifier
                        function onPlaybackChanged() {
                            if (timelineNotifier && !timelineNotifier.isPlaying)
                                _wfDebounce.restart()
                        }
                    }
                }
            }

            // ---- Volume rubber band (inline volume automation) ----
            Canvas {
                id: volumeOverlay
                anchors.fill: waveBar
                visible: waveBar.visible && clipRoot.width > 30
                z: 14

                function volToY(vol, h) { return h * (1.0 - Math.min(2.0, Math.max(0, vol)) / 2.0) }
                function yToVol(y, h) { return Math.max(0, Math.min(2.0, 2.0 * (1.0 - y / h))) }
                function volToDb(vol) {
                    if (vol <= 0.001) return "-\u221E"
                    return (20 * Math.log10(vol)).toFixed(1)
                }

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (width <= 0 || height <= 0) return
                    var h = height
                    var points = timelineNotifier ? timelineNotifier.clipVolumeKeyframes(clipId) : []

                    // 0dB reference line (dashed)
                    var zeroDbY = volToY(1.0, h)
                    ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.2)
                    ctx.lineWidth = 1; ctx.setLineDash([4, 4])
                    ctx.beginPath(); ctx.moveTo(0, zeroDbY); ctx.lineTo(width, zeroDbY); ctx.stroke()
                    ctx.setLineDash([])

                    if (points.length === 0) {
                        var flatVol = (clipRoot.clipInfo && clipRoot.clipInfo.volume !== undefined)
                            ? clipRoot.clipInfo.volume : 1.0
                        var flatY = volToY(flatVol, h)
                        ctx.strokeStyle = Qt.rgba(1, 0.84, 0.0, 0.6)
                        ctx.lineWidth = 1.5
                        ctx.beginPath(); ctx.moveTo(0, flatY); ctx.lineTo(width, flatY); ctx.stroke()
                        return
                    }

                    // Draw automation curve
                    ctx.strokeStyle = "#FFCA28"
                    ctx.lineWidth = 2
                    ctx.beginPath()
                    var firstX = points[0].normalizedTime * width
                    var firstY = volToY(points[0].value, h)
                    ctx.moveTo(0, firstY)
                    ctx.lineTo(firstX, firstY)
                    for (var i = 1; i < points.length; i++) {
                        var px = points[i].normalizedTime * width
                        var py = volToY(points[i].value, h)
                        ctx.lineTo(px, py)
                    }
                    var lastY = volToY(points[points.length - 1].value, h)
                    ctx.lineTo(width, lastY)
                    ctx.stroke()

                    // Semi-transparent fill below curve
                    ctx.fillStyle = Qt.rgba(1, 0.79, 0.16, 0.06)
                    ctx.lineTo(width, h)
                    ctx.lineTo(0, h)
                    ctx.closePath()
                    ctx.fill()

                    // Control point dots
                    for (var j = 0; j < points.length; j++) {
                        var dx = points[j].normalizedTime * width
                        var dy = volToY(points[j].value, h)
                        ctx.fillStyle = "#FFCA28"
                        ctx.beginPath(); ctx.arc(dx, dy, 5, 0, Math.PI * 2); ctx.fill()
                        ctx.strokeStyle = "#0A0A18"; ctx.lineWidth = 1.5
                        ctx.beginPath(); ctx.arc(dx, dy, 5, 0, Math.PI * 2); ctx.stroke()
                    }
                }

                Timer {
                    id: _volRepaintTimer; interval: 100; repeat: false
                    onTriggered: { if (volumeOverlay.visible) volumeOverlay.requestPaint() }
                }
                Connections {
                    target: timelineNotifier
                    function onTracksChanged() {
                        if (volumeOverlay.visible) _volRepaintTimer.restart()
                    }
                }
            }

            // Volume rubber band MouseArea (interactive point dragging)
            MouseArea {
                id: volumeDragMa
                anchors.fill: waveBar
                visible: volumeOverlay.visible
                z: 15
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                cursorShape: _nearPoint >= 0 ? Qt.SizeVerCursor : Qt.CrossCursor

                property int _nearPoint: -1
                property int _draggingIdx: -1
                property real _dragOrigTime: 0
                property var _points: []
                // Rubber band segment dragging
                property int _rubberSeg: -1
                property real _rubberStartY: 0
                property real _rubberOrigA: 0
                property real _rubberOrigB: 0

                function findNearPoint(mx, my) {
                    _points = timelineNotifier ? timelineNotifier.clipVolumeKeyframes(clipId) : []
                    var w = volumeOverlay.width, h = volumeOverlay.height
                    for (var i = 0; i < _points.length; i++) {
                        var px = _points[i].normalizedTime * w
                        var py = volumeOverlay.volToY(_points[i].value, h)
                        if (Math.abs(mx - px) < 8 && Math.abs(my - py) < 8)
                            return i
                    }
                    return -1
                }

                function findSegmentAtX(normX) {
                    for (var i = 0; i < _points.length - 1; i++) {
                        if (normX >= _points[i].normalizedTime && normX <= _points[i + 1].normalizedTime)
                            return i
                    }
                    return -1
                }

                function interpolateVol(normX) {
                    if (_points.length === 0) return 1.0
                    if (normX <= _points[0].normalizedTime) return _points[0].value
                    if (normX >= _points[_points.length - 1].normalizedTime) return _points[_points.length - 1].value
                    for (var i = 0; i < _points.length - 1; i++) {
                        var a = _points[i], b = _points[i + 1]
                        if (normX >= a.normalizedTime && normX <= b.normalizedTime) {
                            var t = (normX - a.normalizedTime) / (b.normalizedTime - a.normalizedTime)
                            return a.value + t * (b.value - a.value)
                        }
                    }
                    return 1.0
                }

                onPositionChanged: function(mouse) {
                    if (_draggingIdx >= 0 && pressed) {
                        var newNorm = Math.max(0, Math.min(1, mouse.x / volumeOverlay.width))
                        var newTime = newNorm * clipRoot.duration
                        var newVol = volumeOverlay.yToVol(mouse.y, volumeOverlay.height)
                        timelineNotifier.moveVolumeKeyframe(clipId, _dragOrigTime, newTime, newVol)
                        _dragOrigTime = newTime
                        volTipLbl.text = volumeOverlay.volToDb(newVol) + " dB"
                        volumeDragTip.x = mouse.x + 12
                        volumeDragTip.y = Math.max(0, mouse.y - 24)
                        volumeDragTip.visible = true
                        volumeOverlay.requestPaint()
                    } else if (_rubberSeg >= 0 && pressed) {
                        // Rubber band: move segment up/down
                        var dy = mouse.y - _rubberStartY
                        var dVol = -dy / volumeOverlay.height * 2.0
                        var newA = Math.max(0, Math.min(2.0, _rubberOrigA + dVol))
                        var newB = Math.max(0, Math.min(2.0, _rubberOrigB + dVol))
                        var pts = timelineNotifier.clipVolumeKeyframes(clipId)
                        if (_rubberSeg < pts.length - 1) {
                            timelineNotifier.moveVolumeKeyframe(clipId, pts[_rubberSeg].time, pts[_rubberSeg].time, newA)
                            pts = timelineNotifier.clipVolumeKeyframes(clipId)
                            if (_rubberSeg + 1 < pts.length)
                                timelineNotifier.moveVolumeKeyframe(clipId, pts[_rubberSeg + 1].time, pts[_rubberSeg + 1].time, newB)
                        }
                        var avgVol = (newA + newB) / 2
                        volTipLbl.text = volumeOverlay.volToDb(avgVol) + " dB"
                        volumeDragTip.x = mouse.x + 12
                        volumeDragTip.y = Math.max(0, mouse.y - 24)
                        volumeDragTip.visible = true
                        volumeOverlay.requestPaint()
                    } else if (!pressed) {
                        _nearPoint = findNearPoint(mouse.x, mouse.y)
                    }
                }

                onPressed: function(mouse) {
                    _points = timelineNotifier ? timelineNotifier.clipVolumeKeyframes(clipId) : []
                    _nearPoint = findNearPoint(mouse.x, mouse.y)

                    if (mouse.button === Qt.RightButton && _nearPoint >= 0) {
                        timelineNotifier.removeKeyframe(clipId, 5, _points[_nearPoint].time)
                        _nearPoint = -1
                        volumeOverlay.requestPaint()
                        mouse.accepted = true
                        return
                    }

                    if (mouse.button === Qt.LeftButton) {
                        if (_nearPoint >= 0) {
                            _draggingIdx = _nearPoint
                            _dragOrigTime = _points[_nearPoint].time
                            _rubberSeg = -1
                        } else {
                            // Check if on the line (rubber band)
                            var normX = mouse.x / volumeOverlay.width
                            if (_points.length >= 2) {
                                var lineVol = interpolateVol(normX)
                                var lineY = volumeOverlay.volToY(lineVol, volumeOverlay.height)
                                if (Math.abs(mouse.y - lineY) < 6) {
                                    _rubberSeg = findSegmentAtX(normX)
                                    if (_rubberSeg >= 0) {
                                        _rubberStartY = mouse.y
                                        _rubberOrigA = _points[_rubberSeg].value
                                        _rubberOrigB = _points[_rubberSeg + 1].value
                                        _draggingIdx = -1
                                        return
                                    }
                                }
                            }
                            // Click on empty space: create new keyframe
                            var newTime = normX * clipRoot.duration
                            var newVol = volumeOverlay.yToVol(mouse.y, volumeOverlay.height)
                            timelineNotifier.addKeyframe(clipId, 5, newTime, newVol)
                            volumeOverlay.requestPaint()
                            _points = timelineNotifier.clipVolumeKeyframes(clipId)
                            _nearPoint = findNearPoint(mouse.x, mouse.y)
                            if (_nearPoint >= 0) {
                                _draggingIdx = _nearPoint
                                _dragOrigTime = newTime
                            }
                            _rubberSeg = -1
                        }
                    }
                }

                onReleased: {
                    _draggingIdx = -1
                    _rubberSeg = -1
                    volumeDragTip.visible = false
                }

                onExited: {
                    _nearPoint = -1
                    volumeDragTip.visible = false
                }

                // dB tooltip
                Rectangle {
                    id: volumeDragTip
                    visible: false; z: 200
                    width: volTipLbl.implicitWidth + 10; height: 18; radius: 3
                    color: "#1A1A30"; border.color: "#FFCA28"; border.width: 1
                    Label {
                        id: volTipLbl; anchors.centerIn: parent
                        font.pixelSize: 9; font.weight: Font.Bold; color: "#FFCA28"
                    }
                }
            }

            // ---- Color tag bar (colored strip at very top of clip) ----
            Rectangle {
                id: colorTagBar
                anchors.top: parent.top
                width: parent.width; height: colorTag !== "" ? 4 : 0
                visible: colorTag !== ""
                color: colorTag !== "" ? colorTag : "transparent"
                radius: 4
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: colorTagBar.color }
            }

            // ---- Top label bar (semi-transparent overlay) ----
            Rectangle {
                id: labelBar
                anchors.top: colorTagBar.visible ? colorTagBar.bottom : parent.top
                width: parent.width
                height: 18
                color: Qt.rgba(0, 0, 0, 0.55)
                radius: colorTagBar.visible ? 0 : 4
                // Square off bottom corners
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 4; color: Qt.rgba(0, 0, 0, 0.55) }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 5; anchors.rightMargin: 5
                    spacing: 3

                    // Color tag dot indicator
                    Rectangle {
                        visible: colorTag !== ""
                        width: 6; height: 6; radius: 3
                        color: colorTag !== "" ? colorTag : "transparent"
                    }

                    // Source type icon
                    Label {
                        text: {
                            var icons = ["\uD83C\uDFA5", "\uD83D\uDDBC", "\uD83D\uDCDD", "\uD83C\uDFA8", "\u2728", "\uD83C\uDFB5"]
                            return icons[Math.min(sourceType, icons.length - 1)]
                        }
                        font.pixelSize: 10
                    }

                    // Clip name
                    Label {
                        text: displayName
                        font.pixelSize: 10; font.weight: Font.DemiBold
                        color: "#F0F0FF"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Reverse indicator
                    Label {
                        visible: isReversed
                        text: "\u25C0"
                        font.pixelSize: 9; font.weight: Font.Bold
                        color: "#FF7043"
                    }

                    // Freeze frame snowflake badge
                    Rectangle {
                        visible: isFreezeFrame
                        width: freezeLbl.implicitWidth + 6; height: 14; radius: 3
                        color: Qt.rgba(0.53, 0.81, 0.92, 0.25)
                        border.color: "#87CEEB"; border.width: 0.5
                        Label {
                            id: freezeLbl; anchors.centerIn: parent
                            text: "\u2744 Freeze"
                            font.pixelSize: 8; font.weight: Font.Bold
                            color: "#87CEEB"
                        }
                    }

                    // FX badge — shows effect count, clickable to open effects panel
                    Rectangle {
                        visible: hasEffects || hasColorGrading
                        width: fxLbl.implicitWidth + 6; height: 14; radius: 3
                        color: Qt.rgba(0.67, 0.28, 0.74, 0.25)
                        border.color: "#AB47BC"; border.width: 0.5
                        Label {
                            id: fxLbl; anchors.centerIn: parent
                            text: "FX" + (effectCount > 0 ? " " + effectCount : "")
                            font.pixelSize: 8; font.weight: Font.Bold
                            color: "#AB47BC"
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                // Select clip and open Effects panel
                                if (timelineNotifier && clipId >= 0) {
                                    timelineNotifier.selectClip(clipId)
                                    timelineNotifier.setActivePanel(3)  // effects panel
                                }
                            }
                        }
                        ToolTip.visible: fxBadgeHov.hovered
                        ToolTip.text: effectCount + " effect(s)" + (hasColorGrading ? " + color grading" : "")
                        HoverHandler { id: fxBadgeHov }
                    }

                    // TR badge — transition applied
                    Rectangle {
                        visible: hasTransition
                        width: trBadgeLbl.implicitWidth + 6; height: 14; radius: 3
                        color: Qt.rgba(0.15, 0.78, 0.85, 0.25)
                        border.color: "#26C6DA"; border.width: 0.5
                        Label {
                            id: trBadgeLbl; anchors.centerIn: parent
                            text: "TR"
                            font.pixelSize: 8; font.weight: Font.Bold
                            color: "#26C6DA"
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (timelineNotifier && clipId >= 0) {
                                    timelineNotifier.selectClip(clipId)
                                    timelineNotifier.setActivePanel(5)  // transitions panel
                                }
                            }
                        }
                        ToolTip.visible: trBadgeHov.hovered
                        ToolTip.text: "Transition applied"
                        HoverHandler { id: trBadgeHov }
                    }

                    // Speed badge
                    Label {
                        visible: (speed !== 1.0 || hasSpeedRamp) && !isFreezeFrame
                        text: hasSpeedRamp ? "RAMP" : (speed.toFixed(1) + "x")
                        font.pixelSize: 9; font.weight: Font.Bold
                        color: "#FFCA28"
                    }
                }
            }

            // ---- Transition zone overlays on clip edges ----
            // Transition In zone (left edge)
            Rectangle {
                visible: transitionInType > 0 && clipRoot.width > 20
                x: 0; y: 0; z: 8
                width: Math.max(4, transitionInDur * pps)
                height: parent.height
                color: Qt.rgba(0.15, 0.78, 0.85, 0.15)
                border.color: "#26C6DA"; border.width: 0.5
                radius: 2

                // Diagonal gradient lines
                Canvas {
                    anchors.fill: parent; visible: parent.width > 8
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = "#26C6DA"
                        ctx.lineWidth = 0.5; ctx.globalAlpha = 0.4
                        for (var i = -height; i < width; i += 8) {
                            ctx.beginPath(); ctx.moveTo(i, height); ctx.lineTo(i + height, 0); ctx.stroke()
                        }
                    }
                }
            }

            // Transition Out zone (right edge)
            Rectangle {
                visible: transitionOutType > 0 && clipRoot.width > 20
                anchors.right: parent.right; y: 0; z: 8
                width: Math.max(4, transitionOutDur * pps)
                height: parent.height
                color: Qt.rgba(0.15, 0.78, 0.85, 0.15)
                border.color: "#26C6DA"; border.width: 0.5
                radius: 2

                Canvas {
                    anchors.fill: parent; visible: parent.width > 8
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = "#26C6DA"
                        ctx.lineWidth = 0.5; ctx.globalAlpha = 0.4
                        for (var i = -height; i < width; i += 8) {
                            ctx.beginPath(); ctx.moveTo(i, 0); ctx.lineTo(i + height, height); ctx.stroke()
                        }
                    }
                }
            }

            // ---- Freeze frame snowflake overlay (centered on clip body) ----
            Label {
                visible: isFreezeFrame && clipRoot.width > 20 && customLabel === ""
                anchors.centerIn: parent
                text: "\u2744"
                font.pixelSize: Math.min(36, clipRoot.height * 0.5)
                color: "#87CEEB"; opacity: 0.4
                style: Text.Outline; styleColor: Qt.rgba(0, 0, 0, 0.3)
            }

            // ---- Transition drop zone ----
            DropArea {
                anchors.fill: parent; z: 6
                keys: ["application/x-gopost-transition"]

                Rectangle {
                    anchors.fill: parent; visible: parent.containsDrag
                    color: Qt.rgba(0.15, 0.78, 0.85, 0.15)
                    border.color: "#26C6DA"; border.width: 1.5; radius: 4
                    Label { anchors.centerIn: parent; text: "Drop transition here"; font.pixelSize: 10; color: "#26C6DA" }
                }

                onDropped: drop => {
                    var data
                    if (drop.hasText) {
                        try { data = JSON.parse(drop.text) } catch(e) { return }
                    }
                    if (!data || data.type !== "transition") return
                    if (timelineNotifier && clipId >= 0) {
                        timelineNotifier.setTransitionIn(clipId, data.transType, 0.5, 3)
                    }
                    drop.accept()
                }
            }

            // ---- Custom label overlay (centered on clip body) ----
            Label {
                visible: customLabel !== "" && clipRoot.width > 30
                anchors.centerIn: parent
                text: customLabel
                font.pixelSize: 12; font.weight: Font.Bold
                color: "#FFFFFF"; opacity: 0.85
                style: Text.Outline; styleColor: Qt.rgba(0, 0, 0, 0.7)
                elide: Text.ElideRight
                width: Math.min(implicitWidth, parent.width - 10)
                horizontalAlignment: Text.AlignHCenter
            }

            // ---- Selection highlight glow (Feature 13: blue for multi-select) ----
            Rectangle {
                anchors.fill: parent
                radius: 4
                visible: isSelected
                color: isSelected && !isPrimarySelected ? Qt.rgba(0.27, 0.54, 1.0, 0.08) : "transparent"
                border.color: isPrimarySelected ? "#FFFFFF" : "#448AFF"
                border.width: 2
            }

            // ---- Feature 10: Chain link icon for linked clips ----
            Rectangle {
                visible: linkedId >= 0
                anchors.bottom: parent.bottom; anchors.left: parent.left
                anchors.leftMargin: 4; anchors.bottomMargin: 2
                width: 16; height: 14; radius: 3
                color: Qt.rgba(0.42, 0.65, 0.97, 0.25)
                border.color: "#6BA3F7"; border.width: 0.5
                Label {
                    anchors.centerIn: parent
                    text: "\uD83D\uDD17"; font.pixelSize: 9
                }
                ToolTip.text: "Linked to clip #" + linkedId
                ToolTip.visible: linkHov.hovered
                HoverHandler { id: linkHov }
            }

            // ---- Disabled clip overlay (dimmed + diagonal stripes) ----
            Rectangle {
                anchors.fill: parent
                radius: 4; z: 16
                visible: isDisabled
                color: Qt.rgba(0.05, 0.05, 0.08, 0.55)

                Canvas {
                    anchors.fill: parent
                    visible: parent.visible
                    onVisibleChanged: if (visible) requestPaint()
                    onWidthChanged: if (visible) requestPaint()
                    onHeightChanged: if (visible) requestPaint()
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = "#505068"
                        ctx.lineWidth = 1
                        ctx.globalAlpha = 0.4
                        for (var i = -height; i < width + height; i += 12) {
                            ctx.beginPath()
                            ctx.moveTo(i, height)
                            ctx.lineTo(i + height, 0)
                            ctx.stroke()
                        }
                    }
                }

                // "DISABLED" label centered
                Label {
                    anchors.centerIn: parent
                    visible: parent.width > 60
                    text: "DISABLED"
                    font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1
                    color: "#808098"
                    style: Text.Outline; styleColor: Qt.rgba(0, 0, 0, 0.5)
                }
            }

            // ---- Speaker-off icon for muted audio clips ----
            Rectangle {
                visible: isDisabled && sourceType === 5  // audio clips
                anchors.bottom: parent.bottom; anchors.right: parent.right
                anchors.rightMargin: 4; anchors.bottomMargin: 2
                width: 18; height: 14; radius: 3; z: 17
                color: Qt.rgba(0.94, 0.33, 0.31, 0.25)
                border.color: "#EF5350"; border.width: 0.5
                Label {
                    anchors.centerIn: parent
                    text: "\uD83D\uDD07"; font.pixelSize: 9
                }
                ToolTip.text: "Audio muted (clip disabled)"
                ToolTip.visible: muteIconHov.hovered
                HoverHandler { id: muteIconHov }
            }

            // ---- Playhead-in-range subtle highlight ----
            Rectangle {
                anchors.fill: parent
                radius: 4
                visible: playheadInRange && !isSelected
                color: "transparent"
                border.color: clipColor
                border.width: 1.5
                opacity: 0.6
            }
            // Top edge glow when playhead is within this clip
            Rectangle {
                width: parent.width; height: 2; radius: 1
                anchors.top: parent.top
                visible: playheadInRange
                color: clipColor
                opacity: 0.4
            }

            // ---- Duplicate frame detection overlay (red hatching at bottom) ----
            Item {
                id: dupeOverlay
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 6
                z: 14
                visible: timelineNotifier && timelineNotifier.duplicateFrameDetectionEnabled
                         && clipRoot.width > 10 && dupeOverlay._hasDuplicates

                // Cache the overlap query result; re-evaluate when trackGeneration changes
                readonly property var _overlaps: {
                    if (!timelineNotifier || !timelineNotifier.duplicateFrameDetectionEnabled
                        || clipId < 0 || timelineNotifier.trackGeneration < 0)
                        return []
                    return timelineNotifier.duplicateFrameOverlaps(clipId)
                }
                readonly property bool _hasDuplicates: _overlaps.length > 0

                // Red hatching canvas
                Canvas {
                    anchors.fill: parent
                    visible: parent.visible
                    onVisibleChanged: if (visible) requestPaint()
                    onWidthChanged: if (visible) requestPaint()
                    onHeightChanged: if (visible) requestPaint()
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        // Red semi-transparent background
                        ctx.fillStyle = Qt.rgba(0.94, 0.33, 0.31, 0.35)
                        ctx.fillRect(0, 0, width, height)
                        // Diagonal hatching lines
                        ctx.strokeStyle = "#EF5350"
                        ctx.lineWidth = 1
                        ctx.globalAlpha = 0.6
                        for (var i = -height; i < width + height; i += 6) {
                            ctx.beginPath()
                            ctx.moveTo(i, height)
                            ctx.lineTo(i + height, 0)
                            ctx.stroke()
                        }
                    }
                }

                // Tooltip on hover showing which clips share frames
                ToolTip.visible: dupeHov.hovered && dupeOverlay._hasDuplicates
                ToolTip.text: {
                    if (!dupeOverlay._hasDuplicates) return ""
                    var lines = []
                    for (var i = 0; i < dupeOverlay._overlaps.length; i++) {
                        var o = dupeOverlay._overlaps[i]
                        lines.push("\"" + o.displayName + "\" (track " + (o.trackIndex + 1)
                                   + ") — " + o.overlapDuration.toFixed(1) + "s shared")
                    }
                    return "Duplicate frames with:\n" + lines.join("\n")
                }
                HoverHandler { id: dupeHov }
            }

            // ---- Dashed border for speed-ramped clips ----
            Canvas {
                anchors.fill: parent
                visible: hasSpeedRamp && clipRoot.width > 20
                z: 15
                onVisibleChanged: if (visible) requestPaint()
                onWidthChanged: if (visible) requestPaint()
                onHeightChanged: if (visible) requestPaint()
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = "#FFCA28"
                    ctx.lineWidth = 2
                    ctx.setLineDash([6, 3])
                    var r = 4
                    ctx.beginPath()
                    ctx.moveTo(r, 0); ctx.lineTo(width - r, 0)
                    ctx.arcTo(width, 0, width, r, r)
                    ctx.lineTo(width, height - r)
                    ctx.arcTo(width, height, width - r, height, r)
                    ctx.lineTo(r, height)
                    ctx.arcTo(0, height, 0, height - r, r)
                    ctx.lineTo(0, r)
                    ctx.arcTo(0, 0, r, 0, r)
                    ctx.stroke()
                }
                // Repaint when ramp status changes
                Connections {
                    target: clipRoot
                    function onHasSpeedRampChanged() { parent.requestPaint() }
                }
            }

            // ---- Speed curve overlay for ramped clips ----
            Canvas {
                id: speedCurveOverlay
                anchors.fill: parent
                visible: hasSpeedRamp && clipRoot.width > 30 && _isVisible
                z: 12
                onVisibleChanged: if (visible) requestPaint()
                onWidthChanged: if (visible) requestPaint()
                onHeightChanged: if (visible) requestPaint()
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (!timelineNotifier || clipId < 0) return

                    var points = timelineNotifier.clipSpeedRampPoints(clipId)
                    if (!points || points.length < 2) return

                    // Baseline at 1x (center of clip body)
                    var baseY = height * 0.5
                    ctx.strokeStyle = Qt.rgba(1, 0.79, 0.16, 0.25)
                    ctx.lineWidth = 1
                    ctx.setLineDash([4, 4])
                    ctx.beginPath()
                    ctx.moveTo(0, baseY); ctx.lineTo(width, baseY)
                    ctx.stroke()
                    ctx.setLineDash([])

                    // Map speed to Y: speed=1 -> center, higher=up, lower=down
                    // Range: 0.1x to 4x mapped to bottom-top
                    function speedToY(s) {
                        var norm = (Math.log(Math.max(0.1, s)) - Math.log(0.1)) / (Math.log(4) - Math.log(0.1))
                        return height - norm * height
                    }

                    // Draw curve
                    ctx.strokeStyle = "#FFCA28"
                    ctx.lineWidth = 2
                    ctx.beginPath()
                    for (var i = 0; i < points.length; i++) {
                        var px = points[i].normalizedTime * width
                        var py = speedToY(points[i].speed)
                        if (i === 0) ctx.moveTo(px, py)
                        else {
                            // Smooth curve via quadratic bezier to midpoint
                            var prev = points[i - 1]
                            var prevX = prev.normalizedTime * width
                            var prevY = speedToY(prev.speed)
                            var cpX = (prevX + px) / 2
                            ctx.quadraticCurveTo(prevX + (px - prevX) * 0.4, prevY,
                                                  cpX, (prevY + py) / 2)
                            ctx.quadraticCurveTo(px - (px - prevX) * 0.4, py,
                                                  px, py)
                        }
                    }
                    ctx.stroke()

                    // Draw control points
                    for (var j = 0; j < points.length; j++) {
                        var dx = points[j].normalizedTime * width
                        var dy = speedToY(points[j].speed)
                        ctx.fillStyle = "#FFCA28"
                        ctx.beginPath()
                        ctx.arc(dx, dy, 4, 0, Math.PI * 2)
                        ctx.fill()
                        ctx.strokeStyle = "#111128"
                        ctx.lineWidth = 1
                        ctx.stroke()
                    }
                }
                // Repaint when tracks change (speed ramp modified)
                Connections {
                    target: timelineNotifier
                    function onTracksChanged() {
                        if (hasSpeedRamp && speedCurveOverlay.visible)
                            speedCurveOverlay.requestPaint()
                    }
                }
            }
        }

        // Click to select / drag to move / right-click context menu
        MouseArea {
            id: clipBodyMa
            anchors.fill: parent
            cursorShape: {
                if (timelineNotifier && timelineNotifier.razorModeEnabled)
                    return Qt.CrossCursor
                return dragActive ? Qt.ClosedHandCursor : Qt.PointingHandCursor
            }
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true

            property bool dragActive: false
            property real dragStartX: 0
            property real dragStartMouseX: 0
            property real origTimelineIn: 0
            property int origTrackIndex: -1
            property bool hasCollision: false
            property real snapEdgeX: -1  // x position of active snap line (-1 = hidden)
            property bool cachedMagneticTarget: false  // cached at drag start

            onPressed: function(mouse) {
                if (mouse.button === Qt.RightButton) return

                // Grab keyboard focus so shortcut keys (D, M, C, V, etc.) work
                root.forceActiveFocus()

                // ---- Razor/Blade tool: click-to-cut ----
                if (timelineNotifier && timelineNotifier.razorModeEnabled
                    && mouse.button === Qt.LeftButton && clipId >= 0) {
                    // Calculate the timeline position from click coordinates
                    var clickXInLane = mapToItem(clipRoot.parent, mouse.x, 0).x
                    var cutTime = clickXInLane / pps
                    if (mouse.modifiers & Qt.ShiftModifier) {
                        // Shift+click: through-cut all tracks at this position
                        timelineNotifier.splitAllAtPosition(cutTime)
                    } else {
                        // Normal click: cut only this clip
                        timelineNotifier.splitClipAtPosition(clipId, cutTime)
                    }
                    // Show cut line animation
                    root._razorCutX = clipRoot.x + mouse.x
                    root._razorCutTrackY = clipRoot.parent
                        ? clipRoot.parent.mapToItem(trackFlick.contentItem, 0, 0).y : 0
                    root._razorCutVisible = true
                    razorCutTimer.restart()
                    mouse.accepted = true
                    return  // don't start drag
                }

                dragActive = false
                hasCollision = false
                snapEdgeX = -1
                dragStartX = clipRoot.timelineIn * pps
                dragStartMouseX = mapToItem(clipRoot.parent, mouse.x, 0).x
                origTimelineIn = clipRoot.timelineIn
                origTrackIndex = clipRoot.trackIndex
                root._anyClipDragging = false
                // Cache magnetic state at drag start (avoid per-frame Q_INVOKABLE calls)
                cachedMagneticTarget = timelineNotifier
                    && timelineNotifier.magneticTimelineEnabled
                    && !timelineNotifier.positionOverrideActive
                    && timelineNotifier.isTrackMagneticPrimary(clipRoot.trackIndex)

                // Selection modes: Ctrl+Click, Shift+Click, Alt+Click
                if (timelineNotifier && clipId >= 0) {
                    if (mouse.modifiers & Qt.ControlModifier) {
                        // Ctrl+Click: toggle individual clip in/out of selection
                        timelineNotifier.toggleClipSelection(clipId)
                    } else if (mouse.modifiers & Qt.ShiftModifier) {
                        // Shift+Click: range select from last selected to this clip
                        var lastSel = timelineNotifier.selectedClipId
                        if (lastSel >= 0) {
                            timelineNotifier.selectClipRange(lastSel, clipId)
                        } else {
                            timelineNotifier.selectClip(clipId)
                        }
                    } else if ((mouse.modifiers & Qt.AltModifier) && linkedId >= 0) {
                        // Alt+Click on linked clip: select only this portion independently
                        timelineNotifier.selectClip(clipId)
                    } else if (timelineNotifier.isClipSelected(clipId) &&
                               timelineNotifier.selectedClipCount > 1) {
                        // Already part of multi-selection — don't deselect others
                        // (user may be starting a group drag)
                    } else {
                        timelineNotifier.selectClip(clipId)
                    }
                }
            }
            // Feature 13: track whether this is a group drag
            readonly property bool _isGroupDrag: timelineNotifier
                && timelineNotifier.selectedClipCount > 1
                && timelineNotifier.isClipSelected(clipId)

            onExited: {
                cutPreviewTimer.stop()
                root._cutPreviewVisible = false
            }

            onPositionChanged: function(mouse) {
                // Cut-point hover detection (when not pressed/dragging)
                if (!pressed && !dragActive) {
                    var edgeThreshold = 12
                    var atLeftEdge = mouse.x < edgeThreshold
                    var atRightEdge = mouse.x > (clipRoot.width - edgeThreshold)

                    if (!atLeftEdge && !atRightEdge) {
                        // Fast path: not near any edge — dismiss preview if showing, skip C++ calls
                        if (root._cutPreviewVisible || cutPreviewTimer.running) {
                            cutPreviewTimer.stop(); root._cutPreviewVisible = false
                        }
                        return
                    }

                    if (atLeftEdge || atRightEdge) {
                        var adjClip = internal.findAdjacentClip(clipRoot.trackIndex, clipRoot.clipIndex, atLeftEdge ? "left" : "right")
                        if (adjClip) {
                            var gPos = mapToItem(root, mouse.x, mouse.y)
                            root._cutPreviewX = gPos.x
                            root._cutPreviewY = gPos.y

                            if (atRightEdge) {
                                root._cutPreviewSourceA = clipRoot.sourcePath
                                root._cutPreviewTimeA = Math.max(0, clipRoot.sourceOut - 0.1)
                                root._cutPreviewNameA = clipRoot.displayName
                                root._cutPreviewSourceB = adjClip.sourcePath || ""
                                root._cutPreviewTimeB = (adjClip.sourceIn || 0) + 0.1
                                root._cutPreviewNameB = adjClip.displayName || ""
                                root._cutPreviewTransType = clipRoot.transitionOutType
                                root._cutPreviewTransDur = clipRoot.transitionOutDur
                            } else {
                                root._cutPreviewSourceA = adjClip.sourcePath || ""
                                root._cutPreviewTimeA = Math.max(0, (adjClip.sourceOut || 0) - 0.1)
                                root._cutPreviewNameA = adjClip.displayName || ""
                                root._cutPreviewSourceB = clipRoot.sourcePath
                                root._cutPreviewTimeB = clipRoot.sourceIn + 0.1
                                root._cutPreviewNameB = clipRoot.displayName
                                root._cutPreviewTransType = clipRoot.transitionInType
                                root._cutPreviewTransDur = clipRoot.transitionInDur
                            }

                            if (!cutPreviewTimer.running && !root._cutPreviewVisible)
                                cutPreviewTimer.restart()
                            return
                        }
                    }
                    // Not at a cut point — dismiss preview
                    if (root._cutPreviewVisible || cutPreviewTimer.running) {
                        cutPreviewTimer.stop()
                        root._cutPreviewVisible = false
                    }
                    return
                }

                if (!pressed) return
                // Dismiss preview when dragging starts
                if (dragActive) { cutPreviewTimer.stop(); root._cutPreviewVisible = false }

                var curX = mapToItem(clipRoot.parent, mouse.x, 0).x
                var deltaX = curX - dragStartMouseX

                if (!dragActive && Math.abs(deltaX) > 4)
                    dragActive = true

                if (!dragActive) return

                root._anyClipDragging = true
                var rawTime = Math.max(0, origTimelineIn + deltaX / pps)

                // Mouse position in the track area content coordinates
                var globalPos = mapToItem(trackFlick.contentItem, mouse.x, mouse.y)
                var trackCount = timelineNotifier ? timelineNotifier.trackCount : 1
                // Account for the "new track above" zone height
                var aboveZoneH = root._showNewTrackAbove ? defaultTrackHeight : 0
                var adjustedY = globalPos.y - aboveZoneH

                // New track zones only for single-clip drag
                var showAbove = false
                var showBelow = false
                if (!_isGroupDrag) {
                    var boundaryThreshold = defaultTrackHeight * 0.4
                    showAbove = adjustedY < -boundaryThreshold
                    showBelow = adjustedY > (trackCount * defaultTrackHeight) + boundaryThreshold
                }
                root._showNewTrackAbove = showAbove
                root._showNewTrackBelow = showBelow

                // Calculate target track
                var rawTrack
                if (showAbove) {
                    rawTrack = 0
                } else if (showBelow) {
                    rawTrack = trackCount
                } else {
                    rawTrack = Math.max(0, Math.min(
                        Math.floor(adjustedY / defaultTrackHeight), trackCount - 1))
                }

                var targetTrack = internal.findCompatibleTrack(
                    Math.min(rawTrack, trackCount - 1), sourceType)

                // Snap edges (left and right) via unified snap function
                var bestTime = rawTime
                if (!_isGroupDrag) {
                    var snapLeft = internal.snapToNearest(rawTime, clipId)
                    var snapRight = internal.snapToNearest(rawTime + clipRoot.duration, clipId)
                    var usedSnap = null
                    if (snapLeft && snapRight) {
                        var dL = Math.abs(snapLeft.time - rawTime)
                        var dR = Math.abs(snapRight.time - (rawTime + clipRoot.duration))
                        if (dL <= dR) { bestTime = snapLeft.time; usedSnap = snapLeft }
                        else { bestTime = snapRight.time - clipRoot.duration; usedSnap = snapRight }
                    } else if (snapLeft) { bestTime = snapLeft.time; usedSnap = snapLeft }
                    else if (snapRight) { bestTime = snapRight.time - clipRoot.duration; usedSnap = snapRight }
                    internal.showSnapGuide(usedSnap)
                }
                bestTime = Math.max(0, bestTime)

                // Collision detection only for single-clip drag
                hasCollision = false
                root._insertIndicatorVisible = false
                root._insertIndicatorTime = -1

                // Magnetic timeline: use cached value (computed at drag start, not per-frame)
                var isMagneticDrag = cachedMagneticTarget

                if (!_isGroupDrag && !showAbove && !showBelow && isMagneticDrag) {
                    // Show insertion indicator on magnetic track (no collision highlight)
                    var insertResult2 = internal.findInsertionPoint(targetTrack, bestTime, clipId)
                    if (insertResult2) {
                        root._insertIndicatorTime = insertResult2.time
                        root._insertIndicatorX = headerWidth + insertResult2.time * pps - trackFlick.contentX
                        root._insertIndicatorVisible = true
                    }
                } else if (!_isGroupDrag && !showAbove && !showBelow) {
                    var moveIn = bestTime
                    var moveOut = bestTime + clipRoot.duration
                    var trackClipCount2 = (targetTrack >= 0 && targetTrack < trackCount)
                        ? (timelineNotifier ? timelineNotifier.clipCountForTrack(targetTrack) : 0) : 0

                    for (var j = 0; j < trackClipCount2; j++) {
                        var oc = timelineNotifier.clipData(targetTrack, j)
                        if (oc.clipId === clipId) continue
                        var ocIn = oc.timelineIn
                        var ocOut = ocIn + (oc.duration || 0)
                        if (moveIn < ocOut - 0.01 && moveOut > ocIn + 0.01) {
                            hasCollision = true
                            break
                        }
                    }

                    if (hasCollision) {
                        var insertResult = internal.findInsertionPoint(targetTrack, bestTime, clipId)
                        if (insertResult) {
                            root._insertIndicatorTime = insertResult.time
                            root._insertIndicatorX = headerWidth + insertResult.time * pps - trackFlick.contentX
                            root._insertIndicatorVisible = true
                        }
                    }
                }

                clipRoot.x = bestTime * pps

                // Cross-timeline drag detection (stacked mode only)
                if (isStacked && dragActive) {
                    var crossGlobalY = mapToItem(root, 0, mouse.y).y
                    var secTop = stackedDivider.y + stackedDivider.height
                    if (crossGlobalY > secTop) {
                        root._crossTimelineDragActive = true
                        var relY = crossGlobalY - secTop - secondarySection.secHeaderH - secondarySection.secRulerH
                        root._crossTimelineDragTrack = Math.max(0, Math.floor(relY / secondarySection.secTrackH))
                        var secTime = (mapToItem(secFlick, mouse.x, 0).x + secFlick.contentX - secondarySection.secLabelW) / secondaryPps
                        root._crossTimelineDragTime = Math.max(0, secTime)
                    } else {
                        root._crossTimelineDragActive = false
                    }
                }
            }
            onReleased: function(mouse) {
                // Cross-timeline drop (stacked mode)
                if (root._crossTimelineDragActive && isStacked && dragActive
                    && timelineNotifier && clipId >= 0) {
                    timelineNotifier.moveClipToSecondaryTimeline(
                        clipId, root._crossTimelineDragTrack, root._crossTimelineDragTime)
                    dragActive = false
                    hasCollision = false; snapEdgeX = -1
                    internal.showSnapGuide(null)
                    root._crossTimelineDragActive = false
                    root._insertIndicatorVisible = false; root._insertIndicatorTime = -1
                    root._showNewTrackAbove = false; root._showNewTrackBelow = false
                    root._anyClipDragging = false
                    clipRoot.x = Qt.binding(function() { return clipRoot.timelineIn * pps })
                    emptyTrackCleanupTimer.restart()
                    return
                }
                root._crossTimelineDragActive = false

                var createdNewTrack = root._showNewTrackAbove || root._showNewTrackBelow

                if (dragActive && timelineNotifier && clipId >= 0) {
                    var newTime = Math.max(0, clipRoot.x / pps)
                    var trackCount = timelineNotifier.trackCount || 1

                    // Feature 13: Group drag — if multiple clips are selected,
                    // move ALL of them by the same time/track delta
                    if (_isGroupDrag) {
                        var deltaTime = newTime - origTimelineIn
                        var globalPos2 = mapToItem(trackFlick.contentItem, mouse.x, mouse.y)
                        var aboveZoneH2 = root._showNewTrackAbove ? defaultTrackHeight : 0
                        var rawTrack2 = Math.max(0, Math.min(
                            Math.floor((globalPos2.y - aboveZoneH2) / defaultTrackHeight),
                            trackCount - 1))
                        var targetTrack2 = internal.findCompatibleTrack(rawTrack2, sourceType)
                        var deltaTrack = targetTrack2 - origTrackIndex
                        timelineNotifier.moveSelectedClips(deltaTime, deltaTrack)
                    } else if (root._showNewTrackAbove || root._showNewTrackBelow) {
                        // Create a new track of the appropriate type, then move clip into it
                        var clipType = sourceType
                        var trackType = 1 // Video (TrackType enum: 0=Video,1=Audio,2=Title,3=Effect)
                        if (clipType === 0 || clipType === 1) trackType = 0  // Video/Image → Video
                        else if (clipType === 2) trackType = 2               // Title
                        else if (clipType === 3 || clipType === 4) trackType = 3 // Color/Adj → Effect

                        timelineNotifier.addTrack(trackType)
                        // New track is added at the end. Move clip to it.
                        var newTrackIdx = timelineNotifier.trackCount - 1
                        timelineNotifier.moveClip(clipId, newTrackIdx, newTime)
                    } else {
                        var globalPos = mapToItem(trackFlick.contentItem, mouse.x, mouse.y)
                        var aboveZoneH = root._showNewTrackAbove ? defaultTrackHeight : 0
                        var rawTrack = Math.max(0, Math.min(
                            Math.floor((globalPos.y - aboveZoneH) / defaultTrackHeight),
                            trackCount - 1))
                        var targetTrack = internal.findCompatibleTrack(rawTrack, sourceType)

                        // Magnetic timeline: use cached value from drag start
                        var isMagneticTarget = cachedMagneticTarget

                        if (isMagneticTarget) {
                            // Magnetic: moveClip handles insert-and-compact
                            timelineNotifier.moveClip(clipId, targetTrack, newTime)
                        } else if (root._insertIndicatorVisible && root._insertIndicatorTime >= 0) {
                            var insertTime = root._insertIndicatorTime
                            if (timelineNotifier.insertMode) {
                                timelineNotifier.reorderClipInsert(clipId, targetTrack, insertTime)
                            } else {
                                timelineNotifier.reorderClipOverwrite(clipId, targetTrack, insertTime)
                            }
                        } else {
                            timelineNotifier.moveClip(clipId, targetTrack, newTime)
                        }
                    }

                    // Schedule cleanup of empty tracks after a delay
                    emptyTrackCleanupTimer.restart()
                }

                // Reset all drag state
                dragActive = false
                hasCollision = false
                snapEdgeX = -1
                internal.showSnapGuide(null)
                root._insertIndicatorVisible = false
                root._insertIndicatorTime = -1
                root._showNewTrackAbove = false
                root._showNewTrackBelow = false
                root._anyClipDragging = false
                clipRoot.x = Qt.binding(function() { return clipRoot.timelineIn * pps })
            }
            onClicked: function(mouse) {
                // Feature 13: Left-click without Ctrl on a multi-selected clip
                // collapses selection to just that clip (deferred from press to click
                // so group drag works)
                if (mouse.button === Qt.LeftButton && timelineNotifier && clipId >= 0
                    && !(mouse.modifiers & Qt.ControlModifier)
                    && timelineNotifier.selectedClipCount > 1
                    && timelineNotifier.isClipSelected(clipId)) {
                    timelineNotifier.selectClip(clipId)
                }
                if (mouse.button === Qt.RightButton && timelineNotifier && clipId >= 0) {
                    console.log("[VE2Clip] RIGHT-CLICK on clip:", clipId, "opening context menu")
                    timelineNotifier.selectClip(clipId)
                    clipContextMenu.targetClipId = clipId
                    clipContextMenu.popup()
                }
            }
            onDoubleClicked: {
                if (timelineNotifier && clipId >= 0) {
                    // Double-click opens compound clip as new tab, or inspector for non-compound
                    if (sourceType === 0 || sourceType === 5) {
                        // Video/Audio clips: open as compound clip in new tab
                        timelineNotifier.openCompoundClip(clipId)
                    } else {
                        timelineNotifier.setActivePanel(1)
                    }
                }
            }
        }

        // Connected clip indicator — small orange anchor line at bottom
        Rectangle {
            visible: clipRoot.isConnectedClip
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 3; z: 12
            color: "#FF7043"
            radius: 1
        }
        // Connected clip anchor icon
        Label {
            visible: clipRoot.isConnectedClip
            text: "\u2693"  // anchor symbol
            font.pixelSize: 9
            color: "#FF7043"
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.bottomMargin: 3
            z: 13
        }

        // Linked audio/video sync indicator (dashed blue line + icon)
        Item {
            visible: clipRoot.linkedId >= 0
            anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            height: 2; z: 12

            Row {
                anchors.fill: parent; spacing: 4
                Repeater {
                    model: Math.max(0, Math.floor(parent.width / 8))
                    Rectangle { width: 4; height: 2; color: "#42A5F5" }
                }
            }
        }
        Label {
            visible: clipRoot.linkedId >= 0
            text: clipRoot.sourceType === 5 ? "\uD83C\uDFB5" : "\uD83D\uDD07"
            font.pixelSize: 9; color: "#42A5F5"
            anchors.bottom: parent.bottom; anchors.right: parent.right
            anchors.rightMargin: 4; anchors.bottomMargin: 2; z: 13
        }

        // Collision overlay — red tint when dragging onto occupied space
        Rectangle {
            anchors.fill: parent; radius: 4; z: 150
            visible: clipBodyMa.dragActive && clipBodyMa.hasCollision
            color: Qt.rgba(1, 0.2, 0.2, 0.25)
            border.color: "#EF5350"; border.width: 2
        }

        // (Per-clip snap line removed — using global snapGuideLine instead)

        // ---- Left trim handle (8px hover zone) ----
        Rectangle {
            id: trimLeftHandle
            width: 8; height: parent.height
            color: trimLeftMa.containsMouse || trimLeftMa.pressed ? "#FFFFFF" : clipColor
            opacity: trimLeftMa.containsMouse || trimLeftMa.pressed ? 1.0 : 0.0
            radius: 2; z: 12

            // Grip dots
            Column {
                anchors.centerIn: parent; spacing: 3; visible: trimLeftMa.containsMouse || trimLeftMa.pressed
                Repeater { model: 3; Rectangle { width: 2; height: 2; radius: 1; color: "#0A0A18" } }
            }

            MouseArea {
                id: trimLeftMa
                anchors.fill: parent; hoverEnabled: true
                anchors.margins: -2  // expand hit area slightly
                cursorShape: Qt.SplitHCursor

                property real startX: 0
                property real origIn: 0
                property real origOut: 0
                property real minBound: 0   // left limit (prev clip end or 0)
                property real maxIn: 0      // right limit (origOut - minDur)
                readonly property real minDur: 0.5

                onPressed: function(mouse) {
                    startX = mapToItem(clipRoot.parent, mouse.x, 0).x
                    origIn = clipRoot.timelineIn
                    origOut = origIn + clipRoot.duration
                    maxIn = origOut - minDur

                    // Find left boundary: end of previous clip on same track, or 0
                    // Also clamp by source extent: can't extend left beyond sourceIn=0
                    var sourceLeftLimit = clipRoot.sourceDuration > 0
                        ? origIn - (clipRoot.sourceIn / Math.max(0.01, clipRoot.speed))
                        : 0
                    minBound = Math.max(0, sourceLeftLimit)
                    if (timelineNotifier) {
                        var count = timelineNotifier.clipCountForTrack(clipRoot.trackIndex)
                        for (var i = 0; i < count; i++) {
                            var other = timelineNotifier.clipData(clipRoot.trackIndex, i)
                            if (other.clipId === clipId) continue
                            var oOut = other.timelineIn + (other.duration || 0)
                            if (oOut <= origIn + 0.001 && oOut > minBound)
                                minBound = oOut
                        }
                    }
                    if (timelineNotifier) timelineNotifier.selectClip(clipId)
                }
                onPositionChanged: function(mouse) {
                    if (!pressed || !timelineNotifier || clipId < 0) return
                    var curX = mapToItem(clipRoot.parent, mouse.x, 0).x
                    var delta = (curX - startX) / pps
                    var newIn = Math.max(minBound, Math.min(maxIn, origIn + delta))

                    // Snap the left edge to nearby points
                    var snap = internal.snapToNearest(newIn, clipId)
                    if (snap) {
                        var snapped = Math.max(minBound, Math.min(maxIn, snap.time))
                        newIn = snapped
                        internal.showSnapGuide(snap)
                    } else {
                        internal.showSnapGuide(null)
                    }

                    // Apply edit based on active trim mode
                    var mode = timelineNotifier.trimEditMode
                    if (mode === 2) {
                        // Roll: move edit point with neighbor
                        timelineNotifier.rollEdit(clipId, true, newIn - origIn)
                    } else if (mode === 3) {
                        // Slip: shift source content inside clip
                        timelineNotifier.slipEdit(clipId, delta)
                    } else {
                        // Normal / Ripple: standard trim (ripple handled in C++)
                        timelineNotifier.trimClip(clipId, newIn, origOut)
                    }
                }
                onReleased: internal.showSnapGuide(null)
            }

            // Trim tooltip — in-point time + mode indicator
            Rectangle {
                visible: trimLeftMa.pressed
                x: 10; y: -2
                width: trimLeftLabel.implicitWidth + 10; height: 18; radius: 3
                color: Qt.rgba(0, 0, 0, 0.85); border.color: clipColor; border.width: 1; z: 200
                Label {
                    id: trimLeftLabel; anchors.centerIn: parent
                    font.pixelSize: 9; font.family: "monospace"; font.weight: Font.Bold; color: "#FFF"
                    text: {
                        var mode = timelineNotifier ? timelineNotifier.trimEditMode : 0
                        var prefix = ["IN", "RIPPLE", "ROLL", "SLIP", "SLIDE"][mode]
                        return prefix + " " + internal.formatTimecodeFF(clipRoot.timelineIn)
                    }
                }
            }
        }

        // ---- Right trim handle (8px hover zone) ----
        Rectangle {
            id: trimRightHandle
            anchors.right: parent.right
            width: 8; height: parent.height
            color: trimRightMa.containsMouse || trimRightMa.pressed ? "#FFFFFF" : clipColor
            opacity: trimRightMa.containsMouse || trimRightMa.pressed ? 1.0 : 0.0
            radius: 2; z: 12

            // Grip dots
            Column {
                anchors.centerIn: parent; spacing: 3; visible: trimRightMa.containsMouse || trimRightMa.pressed
                Repeater { model: 3; Rectangle { width: 2; height: 2; radius: 1; color: "#0A0A18" } }
            }

            MouseArea {
                id: trimRightMa
                anchors.fill: parent; hoverEnabled: true
                anchors.margins: -2
                cursorShape: Qt.SplitHCursor

                property real startX: 0
                property real origIn: 0
                property real origOut: 0
                property real maxBound: 99999  // right limit (next clip start or infinity)
                property real minOut: 0        // left limit (origIn + minDur)
                readonly property real minDur: 0.5

                onPressed: function(mouse) {
                    startX = mapToItem(clipRoot.parent, mouse.x, 0).x
                    origIn = clipRoot.timelineIn
                    origOut = origIn + clipRoot.duration
                    minOut = origIn + minDur

                    // Find right boundary: start of next clip on same track
                    // Also clamp by source extent: can't extend right beyond sourceDuration
                    var sourceRightLimit = clipRoot.sourceDuration > 0
                        ? origIn + ((clipRoot.sourceDuration - clipRoot.sourceIn) / Math.max(0.01, clipRoot.speed))
                        : 99999
                    maxBound = sourceRightLimit
                    if (timelineNotifier) {
                        var count = timelineNotifier.clipCountForTrack(clipRoot.trackIndex)
                        for (var i = 0; i < count; i++) {
                            var other = timelineNotifier.clipData(clipRoot.trackIndex, i)
                            if (other.clipId === clipId) continue
                            var oIn = other.timelineIn
                            if (oIn >= origOut - 0.001 && oIn < maxBound)
                                maxBound = oIn
                        }
                    }
                    if (timelineNotifier) timelineNotifier.selectClip(clipId)
                }
                onPositionChanged: function(mouse) {
                    if (!pressed || !timelineNotifier || clipId < 0) return
                    var curX = mapToItem(clipRoot.parent, mouse.x, 0).x
                    var delta = (curX - startX) / pps
                    var newOut = Math.max(minOut, Math.min(maxBound, origOut + delta))

                    // Snap the right edge to nearby points
                    var snap = internal.snapToNearest(newOut, clipId)
                    if (snap) {
                        var snapped = Math.max(minOut, Math.min(maxBound, snap.time))
                        newOut = snapped
                        internal.showSnapGuide(snap)
                    } else {
                        internal.showSnapGuide(null)
                    }

                    // Apply edit based on active trim mode
                    var mode = timelineNotifier.trimEditMode
                    if (mode === 2) {
                        // Roll: move edit point with neighbor
                        timelineNotifier.rollEdit(clipId, false, newOut - origOut)
                    } else if (mode === 4) {
                        // Slide: move clip, neighbors adjust
                        timelineNotifier.slideEdit(clipId, delta)
                    } else {
                        // Normal / Ripple: standard trim (ripple handled in C++)
                        timelineNotifier.trimClip(clipId, origIn, newOut)
                    }
                }
                onReleased: internal.showSnapGuide(null)
            }

            // Trim tooltip — out-point time + mode indicator
            Rectangle {
                visible: trimRightMa.pressed
                anchors.right: parent.left; anchors.rightMargin: 2; y: -2
                width: trimRightLabel.implicitWidth + 10; height: 18; radius: 3
                color: Qt.rgba(0, 0, 0, 0.85); border.color: clipColor; border.width: 1; z: 200
                Label {
                    id: trimRightLabel; anchors.centerIn: parent
                    font.pixelSize: 9; font.family: "monospace"; font.weight: Font.Bold; color: "#FFF"
                    text: {
                        var mode = timelineNotifier ? timelineNotifier.trimEditMode : 0
                        var prefix = ["OUT", "RIPPLE", "ROLL", "SLIP", "SLIDE"][mode]
                        return prefix + " " + internal.formatTimecodeFF(clipRoot.timelineIn + clipRoot.duration)
                    }
                }
            }
        }
    }

    // ================================================================
    // Internal helpers
    // ================================================================
    QtObject {
        id: internal

        // Feature 11: Enhanced tick density — shows frames at high zoom, minutes at low zoom
        function bestInterval(pxPerSec) {
            if (pxPerSec <= 0) return 1
            // At very high zoom (>200pps), show individual frames (1/30s)
            var intervals = [1/30, 1/15, 1/10, 0.1, 0.25, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600]
            for (var i = 0; i < intervals.length; i++) {
                if (intervals[i] * pxPerSec >= 60) return intervals[i]
            }
            return 3600
        }

        // Feature 11: Show frames at high zoom, minutes:seconds at low zoom
        function formatTime(s) {
            // At high zoom show frames
            if (pps > 200 && s < 60) {
                var sec2 = Math.floor(s)
                var frames = Math.round((s - sec2) * 30)
                return sec2 + ":" + String(frames).padStart(2, "0") + "f"
            }
            if (s < 60) {
                var sec = Math.floor(s)
                var frac = Math.floor((s - sec) * 10)
                return frac > 0 ? sec + "." + frac + "s" : sec + "s"
            }
            var totalSec = Math.floor(s)
            var h = Math.floor(totalSec / 3600)
            var m = Math.floor((totalSec % 3600) / 60)
            var ss = totalSec % 60
            if (h > 0) return h + ":" + String(m).padStart(2, "0") + ":" + String(ss).padStart(2, "0")
            return m + ":" + String(ss).padStart(2, "0")
        }

        // Pre-compute and apply zoom BEFORE addClipsBatch so that when
        // tracksChanged fires, QML creates clip items at the correct zoom
        // level — avoids the 10-20s freeze from rendering at 100% zoom first.
        function preComputeFitZoom(addedDurationSum) {
            if (!root.autoFitOnImport || !timelineNotifier) return
            var viewWidth = trackFlick.width
            if (viewWidth <= 10) return
            var currentDur = timelineNotifier.totalDuration
            var expectedDur = currentDur + addedDurationSum
            if (expectedDur <= 0) return
            // If everything will fit at current zoom, no change needed
            var totalSpanPx = expectedDur * pps
            if (totalSpanPx <= viewWidth) return
            // Zoom to fit expected total duration + 20% padding
            var fitPps = viewWidth / (expectedDur * 1.2)
            fitPps = Math.max(0.01, Math.min(400, fitPps))
            timelineNotifier.setPixelsPerSecond(fitPps)
        }

        // Auto-fit timeline zoom to show all dropped clips (fine-tune after insertion)
        function autoFitToClips(clipIds) {
            if (!root.autoFitOnImport || !timelineNotifier || clipIds.length === 0) return

            var totalDur = timelineNotifier.totalDuration
            if (totalDur <= 0) return

            // Single C++ call replaces the O(D x T x C) nested loop
            var span = timelineNotifier.batchClipSpan(clipIds)
            var dropMinIn  = span.minIn
            var dropMaxOut = span.maxOut
            if (dropMinIn >= dropMaxOut) return

            // trackFlick.width already excludes headerWidth (it's parent.width - headerWidth)
            var viewWidth = trackFlick.width
            if (viewWidth <= 10) return

            // Check if all timeline content already fits at current zoom
            var totalSpanPx = totalDur * pps
            if (totalSpanPx <= viewWidth) return  // everything fits, don't change

            // Zoom to fit total duration + 10% padding
            var fitPps = viewWidth / (totalDur * 1.2)
            fitPps = Math.max(0.01, Math.min(400, fitPps))

            timelineNotifier.setPixelsPerSecond(fitPps)

            // Scroll to center on dropped clips
            var midpoint = (dropMinIn + dropMaxOut) / 2
            var scrollX = Math.max(0, midpoint * fitPps - viewWidth / 2)
            timelineNotifier.setScrollOffset(scrollX)
        }

        function findAdjacentClip(trackIdx, clipIdx, direction) {
            if (!timelineNotifier) return null
            var clipCount = timelineNotifier.clipCountForTrack(trackIdx)
            var adjIdx = direction === "left" ? clipIdx - 1 : clipIdx + 1
            if (adjIdx < 0 || adjIdx >= clipCount) return null
            var adj = timelineNotifier.clipData(trackIdx, adjIdx)
            if (!adj || adj.clipId < 0) return null
            var thisClip = timelineNotifier.clipData(trackIdx, clipIdx)
            if (!thisClip) return null
            var gap
            if (direction === "right")
                gap = Math.abs(adj.timelineIn - (thisClip.timelineIn + thisClip.duration))
            else
                gap = Math.abs(thisClip.timelineIn - (adj.timelineIn + adj.duration))
            if (gap > 0.05) return null
            return adj
        }

        function findInsertionPoint(trackIdx, dropTime, excludeClipId) {
            if (!timelineNotifier) return null
            var clipCount = timelineNotifier.clipCountForTrack(trackIdx)
            if (clipCount === 0) return { time: 0 }

            // Collect edges (clip start/end) sorted by time
            var edges = [0]  // start of timeline
            for (var i = 0; i < clipCount; i++) {
                var cd = timelineNotifier.clipData(trackIdx, i)
                if (cd.clipId === excludeClipId) continue
                edges.push(cd.timelineIn)
                edges.push(cd.timelineIn + (cd.duration || 0))
            }
            edges.sort(function(a, b) { return a - b })

            // Find nearest edge to dropTime
            var bestEdge = 0
            var bestDist = 1e12
            for (var e = 0; e < edges.length; e++) {
                var d = Math.abs(edges[e] - dropTime)
                if (d < bestDist) { bestDist = d; bestEdge = edges[e] }
            }
            return { time: bestEdge }
        }

        // Unified snap function — returns { time, snapX } or null.
        // Checks: all clip edges on ALL tracks, playhead position, markers.
        // edgeTime = the time value of the edge being snapped (e.g., clip left or right edge)
        // excludeClipId = clip being dragged/trimmed (skip its own edges)
        function snapToNearest(edgeTime, excludeClipId) {
            if (!root.snapEnabled || !timelineNotifier) return null

            var threshold = 10 / pps  // 10px in seconds
            var bestTime = edgeTime
            var bestDist = threshold

            // 1. All clip edges on all tracks
            var trackCount = timelineNotifier.trackCount || 0
            for (var t = 0; t < trackCount; t++) {
                var clipCount = timelineNotifier.clipCountForTrack(t)
                for (var c = 0; c < clipCount; c++) {
                    var cd = timelineNotifier.clipData(t, c)
                    if (cd.clipId === excludeClipId) continue
                    var edges = [cd.timelineIn, cd.timelineIn + (cd.duration || 0)]
                    for (var e = 0; e < edges.length; e++) {
                        var d = Math.abs(edgeTime - edges[e])
                        if (d < bestDist) { bestDist = d; bestTime = edges[e] }
                    }
                }
            }

            // 2. Playhead position
            var playPos = timelineNotifier.position
            var dp = Math.abs(edgeTime - playPos)
            if (dp < bestDist) { bestDist = dp; bestTime = playPos }

            // 3. Markers (start and region end positions)
            var markers = timelineNotifier.markers
            if (markers) {
                for (var m = 0; m < markers.length; m++) {
                    var mp = markers[m].position || 0
                    var dm = Math.abs(edgeTime - mp)
                    if (dm < bestDist) { bestDist = dm; bestTime = mp }
                    // Snap to region end position too
                    if (markers[m].isRegion && markers[m].endPosition !== undefined) {
                        var ep = markers[m].endPosition
                        var de = Math.abs(edgeTime - ep)
                        if (de < bestDist) { bestDist = de; bestTime = ep }
                    }
                }
            }

            if (bestDist < threshold) {
                var viewportX = headerWidth + bestTime * pps - trackFlick.contentX
                return { time: bestTime, snapX: viewportX }
            }
            return null
        }

        // Show/hide the global snap guide
        function showSnapGuide(snapResult) {
            if (snapResult) {
                root._snapGuideX = snapResult.snapX
                root._snapGuideVisible = true
            } else {
                root._snapGuideVisible = false
            }
        }

        // Format seconds as MM:SS:FF (frames at 30fps)
        function formatTimecodeFF(s) {
            if (s === undefined || s === null || isNaN(s) || s < 0) return "00:00:00"
            var mins = Math.floor(s / 60)
            var secs = Math.floor(s % 60)
            var frames = Math.floor((s - Math.floor(s)) * 30)
            return String(mins).padStart(2, '0') + ":" +
                   String(secs).padStart(2, '0') + ":" +
                   String(frames).padStart(2, '0')
        }

        // Convert a Y position (in track area local coords) to track index
        // accounting for per-track heights
        function trackIndexFromY(y) {
            if (!timelineNotifier) return 0
            var tc = timelineNotifier.trackCount
            var cumY = 0
            for (var i = 0; i < tc; i++) {
                var ti = timelineNotifier.trackInfo(i)
                var h = (ti.isVisible !== false) ? (ti.trackHeight || defaultTrackHeight) : 12
                if (y < cumY + h) return i
                cumY += h
            }
            return Math.max(0, tc - 1)
        }

        // Feature 13: Lasso selection — select all clips whose rectangles overlap the lasso.
        // Lasso coordinates are in root (VE2TimelinePanel) coordinate space.
        function performLassoSelect() {
            if (!timelineNotifier) return
            timelineNotifier.deselectAll()

            // Convert lasso rect from root coords to content (scrollable) coords
            var lx1 = Math.min(root._lassoStartX, root._lassoEndX) - headerWidth + trackFlick.contentX
            var lx2 = Math.max(root._lassoStartX, root._lassoEndX) - headerWidth + trackFlick.contentX
            var ly1 = Math.min(root._lassoStartY, root._lassoEndY) - rulerHeight + trackFlick.contentY
            var ly2 = Math.max(root._lassoStartY, root._lassoEndY) - rulerHeight + trackFlick.contentY

            var trackCount = timelineNotifier.trackCount || 0
            var aboveZoneH = root._showNewTrackAbove ? defaultTrackHeight : 0
            var trackY = aboveZoneH

            for (var t = 0; t < trackCount; t++) {
                var tInfo = timelineNotifier.trackInfo(t)
                var trackH = (tInfo.isVisible !== false) ? (tInfo.trackHeight || defaultTrackHeight) : 12
                var trackTop = trackY
                var trackBottom = trackY + trackH
                trackY += trackH

                if (trackTop > ly2 || trackBottom < ly1) continue

                var clipCount = timelineNotifier.clipCountForTrack(t)
                for (var c = 0; c < clipCount; c++) {
                    var cd = timelineNotifier.clipData(t, c)
                    var cx1 = cd.timelineIn * pps
                    var cx2 = (cd.timelineIn + (cd.duration || 0)) * pps

                    if (cx1 < lx2 && cx2 > lx1) {
                        timelineNotifier.addToSelection(cd.clipId)
                    }
                }
            }
        }

        // Feature 9: Find clip data by ID (for crossfade visualization)
        function findClipById(clipId) {
            if (!timelineNotifier) return null
            var trackCount = timelineNotifier.trackCount || 0
            for (var t = 0; t < trackCount; t++) {
                var clipCount = timelineNotifier.clipCountForTrack(t)
                for (var c = 0; c < clipCount; c++) {
                    var cd = timelineNotifier.clipData(t, c)
                    if (cd.clipId === clipId) {
                        cd.trackIdx = t
                        return cd
                    }
                }
            }
            return null
        }

        // Feature 12: Show split animation at current playhead
        function showSplitAnimation() {
            if (!timelineNotifier) return
            var pos = timelineNotifier.position
            root._splitLineX = headerWidth + pos * pps - trackFlick.contentX
            root._splitLineVisible = true
            splitFadeAnim.restart()
        }

        // Find the nearest track that is compatible with the given clip source type.
        // Track types: 0=Video, 1=Audio, 2=Title, 3=Effect, 4=Subtitle
        // Clip source types: 0=Video, 1=Image, 2=Title, 3=Color, 4=Adjustment, 5=Audio
        function findCompatibleTrack(rawTrackIdx, clipSourceType) {
            if (!timelineNotifier) return rawTrackIdx
            var tracks = timelineNotifier.tracks
            if (!tracks || tracks.length === 0) return rawTrackIdx

            // Determine which track type(s) this clip belongs on
            // Video/Image → Video track (type 0)
            // Audio → Audio track (type 1)
            // Title → Title (2) or Video (0)
            // Color/Adjustment → Effect track (3)
            function isCompatible(trackType, srcType) {
                switch (srcType) {
                case 0: return trackType === 0  // Video → Video
                case 1: return trackType === 0  // Image → Video
                case 2: return trackType === 2 || trackType === 0  // Title → Title or Video
                case 3: return trackType === 3  // Color → Effect
                case 4: return trackType === 3  // Adjustment → Effect
                case 5: return trackType === 1  // Audio → Audio
                default: return trackType === 0
                }
            }

            // If the raw track is already compatible, use it
            if (rawTrackIdx >= 0 && rawTrackIdx < tracks.length) {
                if (isCompatible(tracks[rawTrackIdx].type, clipSourceType))
                    return rawTrackIdx
            }

            // Find nearest compatible track
            var bestIdx = -1
            var bestDist = 9999
            for (var i = 0; i < tracks.length; i++) {
                if (isCompatible(tracks[i].type, clipSourceType)) {
                    var dist = Math.abs(i - rawTrackIdx)
                    if (dist < bestDist) { bestDist = dist; bestIdx = i }
                }
            }

            // If no compatible track found, return the raw index.
            // C++ moveClip will auto-create a compatible track.
            return bestIdx >= 0 ? bestIdx : rawTrackIdx
        }
    }

    // ---- Right-click context menu for gaps ----
    Menu {
        id: gapContextMenu
        property int _trackIdx: -1
        property double _gapStart: 0
        property double _gapDuration: 0

        MenuItem {
            text: "Close Gap (" + gapContextMenu._gapDuration.toFixed(1) + "s)"
            onTriggered: {
                if (timelineNotifier)
                    timelineNotifier.closeGapAt(gapContextMenu._trackIdx, gapContextMenu._gapStart)
            }
        }
        MenuItem {
            text: "Insert Slug / Placeholder"
            onTriggered: {
                if (timelineNotifier)
                    timelineNotifier.insertSlugAt(gapContextMenu._trackIdx, gapContextMenu._gapStart, gapContextMenu._gapDuration)
            }
        }
        MenuSeparator {}
        MenuItem {
            text: "Close All Gaps on This Track"
            onTriggered: {
                if (timelineNotifier)
                    timelineNotifier.closeTrackGaps(gapContextMenu._trackIdx)
            }
        }
        MenuItem {
            text: "Delete All Gaps (All Tracks)"
            onTriggered: {
                if (timelineNotifier)
                    timelineNotifier.deleteAllGaps()
            }
        }
    }

    // ---- Right-click context menu for clips ----
    Menu {
        id: clipContextMenu
        property int targetClipId: -1

        MenuItem {
            text: "Split at Playhead"
            onTriggered: { if (timelineNotifier) timelineNotifier.splitClipAtPlayhead() }
        }
        MenuItem {
            text: "Split All Tracks at Playhead"
            onTriggered: { if (timelineNotifier) timelineNotifier.splitAllAtPlayhead() }
        }
        MenuSeparator {}
        MenuItem {
            text: "Delete"
            onTriggered: { if (timelineNotifier && clipContextMenu.targetClipId >= 0) timelineNotifier.removeClip(clipContextMenu.targetClipId) }
        }
        MenuItem {
            text: "Ripple Delete"
            onTriggered: { if (timelineNotifier && clipContextMenu.targetClipId >= 0) timelineNotifier.rippleDelete(clipContextMenu.targetClipId) }
        }
        MenuItem {
            text: "Delete Selected"
            enabled: timelineNotifier ? timelineNotifier.selectedClipCount > 1 : false
            onTriggered: { if (timelineNotifier) timelineNotifier.deleteSelectedClips() }
        }
        MenuItem {
            text: "Duplicate"
            onTriggered: { if (timelineNotifier && clipContextMenu.targetClipId >= 0) timelineNotifier.duplicateClip(clipContextMenu.targetClipId) }
        }
        MenuItem {
            text: timelineNotifier && clipContextMenu.targetClipId >= 0
                  && timelineNotifier.isClipDisabled(clipContextMenu.targetClipId)
                  ? "Enable Clip (D)" : "Disable Clip (D)"
            onTriggered: {
                if (timelineNotifier && clipContextMenu.targetClipId >= 0)
                    timelineNotifier.toggleClipDisabled(clipContextMenu.targetClipId)
            }
        }
        MenuSeparator {}
        MenuItem {
            text: "Copy Effects"
            onTriggered: { if (timelineNotifier) timelineNotifier.copyEffects(clipContextMenu.targetClipId) }
        }
        MenuItem {
            text: "Paste Effects"
            onTriggered: { if (timelineNotifier) timelineNotifier.pasteEffects(clipContextMenu.targetClipId) }
        }
        MenuSeparator {}
        // Feature 10: Link/Unlink
        MenuItem {
            text: timelineNotifier && clipContextMenu.targetClipId >= 0 && timelineNotifier.isClipLinked(clipContextMenu.targetClipId)
                  ? "Unlink Clips" : "Link Selected Clips"
            enabled: {
                if (!timelineNotifier || clipContextMenu.targetClipId < 0) return false
                if (timelineNotifier.isClipLinked(clipContextMenu.targetClipId)) return true
                return timelineNotifier.selectedClipCount === 2
            }
            onTriggered: {
                if (!timelineNotifier) return
                var cid = clipContextMenu.targetClipId
                if (timelineNotifier.isClipLinked(cid)) {
                    timelineNotifier.unlinkClip(cid)
                } else {
                    // Link the two selected clips
                    var ids = timelineNotifier.selectedClipIds
                    if (ids.length === 2) timelineNotifier.linkClips(ids[0], ids[1])
                }
            }
        }
        // Detach / Reattach Audio
        MenuSeparator {}
        MenuItem {
            text: "Detach Audio"
            enabled: timelineNotifier && clipContextMenu.targetClipId >= 0
                && timelineNotifier.canDetachAudio(clipContextMenu.targetClipId)
            onTriggered: {
                if (timelineNotifier && clipContextMenu.targetClipId >= 0)
                    timelineNotifier.detachAudio(clipContextMenu.targetClipId)
            }
        }
        MenuItem {
            text: "Reattach Audio"
            enabled: timelineNotifier && clipContextMenu.targetClipId >= 0
                && timelineNotifier.canReattachAudio(clipContextMenu.targetClipId)
            onTriggered: {
                if (timelineNotifier && clipContextMenu.targetClipId >= 0)
                    timelineNotifier.reattachAudio(clipContextMenu.targetClipId)
            }
        }

        // Magnetic timeline: Connect/Disconnect from primary storyline
        MenuSeparator {}
        MenuItem {
            text: {
                if (!timelineNotifier || clipContextMenu.targetClipId < 0) return "Connect to Primary"
                var cid = clipContextMenu.targetClipId
                var sc = timelineNotifier.selectedClip
                if (sc && sc.connectedToPrimaryClipId >= 0) return "Disconnect from Primary"
                return "Connect to Primary"
            }
            enabled: timelineNotifier && timelineNotifier.magneticTimelineEnabled && clipContextMenu.targetClipId >= 0
            onTriggered: {
                if (!timelineNotifier) return
                var cid = clipContextMenu.targetClipId
                var sc = timelineNotifier.selectedClip
                if (sc && sc.connectedToPrimaryClipId >= 0) {
                    timelineNotifier.disconnectClip(cid)
                } else {
                    // Find the primary clip below at the same time position
                    var tracks = timelineNotifier.tracks
                    var clipData = null
                    for (var ti = 0; ti < tracks.length; ti++) {
                        if (tracks[ti].isMagneticPrimary) {
                            var cnt = timelineNotifier.clipCountForTrack(ti)
                            for (var ci = 0; ci < cnt; ci++) {
                                var cd = timelineNotifier.clipData(ti, ci)
                                if (sc && cd.timelineIn <= sc.timelineIn && (cd.timelineIn + cd.duration) > sc.timelineIn) {
                                    clipData = cd
                                    break
                                }
                            }
                        }
                    }
                    if (clipData) {
                        var offset = sc.timelineIn - clipData.timelineIn
                        timelineNotifier.connectClipToPrimary(cid, clipData.clipId, offset)
                    }
                }
            }
        }
        MenuSeparator {}
        // ---- Speed controls (flat — nested Menu unreliable on Windows Material) ----
        MenuItem {
            text: "Speed: 0.25x"
            onTriggered: { console.log("[Speed] 0.25x clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.setClipSpeed(clipContextMenu.targetClipId, 0.25) }
        }
        MenuItem {
            text: "Speed: 0.5x"
            onTriggered: { console.log("[Speed] 0.5x clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.setClipSpeed(clipContextMenu.targetClipId, 0.5) }
        }
        MenuItem {
            text: "Speed: 1x (Normal)"
            onTriggered: { console.log("[Speed] 1x clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.setClipSpeed(clipContextMenu.targetClipId, 1.0) }
        }
        MenuItem {
            text: "Speed: 2x"
            onTriggered: { console.log("[Speed] 2x clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.setClipSpeed(clipContextMenu.targetClipId, 2.0) }
        }
        MenuItem {
            text: "Speed: 4x"
            onTriggered: { console.log("[Speed] 4x clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.setClipSpeed(clipContextMenu.targetClipId, 4.0) }
        }
        MenuItem {
            text: "Custom Speed..."
            onTriggered: {
                console.log("[Speed] Custom clip:", clipContextMenu.targetClipId)
                if (clipContextMenu.targetClipId >= 0)
                    customSpeedDialog.openForClip(clipContextMenu.targetClipId)
            }
        }
        MenuItem {
            text: "Reverse"
            checkable: true
            checked: timelineNotifier && clipContextMenu.targetClipId >= 0
                     ? timelineNotifier.clipIsReversed(clipContextMenu.targetClipId) : false
            onTriggered: { console.log("[Speed] Reverse clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.reverseClip(clipContextMenu.targetClipId) }
        }
        MenuItem {
            text: "Freeze Frame (simple)"
            onTriggered: { if (timelineNotifier) timelineNotifier.freezeFrame(clipContextMenu.targetClipId) }
        }
        MenuItem {
            text: "Add Freeze Frame (2s)"
            onTriggered: { if (timelineNotifier) timelineNotifier.addFreezeFrame(clipContextMenu.targetClipId, 2.0) }
        }
        MenuItem {
            text: "Freeze and Extend (2s)"
            onTriggered: { if (timelineNotifier) timelineNotifier.freezeAndExtend(clipContextMenu.targetClipId, 2.0) }
        }
        MenuSeparator {}
        // ---- Speed Ramp Presets ----
        MenuItem {
            text: "Ramp: Smooth Slow-Mo"
            onTriggered: { console.log("[SpeedRamp] Smooth Slow-Mo clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.applySpeedRamp(clipContextMenu.targetClipId, 0) }
        }
        MenuItem {
            text: "Ramp: Speed Up"
            onTriggered: { console.log("[SpeedRamp] Speed Up clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.applySpeedRamp(clipContextMenu.targetClipId, 1) }
        }
        MenuItem {
            text: "Ramp: Slow Down"
            onTriggered: { console.log("[SpeedRamp] Slow Down clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.applySpeedRamp(clipContextMenu.targetClipId, 2) }
        }
        MenuItem {
            text: "Ramp: Pulse"
            onTriggered: { console.log("[SpeedRamp] Pulse clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.applySpeedRamp(clipContextMenu.targetClipId, 3) }
        }
        MenuItem {
            text: "Ramp: Dramatic Impact"
            onTriggered: { console.log("[SpeedRamp] Dramatic Impact clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.applySpeedRamp(clipContextMenu.targetClipId, 4) }
        }
        MenuItem {
            text: "Clear Speed Ramp"
            enabled: timelineNotifier && clipContextMenu.targetClipId >= 0
                     ? timelineNotifier.clipHasSpeedRamp(clipContextMenu.targetClipId) : false
            onTriggered: { console.log("[SpeedRamp] Clear clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.clearSpeedRamp(clipContextMenu.targetClipId) }
        }
        MenuSeparator {}
        // ---- Clip color coding ----
        Menu {
            title: "Assign Color"

            Repeater {
                model: timelineNotifier ? timelineNotifier.clipColorPresetCount() : 0
                delegate: MenuItem {
                    required property int index
                    readonly property string presetHex: timelineNotifier ? timelineNotifier.clipColorPresetHex(index) : ""
                    readonly property string presetName: timelineNotifier ? timelineNotifier.clipColorPresetName(index) : ""
                    text: presetName
                    icon.color: presetHex
                    onTriggered: {
                        if (timelineNotifier && clipContextMenu.targetClipId >= 0)
                            timelineNotifier.setClipColorTag(clipContextMenu.targetClipId, presetHex)
                    }
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "Auto (by media type)"
                onTriggered: {
                    if (timelineNotifier && clipContextMenu.targetClipId >= 0)
                        timelineNotifier.clearClipColorTag(clipContextMenu.targetClipId)
                }
            }
        }
        MenuItem {
            text: "Edit Label..."
            onTriggered: {
                if (clipContextMenu.targetClipId >= 0)
                    clipLabelDialog.openForClip(clipContextMenu.targetClipId)
            }
        }
    }

    // ---- Clip label edit dialog ----
    Rectangle {
        id: clipLabelDialog
        visible: false; z: 300
        width: 280; height: 130
        anchors.centerIn: parent
        radius: 10; color: "#1A1A34"
        border.color: "#6C63FF"; border.width: 2

        property int editingClipId: -1

        function openForClip(clipId) {
            editingClipId = clipId
            // Pre-fill with current custom label
            var data = timelineNotifier ? timelineNotifier.clipData(-1, -1) : ({})
            // We need to find the clip — iterate all tracks
            _clipLabelField.text = ""
            if (timelineNotifier) {
                var tc = timelineNotifier.trackCount || 0
                for (var t = 0; t < tc; t++) {
                    var cc = timelineNotifier.clipCountForTrack(t)
                    for (var c = 0; c < cc; c++) {
                        var cd = timelineNotifier.clipData(t, c)
                        if (cd.clipId === clipId) {
                            _clipLabelField.text = cd.customLabel || ""
                            break
                        }
                    }
                }
            }
            visible = true
            _clipLabelField.forceActiveFocus()
            _clipLabelField.selectAll()
        }

        function saveAndClose() {
            if (timelineNotifier && editingClipId >= 0)
                timelineNotifier.setClipCustomLabel(editingClipId, _clipLabelField.text.trim())
            visible = false
        }

        Rectangle {
            parent: root; anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.4); visible: clipLabelDialog.visible; z: 299
            MouseArea { anchors.fill: parent; onClicked: clipLabelDialog.visible = false }
        }

        Column {
            anchors.fill: parent; anchors.margins: 14; spacing: 10

            Label { text: "Clip Label"; font.pixelSize: 14; font.weight: Font.Bold; color: "#E0E0F0" }

            TextField {
                id: _clipLabelField
                width: parent.width; height: 32
                font.pixelSize: 12; color: "#E0E0F0"
                placeholderText: "Enter custom label (empty to clear)"
                background: Rectangle { radius: 4; color: "#111128"; border.color: "#353560" }
                onAccepted: clipLabelDialog.saveAndClose()
            }

            Row {
                spacing: 10; anchors.right: parent.right
                Button {
                    text: "Cancel"; width: 70; height: 28
                    onClicked: clipLabelDialog.visible = false
                    background: Rectangle { radius: 6; color: "#252540"; border.color: "#404060" }
                    contentItem: Label { text: parent.text; color: "#9090B0"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "Save"; width: 70; height: 28
                    onClicked: clipLabelDialog.saveAndClose()
                    background: Rectangle { radius: 6; color: "#6C63FF" }
                    contentItem: Label { text: parent.text; color: "white"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
        Keys.onEscapePressed: visible = false
    }

    // ---- Custom speed dialog ----
    Rectangle {
        id: customSpeedDialog
        visible: false; z: 300
        width: 280; height: 130
        anchors.centerIn: parent
        radius: 10; color: "#1A1A34"
        border.color: "#FFCA28"; border.width: 2

        property int editingClipId: -1

        function openForClip(clipId) {
            editingClipId = clipId
            var currentSpeed = timelineNotifier ? timelineNotifier.clipSpeed(clipId) : 1.0
            _customSpeedField.text = currentSpeed.toFixed(2)
            visible = true
            _customSpeedField.forceActiveFocus()
            _customSpeedField.selectAll()
        }

        function applyAndClose() {
            var val = parseFloat(_customSpeedField.text)
            if (!isNaN(val) && val > 0 && val <= 100 && timelineNotifier && editingClipId >= 0)
                timelineNotifier.setClipSpeed(editingClipId, val)
            visible = false
        }

        Rectangle {
            parent: root; anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.4); visible: customSpeedDialog.visible; z: 299
            MouseArea { anchors.fill: parent; onClicked: customSpeedDialog.visible = false }
        }

        Column {
            anchors.fill: parent; anchors.margins: 14; spacing: 10

            Label { text: "Custom Speed"; font.pixelSize: 14; font.weight: Font.Bold; color: "#E0E0F0" }

            TextField {
                id: _customSpeedField
                width: parent.width; height: 32
                font.pixelSize: 12; color: "#E0E0F0"
                placeholderText: "Enter speed (e.g. 1.5)"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                background: Rectangle { radius: 4; color: "#111128"; border.color: "#353560" }
                onAccepted: customSpeedDialog.applyAndClose()
            }

            Row {
                spacing: 10; anchors.right: parent.right
                Button {
                    text: "Cancel"; width: 70; height: 28
                    onClicked: customSpeedDialog.visible = false
                    background: Rectangle { radius: 6; color: "#252540"; border.color: "#404060" }
                    contentItem: Label { text: parent.text; color: "#9090B0"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "Apply"; width: 70; height: 28
                    onClicked: customSpeedDialog.applyAndClose()
                    background: Rectangle { radius: 6; color: "#FFCA28" }
                    contentItem: Label { text: parent.text; color: "#111128"; font.pixelSize: 11; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
        Keys.onEscapePressed: visible = false
    }

    // ---- Track color context menu ----
    Menu {
        id: trackColorMenu
        property int _trackIdx: -1

        Menu {
            title: "Track Color"

            Repeater {
                model: timelineNotifier ? timelineNotifier.clipColorPresetCount() : 0
                delegate: MenuItem {
                    required property int index
                    readonly property string presetHex: timelineNotifier ? timelineNotifier.clipColorPresetHex(index) : ""
                    readonly property string presetName: timelineNotifier ? timelineNotifier.clipColorPresetName(index) : ""
                    text: presetName
                    icon.color: presetHex
                    onTriggered: {
                        if (timelineNotifier && trackColorMenu._trackIdx >= 0)
                            timelineNotifier.setTrackColor(trackColorMenu._trackIdx, presetHex)
                    }
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "Clear Color"
                onTriggered: {
                    if (timelineNotifier && trackColorMenu._trackIdx >= 0)
                        timelineNotifier.clearTrackColor(trackColorMenu._trackIdx)
                }
            }
        }
    }

    // ---- Feature 9: Right-click context menu for crossfade transition zones ----
    Menu {
        id: crossfadeContextMenu
        property int cfClipA: -1
        property int cfClipB: -1

        MenuItem {
            text: "Crossfade"
            onTriggered: { if (timelineNotifier) timelineNotifier.changeCrossfadeType(crossfadeContextMenu.cfClipA, crossfadeContextMenu.cfClipB, 0) }
        }
        MenuItem {
            text: "Dip to Black"
            onTriggered: { if (timelineNotifier) timelineNotifier.changeCrossfadeType(crossfadeContextMenu.cfClipA, crossfadeContextMenu.cfClipB, 1) }
        }
        MenuItem {
            text: "Wipe Left"
            onTriggered: { if (timelineNotifier) timelineNotifier.changeCrossfadeType(crossfadeContextMenu.cfClipA, crossfadeContextMenu.cfClipB, 2) }
        }
        MenuItem {
            text: "Wipe Right"
            onTriggered: { if (timelineNotifier) timelineNotifier.changeCrossfadeType(crossfadeContextMenu.cfClipA, crossfadeContextMenu.cfClipB, 3) }
        }
        MenuItem {
            text: "Wipe Up"
            onTriggered: { if (timelineNotifier) timelineNotifier.changeCrossfadeType(crossfadeContextMenu.cfClipA, crossfadeContextMenu.cfClipB, 4) }
        }
        MenuItem {
            text: "Wipe Down"
            onTriggered: { if (timelineNotifier) timelineNotifier.changeCrossfadeType(crossfadeContextMenu.cfClipA, crossfadeContextMenu.cfClipB, 5) }
        }
        MenuSeparator {}
        MenuItem {
            text: "Remove Transition"
            onTriggered: { if (timelineNotifier) timelineNotifier.removeCrossfade(crossfadeContextMenu.cfClipA, crossfadeContextMenu.cfClipB) }
        }
    }

    // ================================================================
    // Ruler right-click context menu — place marker at clicked position
    // ================================================================
    Menu {
        id: rulerContextMenu
        property real _clickTime: 0

        MenuItem {
            text: "Add Chapter Marker"
            onTriggered: {
                if (timelineNotifier) timelineNotifier.addMarkerAtPosition(
                    rulerContextMenu._clickTime, 0, "Chapter", "")
            }
        }
        MenuItem {
            text: "Add Comment Marker"
            onTriggered: {
                if (timelineNotifier) timelineNotifier.addMarkerAtPosition(
                    rulerContextMenu._clickTime, 1, "Comment", "")
            }
        }
        MenuItem {
            text: "Add ToDo Marker"
            onTriggered: {
                if (timelineNotifier) timelineNotifier.addMarkerAtPosition(
                    rulerContextMenu._clickTime, 2, "ToDo", "")
            }
        }
        MenuItem {
            text: "Add Sync Point"
            onTriggered: {
                if (timelineNotifier) timelineNotifier.addMarkerAtPosition(
                    rulerContextMenu._clickTime, 3, "Sync", "")
            }
        }
        MenuSeparator {}
        MenuItem {
            text: "Add Region (to Out Point)"
            enabled: timelineNotifier ? timelineNotifier.hasOutPoint : false
            onTriggered: {
                if (timelineNotifier) {
                    var end = timelineNotifier.outPoint
                    if (end > rulerContextMenu._clickTime)
                        timelineNotifier.addMarkerRegion(rulerContextMenu._clickTime, end, 0, "Region")
                }
            }
        }
    }

    // ================================================================
    // Marker right-click context menu
    // ================================================================
    Menu {
        id: markerContextMenu
        property int targetMarkerId: -1

        MenuItem {
            text: "Edit Marker..."
            onTriggered: markerEditDialog.openForMarker(markerContextMenu.targetMarkerId)
        }
        MenuItem {
            text: "Jump to Marker"
            onTriggered: { if (timelineNotifier) timelineNotifier.navigateToMarker(markerContextMenu.targetMarkerId) }
        }
        MenuSeparator {}
        MenuItem {
            text: "Delete Marker"
            onTriggered: { if (timelineNotifier) timelineNotifier.removeMarker(markerContextMenu.targetMarkerId) }
        }
    }

    // ================================================================
    // Marker Edit Dialog — opened by double-clicking a marker
    // ================================================================
    Rectangle {
        id: markerEditDialog
        visible: false
        z: 300
        width: 320; height: 340
        anchors.centerIn: parent
        radius: 12
        color: "#1A1A34"
        border.color: "#6C63FF"; border.width: 2

        property int editingMarkerId: -1
        property var markerData: ({})

        function openForMarker(markerId) {
            if (!timelineNotifier) return
            var data = timelineNotifier.markerById(markerId)
            if (!data || data.id === undefined) return
            editingMarkerId = markerId
            markerData = data
            _labelField.text = data.label || ""
            _notesField.text = data.notes || ""
            _colorField.text = data.color || "#4CAF50"
            _typeCombo.currentIndex = data.type !== undefined ? data.type : 0
            visible = true
            _labelField.forceActiveFocus()
        }

        function saveAndClose() {
            if (timelineNotifier && editingMarkerId >= 0) {
                timelineNotifier.updateMarkerFull(
                    editingMarkerId,
                    _labelField.text.trim() || "Marker",
                    _notesField.text,
                    _colorField.text,
                    _typeCombo.currentIndex
                )
            }
            visible = false
        }

        // Dark overlay behind dialog
        Rectangle {
            parent: root
            anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.5)
            visible: markerEditDialog.visible
            z: 299
            MouseArea { anchors.fill: parent; onClicked: markerEditDialog.visible = false }
        }

        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Label {
                text: "Edit Marker"
                font.pixelSize: 16; font.weight: Font.Bold; color: "#E0E0F0"
            }

            // Type selector
            Row {
                spacing: 8
                Label { text: "Type:"; font.pixelSize: 12; color: "#9090B0"; width: 50; y: 4 }
                ComboBox {
                    id: _typeCombo
                    width: 200; height: 30
                    model: ["Chapter", "Comment", "ToDo", "Sync Point"]
                    onCurrentIndexChanged: {
                        var colors = ["#4CAF50", "#2196F3", "#FF9800", "#E91E63"]
                        _colorField.text = colors[currentIndex]
                    }
                }
            }

            // Label
            Row {
                spacing: 8
                Label { text: "Label:"; font.pixelSize: 12; color: "#9090B0"; width: 50; y: 4 }
                TextField {
                    id: _labelField
                    width: 200; height: 30
                    font.pixelSize: 12; color: "#E0E0F0"
                    placeholderText: "Marker label"
                    background: Rectangle { radius: 4; color: "#111128"; border.color: "#353560" }
                    onAccepted: markerEditDialog.saveAndClose()
                }
            }

            // Color
            Row {
                spacing: 8
                Label { text: "Color:"; font.pixelSize: 12; color: "#9090B0"; width: 50; y: 4 }
                TextField {
                    id: _colorField
                    width: 100; height: 30
                    font.pixelSize: 12; color: "#E0E0F0"
                    background: Rectangle { radius: 4; color: "#111128"; border.color: "#353560" }
                }
                Rectangle {
                    width: 30; height: 30; radius: 4
                    color: _colorField.text || "#4CAF50"
                    border.color: "#353560"
                }
            }

            // Notes
            Row {
                spacing: 8
                Label { text: "Notes:"; font.pixelSize: 12; color: "#9090B0"; width: 50 }
                ScrollView {
                    width: 200; height: 80
                    TextArea {
                        id: _notesField
                        font.pixelSize: 11; color: "#E0E0F0"
                        wrapMode: TextArea.Wrap
                        placeholderText: "Optional notes..."
                        background: Rectangle { radius: 4; color: "#111128"; border.color: "#353560" }
                    }
                }
            }

            // Buttons
            Row {
                spacing: 10
                anchors.right: parent.right
                anchors.rightMargin: 0

                Button {
                    text: "Cancel"
                    width: 80; height: 32
                    onClicked: markerEditDialog.visible = false
                    background: Rectangle { radius: 6; color: "#252540"; border.color: "#404060" }
                    contentItem: Label { text: parent.text; color: "#9090B0"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "Save"
                    width: 80; height: 32
                    onClicked: markerEditDialog.saveAndClose()
                    background: Rectangle { radius: 6; color: "#6C63FF" }
                    contentItem: Label { text: parent.text; color: "white"; font.pixelSize: 12; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        Keys.onEscapePressed: visible = false
    }

    // ================================================================
    // Keyboard shortcut: M to add marker at playhead
    // ================================================================
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_M && !event.modifiers && timelineNotifier && !markerEditDialog.visible) {
            timelineNotifier.addMarker()
            event.accepted = true
        }
        // D to toggle disable on selected clip(s)
        if (event.key === Qt.Key_D && !event.modifiers && timelineNotifier) {
            if (timelineNotifier.selectedClipCount > 1) {
                var ids = timelineNotifier.selectedClipIds
                for (var i = 0; i < ids.length; i++)
                    timelineNotifier.toggleClipDisabled(ids[i])
            } else if (timelineNotifier.selectedClipId >= 0) {
                timelineNotifier.toggleClipDisabled(timelineNotifier.selectedClipId)
            }
            event.accepted = true
        }
        // A to select track forward, Shift+A to select track backward
        if (event.key === Qt.Key_A && timelineNotifier && timelineNotifier.selectedClipId >= 0) {
            if (event.modifiers & Qt.ShiftModifier) {
                timelineNotifier.selectTrackBackward(timelineNotifier.selectedClipId)
            } else if (!event.modifiers) {
                timelineNotifier.selectTrackForward(timelineNotifier.selectedClipId)
            }
            event.accepted = true
        }
        // C to activate razor mode
        if (event.key === Qt.Key_C && !event.modifiers && timelineNotifier) {
            timelineNotifier.setRazorMode(true)
            event.accepted = true
        }
        // V to return to selection mode
        if (event.key === Qt.Key_V && !event.modifiers && timelineNotifier) {
            timelineNotifier.setRazorMode(false)
            timelineNotifier.setTrimEditMode(0)
            event.accepted = true
        }
        // JKL key state tracking for combo detection (J+K = slow reverse, K+L = slow forward)
        if (event.key === Qt.Key_J) root._jKeyHeld = true
        if (event.key === Qt.Key_K) root._kKeyHeld = true
        if (event.key === Qt.Key_L) root._lKeyHeld = true

        // Magnetic timeline: hold P for free position override
        if (event.key === Qt.Key_P && !event.isAutoRepeat && timelineNotifier) {
            timelineNotifier.setPositionOverride(true)
            event.accepted = true
        }
    }
    Keys.onReleased: function(event) {
        // JKL key release tracking
        if (event.key === Qt.Key_J) root._jKeyHeld = false
        if (event.key === Qt.Key_K) root._kKeyHeld = false
        if (event.key === Qt.Key_L) root._lKeyHeld = false

        if (event.key === Qt.Key_P && !event.isAutoRepeat && timelineNotifier) {
            timelineNotifier.setPositionOverride(false)
        }
    }
    focus: true

    // ================================================================
    // Markers list panel (toggled via markers panel tab)
    // ================================================================
    Rectangle {
        id: markersListPanel
        visible: timelineNotifier ? timelineNotifier.activePanel === 10 : false  // BottomPanelTab::markers = 10
        anchors.right: parent.right
        anchors.top: rulerRow.bottom
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        width: 260
        z: 150
        color: "#12122A"
        border.color: "#1E1E38"; border.width: 1

        Column {
            anchors.fill: parent
            spacing: 0

            // Header
            Rectangle {
                width: parent.width; height: 32
                color: "#18183A"
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                    Label {
                        text: "Markers (" + (timelineNotifier ? timelineNotifier.markerCount : 0) + ")"
                        font.pixelSize: 12; font.weight: Font.Bold; color: "#C0C0D8"
                        Layout.fillWidth: true
                    }
                    ToolButton {
                        width: 24; height: 24
                        contentItem: Label { text: "+"; font.pixelSize: 16; color: "#6C63FF"; anchors.centerIn: parent }
                        onClicked: { if (timelineNotifier) timelineNotifier.addMarker() }
                        ToolTip.text: "Add marker at playhead (M)"; ToolTip.visible: hovered
                    }
                    ToolButton {
                        width: 24; height: 24
                        contentItem: Label { text: "\u2716"; font.pixelSize: 12; color: "#8888A0"; anchors.centerIn: parent }
                        onClicked: { if (timelineNotifier) timelineNotifier.setActivePanel(0) }
                        ToolTip.text: "Close panel"; ToolTip.visible: hovered
                    }
                }
            }

            // Marker list
            ListView {
                id: markersListView
                width: parent.width
                height: parent.height - 32
                clip: true
                model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.markers : []

                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    width: markersListView.width
                    height: 44
                    color: markerListMa.containsMouse ? "#1E1E3A" : "transparent"

                    readonly property string mColor: modelData.color || "#4CAF50"
                    readonly property int mType: modelData.type !== undefined ? modelData.type : 0

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        spacing: 8

                        // Color indicator
                        Rectangle {
                            width: 4; height: 28; radius: 2
                            color: mColor
                        }

                        Column {
                            Layout.fillWidth: true
                            spacing: 2

                            RowLayout {
                                spacing: 4
                                // Type badge
                                Rectangle {
                                    width: typeLbl.implicitWidth + 8; height: 14; radius: 3
                                    color: Qt.rgba(
                                        parseInt(mColor.substr(1,2), 16) / 255,
                                        parseInt(mColor.substr(3,2), 16) / 255,
                                        parseInt(mColor.substr(5,2), 16) / 255, 0.2)
                                    Label {
                                        id: typeLbl
                                        anchors.centerIn: parent
                                        text: ["CH", "CMT", "TODO", "SYNC"][Math.min(mType, 3)]
                                        font.pixelSize: 8; font.weight: Font.Bold; color: mColor
                                    }
                                }
                                Label {
                                    text: modelData.label || "Marker"
                                    font.pixelSize: 11; font.weight: Font.Medium; color: "#D0D0E0"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Label {
                                text: {
                                    var pos = internal.formatTimecodeFF(modelData.position || 0)
                                    if (modelData.isRegion)
                                        pos += " - " + internal.formatTimecodeFF(modelData.endPosition || 0)
                                    return pos
                                }
                                font.pixelSize: 9; font.family: "monospace"; color: "#7070A0"
                            }
                        }

                        // Navigate button
                        ToolButton {
                            width: 20; height: 20
                            contentItem: Label { text: "\u25B6"; font.pixelSize: 10; color: "#8888A0"; anchors.centerIn: parent }
                            onClicked: { if (timelineNotifier) timelineNotifier.navigateToMarker(modelData.id) }
                            ToolTip.text: "Jump to marker"; ToolTip.visible: hovered
                        }
                    }

                    MouseArea {
                        id: markerListMa
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        z: -1
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                markerContextMenu.targetMarkerId = modelData.id
                                markerContextMenu.popup()
                            } else {
                                if (timelineNotifier) timelineNotifier.navigateToMarker(modelData.id)
                            }
                        }
                        onDoubleClicked: markerEditDialog.openForMarker(modelData.id)
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom; width: parent.width; height: 0.5
                        color: "#1E1E38"
                    }
                }

                // Empty state
                Label {
                    anchors.centerIn: parent
                    visible: markersListView.count === 0
                    text: "No markers yet\nPress M to add one"
                    font.pixelSize: 12; color: "#505068"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    // ================================================================
    // Color Legend panel
    // ================================================================
    Rectangle {
        id: colorLegendPanel
        visible: false
        anchors.left: parent.left; anchors.leftMargin: headerWidth + 8
        anchors.top: rulerRow.bottom; anchors.topMargin: 4
        width: 200; height: legendCol.implicitHeight + 16
        z: 160; radius: 8
        color: "#18183A"; border.color: "#2A2A50"; border.width: 1

        property bool showLegend: false

        Column {
            id: legendCol
            anchors.fill: parent; anchors.margins: 8; spacing: 4

            RowLayout {
                width: parent.width
                Label { text: "Color Legend"; font.pixelSize: 12; font.weight: Font.Bold; color: "#C0C0D8"; Layout.fillWidth: true }
                ToolButton {
                    width: 18; height: 18
                    contentItem: Label { text: "\u2716"; font.pixelSize: 10; color: "#8888A0"; anchors.centerIn: parent }
                    onClicked: colorLegendPanel.visible = false
                }
            }

            Rectangle { width: parent.width; height: 1; color: "#2A2A50" }

            Label { text: "Auto Colors (by media type)"; font.pixelSize: 10; color: "#7070A0"; topPadding: 2 }

            Repeater {
                model: [
                    { label: "Video", color: "#42A5F5" },
                    { label: "Image", color: "#FFEE58" },
                    { label: "Title", color: "#AB47BC" },
                    { label: "Color/Solid", color: "#66BB6A" },
                    { label: "Adjustment", color: "#78909C" },
                    { label: "Audio", color: "#66BB6A" }
                ]
                delegate: Row {
                    required property var modelData
                    spacing: 6
                    Rectangle { width: 12; height: 12; radius: 2; color: modelData.color; y: 1 }
                    Label { text: modelData.label; font.pixelSize: 10; color: "#B0B0C8" }
                }
            }

            Item { width: parent.width; height: 3
                Rectangle { width: parent.width; height: 1; color: "#2A2A50"; anchors.bottom: parent.bottom }
            }

            Label { text: "Preset Palette"; font.pixelSize: 10; color: "#7070A0"; topPadding: 2 }

            // 16-color grid: 4 columns x 4 rows
            Grid {
                columns: 4; spacing: 3
                Repeater {
                    model: 16
                    delegate: Rectangle {
                        required property int index
                        width: 40; height: 16; radius: 3
                        color: timelineNotifier ? timelineNotifier.clipColorPresetHex(index) : "#888"
                        Label {
                            anchors.centerIn: parent
                            text: timelineNotifier ? timelineNotifier.clipColorPresetName(index) : ""
                            font.pixelSize: 7; color: index >= 10 && index <= 11 ? "#333" : "#FFF"
                            elide: Text.ElideRight; width: parent.width - 4
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }
    }

    // ---- Feature 13: Lasso selection rectangle ----
    Rectangle {
        id: lassoRect
        visible: root._lassoActive
        z: 200
        color: Qt.rgba(0.27, 0.54, 1.0, 0.12)
        border.color: "#448AFF"; border.width: 1.5
        radius: 2

        x: Math.min(root._lassoStartX, root._lassoEndX)
        y: Math.min(root._lassoStartY, root._lassoEndY)
        width: Math.abs(root._lassoEndX - root._lassoStartX)
        height: Math.abs(root._lassoEndY - root._lassoStartY)
    }

    // ---- Feature 12: Split line animation ----
    Rectangle {
        id: splitLine
        visible: root._splitLineVisible
        anchors.top: rulerRow.bottom
        anchors.bottom: isStacked ? stackedDivider.top : footer.top
        x: root._splitLineX
        width: 2; z: 110; radius: 1
        color: "#FF5252"
        opacity: 1.0

        NumberAnimation on opacity {
            id: splitFadeAnim
            from: 1.0; to: 0.0; duration: 600
            running: false
            onFinished: root._splitLineVisible = false
        }
    }

    // ---- Feature 9: Crossfade transition visualizations ----
    Repeater {
        model: timelineNotifier ? timelineNotifier.transitions : []
        delegate: Item {
            id: cfItem
            required property var modelData
            readonly property int clipAId: modelData.clipAId
            readonly property int clipBId: modelData.clipBId
            readonly property int cfType: modelData.type
            readonly property real cfDuration: modelData.duration

            // Find clip positions to determine overlap region
            readonly property var clipAData: timelineNotifier ? internal.findClipById(clipAId) : null
            readonly property var clipBData: timelineNotifier ? internal.findClipById(clipBId) : null

            visible: clipAData !== null && clipBData !== null

            // Overlap region: where clip A ends and clip B begins
            readonly property real overlapStart: clipBData ? clipBData.timelineIn : 0
            readonly property real overlapEnd: clipAData ? (clipAData.timelineIn + clipAData.duration) : 0

            x: headerWidth + overlapStart * pps - trackFlick.contentX
            y: rulerHeight + (clipAData ? clipAData.trackIdx * defaultTrackHeight : 0) + 3
            width: Math.max(0, (overlapEnd - overlapStart) * pps)
            height: defaultTrackHeight - 6
            z: 50

            // Diagonal gradient (X pattern) crossfade visualization
            Canvas {
                anchors.fill: parent
                visible: parent.width > 2
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    // Choose color based on crossfade type
                    var colors = ["#26C6DA", "#1A1A1A", "#FF7043", "#FF7043", "#AB47BC", "#AB47BC"]
                    var c = colors[Math.min(cfItem.cfType, colors.length - 1)]

                    ctx.globalAlpha = 0.35
                    ctx.fillStyle = c

                    // Draw X pattern (diagonal lines)
                    ctx.lineWidth = 2
                    ctx.strokeStyle = c
                    var step = 8
                    for (var i = -height; i < width + height; i += step) {
                        ctx.beginPath()
                        ctx.moveTo(i, 0)
                        ctx.lineTo(i + height, height)
                        ctx.stroke()
                        ctx.beginPath()
                        ctx.moveTo(i + height, 0)
                        ctx.lineTo(i, height)
                        ctx.stroke()
                    }

                    // Border
                    ctx.globalAlpha = 0.6
                    ctx.strokeStyle = c
                    ctx.lineWidth = 1.5
                    ctx.strokeRect(0, 0, width, height)
                }

                // Right-click to change transition type
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: function(mouse) {
                        crossfadeContextMenu.cfClipA = cfItem.clipAId
                        crossfadeContextMenu.cfClipB = cfItem.clipBId
                        crossfadeContextMenu.popup()
                    }
                }
            }

            // Label showing transition type
            Rectangle {
                anchors.centerIn: parent
                width: cfLabel.implicitWidth + 8; height: 14; radius: 3
                color: Qt.rgba(0, 0, 0, 0.7)
                visible: parent.width > 40

                Label {
                    id: cfLabel
                    anchors.centerIn: parent
                    font.pixelSize: 8; font.weight: Font.Bold; color: "#E0E0F0"
                    text: {
                        var names = ["XFade", "Dip", "Wipe L", "Wipe R", "Wipe U", "Wipe D"]
                        return names[Math.min(cfItem.cfType, names.length - 1)]
                    }
                }
            }
        }
    }

}
