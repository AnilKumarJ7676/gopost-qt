import QtQuick 2.15
import QtQuick.Controls 2.15
import GopostApp 1.0

/// Canvas preview widget with pan/zoom gestures.
/// Renders canvas pixels via Image provider. Checkerboard background for transparency.
Item {
    id: root

    clip: true

    Rectangle {
        anchors.fill: parent
        color: "#121218"
    }

    // Pan/zoom container
    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: canvasContainer.width * pinchArea.scale
        contentHeight: canvasContainer.height * pinchArea.scale
        boundsBehavior: Flickable.StopAtBounds

        PinchArea {
            id: pinchArea
            anchors.fill: parent

            property real scale: 1.0
            property real minScale: 0.1
            property real maxScale: 10.0

            onPinchUpdated: function(pinch) {
                var newScale = scale * pinch.scale
                scale = Math.max(minScale, Math.min(maxScale, newScale))
            }

            onPinchFinished: {
                if (canvasNotifier) {
                    canvasNotifier.updateViewport(
                        flickable.contentX, flickable.contentY, scale)
                }
            }

            MouseArea {
                anchors.fill: parent
                onWheel: function(wheel) {
                    var factor = wheel.angleDelta.y > 0 ? 1.1 : 0.9
                    pinchArea.scale = Math.max(pinchArea.minScale,
                        Math.min(pinchArea.maxScale, pinchArea.scale * factor))
                    if (canvasNotifier) {
                        canvasNotifier.updateViewport(
                            flickable.contentX, flickable.contentY, pinchArea.scale)
                    }
                }
            }
        }

        Item {
            id: canvasContainer
            width: root.width
            height: root.height
            transform: Scale {
                origin.x: canvasContainer.width / 2
                origin.y: canvasContainer.height / 2
                xScale: pinchArea.scale
                yScale: pinchArea.scale
            }

            // Canvas surface (centered)
            Item {
                id: canvasSurface
                anchors.centerIn: parent
                width: canvasNotifier && canvasNotifier.hasCanvas
                       ? Math.min(parent.width * 0.9, parent.height * 0.9 * (canvasNotifier.canvasWidth / Math.max(1, canvasNotifier.canvasHeight)))
                       : 0
                height: canvasNotifier && canvasNotifier.hasCanvas
                        ? width * (canvasNotifier.canvasHeight / Math.max(1, canvasNotifier.canvasWidth))
                        : 0
                visible: canvasNotifier && canvasNotifier.hasCanvas

                // Shadow
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    color: "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.24)
                    border.width: 0.5

                    layer.enabled: true
                    layer.effect: Item {} // placeholder for shadow
                }

                // Checkerboard background
                Canvas {
                    id: checkerboard
                    anchors.fill: parent

                    onPaint: {
                        var ctx = getContext("2d")
                        var cellSize = 16
                        var lightColor = "#E8E8E8"
                        var darkColor = "#D0D0D0"

                        for (var y = 0; y < height; y += cellSize) {
                            for (var x = 0; x < width; x += cellSize) {
                                var isEven = (Math.floor(x / cellSize) + Math.floor(y / cellSize)) % 2 === 0
                                ctx.fillStyle = isEven ? lightColor : darkColor
                                ctx.fillRect(x, y, cellSize, cellSize)
                            }
                        }
                    }

                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                }

                // Rendered canvas image
                Image {
                    anchors.fill: parent
                    visible: canvasNotifier && canvasNotifier.layerCount > 0
                    fillMode: Image.Stretch
                    // Image source would be provided by a QQuickImageProvider
                    // keyed on canvasId + revision for cache-busting
                    source: canvasNotifier && canvasNotifier.hasCanvas
                            ? "image://canvas/" + canvasNotifier.canvasId + "/" + canvasNotifier.revision
                            : ""
                    cache: false
                }

                // Empty state overlay (no layers)
                Column {
                    anchors.centerIn: parent
                    spacing: 4
                    visible: canvasNotifier && canvasNotifier.layerCount === 0

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: canvasNotifier ? canvasNotifier.canvasWidth + " \u00D7 " + canvasNotifier.canvasHeight : ""
                        color: Qt.rgba(0, 0, 0, 0.38)
                        font.pixelSize: 18
                        font.weight: Font.Light
                    }
                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "0 layers"
                        color: Qt.rgba(0, 0, 0, 0.26)
                        font.pixelSize: 13
                    }
                }
            }

            // Empty state (no canvas at all)
            Column {
                anchors.centerIn: parent
                spacing: 12
                visible: !canvasNotifier || !canvasNotifier.hasCanvas

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "\uD83D\uDDBC"  // framed picture emoji
                    font.pixelSize: 64
                    opacity: 0.3
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Import a photo or create a blank canvas"
                    color: Qt.rgba(1, 1, 1, 0.4)
                    font.pixelSize: 14
                }
            }
        }
    }
}
