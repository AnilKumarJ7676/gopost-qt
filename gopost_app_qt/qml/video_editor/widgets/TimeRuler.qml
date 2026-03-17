import QtQuick

/**
 * TimeRuler — canvas-painted ruler with adaptive intervals and in/out markers.
 *
 * Supports scrollOffset for efficient rendering: only draws ticks in the
 * visible viewport range [scrollOffset, scrollOffset + viewportWidth].
 */
Item {
    id: root

    required property real duration
    required property real pixelsPerSecond
    required property real viewportWidth
    property real scrollOffset: 0
    property real inPoint: -1
    property real outPoint: -1

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            // Background
            ctx.fillStyle = "#14142B"
            ctx.fillRect(0, 0, width, height)

            // Bottom border
            ctx.fillStyle = "#252540"
            ctx.fillRect(0, height - 1, width, 1)

            var interval = bestInterval(pixelsPerSecond)
            if (interval <= 0) return

            // Only draw ticks in the visible range (with margin)
            var visStart = Math.max(0, scrollOffset - interval * pixelsPerSecond)
            var visEnd = scrollOffset + viewportWidth + interval * pixelsPerSecond
            var firstTick = Math.floor(visStart / (interval * pixelsPerSecond))
            var lastTick = Math.ceil(visEnd / (interval * pixelsPerSecond))

            // In/Out range highlight
            if (inPoint >= 0 || outPoint >= 0) {
                var rangeStart = (inPoint >= 0 ? inPoint : 0) * pixelsPerSecond
                var rangeEnd = (outPoint >= 0 ? outPoint : duration) * pixelsPerSecond
                if (rangeEnd > rangeStart) {
                    ctx.fillStyle = Qt.rgba(0.424, 0.388, 1.0, 0.31)
                    ctx.fillRect(rangeStart, height - 3, rangeEnd - rangeStart, 3)
                }
            }

            // Ticks and labels
            ctx.font = "11px monospace"
            for (var i = firstTick; i <= lastTick; i++) {
                var t = i * interval
                if (t < 0 || t > duration) continue
                var x = t * pixelsPerSecond

                // Major tick
                ctx.strokeStyle = "#404060"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(x, height - 12)
                ctx.lineTo(x, height - 1)
                ctx.stroke()

                // Label
                ctx.fillStyle = "#8888A0"
                ctx.fillText(formatTime(t), x + 4, 13)

                // Minor ticks (quarter intervals)
                ctx.strokeStyle = "#303050"
                var sub = interval / 4
                for (var j = 1; j < 4; j++) {
                    var st = t + j * sub
                    if (st > duration) break
                    var sx = st * pixelsPerSecond
                    ctx.beginPath()
                    ctx.moveTo(sx, height - 6)
                    ctx.lineTo(sx, height - 1)
                    ctx.stroke()
                }
            }

            // In-point marker
            if (inPoint >= 0) {
                var ix = inPoint * pixelsPerSecond
                ctx.strokeStyle = "#66BB6A"
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.moveTo(ix, 0)
                ctx.lineTo(ix, height)
                ctx.stroke()

                // In label
                ctx.fillStyle = "#66BB6A"
                ctx.font = "9px sans-serif"
                ctx.fillText("IN", ix + 3, height - 5)
                ctx.font = "11px monospace"
            }

            // Out-point marker
            if (outPoint >= 0) {
                var ox = outPoint * pixelsPerSecond
                ctx.strokeStyle = "#EF5350"
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.moveTo(ox, 0)
                ctx.lineTo(ox, height)
                ctx.stroke()

                // Out label
                ctx.fillStyle = "#EF5350"
                ctx.font = "9px sans-serif"
                ctx.fillText("OUT", ox - 22, height - 5)
                ctx.font = "11px monospace"
            }
        }
    }

    // Repaint triggers
    onDurationChanged: canvas.requestPaint()
    onPixelsPerSecondChanged: canvas.requestPaint()
    onScrollOffsetChanged: canvas.requestPaint()
    onInPointChanged: canvas.requestPaint()
    onOutPointChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    // Click on ruler to seek
    MouseArea {
        anchors.fill: parent
        onClicked: mouse => {
            if (timelineNotifier) {
                var time = mouse.x / pixelsPerSecond
                time = Math.max(0, Math.min(time, duration))
                console.log("[TimeRuler] seek to:", time.toFixed(2))
                timelineNotifier.scrubTo(time)
            }
        }
        onPositionChanged: mouse => {
            if (pressed && timelineNotifier) {
                var time = mouse.x / pixelsPerSecond
                time = Math.max(0, Math.min(time, duration))
                timelineNotifier.scrubTo(time)
            }
        }
        cursorShape: Qt.PointingHandCursor
    }

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
