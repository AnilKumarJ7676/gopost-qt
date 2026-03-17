import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Bottom sheet for adding stickers to the image editor.
/// Displays category tabs and a grid of emoji stickers.
Popup {
    id: root

    signal stickerSelected(string emoji)

    anchors.centerIn: Overlay.overlay
    width: Math.min(parent.width - 32, 400)
    height: 400
    modal: true
    dim: true
    padding: 0

    property int selectedCategoryIndex: 0

    readonly property var categories: ["Smileys", "Animals", "Food", "Travel", "Objects", "Symbols"]

    readonly property var stickerMap: ({
        "Smileys": ["\uD83D\uDE00", "\uD83D\uDE03", "\uD83D\uDE04", "\uD83D\uDE01", "\uD83D\uDE06", "\uD83D\uDE05", "\uD83E\uDD23", "\uD83D\uDE02"],
        "Animals": ["\uD83D\uDC36", "\uD83D\uDC31", "\uD83D\uDC2D", "\uD83D\uDC39", "\uD83D\uDC30", "\uD83E\uDD8A", "\uD83D\uDC3B", "\uD83D\uDC3C"],
        "Food":    ["\uD83C\uDF4E", "\uD83C\uDF55", "\uD83C\uDF54", "\uD83C\uDF5F", "\uD83C\uDF70", "\uD83C\uDF69", "\uD83C\uDF6A", "\u2615"],
        "Travel":  ["\uD83C\uDFE0", "\u2708", "\uD83D\uDE97", "\uD83D\uDEA2", "\uD83C\uDFD6", "\uD83D\uDDFA", "\uD83E\uDDF3", "\uD83C\uDF92"],
        "Objects": ["\uD83C\uDFB5", "\uD83D\uDCF1", "\uD83D\uDCA1", "\uD83D\uDD11", "\uD83C\uDFA8", "\uD83D\uDCF7", "\uD83D\uDCDA", "\u270F"],
        "Symbols": ["\u2764", "\u2B50", "\uD83D\uDC8E", "\uD83D\uDD25", "\uD83C\uDF1F", "\u2705", "\u274C", "\uD83D\uDCAA"]
    })

    background: Rectangle {
        color: "#1E1E2E"
        radius: 16
    }

    contentItem: ColumnLayout {
        spacing: 0

        // Drag handle
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 20
            Rectangle {
                anchors.centerIn: parent
                width: 36; height: 4; radius: 2
                color: Qt.rgba(1, 1, 1, 0.24)
            }
        }

        // Title
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Add Sticker"
            color: "white"
            font.pixelSize: 18; font.weight: Font.DemiBold
        }

        Item { Layout.preferredHeight: 16 }

        // Category tabs
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.leftMargin: 8
            orientation: ListView.Horizontal
            clip: true
            spacing: 4

            model: root.categories

            delegate: Rectangle {
                width: catText.implicitWidth + 24
                height: 32
                radius: 8
                color: root.selectedCategoryIndex === index
                       ? Qt.rgba(0.42, 0.39, 1.0, 0.3) : "transparent"

                Label {
                    id: catText
                    anchors.centerIn: parent
                    text: modelData
                    color: root.selectedCategoryIndex === index
                           ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.7)
                    font.pixelSize: 14
                    font.weight: root.selectedCategoryIndex === index ? Font.DemiBold : Font.Normal
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.selectedCategoryIndex = index
                }
            }
        }

        Item { Layout.preferredHeight: 12 }

        // Sticker grid
        GridView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16; Layout.rightMargin: 16
            cellWidth: (width - 32) / 4
            cellHeight: cellWidth
            clip: true

            model: {
                var cat = root.categories[root.selectedCategoryIndex]
                return root.stickerMap[cat] || []
            }

            delegate: Rectangle {
                width: GridView.view ? GridView.view.cellWidth - 8 : 60
                height: width
                radius: 8
                color: Qt.rgba(1, 1, 1, 0.05)

                Label {
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 32
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.stickerSelected(modelData)
                        root.close()
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 16 }
    }
}
