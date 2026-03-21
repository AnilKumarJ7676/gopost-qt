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
    readonly property double fps: timelineNotifier ? timelineNotifier.frameRate : 30.0
    property int timecodeMode: 0   // 0=HH:MM:SS:FF, 1=Seconds, 2=Frames, 3=Feet+Frames
    property bool timecodeEditMode: false
    property bool isFullscreen: false

    // Color grading state — read from the selected clip
    readonly property var cgMap: timelineNotifier ? timelineNotifier.clipColorGradingMap : ({})
    readonly property bool hasColorGrading: {
        if (!cgMap || Object.keys(cgMap).length === 0) return false
        var keys = ["brightness","contrast","saturation","exposure","temperature",
                     "tint","highlights","shadows","vibrance","hue","fade","vignette"]
        for (var i = 0; i < keys.length; i++) {
            if (cgMap[keys[i]] !== undefined && cgMap[keys[i]] !== 0) return true
        }
        return false
    }

    Component.onCompleted: {
        console.log("[VE2Preview] created — timelineReady:", timelineReady)
    }

    Component.onDestruction: {
        // Stop all playback on cleanup to prevent audio/video
        // continuing after the window is closed (especially in fullscreen)
        scrubPauseTimer.stop()
        player.stop()
        player.source = ""
        audioPlayer.stop()
        audioPlayer.source = ""
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

                // Content rect in UV space from paintedWidth/paintedHeight
                readonly property real crLeft:   width  > 0 ? (width  - paintedWidth)  / 2 / width  : 0
                readonly property real crTop:    height > 0 ? (height - paintedHeight) / 2 / height : 0
                readonly property real crRight:  width  > 0 ? (width  + paintedWidth)  / 2 / width  : 1
                readonly property real crBottom: height > 0 ? (height + paintedHeight) / 2 / height : 1

                // Color grading shader for non-video clips (images, titles, etc.)
                layer.enabled: root.hasColorGrading && visible
                layer.effect: ShaderEffect {
                    property var source: stubFrame
                    // Content rect bounds (UV 0-1)
                    property real crLeft:   stubFrame.crLeft
                    property real crTop:    stubFrame.crTop
                    property real crRight:  stubFrame.crRight
                    property real crBottom: stubFrame.crBottom
                    // Color grading params
                    property real brightness:   root.cgMap.brightness   || 0
                    property real contrast:     root.cgMap.contrast     || 0
                    property real saturation:   root.cgMap.saturation   || 0
                    property real exposure:     root.cgMap.exposure     || 0
                    property real temperature:  root.cgMap.temperature  || 0
                    property real tintVal:      root.cgMap.tint         || 0
                    property real highlights:   root.cgMap.highlights   || 0
                    property real shadows:      root.cgMap.shadows      || 0
                    property real vibrance:     root.cgMap.vibrance     || 0
                    property real hueShift:     root.cgMap.hue          || 0
                    property real fade:         root.cgMap.fade         || 0
                    property real vignetteAmt:  root.cgMap.vignette     || 0
                    fragmentShader: "qrc:/GopostApp/shaders/colorgrading.frag.qsb"
                }
            }

            // Audio output (must be declared before MediaPlayer references it)
            // Muted during scrub-play to avoid audio blips when seeking
            AudioOutput {
                id: audioOut
                volume: timelineNotifier && !timelineNotifier.activeClipMuted
                        ? timelineNotifier.activeClipVolume : 0.0
                muted: (timelineNotifier ? timelineNotifier.activeClipMuted : false)
                       || internal.scrubPauseScheduled
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

                    // When media finishes loading, seek to the pending position.
                    // This fixes the bug where jumping the playhead to a different
                    // clip starts from 0 instead of the correct source offset,
                    // because player.position = X is ignored while media is loading.
                    if (mediaStatus === MediaPlayer.LoadedMedia ||
                        mediaStatus === MediaPlayer.BufferedMedia) {
                        if (internal.pendingSeekMs >= 0) {
                            console.log("[VE2Preview] deferred seek to:", internal.pendingSeekMs, "ms",
                                        "play:", internal.pendingPlay)
                            player.position = internal.pendingSeekMs
                            if (internal.pendingPlay) {
                                player.play()
                            } else {
                                // Must pause to put player in PausedState so it
                                // renders the frame. StoppedState shows black.
                                player.pause()
                            }
                            internal.pendingSeekMs = -1
                            internal.pendingPlay = false
                        } else if (player.playbackState === MediaPlayer.StoppedState) {
                            // Media just loaded but no pending seek — put player
                            // in PausedState so future position changes render frames.
                            // A StoppedState player ignores position changes.
                            player.pause()
                        }
                    }
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

                // Content rect in UV space — shader only applies grading inside this area
                readonly property real crLeft:   videoOut.width  > 0 ? videoOut.contentRect.x / videoOut.width  : 0
                readonly property real crTop:    videoOut.height > 0 ? videoOut.contentRect.y / videoOut.height : 0
                readonly property real crRight:  videoOut.width  > 0 ? (videoOut.contentRect.x + videoOut.contentRect.width)  / videoOut.width  : 1
                readonly property real crBottom: videoOut.height > 0 ? (videoOut.contentRect.y + videoOut.contentRect.height) / videoOut.height : 1

                // Color grading shader applied as layer effect
                layer.enabled: root.hasColorGrading
                layer.effect: ShaderEffect {
                    property var source: videoOut
                    // Content rect bounds (UV 0-1)
                    property real crLeft:   videoOut.crLeft
                    property real crTop:    videoOut.crTop
                    property real crRight:  videoOut.crRight
                    property real crBottom: videoOut.crBottom
                    // Color grading params
                    property real brightness:   root.cgMap.brightness   || 0
                    property real contrast:     root.cgMap.contrast     || 0
                    property real saturation:   root.cgMap.saturation   || 0
                    property real exposure:     root.cgMap.exposure     || 0
                    property real temperature:  root.cgMap.temperature  || 0
                    property real tintVal:      root.cgMap.tint         || 0
                    property real highlights:   root.cgMap.highlights   || 0
                    property real shadows:      root.cgMap.shadows      || 0
                    property real vibrance:     root.cgMap.vibrance     || 0
                    property real hueShift:     root.cgMap.hue          || 0
                    property real fade:         root.cgMap.fade         || 0
                    property real vignetteAmt:  root.cgMap.vignette     || 0
                    fragmentShader: "qrc:/GopostApp/shaders/colorgrading.frag.qsb"
                }
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

            // Status overlay — loading spinner + message when MediaPlayer is buffering
            Rectangle {
                id: loadingOverlay
                anchors.centerIn: parent
                width: loadingCol.width + 32
                height: loadingCol.height + 24
                radius: 8
                color: Qt.rgba(0.03, 0.03, 0.08, 0.85)
                border.color: "#2A2A4A"
                border.width: 1
                visible: player.source != "" && !player.hasVideo

                Column {
                    id: loadingCol
                    anchors.centerIn: parent
                    spacing: 10

                    // Spinning arc indicator
                    Item {
                        width: 28; height: 28
                        anchors.horizontalCenter: parent.horizontalCenter

                        Rectangle {
                            id: spinnerArc
                            anchors.fill: parent
                            radius: width / 2
                            color: "transparent"
                            border.color: "#6C63FF"
                            border.width: 3
                            opacity: 0.7

                            // Clip to show only partial arc
                            layer.enabled: true
                            layer.effect: Item {}  // identity — just enables layer for rotation

                            RotationAnimation on rotation {
                                from: 0; to: 360
                                duration: 1000
                                loops: Animation.Infinite
                                running: loadingOverlay.visible
                            }
                        }

                        // Background ring
                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: "transparent"
                            border.color: "#1E1E38"
                            border.width: 3
                            z: -1
                        }
                    }

                    Label {
                        text: "Loading preview..."
                        font.pixelSize: 11
                        color: "#8888A0"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
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

                    // Timecode display with click-to-edit and mode toggle
                    Item {
                        Layout.preferredWidth: 160
                        Layout.fillHeight: true

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 1

                            // Main timecode — click to edit
                            Item {
                                width: 150; height: 20

                                Label {
                                    id: tcLabel
                                    visible: !root.timecodeEditMode
                                    text: internal.formatTimecode(pos)
                                    font.pixelSize: 16; font.family: "monospace"; font.weight: Font.Bold
                                    color: "#E0E0F0"

                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.IBeamCursor
                                        onClicked: {
                                            root.timecodeEditMode = true
                                            tcInput.text = internal.formatTimecode(pos)
                                            tcInput.forceActiveFocus()
                                            tcInput.selectAll()
                                        }
                                    }
                                }

                                TextInput {
                                    id: tcInput
                                    visible: root.timecodeEditMode
                                    width: parent.width; height: parent.height
                                    font.pixelSize: 16; font.family: "monospace"; font.weight: Font.Bold
                                    color: "#E0E0F0"; selectionColor: "#6C63FF"; selectedTextColor: "#FFFFFF"
                                    verticalAlignment: TextInput.AlignVCenter

                                    Rectangle {
                                        anchors.fill: parent; z: -1; radius: 3
                                        color: "#1A1A34"; border.color: "#6C63FF"; border.width: 1
                                    }

                                    onAccepted: {
                                        var secs = internal.parseTimecodeToSeconds(text, root.fps)
                                        if (secs >= 0 && timelineNotifier)
                                            timelineNotifier.seek(secs)
                                        root.timecodeEditMode = false
                                    }
                                    Keys.onEscapePressed: root.timecodeEditMode = false
                                    onActiveFocusChanged: {
                                        if (!activeFocus) root.timecodeEditMode = false
                                    }
                                }
                            }

                            // Source timecode + mode toggle row
                            Row {
                                spacing: 4

                                Label {
                                    text: internal.sourceTimecode()
                                    font.pixelSize: 9; font.family: "monospace"
                                    color: "#6C63FF"
                                    visible: text !== ""
                                }

                                Rectangle {
                                    width: modeLbl.implicitWidth + 8; height: 14; radius: 3
                                    color: modeHov.hovered ? Qt.rgba(1,1,1,0.08) : Qt.rgba(1,1,1,0.03)
                                    border.color: "#3A3A5A"; border.width: 1

                                    Label {
                                        id: modeLbl; anchors.centerIn: parent
                                        readonly property var labels: ["TC", "SEC", "FRM", "FT"]
                                        readonly property var tips: [
                                            "Timecode (HH:MM:SS:FF)", "Seconds (SS.ms)",
                                            "Frame Count", "Feet + Frames"
                                        ]
                                        text: labels[root.timecodeMode]
                                        font.pixelSize: 8; font.weight: Font.Bold; color: "#8888A0"
                                    }
                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: root.timecodeMode = (root.timecodeMode + 1) % 4
                                    }
                                    HoverHandler { id: modeHov }
                                    ToolTip.visible: modeHov.hovered
                                    ToolTip.text: modeLbl.tips[root.timecodeMode]
                                }
                            }
                        }
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
                        font.pixelSize: 11; font.family: "monospace"
                        color: "#6B6B88"
                        Layout.preferredWidth: 100
                        horizontalAlignment: Text.AlignRight
                    }

                    // Fullscreen toggle button
                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: fsHover.hovered ? Qt.rgba(1,1,1,0.1) : "transparent"
                        Layout.leftMargin: 4

                        Label {
                            anchors.centerIn: parent
                            text: root.isFullscreen ? "\u2716" : "\u26F6"
                            font.pixelSize: 14; color: "#B0B0C8"
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: root.isFullscreen = !root.isFullscreen
                        }
                        HoverHandler { id: fsHover }
                        ToolTip.visible: fsHover.hovered
                        ToolTip.text: root.isFullscreen ? "Exit fullscreen (Esc)" : "Fullscreen preview (F)"
                    }
                }
            }
        }
    }

    // Debounce timer for play→pause seek.
    // Waits 150ms after the last scrub to pause, giving Qt's decoder time
    // to start and decode a frame. Resets on each new scrub so rapid
    // scrubbing doesn't thrash the demuxer.
    Timer {
        id: scrubPauseTimer
        interval: 100
        repeat: false
        onTriggered: {
            if (internal.scrubPauseScheduled && timelineNotifier && !timelineNotifier.isPlaying) {
                // Re-seek to the exact target position before pausing.
                // During the brief play(), the player may have advanced
                // past the target — this ensures the paused frame is correct.
                if (internal.scrubTargetMs >= 0) {
                    player.position = internal.scrubTargetMs
                }
                player.pause()
                internal.scrubPauseScheduled = false
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
        property int pendingSeekMs: -1  // deferred seek when source changes (media loading)
        property bool pendingPlay: false
        property bool scrubPauseScheduled: false  // debounce flag for play→pause seek
        property int scrubTargetMs: -1  // the target position to land on when pause fires

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

        // Load the video file into MediaPlayer when activeClipSource changes.
        // When the source changes, the seek is deferred until media finishes
        // loading (onMediaStatusChanged), because MediaPlayer ignores
        // position changes during async loading.
        function syncSource() {
            if (!timelineNotifier) return

            var src = timelineNotifier.activeClipSource
            if (src !== "" && src !== currentSource) {
                currentSource = src
                var fileUrl = pathToUrl(src)
                console.log("[VE2Preview] loading:", fileUrl)

                // Store the target seek position for deferred apply
                var offset = timelineNotifier.activeClipOffset
                pendingSeekMs = offset >= 0 ? Math.round(offset * 1000) : 0
                pendingPlay = timelineNotifier.isPlaying

                player.source = fileUrl
                // Don't play or seek here — wait for LoadedMedia in onMediaStatusChanged
            }

            if (src === "" && currentSource !== "") {
                currentSource = ""
                pendingSeekMs = -1
                pendingPlay = false
                player.stop()
                player.source = ""
            }
        }

        // Seek MediaPlayer and show the frame at posMs.
        // Qt6 MediaPlayer on Windows only renders frames during playback
        // or on the Playing→Paused transition. A stopped/paused player
        // ignores position changes (black screen).
        //
        // Strategy: play() to kick the decoder, set position, then
        // debounce-pause after 150ms. Audio is muted during scrub-play
        // (see AudioOutput binding on scrubPauseScheduled).
        function seekAndShowFrame(posMs) {
            if (posMs < 0) return
            // Store target so the pause timer can re-seek to exact frame
            scrubTargetMs = posMs
            // Start decoder FIRST, then seek — on Qt6/Windows, seeking
            // a stopped player is ignored. Playing starts the decode
            // pipeline so the subsequent position change takes effect.
            if (player.playbackState !== MediaPlayer.PlayingState) {
                player.play()
            }
            player.position = posMs
            // Schedule debounced pause to freeze on the correct frame
            scrubPauseScheduled = true
            scrubPauseTimer.restart()
        }

        // Sync play/pause state with timeline notifier.
        function syncPlayState() {
            if (!timelineNotifier) return
            if (!player.hasVideo && player.source == "") return

            // Source still loading — deferred seek will handle it
            if (pendingSeekMs >= 0) return

            var playing = timelineNotifier.isPlaying
            var pos = timelineNotifier.position
            var offset = timelineNotifier.activeClipOffset
            var posMs = offset >= 0 ? Math.round(offset * 1000) : -1

            if (playing && !wasPlaying) {
                // User pressed Play — cancel any pending scrub-pause
                scrubPauseTimer.stop()
                scrubPauseScheduled = false
                scrubTargetMs = -1
                // Seek to correct position then play
                if (posMs >= 0) player.position = posMs
                player.play()

            } else if (!playing && wasPlaying) {
                // User pressed Pause — show the frame at current position
                seekAndShowFrame(posMs)

            } else if (!playing) {
                // Not playing, wasn't playing — user clicked/dragged on
                // the timeline to jump the playhead. Always seek and show.
                if (posMs >= 0) {
                    seekAndShowFrame(posMs)
                }

            } else if (playing) {
                // During playback — detect large jumps (user clicked ruler)
                var posDelta = Math.abs(pos - lastPos)
                if (lastPos >= 0 && posDelta > 0.5 && posMs >= 0) {
                    player.position = posMs
                }
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
            return formatTC(s, root.timecodeMode, root.fps)
        }

        function formatTC(s, mode, fps) {
            if (s === undefined || s === null || isNaN(s)) s = 0
            fps = fps || 30
            switch (mode) {
            case 0: // HH:MM:SS:FF
                var h = Math.floor(s / 3600)
                var m = Math.floor((s % 3600) / 60)
                var sec = Math.floor(s % 60)
                var fr = Math.floor((s % 1) * fps)
                return String(h).padStart(2, '0') + ":" + String(m).padStart(2, '0') + ":"
                     + String(sec).padStart(2, '0') + ":" + String(fr).padStart(2, '0')
            case 1: // Seconds
                return s.toFixed(3) + "s"
            case 2: // Frame count
                return Math.floor(s * fps) + "f"
            case 3: // Feet+Frames (35mm, 16 frames/foot)
                var totalFrames = Math.floor(s * fps)
                var feet = Math.floor(totalFrames / 16)
                var rem = totalFrames % 16
                return feet + "+" + String(rem).padStart(2, '0') + " ft"
            }
            return "00:00:00:00"
        }

        function parseTimecodeToSeconds(text, fps) {
            fps = fps || 30
            text = text.trim()
            var parts = text.split(":")
            if (parts.length === 4)
                return parseInt(parts[0]) * 3600 + parseInt(parts[1]) * 60
                     + parseInt(parts[2]) + parseInt(parts[3]) / fps
            if (parts.length === 3)
                return parseInt(parts[0]) * 60 + parseInt(parts[1]) + parseInt(parts[2]) / fps
            if (parts.length === 2)
                return parseInt(parts[0]) * 60 + parseInt(parts[1])
            if (text.endsWith("f")) {
                var fn = parseInt(text)
                if (!isNaN(fn)) return fn / fps
            }
            var num = parseFloat(text)
            if (!isNaN(num)) return num
            return -1
        }

        function sourceTimecode() {
            if (!timelineNotifier) return ""
            var clip = timelineNotifier.selectedClip
            if (!clip || clip.clipId === undefined || clip.clipId < 0) return ""
            var offset = root.pos - (clip.timelineIn || 0)
            if (offset < 0 || offset > (clip.duration || 0)) return ""
            var speed = clip.speed || 1.0
            var srcTime
            if (clip.isReversed)
                srcTime = (clip.sourceOut || 0) - offset / speed
            else
                srcTime = (clip.sourceIn || 0) + offset / speed
            return "SRC " + formatTC(Math.max(0, srcTime), root.timecodeMode, root.fps)
        }
    }
}
