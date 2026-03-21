import QtQuick
import QtQuick.Effects

Item {
    id: root

    property alias content: contentItem.children
    property bool isActive: false
    property color baseColor: "#6C63FF"
    property real glowRadius: 8
    property real glowSpread: 2
    property real glowBlur: 12

    implicitWidth: contentItem.childrenRect.width
    implicitHeight: contentItem.childrenRect.height

    // Glow effect layer
    Rectangle {
        id: glowRect
        anchors.fill: contentItem
        visible: root.isActive
        radius: root.glowRadius
        color: "transparent"

        layer.enabled: root.isActive
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 0.5
            blurMax: root.glowBlur
            shadowEnabled: true
            shadowColor: root.baseColor
            shadowBlur: 0.8
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 0
            shadowOpacity: 0.4
            shadowScale: 1.0 + root.glowSpread * 0.01
        }
    }

    // Content container
    Item {
        id: contentItem
        anchors.fill: parent
    }
}
