import QtQuick

Rectangle {
    id: overlay
    width: overlayText.implicitWidth + 24
    height: overlayText.implicitHeight + 12
    radius: 6
    color: "#000000AA"
    visible: diagnosticOverlay ? diagnosticOverlay.visible : false

    Text {
        id: overlayText
        anchors.centerIn: parent
        font.family: "Consolas"
        font.pixelSize: 12
        color: "#00FF88"
        text: diagnosticOverlay
              ? "FPS: " + diagnosticOverlay.fps.toFixed(1)
                + " | Frame: " + diagnosticOverlay.frameTime.toFixed(1) + "ms"
                + " | Dropped: " + diagnosticOverlay.droppedFrames
                + " | Mem: " + diagnosticOverlay.memoryUsed
              : "FPS: --"
    }
}
