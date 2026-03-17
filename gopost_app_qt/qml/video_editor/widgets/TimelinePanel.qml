import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * TimelinePanel — ruler + scrollable track area + playhead + drag-drop.
 *
 * Layout (no overlaps):
 *   ┌──────────────────────────────────────────────┐
 *   │ [header spacer 120px] │ [TimeRuler ─────────]│ ← row 0
 *   ├──────────────────────────────────────────────┤
 *   │ [Track headers fixed] │ [Clip lanes scroll ─]│ ← row 1 (Flickable)
 *   ├──────────────────────────────────────────────┤
 *   │ [scrollbar ─────────────────────────────────]│ ← row 2
 *   └──────────────────────────────────────────────┘
 *
 * Track headers stay fixed on the left; clip lanes + ruler scroll together.
 * Playhead is drawn across both ruler and clip lanes.
 */
Item {
    id: root

    readonly property real rulerHeight: 34
    readonly property real scrollbarHeight: 20
    readonly property real headerWidth: 120
    readonly property real defaultTrackHeight: 68

    readonly property real pps: timelineNotifier ? timelineNotifier.pixelsPerSecond : 100
    readonly property real totalDur: timelineNotifier && timelineNotifier.isReady
                                     ? Math.max(timelineNotifier.totalDuration, 1) : 10

    clip: true

    Component.onCompleted: console.log("[TimelinePanel] created, size:", width, "x", height)

    // ================================================================
    // Row 0: Ruler bar (header spacer + scrolling ruler)
    // ================================================================
    Row {
        id: rulerRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: rulerHeight

        // Header spacer — aligns with track headers below
        Rectangle {
            width: headerWidth
            height: parent.height
            color: "#14142B"

            Rectangle {
                anchors.right: parent.right
                width: 1; height: parent.height
                color: "#252540"
            }
        }

        // Ruler viewport (clips to visible area, offsets by scrollX)
        Item {
            width: parent.width - headerWidth
            height: parent.height
            clip: true

            TimeRuler {
                id: ruler
                width: Math.max(parent.width, totalDur * pps)
                height: parent.height
                x: -trackFlick.contentX
                duration: totalDur
                pixelsPerSecond: pps
                viewportWidth: parent.width
                scrollOffset: trackFlick.contentX
            }
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
        anchors.bottom: scrollBar.top

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
                id: headerColumn
                width: headerWidth

                Repeater {
                    model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.trackCount : 0
                    delegate: Rectangle {
                        required property int index
                        width: headerWidth
                        height: editorLayoutNotifier ? editorLayoutNotifier.trackHeight(index) : defaultTrackHeight
                        color: "#14142B"
                        border.color: "#252540"
                        border.width: 0.5

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 2

                            RowLayout {
                                spacing: 4

                                Label {
                                    text: {
                                        var icons = ["\u2728", "\uD83C\uDFA5", "\uD83C\uDFB5", "\uD83D\uDCDD"]
                                        return icons[Math.min(index, icons.length - 1)]
                                    }
                                    font.pixelSize: 14
                                }

                                Label {
                                    text: {
                                        var names = ["Effect", "Video", "Audio", "Title"]
                                        return (index < names.length ? names[index] : "Track") + " " + (index + 1)
                                    }
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    color: "#D0D0E8"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            RowLayout {
                                spacing: 2

                                ToolButton {
                                    width: 22; height: 22
                                    icon.name: "visibility"
                                    icon.width: 14; icon.height: 14
                                    icon.color: "#8888A0"
                                    ToolTip.text: "Toggle visibility"
                                    onClicked: {
                                        console.log("[Timeline] toggleTrackVisibility:", index)
                                        timelineNotifier.toggleTrackVisibility(index)
                                    }
                                }
                                ToolButton {
                                    width: 22; height: 22
                                    icon.name: "lock"
                                    icon.width: 14; icon.height: 14
                                    icon.color: "#8888A0"
                                    ToolTip.text: "Toggle lock"
                                    onClicked: {
                                        console.log("[Timeline] toggleTrackLock:", index)
                                        timelineNotifier.toggleTrackLock(index)
                                    }
                                }
                                ToolButton {
                                    width: 22; height: 22
                                    icon.name: "audio-volume-muted"
                                    icon.width: 14; icon.height: 14
                                    icon.color: "#8888A0"
                                    ToolTip.text: "Toggle mute"
                                    onClicked: {
                                        console.log("[Timeline] toggleTrackMute:", index)
                                        timelineNotifier.toggleTrackMute(index)
                                    }
                                }
                            }
                        }

                        // Right border
                        Rectangle {
                            anchors.right: parent.right
                            width: 1; height: parent.height
                            color: "#252540"
                        }
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
            interactive: true
            flickableDirection: Flickable.AutoFlickIfNeeded

            onContentXChanged: {
                if (timelineNotifier) timelineNotifier.setScrollOffset(contentX)
            }

            Column {
                id: lanesColumn
                width: trackFlick.contentWidth

                Repeater {
                    model: timelineNotifier && timelineNotifier.isReady ? timelineNotifier.trackCount : 0
                    delegate: Item {
                        required property int index
                        width: lanesColumn.width
                        height: editorLayoutNotifier ? editorLayoutNotifier.trackHeight(index) : defaultTrackHeight

                        // Track lane background
                        Rectangle {
                            anchors.fill: parent
                            color: index % 2 === 0 ? "#0F0F22" : "#121230"

                            // Bottom divider
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width; height: 0.5
                                color: "#252540"
                            }
                        }

                        // Clips
                        Repeater {
                            id: clipRepeater
                            model: timelineNotifier && timelineNotifier.trackCount >= 0
                                   ? timelineNotifier.clipCountForTrack(index) : 0
                            onModelChanged: console.log("[Timeline] track", index,
                                                        "clipCount changed to:", model)
                            delegate: ClipWidget {
                                required property int index  // clip index within track
                                clipIndex: index
                                trackIndex: parent.parent.index  // track index
                                y: 2
                                height: parent.parent.height - 4
                            }
                        }

                        // Drop zone — accepts media drops directly on this track
                        DropArea {
                            anchors.fill: parent
                            keys: ["application/x-gopost-media"]

                            Rectangle {
                                anchors.fill: parent
                                color: parent.containsDrag ? Qt.rgba(0.424, 0.388, 1.0, 0.08) : "transparent"
                                border.color: parent.containsDrag ? "#6C63FF" : "transparent"
                                border.width: parent.containsDrag ? 1.5 : 0
                                radius: 3
                            }

                            onDropped: drop => {
                                if (!drop.hasText) return
                                var data
                                try { data = JSON.parse(drop.text) } catch (e) { return }
                                if (data.type !== "media") return

                                var sourceType = 0
                                if (data.mediaType === "image") sourceType = 1

                                var sourcePath = data.sourcePath || ""
                                var displayName = data.displayName || "Untitled"
                                var duration = data.duration || 5.0

                                if (sourcePath === "") return

                                console.log("[Timeline] track-drop: track=", index, "sourceType=", sourceType,
                                            "name=", displayName, "dur=", duration)
                                timelineNotifier.addClip(index, sourceType, sourcePath, displayName, duration)
                                drop.accept()
                            }
                        }
                    }
                }
            }

            // Pinch zoom
            PinchArea {
                anchors.fill: parent
                onPinchUpdated: pinch => {
                    var factor = pinch.scale / pinch.previousScale
                    if (timelineNotifier) {
                        console.log("[Timeline] pinch zoom factor:", factor.toFixed(2))
                        timelineNotifier.zoomAtPosition(factor, pinch.center.x + trackFlick.contentX)
                    }
                }
            }
        }
    }

    // ================================================================
    // In/Out point markers — shaded region + markers
    // ================================================================

    // In point marker
    Rectangle {
        id: inPointMarker
        visible: timelineNotifier ? timelineNotifier.hasInPoint : false
        anchors.top: rulerRow.top
        anchors.bottom: scrollBar.top
        x: headerWidth + (timelineNotifier ? timelineNotifier.inPoint * pps : 0) - trackFlick.contentX
        width: 2
        z: 90
        color: "#66BB6A"
        opacity: 0.8

        // "I" label at top
        Rectangle {
            width: 14; height: 14
            x: -7; y: 0
            radius: 2
            color: "#66BB6A"
            Label {
                anchors.centerIn: parent
                text: "I"
                font.pixelSize: 9; font.weight: Font.Bold
                color: "white"
            }
        }
    }

    // Out point marker
    Rectangle {
        id: outPointMarker
        visible: timelineNotifier ? timelineNotifier.hasOutPoint : false
        anchors.top: rulerRow.top
        anchors.bottom: scrollBar.top
        x: headerWidth + (timelineNotifier ? timelineNotifier.outPoint * pps : 0) - trackFlick.contentX
        width: 2
        z: 90
        color: "#EF5350"
        opacity: 0.8

        // "O" label at top
        Rectangle {
            width: 14; height: 14
            x: -7; y: 0
            radius: 2
            color: "#EF5350"
            Label {
                anchors.centerIn: parent
                text: "O"
                font.pixelSize: 9; font.weight: Font.Bold
                color: "white"
            }
        }
    }

    // Shaded region between In and Out points
    Rectangle {
        visible: timelineNotifier ? (timelineNotifier.hasInPoint && timelineNotifier.hasOutPoint) : false
        anchors.top: rulerRow.bottom
        anchors.bottom: scrollBar.top
        x: headerWidth + (timelineNotifier ? timelineNotifier.inPoint * pps : 0) - trackFlick.contentX
        width: timelineNotifier ? (timelineNotifier.outPoint - timelineNotifier.inPoint) * pps : 0
        z: 80
        color: Qt.rgba(0.424, 0.388, 1.0, 0.06)
        border.color: Qt.rgba(0.424, 0.388, 1.0, 0.15)
        border.width: 1
    }

    // ================================================================
    // Playhead — drawn across ruler + clip lanes, above everything
    // ================================================================
    Item {
        id: playheadContainer
        anchors.top: parent.top
        anchors.bottom: scrollBar.top
        x: headerWidth + (timelineNotifier ? timelineNotifier.position * pps : 0) - trackFlick.contentX
        width: 1
        z: 100
        visible: x >= headerWidth - 7 && x <= root.width + 7
        clip: false

        // Triangle head (in ruler area)
        Canvas {
            id: playheadHead
            width: 14; height: 10
            x: -7; y: 0

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "#6C63FF"
                ctx.beginPath()
                ctx.moveTo(0, 0)
                ctx.lineTo(14, 0)
                ctx.lineTo(7, 10)
                ctx.closePath()
                ctx.fill()
            }
        }

        // Vertical line
        Rectangle {
            width: 1
            anchors.top: playheadHead.bottom
            anchors.bottom: parent.bottom
            color: "#6C63FF"
        }

        // Drag to scrub
        MouseArea {
            width: 20; height: parent.height
            x: -10
            cursorShape: Qt.SizeHorCursor

            property real dragStartMouseX: 0
            property real dragStartPos: 0

            onPressed: mouse => {
                dragStartMouseX = mouse.x
                dragStartPos = timelineNotifier ? timelineNotifier.position : 0
                console.log("[Timeline] playhead drag started at pos:", dragStartPos.toFixed(2))
            }
            onPositionChanged: mouse => {
                if (pressed && timelineNotifier) {
                    var deltaPx = mouse.x - dragStartMouseX
                    var deltaTime = deltaPx / pps
                    var newPos = Math.max(0, dragStartPos + deltaTime)
                    timelineNotifier.scrubTo(newPos)
                }
            }
            onReleased: {
                console.log("[Timeline] playhead drag ended at pos:",
                            timelineNotifier ? timelineNotifier.position.toFixed(2) : "n/a")
            }
        }
    }

    // ================================================================
    // Row 2: Horizontal scrollbar
    // ================================================================
    ScrollBar {
        id: scrollBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: headerWidth
        anchors.right: parent.right
        height: scrollbarHeight
        orientation: Qt.Horizontal
        policy: ScrollBar.AlwaysOn
        size: trackFlick.width / Math.max(1, trackFlick.contentWidth)

        onPositionChanged: {
            if (active) {
                trackFlick.contentX = position * trackFlick.contentWidth
            }
        }

        Binding {
            target: scrollBar
            property: "position"
            value: trackFlick.contentX / Math.max(1, trackFlick.contentWidth)
            when: !scrollBar.active
        }
    }

    // ================================================================
    // Drop target (full panel — fallback for drops not on a track)
    // ================================================================
    DropArea {
        anchors.top: rulerRow.bottom
        anchors.left: parent.left
        anchors.leftMargin: headerWidth
        anchors.right: parent.right
        anchors.bottom: scrollBar.top
        z: -1
        keys: ["application/x-gopost-media", "application/x-gopost-effect",
               "application/x-gopost-transition", "application/x-gopost-adjustment"]

        onDropped: drop => {
            console.log("[Timeline] drop received at x:", drop.x, "hasText:", drop.hasText)
            if (!drop.hasText) {
                console.warn("[Timeline] drop has no text data — ignoring")
                return
            }

            var data
            try {
                data = JSON.parse(drop.text)
            } catch (e) {
                console.warn("[Timeline] failed to parse drop data:", e, "raw:", drop.text)
                return
            }

            console.log("[Timeline] parsed drop data:", JSON.stringify(data))

            if (data.type === "media") {
                var sourceType = 0
                if (data.mediaType === "image") sourceType = 1
                else if (data.mediaType === "audio") sourceType = 0

                var trackIndex = 1
                if (data.mediaType === "audio") trackIndex = 2

                var sourcePath = data.sourcePath || ""
                var displayName = data.displayName || "Untitled"
                var duration = data.duration || 5.0

                if (sourcePath === "") {
                    console.warn("[Timeline] drop: sourcePath is empty — cannot add clip")
                    return
                }

                console.log("[Timeline] addClip: track=", trackIndex, "sourceType=", sourceType,
                            "path=", sourcePath, "name=", displayName, "dur=", duration)
                timelineNotifier.addClip(trackIndex, sourceType, sourcePath, displayName, duration)
            }
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

        Label {
            text: "\uD83C\uDFAC"
            font.pixelSize: 38
            color: "#404060"
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: "Drop media here to start editing"
            font.pixelSize: 14
            color: "#6B6B88"
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
