import QtQuick

/**
 * Playhead — triangle head + vertical line, draggable.
 *
 * Converted 1:1 from playhead_widget.dart.
 */
Item {
    id: root

    width: 1
    z: 100

    readonly property color headColor: "#6C63FF"

    // Triangle head
    Canvas {
        id: head
        width: 14
        height: 10
        x: -7
        y: 0

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.fillStyle = headColor;
            ctx.beginPath();
            ctx.moveTo(0, 0);
            ctx.lineTo(14, 0);
            ctx.lineTo(7, 10);
            ctx.closePath();
            ctx.fill();
        }
    }

    // Vertical line
    Rectangle {
        width: 1
        anchors.top: head.bottom
        anchors.bottom: parent.bottom
        color: headColor
    }

    // Drag area (wider hit target)
    MouseArea {
        width: 20
        height: parent.height
        x: -10
        cursorShape: Qt.SizeHorCursor

        property real startX: 0

        onPressed: mouse => { startX = mouse.x; }
        onPositionChanged: mouse => {
            if (pressed) {
                var delta = (mouse.x - startX) / timelineNotifier.pixelsPerSecond;
                var newPos = timelineNotifier.position + delta;
                timelineNotifier.scrubTo(Math.max(0, newPos));
            }
        }
    }
}
