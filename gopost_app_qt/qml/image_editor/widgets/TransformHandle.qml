import QtQuick 2.15

/// Single resize handle used by TransformOverlay.
/// Tracks drag distance from a pivot point to compute scale factor.
Rectangle {
    id: handle

    property real handleX: 0
    property real handleY: 0
    property real handleSize: 10
    property real pivotX: 0
    property real pivotY: 0

    signal scaleChanged(real scaleFactor)

    x: handleX - handleSize / 2
    y: handleY - handleSize / 2
    width: handleSize
    height: handleSize
    radius: handleSize / 2
    color: "white"
    border.color: "#6C63FF"
    border.width: 1

    property real _startHandleX: 0
    property real _startHandleY: 0
    property bool _dragging: false

    MouseArea {
        anchors.fill: parent
        anchors.margins: -4 // expand hit area

        onPressed: function(mouse) {
            handle._startHandleX = handle.handleX
            handle._startHandleY = handle.handleY
            handle._dragging = true
        }
        onPositionChanged: function(mouse) {
            if (!handle._dragging) return

            // Map mouse position to overlay (parent of handle) coordinates
            var overlayPos = mapToItem(handle.parent, mouse.x, mouse.y)
            var newX = overlayPos.x
            var newY = overlayPos.y

            var startDx = handle._startHandleX - handle.pivotX
            var startDy = handle._startHandleY - handle.pivotY
            var startDist = Math.sqrt(startDx * startDx + startDy * startDy)

            var newDx = newX - handle.pivotX
            var newDy = newY - handle.pivotY
            var newDist = Math.sqrt(newDx * newDx + newDy * newDy)

            if (startDist > 0) {
                var factor = newDist / startDist
                handle.scaleChanged(factor)
                handle._startHandleX = newX
                handle._startHandleY = newY
            }
        }
        onReleased: handle._dragging = false
    }
}
