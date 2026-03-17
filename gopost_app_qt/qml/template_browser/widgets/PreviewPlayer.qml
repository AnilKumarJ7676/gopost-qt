import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: root

    property string previewUrl: ""
    property string thumbnailUrl: ""
    property bool isVideo: false
    property int videoWidth: 1080
    property int videoHeight: 1920
    property bool autoPlay: true
    property bool looping: true

    property bool _initialized: false
    property bool _showControls: true
    property bool _isPlaying: false
    property bool _isMuted: true

    clip: true

    // Image preview
    Image {
        id: thumbnailImage
        anchors.fill: parent
        source: root.thumbnailUrl
        fillMode: Image.PreserveAspectCrop
        visible: !root.isVideo || !root._initialized
        asynchronous: true
    }

    // Placeholder when no thumbnail
    Rectangle {
        anchors.fill: parent
        color: palette.alternateBase
        visible: (!root.thumbnailUrl || root.thumbnailUrl === "") && !root._initialized

        Label {
            anchors.centerIn: parent
            text: root.isVideo ? "\ue04b" : "\ue3f4"
            font.pixelSize: 48
            color: palette.placeholderText
        }

        BusyIndicator {
            anchors.centerIn: parent
            visible: root.isVideo && !root._initialized && root.previewUrl !== ""
            running: visible
        }
    }

    // Video player
    MediaPlayer {
        id: mediaPlayer
        source: root.isVideo && root.previewUrl ? root.previewUrl : ""
        audioOutput: AudioOutput {
            id: audioOut
            volume: root._isMuted ? 0.0 : 1.0
        }
        videoOutput: videoOutput
        loops: root.looping ? MediaPlayer.Infinite : 1

        onPlaybackStateChanged: {
            root._isPlaying = (playbackState === MediaPlayer.PlayingState)
        }

        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.LoadedMedia) {
                root._initialized = true
                if (root.autoPlay) {
                    mediaPlayer.play()
                    root._showControls = false
                }
            }
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: root.isVideo && root._initialized
    }

    // Play/Pause overlay
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.3)
        visible: root.isVideo
        opacity: root._showControls ? 1.0 : 0.0

        Behavior on opacity { NumberAnimation { duration: 200 } }

        Label {
            anchors.centerIn: parent
            text: root._isPlaying ? "\ue034" : "\ue037" // pause_circle / play_circle
            font.pixelSize: 64
            color: "white"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (!root._initialized) {
                    root._showControls = !root._showControls
                    return
                }
                if (root._isPlaying) {
                    mediaPlayer.pause()
                    root._showControls = true
                } else {
                    mediaPlayer.play()
                    controlsHideTimer.restart()
                }
            }
        }
    }

    // Mute button
    Rectangle {
        visible: root.isVideo && root._initialized
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.bottomMargin: root._initialized ? 16 : 12
        width: 36; height: 36
        radius: 18
        color: Qt.rgba(0, 0, 0, 0.6)
        opacity: root._showControls || !root._isPlaying ? 1.0 : 0.0

        Behavior on opacity { NumberAnimation { duration: 200 } }

        Label {
            anchors.centerIn: parent
            text: root._isMuted ? "\ue04f" : "\ue050" // volume_off / volume_up
            font.pixelSize: 20
            color: "white"
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root._isMuted = !root._isMuted
        }
    }

    // Progress bar
    Rectangle {
        visible: root.isVideo && root._initialized
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 3
        color: Qt.rgba(1, 1, 1, 0.3)
        opacity: root._showControls || !root._isPlaying ? 1.0 : 0.3

        Behavior on opacity { NumberAnimation { duration: 200 } }

        Rectangle {
            width: mediaPlayer.duration > 0
                   ? parent.width * (mediaPlayer.position / mediaPlayer.duration) : 0
            height: parent.height
            color: palette.highlight
        }
    }

    // Auto-hide controls timer
    Timer {
        id: controlsHideTimer
        interval: 3000
        onTriggered: {
            if (root._isPlaying)
                root._showControls = false
        }
    }

    // Pause on app deactivation
    Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive && root.isVideo)
                mediaPlayer.pause()
        }
    }
}
