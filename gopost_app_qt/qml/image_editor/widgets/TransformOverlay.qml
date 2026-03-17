import QtQuick 2.15
import GopostApp 1.0

/// Overlay widget that shows transform handles for the selected layer.
/// Displays 8 resize handles, a rotation handle, and a dashed border.
Item {
    id: root

    // Layer bounds in canvas-preview coordinates
    property rect layerBounds: Qt.rect(0, 0, 0, 0)

    signal moveStarted()
    signal moved(real dx, real dy)
    signal scaled(real scaleFactor)
    signal rotated(real angleDelta)

    readonly property real handleSize: 10
    readonly property real rotationHandleSize: 8
    readonly property real rotationHandleOffset: 20

    visible: layerBounds.width > 0 && layerBounds.height > 0

    // Dashed border + rotation line + handle circles
    Canvas {
        id: overlayCanvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            if (!root.visible) return

            var r = root.layerBounds

            // Dashed border
            ctx.strokeStyle = "white"
            ctx.lineWidth = 1
            ctx.setLineDash([4, 3])
            ctx.strokeRect(r.x, r.y, r.width, r.height)

            // Rotation line
            ctx.setLineDash([])
            var topCenterX = r.x + r.width / 2
            var topCenterY = r.y
            var rotHandleY = topCenterY - root.rotationHandleOffset
            ctx.beginPath()
            ctx.moveTo(topCenterX, topCenterY)
            ctx.lineTo(topCenterX, rotHandleY)
            ctx.stroke()

            // Handle positions
            var positions = [
                [r.x, r.y],
                [r.x + r.width / 2, r.y],
                [r.x + r.width, r.y],
                [r.x + r.width, r.y + r.height / 2],
                [r.x + r.width, r.y + r.height],
                [r.x + r.width / 2, r.y + r.height],
                [r.x, r.y + r.height],
                [r.x, r.y + r.height / 2]
            ]
            for (var i = 0; i < positions.length; ++i) {
                var px = positions[i][0]
                var py = positions[i][1]
                ctx.fillStyle = "white"
                ctx.beginPath()
                ctx.arc(px, py, root.handleSize / 2, 0, 2 * Math.PI)
                ctx.fill()
                ctx.strokeStyle = "#42A5F5"
                ctx.lineWidth = 1
                ctx.stroke()
            }

            // Rotation handle
            ctx.fillStyle = "white"
            ctx.beginPath()
            ctx.arc(topCenterX, rotHandleY, root.rotationHandleSize / 2, 0, 2 * Math.PI)
            ctx.fill()
            ctx.strokeStyle = "#42A5F5"
            ctx.lineWidth = 1
            ctx.stroke()
        }
    }

    onLayerBoundsChanged: overlayCanvas.requestPaint()

    // Move area (inside the bounds)
    MouseArea {
        id: moveArea
        x: root.layerBounds.x
        y: root.layerBounds.y
        width: root.layerBounds.width
        height: root.layerBounds.height

        property real lastX: 0
        property real lastY: 0

        onPressed: function(mouse) {
            lastX = mouse.x
            lastY = mouse.y
            root.moveStarted()
        }

        onPositionChanged: function(mouse) {
            var dx = mouse.x - lastX
            var dy = mouse.y - lastY
            lastX = mouse.x
            lastY = mouse.y
            root.moved(dx, dy)
        }
    }

    // Resize handles (8 corners/edges)
    Repeater {
        model: 8

        MouseArea {
            id: resizeHandle
            width: root.handleSize + 6
            height: root.handleSize + 6

            property int handleIndex: index
            property real startDist: 0
            property point pivot: Qt.point(0, 0)

            x: internal.handleX(index) - width / 2
            y: internal.handleY(index) - height / 2

            onPressed: function(mouse) {
                var hx = internal.handleX(handleIndex)
                var hy = internal.handleY(handleIndex)
                var px = internal.pivotX(handleIndex)
                var py = internal.pivotY(handleIndex)
                pivot = Qt.point(px, py)
                startDist = Math.sqrt(Math.pow(hx - px, 2) + Math.pow(hy - py, 2))
            }

            onPositionChanged: function(mouse) {
                if (startDist <= 0) return
                var globalX = resizeHandle.x + mouse.x + resizeHandle.width / 2
                var globalY = resizeHandle.y + mouse.y + resizeHandle.height / 2
                var newDist = Math.sqrt(Math.pow(globalX - pivot.x, 2) + Math.pow(globalY - pivot.y, 2))
                var factor = newDist / startDist
                root.scaled(factor)
                startDist = newDist
            }
        }
    }

    // Rotation handle
    MouseArea {
        id: rotationHandle
        width: root.rotationHandleSize + 8
        height: root.rotationHandleSize + 8
        x: root.layerBounds.x + root.layerBounds.width / 2 - width / 2
        y: root.layerBounds.y - root.rotationHandleOffset - height / 2

        property real lastAngle: 0

        onPressed: function(mouse) {
            var cx = root.layerBounds.x + root.layerBounds.width / 2
            var cy = root.layerBounds.y + root.layerBounds.height / 2
            var gx = rotationHandle.x + mouse.x
            var gy = rotationHandle.y + mouse.y
            lastAngle = Math.atan2(gy - cy, gx - cx)
        }

        onPositionChanged: function(mouse) {
            var cx = root.layerBounds.x + root.layerBounds.width / 2
            var cy = root.layerBounds.y + root.layerBounds.height / 2
            var gx = rotationHandle.x + mouse.x
            var gy = rotationHandle.y + mouse.y
            var currentAngle = Math.atan2(gy - cy, gx - cx)
            var delta = currentAngle - lastAngle

            // Normalize to [-PI, PI]
            while (delta > Math.PI) delta -= 2 * Math.PI
            while (delta < -Math.PI) delta += 2 * Math.PI

            root.rotated(delta)
            lastAngle = currentAngle
        }
    }

    QtObject {
        id: internal

        function handleX(idx) {
            var r = root.layerBounds
            switch (idx) {
            case 0: return r.x
            case 1: return r.x + r.width / 2
            case 2: return r.x + r.width
            case 3: return r.x + r.width
            case 4: return r.x + r.width
            case 5: return r.x + r.width / 2
            case 6: return r.x
            case 7: return r.x
            default: return 0
            }
        }

        function handleY(idx) {
            var r = root.layerBounds
            switch (idx) {
            case 0: return r.y
            case 1: return r.y
            case 2: return r.y
            case 3: return r.y + r.height / 2
            case 4: return r.y + r.height
            case 5: return r.y + r.height
            case 6: return r.y + r.height
            case 7: return r.y + r.height / 2
            default: return 0
            }
        }

        // Pivot is the opposite corner/edge
        function pivotX(idx) {
            var r = root.layerBounds
            switch (idx) {
            case 0: return r.x + r.width
            case 1: return r.x + r.width / 2
            case 2: return r.x
            case 3: return r.x
            case 4: return r.x
            case 5: return r.x + r.width / 2
            case 6: return r.x + r.width
            case 7: return r.x + r.width
            default: return 0
            }
        }

        function pivotY(idx) {
            var r = root.layerBounds
            switch (idx) {
            case 0: return r.y + r.height
            case 1: return r.y + r.height
            case 2: return r.y + r.height
            case 3: return r.y + r.height / 2
            case 4: return r.y
            case 5: return r.y
            case 6: return r.y
            case 7: return r.y + r.height / 2
            default: return 0
            }
        }
    }
}
