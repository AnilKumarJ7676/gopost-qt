import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var filter: null

    signal filterUpdated(var newFilter)

    implicitHeight: 36

    ListView {
        anchors.fill: parent
        orientation: ListView.Horizontal
        spacing: 8
        leftMargin: 16
        rightMargin: 16
        clip: true
        interactive: true

        model: ListModel {
            ListElement { label: "Video"; filterKey: "type"; filterValue: "video"; iconText: "\ue04b" }
            ListElement { label: "Image"; filterKey: "type"; filterValue: "image"; iconText: "\ue3f4" }
            ListElement { label: "Free"; filterKey: "premium"; filterValue: "false"; iconText: "\ue898" }
            ListElement { label: "Premium"; filterKey: "premium"; filterValue: "true"; iconText: "\ue8d0" }
        }

        delegate: Rectangle {
            id: filterChip
            required property string label
            required property string filterKey
            required property string filterValue
            required property string iconText
            required property int index

            property bool isSelected: {
                if (!root.filter) return false
                if (filterKey === "type")
                    return root.filter.type === filterValue
                if (filterKey === "premium") {
                    if (filterValue === "true") return root.filter.isPremium === true
                    if (filterValue === "false") return root.filter.isPremium === false
                }
                return false
            }

            width: chipRow.implicitWidth + 24
            height: 32
            radius: height / 2
            color: isSelected ? palette.alternateBase : "transparent"
            border.width: 1
            border.color: isSelected ? palette.highlight : palette.mid

            RowLayout {
                id: chipRow
                anchors.centerIn: parent
                spacing: 4

                Label {
                    text: iconText
                    font.pixelSize: 14
                    color: isSelected ? palette.highlight : palette.placeholderText
                }

                Label {
                    text: label
                    font.pixelSize: 12
                    font.weight: isSelected ? Font.DemiBold : Font.Medium
                    color: isSelected ? palette.highlight : palette.placeholderText
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!root.filter) return
                    var updated = Object.assign({}, root.filter)
                    if (filterKey === "type") {
                        updated.type = isSelected ? "" : filterValue
                        updated.cursor = ""
                    } else if (filterKey === "premium") {
                        if (isSelected) {
                            updated.isPremium = null
                        } else {
                            updated.isPremium = filterValue === "true"
                        }
                        updated.cursor = ""
                    }
                    root.filterUpdated(updated)
                }
            }
        }

        // Sort dropdown at end
        footer: Item {
            width: sortButton.width + 16
            height: 32

            Rectangle {
                id: sortButton
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 8
                width: sortRow.implicitWidth + 24
                height: 32
                radius: height / 2
                border.width: 1
                border.color: palette.mid
                color: "transparent"

                RowLayout {
                    id: sortRow
                    anchors.centerIn: parent
                    spacing: 4

                    Label {
                        text: "\ue164" // sort
                        font.pixelSize: 14
                        color: palette.placeholderText
                    }

                    Label {
                        text: {
                            if (!root.filter || !root.filter.sortBy) return "Popular"
                            var s = root.filter.sortBy
                            return s.charAt(0).toUpperCase() + s.slice(1)
                        }
                        font.pixelSize: 12
                        color: palette.placeholderText
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sortMenu.open()
                }

                Menu {
                    id: sortMenu
                    y: parent.height

                    MenuItem {
                        text: "Popular"
                        onTriggered: {
                            if (!root.filter) return
                            var updated = Object.assign({}, root.filter)
                            updated.sortBy = "popular"
                            updated.cursor = ""
                            root.filterUpdated(updated)
                        }
                    }

                    MenuItem {
                        text: "Newest"
                        onTriggered: {
                            if (!root.filter) return
                            var updated = Object.assign({}, root.filter)
                            updated.sortBy = "newest"
                            updated.cursor = ""
                            root.filterUpdated(updated)
                        }
                    }

                    MenuItem {
                        text: "Trending"
                        onTriggered: {
                            if (!root.filter) return
                            var updated = Object.assign({}, root.filter)
                            updated.sortBy = "trending"
                            updated.cursor = ""
                            root.filterUpdated(updated)
                        }
                    }
                }
            }
        }
    }
}
