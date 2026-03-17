import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * ClipWidget — colour-coded clip with trim handles, badges, thumbnails.
 *
 * Interaction zones (no overlap):
 *   [Left trim 6px] [Main body ──────────────────] [Right trim 6px]
 *
 * - Left/right trim: SizeHorCursor, drag changes in/out point
 * - Main body: click to select, double-click to inspect, drag to move
 * - Trim handles have higher z-order so they always intercept first
 */
Item {
    id: root

    required property int clipIndex
    required property int trackIndex

    // Fetch clip data from notifier — trackCount read creates binding on stateChanged
    readonly property var clipInfo: timelineNotifier && timelineNotifier.trackCount >= 0
                                    ? timelineNotifier.clipData(trackIndex, clipIndex) : ({})
    readonly property int clipId: clipInfo.clipId !== undefined ? clipInfo.clipId : -1
    readonly property string displayName: clipInfo.displayName || ""
    readonly property real timelineIn: clipInfo.timelineIn !== undefined ? clipInfo.timelineIn : 0
    readonly property real timelineOut: clipInfo.timelineOut !== undefined ? clipInfo.timelineOut : 1
    readonly property real duration: clipInfo.duration !== undefined ? clipInfo.duration : 1
    readonly property int sourceType: clipInfo.sourceType !== undefined ? clipInfo.sourceType : 0
    readonly property bool isSelected: clipId >= 0
                                       && (timelineNotifier ? timelineNotifier.selectedClipId === clipId : false)
    readonly property real speed: clipInfo.speed !== undefined ? clipInfo.speed : 1.0
    readonly property string sourcePath: clipInfo.sourcePath || ""

    readonly property real pps: timelineNotifier ? timelineNotifier.pixelsPerSecond : 100

    // Base64-encode sourcePath for use in image:// URL
    readonly property string pathBase64: sourcePath !== "" ? Qt.btoa(sourcePath) : ""

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

    Component.onCompleted: console.log("[ClipWidget] created track:", trackIndex,
        "clip:", clipIndex, "id:", clipId, "x:", x.toFixed(1), "w:", width.toFixed(1),
        "name:", displayName)

    // ================================================================
    // Clip body
    // ================================================================
    Rectangle {
        id: body
        anchors.fill: parent
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        radius: 4
        color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.25)
        border.color: isSelected ? "#6C63FF" : Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.5)
        border.width: isSelected ? 2 : 0.5

        // Top bar with name + badges
        Rectangle {
            id: topBar
            anchors.top: parent.top
            width: parent.width
            height: 18
            radius: 4
            color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.4)

            // Bottom-left/right corners should not be rounded
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 4
                color: parent.color
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8  // leave room for trim handle
                anchors.rightMargin: 8
                spacing: 3

                Label {
                    text: displayName
                    font.pixelSize: 10
                    color: "white"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                // Speed badge
                Rectangle {
                    visible: speed !== 1.0
                    width: speedLabel.implicitWidth + 6
                    height: 14
                    radius: 3
                    color: Qt.rgba(0, 0, 0, 0.3)
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        id: speedLabel
                        anchors.centerIn: parent
                        text: speed.toFixed(1) + "x"
                        font.pixelSize: 9
                        color: "white"
                    }
                }

                // Source type icon
                Label {
                    text: {
                        switch (sourceType) {
                        case 0: return "\uD83C\uDFA5"  // video
                        case 1: return "\uD83D\uDDBC"  // image
                        case 2: return "T"              // title
                        case 3: return "\uD83C\uDFA8"  // color
                        case 4: return "\u2728"         // adjustment
                        default: return ""
                        }
                    }
                    font.pixelSize: 10
                    color: "white"
                    opacity: 0.7
                }
            }
        }

        // Thumbnail area — shows actual video frames for video/image clips
        Item {
            id: thumbArea
            anchors.top: topBar.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            clip: true

            // Background
            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.06)
                radius: 2
            }

            // Video/image thumbnail strip
            Row {
                id: thumbnailRow
                anchors.fill: parent
                visible: (sourceType === 0 || sourceType === 1) && pathBase64 !== ""
                clip: true

                // Each thumbnail is thumbHeight-tall with 16:9 aspect
                readonly property int thumbHeight: Math.max(20, parent.height)
                readonly property int thumbWidth: Math.max(30, Math.round(thumbHeight * 16.0 / 9.0))
                readonly property int thumbCount: Math.max(1, Math.min(Math.ceil(parent.width / thumbWidth), 30))

                Repeater {
                    model: thumbnailRow.thumbCount
                    Image {
                        required property int index
                        width: thumbnailRow.thumbWidth
                        height: thumbnailRow.thumbHeight
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                        // Calculate source time for this thumbnail position
                        source: {
                            if (pathBase64 === "" || duration <= 0) return ""
                            var fraction = thumbnailRow.thumbCount > 1
                                           ? index / (thumbnailRow.thumbCount - 1)
                                           : 0.5
                            var timeSec = timelineIn + fraction * duration
                            return "image://videothumbnail/" + pathBase64 + "@"
                                   + timeSec.toFixed(1) + "@" + thumbnailRow.thumbHeight
                        }
                    }
                }
            }

            // Color overlay for non-video clips (title, color, adjustment)
            Rectangle {
                anchors.fill: parent
                visible: sourceType >= 2 || pathBase64 === ""
                color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.12)
                radius: 2

                Label {
                    anchors.centerIn: parent
                    text: {
                        switch (sourceType) {
                        case 2: return "TITLE"
                        case 3: return "COLOR"
                        case 4: return "ADJ"
                        default: return ""
                        }
                    }
                    font.pixelSize: 9
                    color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.5)
                    visible: text !== ""
                }
            }

            // Waveform at bottom for video clips
            Row {
                visible: sourceType === 0
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottomMargin: 1
                anchors.leftMargin: 1
                anchors.rightMargin: 1
                height: 10
                spacing: 1
                clip: true
                z: 1

                // Semi-transparent background
                Rectangle {
                    anchors.fill: parent
                    color: Qt.rgba(0, 0, 0, 0.3)
                    radius: 1
                }

                Repeater {
                    model: Math.min(Math.floor(parent.width / 3), 50)
                    Rectangle {
                        width: 2
                        height: 1 + Math.random() * 8
                        y: parent.height - height
                        radius: 1
                        color: Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.4)
                    }
                }
            }
        }
    }

    // ================================================================
    // Main body interaction: click to select, double-click to inspect, drag to move
    // ================================================================
    MouseArea {
        id: bodyArea
        anchors.fill: parent
        anchors.leftMargin: 6   // leave trim handle zones
        anchors.rightMargin: 6
        z: 1
        cursorShape: Qt.PointingHandCursor

        drag.target: root
        drag.axis: Drag.XAxis
        drag.minimumX: 0

        property real dragStartX: 0

        onPressed: {
            dragStartX = root.x
            if (timelineNotifier) timelineNotifier.selectClip(clipId)
        }

        onClicked: {
            console.log("[ClipWidget] selected clip:", clipId)
        }

        onDoubleClicked: {
            console.log("[ClipWidget] double-click clip:", clipId, "sourceType:", sourceType)
            if (timelineNotifier) {
                if (sourceType === 2) timelineNotifier.setActivePanel(2)  // text
                else timelineNotifier.setActivePanel(1)  // inspector
            }
        }

        onReleased: {
            if (Math.abs(root.x - dragStartX) > 2) {
                var newTime = root.x / pps
                console.log("[ClipWidget] moveClip:", clipId, "to time:", newTime.toFixed(2))
                if (timelineNotifier) timelineNotifier.moveClip(clipId, trackIndex, newTime)
            }
        }
    }

    // ================================================================
    // Left trim handle
    // ================================================================
    Rectangle {
        id: leftHandle
        width: 6; height: parent.height
        x: 0; z: 10
        radius: 2
        color: leftTrimArea.containsMouse || leftTrimArea.pressed
               ? "#6C63FF"
               : (isSelected ? Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.8) : "transparent")

        // Grip dots
        Column {
            anchors.centerIn: parent
            spacing: 3
            visible: leftTrimArea.containsMouse || leftTrimArea.pressed || isSelected
            Repeater {
                model: 3
                Rectangle { width: 2; height: 2; radius: 1; color: "white"; opacity: 0.6 }
            }
        }

        MouseArea {
            id: leftTrimArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor

            property real startMouseX: 0
            property real startTimelineIn: 0

            onPressed: mouse => {
                startMouseX = mapToItem(root.parent, mouse.x, 0).x
                startTimelineIn = timelineIn
                console.log("[ClipWidget] left trim started, clip:", clipId, "in:", timelineIn.toFixed(2))
            }
            onPositionChanged: mouse => {
                if (pressed && timelineNotifier) {
                    var currentX = mapToItem(root.parent, mouse.x, 0).x
                    var deltaPx = currentX - startMouseX
                    var deltaTime = deltaPx / pps
                    var newIn = Math.max(0, startTimelineIn + deltaTime)
                    // Don't let in-point pass out-point
                    if (newIn < timelineOut - 0.1) {
                        timelineNotifier.trimClip(clipId, newIn, timelineOut)
                    }
                }
            }
            onReleased: {
                console.log("[ClipWidget] left trim ended, clip:", clipId, "newIn:", timelineIn.toFixed(2))
            }
        }
    }

    // ================================================================
    // Right trim handle
    // ================================================================
    Rectangle {
        id: rightHandle
        width: 6; height: parent.height
        anchors.right: parent.right
        z: 10
        radius: 2
        color: rightTrimArea.containsMouse || rightTrimArea.pressed
               ? "#6C63FF"
               : (isSelected ? Qt.rgba(clipColor.r, clipColor.g, clipColor.b, 0.8) : "transparent")

        // Grip dots
        Column {
            anchors.centerIn: parent
            spacing: 3
            visible: rightTrimArea.containsMouse || rightTrimArea.pressed || isSelected
            Repeater {
                model: 3
                Rectangle { width: 2; height: 2; radius: 1; color: "white"; opacity: 0.6 }
            }
        }

        MouseArea {
            id: rightTrimArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor

            property real startMouseX: 0
            property real startTimelineOut: 0

            onPressed: mouse => {
                startMouseX = mapToItem(root.parent, mouse.x, 0).x
                startTimelineOut = timelineOut
                console.log("[ClipWidget] right trim started, clip:", clipId, "out:", timelineOut.toFixed(2))
            }
            onPositionChanged: mouse => {
                if (pressed && timelineNotifier) {
                    var currentX = mapToItem(root.parent, mouse.x, 0).x
                    var deltaPx = currentX - startMouseX
                    var deltaTime = deltaPx / pps
                    var newOut = startTimelineOut + deltaTime
                    // Don't let out-point pass in-point
                    if (newOut > timelineIn + 0.1) {
                        timelineNotifier.trimClip(clipId, timelineIn, newOut)
                    }
                }
            }
            onReleased: {
                console.log("[ClipWidget] right trim ended, clip:", clipId, "newOut:", timelineOut.toFixed(2))
            }
        }
    }

    // ================================================================
    // Selection glow (subtle outline when selected)
    // ================================================================
    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        radius: 5
        color: "transparent"
        border.color: isSelected ? Qt.rgba(0.424, 0.388, 1.0, 0.4) : "transparent"
        border.width: isSelected ? 1 : 0
        z: -1
        visible: isSelected
    }
}
