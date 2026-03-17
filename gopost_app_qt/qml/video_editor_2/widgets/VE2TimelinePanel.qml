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
    readonly property real totalDur: timelineNotifier && timelineNotifier.isReady
                                     ? Math.max(timelineNotifier.totalDuration, 1) : 10

    clip: true

    Component.onCompleted: console.log("[VE2Timeline] created, size:", width, "x", height)

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

                // Click to seek
                MouseArea {
                    anchors.fill: parent
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

            // Throttled repaint — avoid repainting ruler every frame during playback
            Timer {
                id: rulerRepaintTimer
                interval: 100; repeat: false
                onTriggered: rulerCanvas.requestPaint()
            }
            Connections {
                target: timelineNotifier
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
                        required property int index
                        width: headerWidth
                        height: defaultTrackHeight
                        color: "#10102A"
                        border.color: "#1E1E38"
                        border.width: 0.5

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 2

                            RowLayout {
                                spacing: 3
                                Label {
                                    text: {
                                        var icons = ["\u2728", "\uD83C\uDFA5", "\uD83C\uDFB5", "\uD83D\uDCDD"]
                                        return icons[Math.min(index, icons.length - 1)]
                                    }
                                    font.pixelSize: 12
                                }
                                Label {
                                    text: {
                                        var names = ["Effect", "Video", "Audio", "Title"]
                                        return (index < names.length ? names[index] : "Track") + " " + (index + 1)
                                    }
                                    font.pixelSize: 11; font.weight: Font.Medium
                                    color: "#C0C0D8"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            RowLayout {
                                spacing: 1
                                ToolButton {
                                    width: 20; height: 20
                                    contentItem: Label { text: "\uD83D\uDC41"; font.pixelSize: 10; anchors.centerIn: parent; color: "#8888A0" }
                                    onClicked: { if (timelineNotifier) timelineNotifier.toggleTrackVisibility(index) }
                                    ToolTip.text: "Toggle visibility"; ToolTip.visible: hovered
                                }
                                ToolButton {
                                    width: 20; height: 20
                                    contentItem: Label { text: "\uD83D\uDD12"; font.pixelSize: 10; anchors.centerIn: parent; color: "#8888A0" }
                                    onClicked: { if (timelineNotifier) timelineNotifier.toggleTrackLock(index) }
                                    ToolTip.text: "Toggle lock"; ToolTip.visible: hovered
                                }
                                ToolButton {
                                    width: 20; height: 20
                                    contentItem: Label { text: "\uD83D\uDD07"; font.pixelSize: 10; anchors.centerIn: parent; color: "#8888A0" }
                                    onClicked: { if (timelineNotifier) timelineNotifier.toggleTrackMute(index) }
                                    ToolTip.text: "Toggle mute"; ToolTip.visible: hovered
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
                function onStateChanged() {
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

                Repeater {
                    model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.trackCount : 0
                    delegate: Item {
                        id: trackLane
                        required property int index
                        property int trackIdx: index  // stable alias for child access
                        width: lanesColumn.width
                        height: defaultTrackHeight

                        // Lane background
                        Rectangle {
                            anchors.fill: parent
                            color: trackLane.index % 2 === 0 ? "#0C0C20" : "#0E0E24"
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 0.5; color: "#1E1E38" }
                        }

                        // Clips
                        Repeater {
                            model: timelineNotifier && timelineNotifier.trackCount >= 0
                                   ? timelineNotifier.clipCountForTrack(trackLane.trackIdx) : 0
                            delegate: VE2ClipItem {
                                required property int index
                                clipIndex: index
                                trackIndex: trackLane.trackIdx
                                y: 3
                                height: trackLane.height - 6
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
    // Playhead
    // ================================================================
    Item {
        id: playheadItem
        anchors.top: parent.top
        anchors.bottom: footer.top
        x: headerWidth + (timelineNotifier ? timelineNotifier.position * pps : 0) - trackFlick.contentX
        width: 1; z: 100
        visible: x >= headerWidth - 7 && x <= root.width + 7
        clip: false

        Canvas {
            width: 12; height: 8; x: -6; y: 0
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "#6C63FF"
                ctx.beginPath()
                ctx.moveTo(0, 0); ctx.lineTo(12, 0); ctx.lineTo(6, 8)
                ctx.closePath(); ctx.fill()
            }
        }
        Rectangle { width: 1; anchors.top: parent.top; anchors.topMargin: 8; anchors.bottom: parent.bottom; color: "#6C63FF" }

        MouseArea {
            width: 20; height: parent.height; x: -10
            cursorShape: Qt.SizeHorCursor
            property real dragStartX: 0
            property real dragStartPos: 0
            onPressed: mouse => { dragStartX = mouse.x; dragStartPos = timelineNotifier ? timelineNotifier.position : 0 }
            onPositionChanged: mouse => {
                if (pressed && timelineNotifier) {
                    var delta = (mouse.x - dragStartX) / pps
                    timelineNotifier.scrubTo(Math.max(0, dragStartPos + delta))
                }
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

        readonly property var clipInfo: timelineNotifier && timelineNotifier.trackCount >= 0
                                        ? timelineNotifier.clipData(trackIndex, clipIndex) : ({})
        readonly property int clipId: clipInfo.clipId !== undefined ? clipInfo.clipId : -1
        readonly property string displayName: clipInfo.displayName || ""
        readonly property real timelineIn: clipInfo.timelineIn !== undefined ? clipInfo.timelineIn : 0
        readonly property real duration: clipInfo.duration !== undefined ? clipInfo.duration : 1
        readonly property int sourceType: clipInfo.sourceType !== undefined ? clipInfo.sourceType : 0
        readonly property bool isSelected: clipId >= 0
                                           && (timelineNotifier ? timelineNotifier.selectedClipId === clipId : false)
        readonly property real speed: clipInfo.speed !== undefined ? clipInfo.speed : 1.0
        readonly property string sourcePath: clipInfo.sourcePath || ""

        x: timelineIn * pps
        width: Math.max(4, duration * pps)

        readonly property color clipColor: {
            switch (sourceType) {
            case 0: return "#26C6DA"    // video
            case 1: return "#FF7043"    // image
            case 2: return "#AB47BC"    // title
            case 3: return "#66BB6A"    // color
            case 4: return "#42A5F5"    // adjustment
            default: return "#6C63FF"
            }
        }

        // Encoded path for thumbnail provider
        readonly property string pathB64: sourcePath !== "" ? Qt.btoa(sourcePath) : ""

        // ---- Viewport-aware thumbnail calculation ----
        // Only create thumbnails for the visible portion of the clip (virtual scrolling).
        // Quantized to grid cells to avoid recreating items on every scroll pixel.
        readonly property real thumbCellWidth: 80  // target width per thumbnail in pixels
        readonly property real clipW: Math.max(1, clipRoot.width)

        // Raw visible range of this clip in LOCAL coordinates (0 = clip left edge)
        readonly property real _rawVisLeft:  Math.max(0, trackFlick.contentX - clipRoot.x)
        readonly property real _rawVisRight: Math.min(clipW, trackFlick.contentX + trackFlick.width - clipRoot.x)
        readonly property bool _isVisible: _rawVisRight > 0 && _rawVisLeft < clipW

        // Quantize to grid boundaries with 2-cell buffer, clamped to clip bounds
        readonly property real clipVisLeft:  _isVisible ? Math.max(0, Math.floor(_rawVisLeft / thumbCellWidth - 2) * thumbCellWidth) : 0
        readonly property real clipVisRight: _isVisible ? Math.min(clipW, Math.ceil(_rawVisRight / thumbCellWidth + 2) * thumbCellWidth) : 0
        readonly property real clipVisWidth: Math.max(0, clipVisRight - clipVisLeft)

        // Number of thumbnails to fill the quantized visible range
        readonly property int thumbCount: _isVisible && (sourceType === 0 || sourceType === 1) && pathB64 !== "" && clipVisWidth > 0
                                           ? Math.max(1, Math.round(clipVisWidth / thumbCellWidth))
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
            border.color: isSelected ? "#FFFFFF" : clipColor
            border.width: isSelected ? 2 : 1
            clip: true

            // ---- Video thumbnail strip (viewport-aware virtual thumbnails) ----
            Row {
                id: thumbRow
                x: thumbRowX
                width: thumbCount * thumbCellWidth
                height: parent.height
                visible: thumbCount > 0

                Repeater {
                    model: thumbCount
                    delegate: Image {
                        required property int index
                        width: thumbCellWidth
                        height: clipBody.height
                        fillMode: Image.PreserveAspectCrop
                        // Sample time evenly across the visible range
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

                        // Placeholder while loading
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

            // ---- Top label bar (semi-transparent overlay) ----
            Rectangle {
                id: labelBar
                anchors.top: parent.top
                width: parent.width
                height: 18
                color: Qt.rgba(0, 0, 0, 0.55)
                radius: 4
                // Square off bottom corners
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 4; color: Qt.rgba(0, 0, 0, 0.55) }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 5; anchors.rightMargin: 5
                    spacing: 3

                    // Source type icon
                    Label {
                        text: {
                            var icons = ["\uD83C\uDFA5", "\uD83D\uDDBC", "\uD83D\uDCDD", "\uD83C\uDFA8", "\u2728"]
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

                    // Speed badge
                    Label {
                        visible: speed !== 1.0
                        text: speed.toFixed(1) + "x"
                        font.pixelSize: 9; font.weight: Font.Bold
                        color: "#FFCA28"
                    }
                }
            }

            // ---- Selection highlight glow ----
            Rectangle {
                anchors.fill: parent
                radius: 4
                visible: isSelected
                color: "transparent"
                border.color: "#FFFFFF"
                border.width: 2
            }
        }

        // Click to select
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (timelineNotifier && clipId >= 0) {
                    timelineNotifier.selectClip(clipId)
                }
            }
            onDoubleClicked: {
                if (timelineNotifier && clipId >= 0)
                    timelineNotifier.setActivePanel(1)
            }
        }

        // Left trim handle
        Rectangle {
            width: 6; height: parent.height; radius: 3
            color: trimLeftMa.containsMouse || trimLeftMa.pressed ? "#FFFFFF" : clipColor
            opacity: trimLeftMa.containsMouse || trimLeftMa.pressed ? 1.0 : 0.7
            z: 10

            MouseArea {
                id: trimLeftMa
                anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.SizeHorCursor
                property real startX: 0
                property real origIn: 0
                property real origOut: 0
                onPressed: mouse => {
                    startX = mapToItem(clipRoot.parent, mouse.x, 0).x
                    origIn = clipRoot.timelineIn
                    origOut = origIn + clipRoot.duration
                }
                onPositionChanged: mouse => {
                    if (pressed && timelineNotifier && clipId >= 0) {
                        var curX = mapToItem(clipRoot.parent, mouse.x, 0).x
                        var delta = (curX - startX) / pps
                        var newIn = Math.max(0, origIn + delta)
                        if (newIn < origOut - 0.1)
                            timelineNotifier.trimClip(clipId, newIn, origOut)
                    }
                }
            }
        }

        // Right trim handle
        Rectangle {
            anchors.right: parent.right
            width: 6; height: parent.height; radius: 3
            color: trimRightMa.containsMouse || trimRightMa.pressed ? "#FFFFFF" : clipColor
            opacity: trimRightMa.containsMouse || trimRightMa.pressed ? 1.0 : 0.7
            z: 10

            MouseArea {
                id: trimRightMa
                anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.SizeHorCursor
                property real startX: 0
                property real origIn: 0
                property real origOut: 0
                onPressed: mouse => {
                    startX = mapToItem(clipRoot.parent, mouse.x, 0).x
                    origIn = clipRoot.timelineIn
                    origOut = origIn + clipRoot.duration
                }
                onPositionChanged: mouse => {
                    if (pressed && timelineNotifier && clipId >= 0) {
                        var curX = mapToItem(clipRoot.parent, mouse.x, 0).x
                        var delta = (curX - startX) / pps
                        var newOut = Math.max(origIn + 0.1, origOut + delta)
                        timelineNotifier.trimClip(clipId, origIn, newOut)
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

        function bestInterval(pxPerSec) {
            if (pxPerSec <= 0) return 1
            var intervals = [0.1, 0.25, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600]
            for (var i = 0; i < intervals.length; i++) {
                if (intervals[i] * pxPerSec >= 60) return intervals[i]
            }
            return 3600
        }

        function formatTime(s) {
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
    }
}
