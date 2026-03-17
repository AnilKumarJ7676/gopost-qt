import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property int itemCount: 6

    color: palette.alternateBase
    clip: true

    // Shimmer highlight that slides across
    Rectangle {
        id: shimmerHighlight
        width: root.width * 0.4
        height: root.height
        radius: root.radius
        opacity: 0.5

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.15) }
            GradientStop { position: 1.0; color: "transparent" }
        }

        SequentialAnimation on x {
            loops: Animation.Infinite

            NumberAnimation {
                from: -shimmerHighlight.width
                to: root.width
                duration: 1500
                easing.type: Easing.InOutQuad
            }

            PauseAnimation { duration: 200 }
        }
    }
}
