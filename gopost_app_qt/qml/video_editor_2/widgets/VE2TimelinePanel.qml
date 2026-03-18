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
    Row {
        id: rulerRow
        anchors.top: parent.top
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
        anchors.bottom: footer.top

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
                        readonly property string trackLabel: tInfo.label || ("Track " + (index + 1))
                        readonly property string trackColor: tInfo.color || ""

                        height: trackVisible ? defaultTrackHeight : 12
                        color: trackLocked ? "#0E0A1A" : "#10102A"
                        border.color: "#1E1E38"
                        border.width: 0.5

                        Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }

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
                                    property bool isSoloed: tInfo.solo !== undefined ? tInfo.solo : false
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
                                    ToolTip.text: soloBtn.isSoloed ? "Unsolo track" : "Solo track"
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
                        height: laneVisible ? defaultTrackHeight : 12

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

                        // Per-track drop zone
                        DropArea {
                            anchors.fill: parent
                            keys: ["application/x-gopost-media"]

                            Rectangle {
                                anchors.fill: parent
                                color: parent.containsDrag ? Qt.rgba(0.424, 0.388, 1.0, 0.08) : "transparent"
                                border.color: parent.containsDrag ? "#6C63FF" : "transparent"
                                border.width: parent.containsDrag ? 1.5 : 0
                                radius: 2
                            }

                            onDropped: drop => {
                                if (!timelineNotifier || !timelineNotifier.isReady) {
                                    console.warn("[VE2Timeline] track-drop rejected: timeline not ready")
                                    return
                                }
                                if (!drop.hasText) {
                                    console.warn("[VE2Timeline] track-drop: no text data")
                                    return
                                }

                                var data
                                try { data = JSON.parse(drop.text) } catch (e) {
                                    console.warn("[VE2Timeline] track-drop: invalid JSON:", e)
                                    return
                                }
                                if (data.type !== "media") return

                                var sourceType = 0  // video
                                if (data.mediaType === "image") sourceType = 1
                                else if (data.mediaType === "audio") sourceType = 5

                                var sourcePath = data.sourcePath || ""
                                var displayName = data.displayName || "Untitled"
                                var duration = (data.duration && data.duration > 0) ? data.duration : 5.0

                                if (sourcePath === "") {
                                    console.warn("[VE2Timeline] track-drop: empty sourcePath for", displayName)
                                    return
                                }

                                console.log("[VE2Timeline] track-drop: track=", trackLane.trackIdx,
                                            "type=", data.mediaType, "name=", displayName, "dur=", duration)
                                var clipId = timelineNotifier.addClip(trackLane.trackIdx, sourceType, sourcePath, displayName, duration)
                                if (clipId >= 0) {
                                    timelineNotifier.selectClip(clipId)
                                    console.log("[VE2Timeline] clip added: id=", clipId)
                                } else {
                                    console.warn("[VE2Timeline] track-drop: addClip failed for", displayName)
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
        }
    }

    // ================================================================
    // In/Out point markers
    // ================================================================
    Rectangle {
        visible: timelineNotifier ? timelineNotifier.hasInPoint : false
        anchors.top: rulerRow.top
        anchors.bottom: footer.top
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
        anchors.bottom: footer.top
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
        anchors.bottom: footer.top
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
            anchors.bottom: footer.top
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
        anchors.bottom: footer.top
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
    // Insertion indicator line (blue, for drag-to-reorder)
    // ================================================================
    Rectangle {
        id: insertIndicatorLine
        visible: root._insertIndicatorVisible && root._insertIndicatorX >= 0
        anchors.top: rulerRow.bottom
        anchors.bottom: footer.top
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
        anchors.bottom: footer.top
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
    // Row 2: Footer with zoom controls + scrollbar
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

            // Zoom controls — logarithmic slider for extreme range (0.01–400 pps)
            ToolButton {
                width: 24; height: 24
                contentItem: Label { text: "\u2796"; font.pixelSize: 12; color: "#8888A0"; anchors.centerIn: parent }
                onClicked: {
                    if (timelineNotifier) {
                        var anchorX = trackFlick.width / 2
                        timelineNotifier.zoomAtPosition(0.5, anchorX + trackFlick.contentX)
                    }
                }
                ToolTip.text: "Zoom out (0.5x)"; ToolTip.visible: hovered
            }

            Slider {
                id: zoomSlider
                Layout.preferredWidth: 100
                from: 0; to: 1   // normalized [0..1] → log scale

                // Log scale constants
                readonly property real logMin: Math.log(0.01)    // ln(0.01)
                readonly property real logMax: Math.log(400)     // ln(400)
                readonly property real logRange: logMax - logMin

                // Convert pps → slider [0..1]
                function ppsToSlider(p) {
                    return (Math.log(Math.max(0.01, p)) - logMin) / logRange
                }
                // Convert slider [0..1] → pps
                function sliderToPps(s) {
                    return Math.exp(logMin + s * logRange)
                }

                onMoved: {
                    if (timelineNotifier) {
                        var newPps = sliderToPps(value)
                        var anchorX = trackFlick.width / 2
                        timelineNotifier.zoomAtPosition(newPps / pps, anchorX + trackFlick.contentX)
                    }
                }

                // Sync slider position from pps (log scale)
                Binding on value { value: zoomSlider.ppsToSlider(pps); when: !zoomSlider.pressed }

                background: Rectangle {
                    x: zoomSlider.leftPadding; y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - 1
                    width: zoomSlider.availableWidth; height: 2; radius: 1; color: "#1A1A34"
                    Rectangle { width: zoomSlider.visualPosition * parent.width; height: parent.height; radius: 1; color: "#6C63FF" }
                }
            }

            ToolButton {
                width: 24; height: 24
                contentItem: Label { text: "\u2795"; font.pixelSize: 12; color: "#8888A0"; anchors.centerIn: parent }
                onClicked: {
                    if (timelineNotifier) {
                        var anchorX = trackFlick.width / 2
                        timelineNotifier.zoomAtPosition(2.0, anchorX + trackFlick.contentX)
                    }
                }
                ToolTip.text: "Zoom in (2x)"; ToolTip.visible: hovered
            }

            // Feature 11: Zoom percentage label
            Label {
                text: timelineNotifier ? Math.round(timelineNotifier.zoomPercentage) + "%" : "100%"
                font.pixelSize: 10; font.family: "monospace"
                color: "#7070A0"
                Layout.preferredWidth: 36
                horizontalAlignment: Text.AlignHCenter
            }

            // Zoom to fit all clips
            ToolButton {
                width: 24; height: 24
                contentItem: Label { text: "\u2922"; font.pixelSize: 14; color: "#8888A0"; anchors.centerIn: parent }
                onClicked: {
                    if (!timelineNotifier) return
                    var totalDur = timelineNotifier.totalDuration
                    if (totalDur <= 0) return
                    var visibleWidth = trackFlick.width > 100 ? trackFlick.width * 0.95 : 600
                    var fitPps = visibleWidth / totalDur
                    fitPps = Math.max(0.01, Math.min(400, fitPps))
                    timelineNotifier.setPixelsPerSecond(fitPps)
                    timelineNotifier.setScrollOffset(0)
                }
                ToolTip.text: "Zoom to fit all clips"; ToolTip.visible: hovered
            }

            Rectangle { width: 1; height: 16; color: "#252540" }

            // Snap toggle (magnet icon)
            ToolButton {
                width: 26; height: 24
                contentItem: Label {
                    text: "\uD83E\uDDF2"  // magnet emoji
                    font.pixelSize: 13
                    color: root.snapEnabled ? "#FFCA28" : "#505068"
                    anchors.centerIn: parent
                }
                onClicked: root.snapEnabled = !root.snapEnabled
                ToolTip.text: root.snapEnabled ? "Snapping ON (click to disable)" : "Snapping OFF (click to enable)"
                ToolTip.visible: hovered
                background: Rectangle {
                    radius: 4
                    color: root.snapEnabled ? Qt.rgba(1, 0.79, 0.16, 0.1) : "transparent"
                    border.color: root.snapEnabled ? "#FFCA28" : "transparent"
                    border.width: root.snapEnabled ? 1 : 0
                }
            }

            // Color legend toggle
            ToolButton {
                width: 26; height: 24
                contentItem: Label {
                    text: "\uD83C\uDFA8"  // palette emoji
                    font.pixelSize: 13
                    color: colorLegendPanel.visible ? "#6C63FF" : "#505068"
                    anchors.centerIn: parent
                }
                onClicked: colorLegendPanel.visible = !colorLegendPanel.visible
                ToolTip.text: "Color legend"
                ToolTip.visible: hovered
                background: Rectangle {
                    radius: 4
                    color: colorLegendPanel.visible ? Qt.rgba(0.42, 0.39, 1.0, 0.1) : "transparent"
                    border.color: colorLegendPanel.visible ? "#6C63FF" : "transparent"
                    border.width: colorLegendPanel.visible ? 1 : 0
                }
            }

            Rectangle { width: 1; height: 16; color: "#252540" }

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
        anchors.top: rulerRow.bottom
        anchors.left: parent.left; anchors.leftMargin: headerWidth
        anchors.right: parent.right
        anchors.bottom: footer.top
        z: -1
        keys: ["application/x-gopost-media"]

        onDropped: drop => {
            if (!timelineNotifier || !timelineNotifier.isReady) {
                console.warn("[VE2Timeline] fallback-drop rejected: timeline not ready")
                return
            }
            if (!drop.hasText) {
                console.warn("[VE2Timeline] fallback-drop: no text data")
                return
            }

            var data
            try { data = JSON.parse(drop.text) } catch (e) {
                console.warn("[VE2Timeline] fallback-drop: invalid JSON:", e)
                return
            }
            if (data.type !== "media") return

            // Map media type to source type and target track
            var sourceType = 0  // video
            var trackIndex = 1  // video track
            if (data.mediaType === "image") {
                sourceType = 1
            } else if (data.mediaType === "audio") {
                sourceType = 5
                trackIndex = 2  // audio track
            }

            var sourcePath = data.sourcePath || ""
            var displayName = data.displayName || "Untitled"
            var duration = (data.duration && data.duration > 0) ? data.duration : 5.0

            if (sourcePath === "") {
                console.warn("[VE2Timeline] fallback-drop: empty sourcePath for", displayName)
                return
            }

            console.log("[VE2Timeline] fallback-drop: track=", trackIndex,
                        "type=", data.mediaType, "name=", displayName, "dur=", duration)
            var clipId = timelineNotifier.addClip(trackIndex, sourceType, sourcePath, displayName, duration)
            if (clipId >= 0) {
                timelineNotifier.selectClip(clipId)
                console.log("[VE2Timeline] clip added: id=", clipId)
            } else {
                console.warn("[VE2Timeline] fallback-drop: addClip failed for", displayName)
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

        x: timelineIn * pps
        width: Math.max(4, duration * pps)

        // Smooth animation for ripple shifts (200ms ease-out)
        Behavior on x {
            enabled: !clipBodyMa.dragActive && !trimLeftMa.pressed && !trimRightMa.pressed
            NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
        }
        Behavior on width {
            enabled: !clipBodyMa.dragActive && !trimLeftMa.pressed && !trimRightMa.pressed
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
        readonly property int thumbCount: _isVisible && (sourceType === 0 || sourceType === 1) && pathB64 !== "" && clipVisWidth > 0
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

            // ---- Video thumbnail strip (top section) ----
            Row {
                id: thumbRow
                x: thumbRowX
                width: thumbCount * thumbCellWidth
                height: clipBody.thumbAreaHeight
                visible: thumbCount > 0

                Repeater {
                    model: thumbCount
                    delegate: Image {
                        required property int index
                        width: thumbCellWidth
                        height: clipBody.thumbAreaHeight
                        fillMode: Image.PreserveAspectCrop
                        property real timeSec: duration > 0
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

                        Rectangle {
                            anchors.fill: parent
                            visible: parent.status !== Image.Ready
                            color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.15)
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

                    // Speed badge
                    Label {
                        visible: speed !== 1.0 || hasSpeedRamp
                        text: hasSpeedRamp ? "RAMP" : (speed.toFixed(1) + "x")
                        font.pixelSize: 9; font.weight: Font.Bold
                        color: "#FFCA28"
                    }
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
            cursorShape: dragActive ? Qt.ClosedHandCursor : Qt.PointingHandCursor
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true

            property bool dragActive: false
            property real dragStartX: 0
            property real dragStartMouseX: 0
            property real origTimelineIn: 0
            property int origTrackIndex: -1
            property bool hasCollision: false
            property real snapEdgeX: -1  // x position of active snap line (-1 = hidden)

            onPressed: function(mouse) {
                if (mouse.button === Qt.RightButton) return
                dragActive = false
                hasCollision = false
                snapEdgeX = -1
                dragStartX = clipRoot.timelineIn * pps
                dragStartMouseX = mapToItem(clipRoot.parent, mouse.x, 0).x
                origTimelineIn = clipRoot.timelineIn
                origTrackIndex = clipRoot.trackIndex
                root._anyClipDragging = false

                // Feature 13: Ctrl+Click toggles multi-select
                if (timelineNotifier && clipId >= 0) {
                    if (mouse.modifiers & Qt.ControlModifier) {
                        timelineNotifier.toggleClipSelection(clipId)
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

            onPositionChanged: function(mouse) {
                if (!pressed) return
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

                if (!_isGroupDrag && !showAbove && !showBelow) {
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
            }
            onReleased: function(mouse) {
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

                        // If insertion indicator is showing, use insert/overwrite mode
                        if (root._insertIndicatorVisible && root._insertIndicatorTime >= 0) {
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
                if (timelineNotifier && clipId >= 0)
                    timelineNotifier.setActivePanel(1)
            }
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
                    minBound = 0
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

                    timelineNotifier.trimClip(clipId, newIn, origOut)
                }
                onReleased: internal.showSnapGuide(null)
            }

            // Trim tooltip — in-point time
            Rectangle {
                visible: trimLeftMa.pressed
                x: 10; y: -2
                width: trimLeftLabel.implicitWidth + 10; height: 18; radius: 3
                color: Qt.rgba(0, 0, 0, 0.85); border.color: clipColor; border.width: 1; z: 200
                Label {
                    id: trimLeftLabel; anchors.centerIn: parent
                    font.pixelSize: 9; font.family: "monospace"; font.weight: Font.Bold; color: "#FFF"
                    text: "IN " + internal.formatTimecodeFF(clipRoot.timelineIn)
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
                    maxBound = 99999
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

                    timelineNotifier.trimClip(clipId, origIn, newOut)
                }
                onReleased: internal.showSnapGuide(null)
            }

            // Trim tooltip — out-point time
            Rectangle {
                visible: trimRightMa.pressed
                anchors.right: parent.left; anchors.rightMargin: 2; y: -2
                width: trimRightLabel.implicitWidth + 10; height: 18; radius: 3
                color: Qt.rgba(0, 0, 0, 0.85); border.color: clipColor; border.width: 1; z: 200
                Label {
                    id: trimRightLabel; anchors.centerIn: parent
                    font.pixelSize: 9; font.family: "monospace"; font.weight: Font.Bold; color: "#FFF"
                    text: "OUT " + internal.formatTimecodeFF(clipRoot.timelineIn + clipRoot.duration)
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

        // Find the best insertion point on a track near the given time.
        // Returns { time } at the nearest clip boundary, or null.
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
                var trackH = (tInfo.isVisible !== false) ? defaultTrackHeight : 12
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
            text: "Freeze Frame"
            onTriggered: { console.log("[Speed] Freeze clip:", clipContextMenu.targetClipId); if (timelineNotifier) timelineNotifier.freezeFrame(clipContextMenu.targetClipId) }
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
        anchors.bottom: footer.top
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
        anchors.bottom: footer.top
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
