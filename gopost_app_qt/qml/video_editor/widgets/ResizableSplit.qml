import QtQuick
import QtQuick.Controls

/// Resizable split panel — supports vertical (top/bottom) and horizontal (left/right) orientation.
/// Used by VideoEditorScreen for the main preview/timeline split.
Item {
    id: root

    /// Qt.Vertical = top/bottom split, Qt.Horizontal = left/right split
    property int orientation: Qt.Vertical

    /// Fraction of total space allocated to the first panel (top or left)
    property real fraction: 0.5
    property real minFraction: 0.15
    property real maxFraction: 0.85

    /// Content items — assign inline QML items to these
    property Item topItem
    property Item bottomItem

    /// Emitted when fraction changes via drag
    signal fractionChanged(real f)

    readonly property real _splitterThickness: 6

    onTopItemChanged: {
        if (topItem) {
            topItem.parent = topContainer
            topItem.anchors.fill = topContainer
        }
    }
    onBottomItemChanged: {
        if (bottomItem) {
            bottomItem.parent = bottomContainer
            bottomItem.anchors.fill = bottomContainer
        }
    }

    Component.onCompleted: {
        if (topItem) {
            topItem.parent = topContainer
            topItem.anchors.fill = topContainer
        }
        if (bottomItem) {
            bottomItem.parent = bottomContainer
            bottomItem.anchors.fill = bottomContainer
        }
    }

    // ---- Vertical split (top/bottom) ----
    Column {
        anchors.fill: parent
        visible: root.orientation === Qt.Vertical

        Item {
            id: topContainer
            width: parent.width
            height: {
                var available = root.height - root._splitterThickness
                return Math.max(0, available * root.fraction)
            }
        }

        Rectangle {
            width: parent.width
            height: root._splitterThickness
            color: vSplitterMouse.containsMouse || vSplitterMouse.pressed ? "#6C63FF" : "#252540"

            Rectangle {
                anchors.centerIn: parent
                width: 40; height: 2; radius: 1
                color: vSplitterMouse.containsMouse || vSplitterMouse.pressed
                       ? Qt.rgba(1, 1, 1, 0.6) : "#404060"
            }

            MouseArea {
                id: vSplitterMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeVerCursor

                property real dragStartY: 0
                property real dragStartFrac: 0

                onPressed: mouse => {
                    dragStartY = mouse.y
                    dragStartFrac = root.fraction
                }
                onPositionChanged: mouse => {
                    if (pressed) {
                        var delta = (mouse.y - dragStartY) / (root.height - root._splitterThickness)
                        var newFrac = Math.max(root.minFraction, Math.min(root.maxFraction, dragStartFrac + delta))
                        root.fraction = newFrac
                        root.fractionChanged(newFrac)
                    }
                }
            }
        }

        Item {
            id: bottomContainer
            width: parent.width
            height: {
                var available = root.height - root._splitterThickness
                return Math.max(0, available - available * root.fraction)
            }
        }
    }
}
