import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var template: null

    signal clicked()

    implicitHeight: cardColumn.implicitHeight

    ColumnLayout {
        id: cardColumn
        anchors.fill: parent
        spacing: 8

        // Thumbnail with overlays
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: {
                if (!root.template || !root.template.width || !root.template.height)
                    return width
                return width * (root.template.height / Math.max(root.template.width, 1))
            }

            Rectangle {
                anchors.fill: parent
                radius: 12
                clip: true
                color: palette.alternateBase

                Image {
                    anchors.fill: parent
                    source: root.template && root.template.thumbnailUrl
                            ? root.template.thumbnailUrl : ""
                    fillMode: Image.PreserveAspectCrop
                    visible: source !== ""
                    asynchronous: true
                }

                // Placeholder icon
                Label {
                    visible: !(root.template && root.template.thumbnailUrl)
                    anchors.centerIn: parent
                    text: root.template && root.template.isVideo ? "\ue04b" : "\ue3f4"
                    font.pixelSize: 32
                    color: palette.placeholderText
                }
            }

            // PRO badge
            Rectangle {
                visible: root.template && root.template.isPremium ? true : false
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 8
                anchors.rightMargin: 8
                width: proBadgeLabel.implicitWidth + 12
                height: proBadgeLabel.implicitHeight + 4
                radius: 6
                color: palette.highlight

                Label {
                    id: proBadgeLabel
                    anchors.centerIn: parent
                    text: "PRO"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 0.5
                    color: "white"
                }
            }

            // Duration badge (video only)
            Rectangle {
                visible: root.template && root.template.isVideo &&
                         root.template.durationMs ? true : false
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.bottomMargin: 8
                anchors.rightMargin: 8
                width: durationLabel.implicitWidth + 8
                height: durationLabel.implicitHeight + 4
                radius: 4
                color: Qt.rgba(0, 0, 0, 0.7)

                Label {
                    id: durationLabel
                    anchors.centerIn: parent
                    text: {
                        if (!root.template || !root.template.durationMs) return ""
                        var seconds = Math.round(root.template.durationMs / 1000)
                        var m = Math.floor(seconds / 60)
                        var s = seconds % 60
                        return m + ":" + (s < 10 ? "0" + s : s)
                    }
                    font.pixelSize: 11
                    color: "white"
                }
            }

            // Play button overlay (video only)
            Rectangle {
                visible: root.template && root.template.isVideo ? true : false
                anchors.centerIn: parent
                width: 36; height: 36
                radius: 18
                color: Qt.rgba(0, 0, 0, 0.4)

                Label {
                    anchors.centerIn: parent
                    text: "\ue037" // play_arrow_rounded
                    font.pixelSize: 24
                    color: "white"
                }
            }
        }

        // Info section
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 2
            Layout.rightMargin: 2
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.template ? root.template.name || "" : ""
                font.pixelSize: 14
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: root.template && root.template.isVideo ? "\ue04b" : "\ue3f4"
                    font.pixelSize: 12
                    color: palette.placeholderText
                }

                Label {
                    Layout.fillWidth: true
                    text: root.template
                          ? (root.template.categoryName || (root.template.isVideo ? "video" : "image"))
                          : ""
                    font.pixelSize: 11
                    color: palette.placeholderText
                    elide: Text.ElideRight
                }

                Label {
                    text: "\ue87d" // favorite_outline
                    font.pixelSize: 12
                    color: palette.placeholderText
                }

                Label {
                    text: {
                        if (!root.template) return "0"
                        var count = root.template.usageCount || 0
                        if (count >= 1000000) return (count / 1000000).toFixed(1) + "M"
                        if (count >= 1000) return (count / 1000).toFixed(1) + "K"
                        return "" + count
                    }
                    font.pixelSize: 11
                    color: palette.placeholderText
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
