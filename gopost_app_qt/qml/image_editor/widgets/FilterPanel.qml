import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Bottom panel for browsing and applying preset filters.
/// Category tabs, horizontal filter grid, intensity slider.
Rectangle {
    id: root

    property int selectedCategoryIndex: 0

    height: 280
    color: "#1E1E2E"

    Rectangle {
        anchors.top: parent.top
        width: parent.width; height: 0.5
        color: Qt.rgba(1, 1, 1, 0.1)
    }

    Component.onCompleted: {
        if (filterNotifier) filterNotifier.loadPresets()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Category tabs
        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            contentWidth: categoryRow.width
            flickableDirection: Flickable.HorizontalFlick
            clip: true

            Row {
                id: categoryRow
                leftPadding: 12; rightPadding: 12; topPadding: 8; bottomPadding: 8
                spacing: 8

                Repeater {
                    model: filterNotifier ? filterNotifier.categories : []

                    delegate: Rectangle {
                        width: catLabel.implicitWidth + 24
                        height: 28
                        radius: 14
                        color: root.selectedCategoryIndex === index
                               ? Qt.rgba(0.42, 0.39, 1.0, 0.3)
                               : Qt.rgba(1, 1, 1, 0.08)
                        border.color: root.selectedCategoryIndex === index
                                      ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.24)
                        border.width: root.selectedCategoryIndex === index ? 1.5 : 1

                        Label {
                            id: catLabel
                            anchors.centerIn: parent
                            text: modelData.name
                            color: root.selectedCategoryIndex === index ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.7)
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.selectedCategoryIndex = index
                        }
                    }
                }
            }
        }

        // Loading
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            running: filterNotifier ? filterNotifier.isLoading : false
            visible: running
        }

        // Empty state
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: filterNotifier
                     && !filterNotifier.isLoading
                     && (!filterNotifier.categories || filterNotifier.categories.length === 0)
            text: "No filters available"
            color: Qt.rgba(1, 1, 1, 0.38)
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
        }

        // Filter grid (horizontal)
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: filterNotifier
                     && !filterNotifier.isLoading
                     && filterNotifier.categories
                     && filterNotifier.categories.length > 0
            orientation: ListView.Horizontal
            clip: true
            spacing: 12

            model: {
                if (!filterNotifier || !filterNotifier.categories
                        || root.selectedCategoryIndex >= filterNotifier.categories.length)
                    return []
                return filterNotifier.categories[root.selectedCategoryIndex].filters
            }

            delegate: Item {
                width: 80
                height: parent ? parent.height : 100

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Rectangle {
                        width: 72; height: 72
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.1)
                        border.color: filterNotifier && filterNotifier.activePresetIndex === modelData.index
                                      ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.24)
                        border.width: filterNotifier && filterNotifier.activePresetIndex === modelData.index ? 2 : 1
                        anchors.horizontalCenter: parent.horizontalCenter

                        // Glow for active
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -2
                            radius: parent.radius + 2
                            color: "transparent"
                            border.color: filterNotifier && filterNotifier.activePresetIndex === modelData.index
                                          ? Qt.rgba(0.42, 0.39, 1.0, 0.3) : "transparent"
                            border.width: 2
                            visible: filterNotifier && filterNotifier.activePresetIndex === modelData.index
                        }

                        Label {
                            anchors.centerIn: parent
                            width: parent.width - 8
                            text: modelData.name
                            color: filterNotifier && filterNotifier.activePresetIndex === modelData.index
                                   ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.7)
                            font.pixelSize: 10
                            font.weight: filterNotifier && filterNotifier.activePresetIndex === modelData.index
                                         ? Font.DemiBold : Font.Normal
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (!filterNotifier || !canvasNotifier) return
                                if (canvasNotifier.canvasId < 0 || canvasNotifier.selectedLayerId < 0) return
                                filterNotifier.selectPreset(
                                    canvasNotifier.canvasId,
                                    canvasNotifier.selectedLayerId,
                                    modelData.index)
                            }
                        }
                    }

                    Label {
                        width: 72
                        text: modelData.name
                        color: Qt.rgba(1, 1, 1, 0.54)
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }

        // Intensity slider
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12; Layout.rightMargin: 12
            Layout.bottomMargin: 12; Layout.topMargin: 8
            visible: filterNotifier && filterNotifier.activePresetIndex !== undefined
                     && filterNotifier.activePresetIndex >= 0
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Intensity"
                    color: Qt.rgba(1, 1, 1, 0.7)
                    font.pixelSize: 12
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: filterNotifier ? Math.round(filterNotifier.intensity) : "0"
                    color: "#6C63FF"
                    font.pixelSize: 12; font.weight: Font.DemiBold
                }
            }

            Slider {
                Layout.fillWidth: true
                from: 0; to: 100
                value: filterNotifier ? filterNotifier.intensity : 0
                onMoved: {
                    if (filterNotifier && canvasNotifier
                            && canvasNotifier.canvasId >= 0
                            && canvasNotifier.selectedLayerId >= 0) {
                        filterNotifier.setIntensity(
                            canvasNotifier.canvasId,
                            canvasNotifier.selectedLayerId,
                            value)
                    }
                }
            }
        }
    }
}
