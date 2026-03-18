import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

/**
 * VE2PreviewPanel — video preview with Qt6 MediaPlayer for real playback.
 *
 * Uses MediaPlayer + VideoOutput for actual video decode and display.
 * Falls back to stub engine frame (image://videopreview/) when no video is loaded.
 *
 * Cross-platform:
 *   - Desktop (Win/Mac/Linux): FFmpeg backend (bundled with Qt 6.8)
 *   - iOS: AVFoundation backend
 *   - Android: MediaCodec backend
 */
Item {
    id: root

    readonly property double pos: timelineNotifier ? timelineNotifier.position : 0
    readonly property double dur: timelineNotifier ? timelineNotifier.totalDuration : 0
    readonly property bool timelineReady: timelineNotifier ? timelineNotifier.isReady : false

    Component.onCompleted: {
        console.log("[VE2Preview] created — timelineReady:", timelineReady)
    }

    Rectangle {
        anchors.fill: parent
        color: "#0A0A18"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Video display area ----
        Item {
            id: displayArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Layer 0: Stub engine frame (background — always there)
            Image {
                id: stubFrame
                anchors.fill: parent
                anchors.margins: 4
                fillMode: Image.PreserveAspectFit
                cache: false
                visible: !videoOut.visible  // only show when MediaPlayer has no video
                source: ""
            }

            // Audio output (must be declared before MediaPlayer references it)
            AudioOutput {
                id: audioOut
                volume: timelineNotifier && !timelineNotifier.activeClipMuted
                        ? timelineNotifier.activeClipVolume : 0.0
                muted: timelineNotifier ? timelineNotifier.activeClipMuted : false
            }

            // Layer 1: MediaPlayer + VideoOutput + AudioOutput for real playback
            MediaPlayer {
                id: player
                videoOutput: videoOut
                audioOutput: audioOut
                source: ""
                playbackRate: timelineNotifier ? timelineNotifier.activeClipSpeed : 1.0

                onMediaStatusChanged: {
                    console.log("[VE2Preview] MediaPlayer status:", mediaStatus,
                                "hasVideo:", hasVideo, "duration:", duration,
                                "source:", source)
                }

                onPlaybackStateChanged: {
                    console.log("[VE2Preview] playbackState:", playbackState)
                }

                onErrorOccurred: (error, errorString) => {
                    console.warn("[VE2Preview] MediaPlayer error:", error, errorString)
                }
            }

            VideoOutput {
                id: videoOut
                anchors.fill: parent
                anchors.margins: 4
                fillMode: VideoOutput.PreserveAspectFit
                visible: player.hasVideo
            }

            // Separate audio player for audio-only clips on Audio tracks
            AudioOutput {
                id: audioOnlyOut
                volume: timelineNotifier && !timelineNotifier.activeAudioMuted
                        ? timelineNotifier.activeAudioVolume : 0.0
                muted: timelineNotifier ? timelineNotifier.activeAudioMuted : false
            }
            MediaPlayer {
                id: audioPlayer
                audioOutput: audioOnlyOut
                source: ""
                playbackRate: timelineNotifier ? timelineNotifier.activeClipSpeed : 1.0
            }

            // "No media" placeholder
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                visible: !timelineReady || (dur <= 0 && player.source == "")

                Label { text: "\uD83C\uDFA5"; font.pixelSize: 48; color: "#303050"; Layout.alignment: Qt.AlignHCenter }
                Label { text: "Import media to preview"; font.pixelSize: 14; color: "#505068"; Layout.alignment: Qt.AlignHCenter }
            }

            // Status overlay (shows what's happening)
            Label {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 8
                visible: player.source != "" && !player.hasVideo
                text: "Loading video..."
                font.pixelSize: 11
                color: "#8888A0"
                padding: 4
                background: Rectangle { radius: 4; color: Qt.rgba(0, 0, 0, 0.5) }
            }
        }

        // ---- Transport controls ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "#0D0D1A"

            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#1E1E38" }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4

                // Seek slider
                Slider {
                    id: seekSlider
                    Layout.fillWidth: true
                    from: 0; to: Math.max(0.01, dur)
                    value: pressed ? value : pos  // don't fight user drag
                    live: true

                    onMoved: {
                        if (timelineNotifier) {
                            timelineNotifier.scrubTo(value)
                        }
                    }

                    background: Rectangle {
                        x: seekSlider.leftPadding; y: seekSlider.topPadding + seekSlider.availableHeight / 2 - 2
                        width: seekSlider.availableWidth; height: 4; radius: 2; color: "#1A1A34"

                        Rectangle {
                            width: seekSlider.visualPosition * parent.width; height: parent.height
                            radius: 2; color: "#6C63FF"
                        }
                    }
                }

                // Buttons row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: internal.formatTimecode(pos)
                        font.pixelSize: 12; font.family: "monospace"
                        color: "#B0B0C8"
                        Layout.preferredWidth: 80
                    }

                    Item { Layout.fillWidth: true }

                    ToolButton {
                        icon.name: "media-skip-backward"; icon.color: "#B0B0C8"; icon.width: 18; icon.height: 18
                        onClicked: { if (timelineNotifier) timelineNotifier.jumpToStart() }
                    }
                    ToolButton {
                        icon.name: "media-seek-backward"; icon.color: "#B0B0C8"; icon.width: 18; icon.height: 18
                        onClicked: { if (timelineNotifier) timelineNotifier.stepBackward() }
                    }
                    Button {
                        id: playBtn
                        width: 40; height: 40
                        onClicked: {
                            if (!timelineNotifier) return
                            timelineNotifier.togglePlayPause()
                            // MediaPlayer play/pause is handled by syncPlayState via onStateChanged
                        }
                        background: Rectangle {
                            radius: 20; color: playBtn.pressed ? "#5A52E0" : "#6C63FF"
                        }
                        contentItem: Label {
                            text: timelineNotifier && timelineNotifier.isPlaying ? "\u23F8" : "\u25B6"
                            font.pixelSize: 18; color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    ToolButton {
                        icon.name: "media-seek-forward"; icon.color: "#B0B0C8"; icon.width: 18; icon.height: 18
                        onClicked: { if (timelineNotifier) timelineNotifier.stepForward() }
                    }
                    ToolButton {
                        icon.name: "media-skip-forward"; icon.color: "#B0B0C8"; icon.width: 18; icon.height: 18
                        onClicked: { if (timelineNotifier) timelineNotifier.jumpToEnd() }
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: internal.formatTimecode(dur)
                        font.pixelSize: 12; font.family: "monospace"
                        color: "#6B6B88"
                        Layout.preferredWidth: 80
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }
    }

    // ---- Sync: timeline state → MediaPlayer ----
    Connections {
        target: timelineNotifier
        enabled: timelineNotifier !== null

        function onPlaybackChanged() {
            internal.syncSource()
            internal.syncPlayState()
            internal.syncAudio()
            internal.commitPlayState()
            if (!player.hasVideo) internal.refreshStub()
        }

        function onTracksChanged() {
            internal.syncSource()
            internal.syncAudio()
            internal.commitPlayState()
        }

        function onStateChanged() {
            internal.syncSource()
            internal.syncPlayState()
            internal.syncAudio()
            internal.commitPlayState()
            if (!player.hasVideo) internal.refreshStub()
        }

        function onFrameReady() {
            if (!player.hasVideo) internal.refreshStub()
        }
    }

    // Handle MediaPlayer reaching EndOfMedia while C++ still says "playing"
    Connections {
        target: player
        function onPlaybackStateChanged() {
            if (player.playbackState === MediaPlayer.StoppedState && timelineNotifier && timelineNotifier.isPlaying) {
                // MediaPlayer stopped (EndOfMedia) but timeline still playing — restart
                var offset = timelineNotifier.activeClipOffset
                var posMs = offset >= 0 ? Math.round(offset * 1000) : -1
                if (posMs >= 0) player.position = posMs
                player.play()
            }
        }
    }

    QtObject {
        id: internal

        property string currentSource: ""
        property string currentAudioSource: ""
        property bool wasPlaying: false
        property double lastPos: -1  // previous C++ position — for detecting user jumps

        // Sync audio-only clip playback
        function syncAudio() {
            if (!timelineNotifier) return

            var audioSrc = timelineNotifier.activeAudioSource
            if (audioSrc !== "" && audioSrc !== currentAudioSource) {
                currentAudioSource = audioSrc
                audioPlayer.source = pathToUrl(audioSrc)
                var audioOffset = timelineNotifier.activeAudioOffset
                var posMs = audioOffset >= 0 ? Math.round(audioOffset * 1000) : 0
                if (posMs >= 0) audioPlayer.position = posMs
                if (timelineNotifier.isPlaying) audioPlayer.play()
                else audioPlayer.pause()
            } else if (audioSrc === "" && currentAudioSource !== "") {
                currentAudioSource = ""
                audioPlayer.stop()
                audioPlayer.source = ""
            }

            // Sync play/pause
            if (audioPlayer.source != "") {
                var playing = timelineNotifier.isPlaying
                if (playing && !wasPlaying) {
                    var ao = timelineNotifier.activeAudioOffset
                    if (ao >= 0) audioPlayer.position = Math.round(ao * 1000)
                    audioPlayer.play()
                } else if (!playing && wasPlaying) {
                    audioPlayer.pause()
                    var ao2 = timelineNotifier.activeAudioOffset
                    if (ao2 >= 0) audioPlayer.position = Math.round(ao2 * 1000)
                } else if (!playing) {
                    var ao3 = timelineNotifier.activeAudioOffset
                    if (ao3 >= 0 && Math.abs(audioPlayer.position - Math.round(ao3 * 1000)) > 100)
                        audioPlayer.position = Math.round(ao3 * 1000)
                }
            }
        }

        // Load the video file into MediaPlayer when activeClipSource changes
        function syncSource() {
            if (!timelineNotifier) return

            var src = timelineNotifier.activeClipSource
            if (src !== "" && src !== currentSource) {
                currentSource = src
                var fileUrl = pathToUrl(src)
                console.log("[VE2Preview] loading:", fileUrl)
                player.source = fileUrl

                if (timelineNotifier.isPlaying) {
                    player.play()
                } else {
                    player.pause()
                }
            }

            if (src === "" && currentSource !== "") {
                currentSource = ""
                player.stop()
                player.source = ""
            }
        }

        // Sync play/pause state
        // Key principle: during normal playback, let MediaPlayer run freely.
        // Only seek on play/pause transitions, user-initiated jumps, or scrubbing.
        function syncPlayState() {
            if (!timelineNotifier) return
            // No video loaded — nothing to sync for the video player
            if (!player.hasVideo && player.source == "") {
                return
            }

            var playing = timelineNotifier.isPlaying
            var pos = timelineNotifier.position  // timeline position in seconds
            var offset = timelineNotifier.activeClipOffset
            var posMs = offset >= 0 ? Math.round(offset * 1000) : -1

            if (playing && !wasPlaying) {
                // Play started — seek to correct position then play
                if (posMs >= 0) player.position = posMs
                player.play()
            } else if (!playing && wasPlaying) {
                // Paused — stop playback, seek to exact frame
                player.pause()
                if (posMs >= 0) player.position = posMs
            } else if (!playing) {
                // Scrubbing while paused — seek to current offset
                if (posMs >= 0 && Math.abs(player.position - posMs) > 100) {
                    player.position = posMs
                }
            } else if (playing) {
                // Normal playback — detect user-initiated jumps by checking
                // how much the C++ position changed since last tick.
                // Normal tick: ~33ms change. User click/scrub: >>500ms change.
                var posDelta = Math.abs(pos - lastPos)
                if (lastPos >= 0 && posDelta > 0.5 && posMs >= 0) {
                    // User jumped to a new position — seek MediaPlayer
                    player.position = posMs
                }
                // Otherwise: let MediaPlayer play freely, no seeking.
                // EndOfMedia is handled by the player.onPlaybackStateChanged connection above.
            }

        }

        // Update wasPlaying/lastPos AFTER both syncPlayState and syncAudio
        // so both functions see the previous state for transition detection.
        function commitPlayState() {
            if (!timelineNotifier) return
            wasPlaying = timelineNotifier.isPlaying
            lastPos = timelineNotifier.position
        }

        function pathToUrl(path) {
            if (path.startsWith("file:")) return path
            if (path.match(/^[A-Za-z]:/))
                return "file:///" + path
            return "file://" + path
        }

        // Stub frame — only used when MediaPlayer has no video loaded
        function refreshStub() {
            if (!timelineNotifier) return
            var fv = timelineNotifier.frameVersion
            if (fv > 0) {
                stubFrame.source = "image://videopreview/" + fv
            }
        }

        function formatTimecode(s) {
            if (s === undefined || s === null || isNaN(s)) return "00:00"
            var h = Math.floor(s / 3600)
            var m = Math.floor((s % 3600) / 60)
            var sec = Math.floor(s % 60)
            if (h > 0)
                return String(h) + ":" + String(m).padStart(2, '0') + ":" + String(sec).padStart(2, '0')
            return String(m).padStart(2, '0') + ":" + String(sec).padStart(2, '0')
        }
    }
}
