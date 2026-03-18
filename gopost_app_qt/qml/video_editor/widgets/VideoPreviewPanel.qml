import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

/**
 * VideoPreviewPanel — real video playback using Qt6 MediaPlayer + VideoOutput.
 *
 * Syncs with the timeline: loads the source file of the clip at the current
 * playhead position, seeks to the correct offset within that file.
 * Transport controls: play/pause, step, shuttle, jump, seek slider, timecode.
 */
Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "#0D0D1A"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ============================================================
        // Video viewport
        // ============================================================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Qt6 MediaPlayer for actual video decoding
            MediaPlayer {
                id: mediaPlayer
                videoOutput: videoOutput
                audioOutput: AudioOutput { id: audioOut; volume: 1.0 }

                onErrorOccurred: (error, errorString) => {
                    console.warn("[VideoPlayer] MediaPlayer error:", error, errorString)
                }
                onMediaStatusChanged: {
                    console.log("[VideoPlayer] mediaStatus:", mediaPlayer.mediaStatus,
                                "duration:", mediaPlayer.duration, "ms")
                }
            }

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                anchors.margins: 2
                visible: mediaPlayer.hasVideo
            }

            // Fallback: stub engine frame (when no real video loaded)
            Image {
                id: fallbackImage
                anchors.fill: parent
                anchors.margins: 2
                fillMode: Image.PreserveAspectFit
                cache: false
                source: timelineNotifier && timelineNotifier.isReady
                        ? ("image://videopreview/" + timelineNotifier.frameVersion)
                        : ""
                visible: !mediaPlayer.hasVideo && (timelineNotifier ? timelineNotifier.isReady : false)
            }

            // No-clip placeholder
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                visible: !mediaPlayer.hasVideo && (timelineNotifier ? !timelineNotifier.isReady : true)

                Label {
                    text: "\u25B6"
                    font.pixelSize: 48
                    color: "#404060"
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "Add media to preview"
                    font.pixelSize: 14
                    color: "#6B6B88"
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Click to play/pause
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (timelineNotifier) {
                        timelineNotifier.togglePlayPause()
                        playIndicatorAnim.restart()
                    }
                }
            }

            // Play/pause indicator overlay
            Rectangle {
                id: playIndicator
                anchors.centerIn: parent
                width: 64; height: 64
                radius: 32
                color: Qt.rgba(0, 0, 0, 0.55)
                opacity: 0
                visible: opacity > 0

                Label {
                    anchors.centerIn: parent
                    text: (timelineNotifier && timelineNotifier.isPlaying) ? "\u25B6" : "\u23F8"
                    font.pixelSize: 28
                    color: "white"
                }

                NumberAnimation on opacity {
                    id: playIndicatorAnim
                    from: 1.0; to: 0.0
                    duration: 700
                    easing.type: Easing.OutCubic
                    running: false
                }
            }
        }

        // ============================================================
        // Seek slider
        // ============================================================
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 24

            Rectangle {
                anchors.fill: parent
                color: "#14142B"
            }

            Slider {
                id: seekSlider
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                from: 0
                to: timelineNotifier ? Math.max(timelineNotifier.totalDuration, 0.01) : 1.0
                value: internal.isScrubbing ? seekSlider.value
                       : (timelineNotifier ? timelineNotifier.position : 0)
                stepSize: 0.001
                live: true

                onPressedChanged: {
                    if (pressed) {
                        internal.isScrubbing = true
                    } else {
                        if (timelineNotifier) {
                            timelineNotifier.scrubTo(value)
                        }
                        internal.isScrubbing = false
                    }
                }

                onMoved: {
                    if (internal.isScrubbing && timelineNotifier) {
                        timelineNotifier.scrubTo(value)
                    }
                }

                background: Rectangle {
                    x: seekSlider.leftPadding
                    y: seekSlider.topPadding + seekSlider.availableHeight / 2 - height / 2
                    width: seekSlider.availableWidth
                    height: 4
                    radius: 2
                    color: "#252540"

                    Rectangle {
                        width: seekSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 2
                        color: "#6C63FF"
                    }
                }

                handle: Rectangle {
                    x: seekSlider.leftPadding + seekSlider.visualPosition * (seekSlider.availableWidth - width)
                    y: seekSlider.topPadding + seekSlider.availableHeight / 2 - height / 2
                    width: seekSlider.pressed ? 14 : 10
                    height: width
                    radius: width / 2
                    color: seekSlider.pressed ? "#8B83FF" : "#6C63FF"
                    border.color: "#0D0D1A"
                    border.width: 1

                    Behavior on width { NumberAnimation { duration: 100 } }
                }
            }
        }

        // ============================================================
        // Transport controls bar
        // ============================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#14142B"

            Rectangle {
                anchors.top: parent.top
                width: parent.width; height: 1
                color: "#252540"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 0

                // --- Left: timecode ---
                Label {
                    id: positionLabel
                    text: internal.formatTimecode(timelineNotifier ? timelineNotifier.position : 0)
                    font.pixelSize: 13
                    font.family: "monospace"
                    font.weight: Font.Medium
                    color: "#E0E0F0"
                    Layout.preferredWidth: 76
                }

                Item { Layout.fillWidth: true }

                // --- Center: transport buttons ---
                RowLayout {
                    spacing: 2
                    Layout.alignment: Qt.AlignHCenter

                    TransportBtn {
                        iconText: "\u23EE"
                        toolTipText: "Jump to Start (Home)"
                        onClicked: { if (timelineNotifier) timelineNotifier.jumpToStart() }
                    }
                    TransportBtn {
                        iconText: "\u23EA"
                        toolTipText: "Previous (Up)"
                        onClicked: { if (timelineNotifier) timelineNotifier.jumpToPreviousSnapPoint() }
                    }
                    TransportBtn {
                        iconText: "\u25C0\u25C0"
                        toolTipText: "Fast Backward (J)"
                        onClicked: { if (timelineNotifier) timelineNotifier.shuttleReverse() }
                    }
                    TransportBtn {
                        iconText: "\u25C0|"
                        toolTipText: "Step Back (Left)"
                        onClicked: { if (timelineNotifier) timelineNotifier.stepBackward() }
                    }

                    // Play / Pause (larger button)
                    Rectangle {
                        width: 40; height: 40
                        radius: 20
                        color: playPauseMa.containsMouse ? "#7B73FF" : "#6C63FF"
                        Layout.leftMargin: 4
                        Layout.rightMargin: 4

                        Label {
                            anchors.centerIn: parent
                            text: (timelineNotifier && timelineNotifier.isPlaying) ? "\u23F8" : "\u25B6"
                            font.pixelSize: 18
                            color: "white"
                        }

                        MouseArea {
                            id: playPauseMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (timelineNotifier) {
                                    timelineNotifier.togglePlayPause()
                                    playIndicatorAnim.restart()
                                }
                            }
                        }

                        ToolTip.visible: playPauseMa.containsMouse
                        ToolTip.text: (timelineNotifier && timelineNotifier.isPlaying) ? "Pause (Space)" : "Play (Space)"
                    }

                    TransportBtn {
                        iconText: "|\u25B6"
                        toolTipText: "Step Forward (Right)"
                        onClicked: { if (timelineNotifier) timelineNotifier.stepForward() }
                    }
                    TransportBtn {
                        iconText: "\u25B6\u25B6"
                        toolTipText: "Fast Forward (L)"
                        onClicked: { if (timelineNotifier) timelineNotifier.shuttleForward() }
                    }
                    TransportBtn {
                        iconText: "\u23E9"
                        toolTipText: "Next (Down)"
                        onClicked: { if (timelineNotifier) timelineNotifier.jumpToNextSnapPoint() }
                    }
                    TransportBtn {
                        iconText: "\u23ED"
                        toolTipText: "Jump to End (End)"
                        onClicked: { if (timelineNotifier) timelineNotifier.jumpToEnd() }
                    }
                }

                Item { Layout.fillWidth: true }

                // --- Right: duration / total ---
                Label {
                    text: internal.formatTimecode(timelineNotifier ? timelineNotifier.totalDuration : 0)
                    font.pixelSize: 13
                    font.family: "monospace"
                    color: "#6B6B88"
                    Layout.preferredWidth: 76
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }

    // ================================================================
    // Sync MediaPlayer with timeline — load source and seek
    // ================================================================
    Connections {
        target: timelineNotifier
        function onPlaybackChanged() {
            internal.syncMediaPlayer()
        }
        function onTracksChanged() {
            internal.syncMediaPlayer()
        }
        function onStateChanged() {
            internal.syncMediaPlayer()
        }
    }

    // ================================================================
    // Transport button component
    // ================================================================
    component TransportBtn: Item {
        property string iconText: ""
        property string toolTipText: ""
        signal clicked()

        width: 32; height: 32
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        Rectangle {
            anchors.fill: parent
            radius: 6
            color: btnMa.containsMouse ? "#2A2A4A" : "transparent"
        }

        Label {
            anchors.centerIn: parent
            text: iconText
            font.pixelSize: 13
            color: btnMa.containsMouse ? "#E0E0F0" : "#B0B0C8"
        }

        MouseArea {
            id: btnMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }

        ToolTip.visible: btnMa.containsMouse && toolTipText.length > 0
        ToolTip.text: toolTipText
    }

    // ================================================================
    // Internal helpers
    // ================================================================
    QtObject {
        id: internal

        property bool isScrubbing: false
        property string currentSource: ""
        property bool isPlaying: false

        function formatTimecode(s) {
            if (s === undefined || s === null || isNaN(s)) return "00:00:00"
            var totalSec = Math.max(0, s)
            var h = Math.floor(totalSec / 3600)
            var m = Math.floor((totalSec % 3600) / 60)
            var sec = Math.floor(totalSec % 60)
            var frames = Math.floor((totalSec % 1) * 30)
            if (h > 0) {
                return String(h) + ":" +
                       String(m).padStart(2, '0') + ":" +
                       String(sec).padStart(2, '0') + ":" +
                       String(frames).padStart(2, '0')
            }
            return String(m).padStart(2, '0') + ":" +
                   String(sec).padStart(2, '0') + ":" +
                   String(frames).padStart(2, '0')
        }

        function syncMediaPlayer() {
            if (!timelineNotifier) return

            var source = timelineNotifier.activeClipSource
            var offset = timelineNotifier.activeClipOffset
            var playing = timelineNotifier.isPlaying

            // Load new source if changed
            if (source && source !== "" && source !== currentSource) {
                console.log("[VideoPlayer] loading source:", source)
                currentSource = source
                mediaPlayer.source = Qt.resolvedUrl("file:///" + source)
                // Wait for media to load, then seek
                mediaPlayer.pause()
            }

            if (!source || source === "") {
                if (currentSource !== "") {
                    console.log("[VideoPlayer] no active clip, stopping")
                    currentSource = ""
                    mediaPlayer.stop()
                    mediaPlayer.source = ""
                }
                return
            }

            // Sync playback state
            if (playing && !isPlaying) {
                isPlaying = true
                mediaPlayer.play()
            } else if (!playing && isPlaying) {
                isPlaying = false
                mediaPlayer.pause()
            }

            // Seek to correct position within source (only when not playing or scrubbing)
            if (!playing || isScrubbing) {
                var targetMs = Math.round(offset * 1000)
                var currentMs = mediaPlayer.position
                // Only seek if difference is significant (> 100ms)
                if (Math.abs(targetMs - currentMs) > 100) {
                    mediaPlayer.setPosition(targetMs)
                }
            }
        }
    }
}
