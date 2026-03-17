import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

Page {
    id: root

    // templateListNotifier, homeNotifier, templateSearchNotifier available via context properties

    property bool isSearchActive: templateSearchNotifier &&
                                   templateSearchNotifier.query &&
                                   templateSearchNotifier.query.length > 0

    Component.onCompleted: {
        if (templateListNotifier)
            templateListNotifier.loadTemplates()
        if (homeNotifier)
            homeNotifier.load()
    }

    ScrollView {
        anchors.fill: parent

        Flickable {
            id: flickable
            contentWidth: parent.width
            contentHeight: contentColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.DragOverBounds

            // Pull-to-refresh
            onContentYChanged: {
                if (contentY < -80 && !refreshTimer.running && dragging) {
                    refreshTimer.start()
                    if (templateListNotifier)
                        templateListNotifier.refresh()
                }
                // Infinite scroll
                if (contentY + height >= contentHeight * 0.8 && !root.isSearchActive) {
                    if (templateListNotifier && !templateListNotifier.isLoadingMore)
                        templateListNotifier.loadMore()
                }
            }

            Timer {
                id: refreshTimer
                interval: 1000
            }

            ColumnLayout {
                id: contentColumn
                width: parent.width
                spacing: 0

                // Title
                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 12
                    text: "Templates"
                    font.pixelSize: 28
                    font.weight: Font.Bold
                }

                // Search Bar
                SearchBarWidget {
                    Layout.fillWidth: true

                    onSearchChanged: function(query) {
                        if (templateSearchNotifier)
                            templateSearchNotifier.search(query)
                    }

                    onSearchCleared: {
                        if (templateSearchNotifier)
                            templateSearchNotifier.clear()
                    }
                }

                // Category Chips (hidden during search)
                CategoryChips {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    visible: !root.isSearchActive
                    categories: homeNotifier ? homeNotifier.categories : []
                    selectedCategoryId: templateListNotifier && templateListNotifier.filter
                                        ? templateListNotifier.filter.categoryId || "" : ""

                    onCategorySelected: function(id) {
                        if (!templateListNotifier) return
                        if (id && id !== "") {
                            templateListNotifier.updateFilterCategory(id)
                        } else {
                            templateListNotifier.clearFilterCategory()
                        }
                    }
                }

                // Filter Bar (hidden during search)
                FilterBar {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    visible: !root.isSearchActive
                    filter: templateListNotifier ? templateListNotifier.filter : null

                    onFilterUpdated: function(newFilter) {
                        if (templateListNotifier)
                            templateListNotifier.updateFilter(newFilter)
                    }
                }

                Item {
                    Layout.preferredHeight: 16
                    visible: !root.isSearchActive
                }

                // === Search Results ===

                // Search shimmer loading
                Loader {
                    Layout.fillWidth: true
                    active: root.isSearchActive && templateSearchNotifier &&
                            templateSearchNotifier.isSearching &&
                            (!templateSearchNotifier.results || templateSearchNotifier.results.length === 0)
                    visible: active
                    sourceComponent: shimmerGridComponent
                }

                // Search empty state
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 64
                    spacing: 8
                    visible: root.isSearchActive && templateSearchNotifier &&
                             !templateSearchNotifier.isSearching &&
                             (!templateSearchNotifier.results || templateSearchNotifier.results.length === 0)

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "\ue9b6" // search_off
                        font.pixelSize: 64
                        color: palette.placeholderText
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "No templates found"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: palette.placeholderText
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Try adjusting your search terms"
                        font.pixelSize: 13
                        color: palette.placeholderText
                    }
                }

                // Search results grid
                GridView {
                    id: searchGrid
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    cellWidth: (width - 12) / 2
                    cellHeight: cellWidth / 0.65
                    interactive: false
                    visible: root.isSearchActive && templateSearchNotifier &&
                             templateSearchNotifier.results && templateSearchNotifier.results.length > 0
                    model: root.isSearchActive && templateSearchNotifier
                           ? templateSearchNotifier.results : []

                    delegate: Item {
                        required property var modelData
                        required property int index
                        width: searchGrid.cellWidth
                        height: searchGrid.cellHeight

                        TemplateCard {
                            anchors.fill: parent
                            anchors.rightMargin: index % 2 === 0 ? 6 : 0
                            anchors.leftMargin: index % 2 === 1 ? 6 : 0
                            anchors.bottomMargin: 16
                            template: modelData
                            onClicked: router.push("/templates/" + modelData.id)
                        }
                    }
                }

                // === Template List (non-search) ===

                // List shimmer loading
                Loader {
                    Layout.fillWidth: true
                    active: !root.isSearchActive && templateListNotifier &&
                            templateListNotifier.isLoading &&
                            (!templateListNotifier.templates || templateListNotifier.templates.length === 0)
                    visible: active
                    sourceComponent: shimmerGridComponent
                }

                // List error state
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 64
                    spacing: 8
                    visible: !root.isSearchActive && templateListNotifier &&
                             templateListNotifier.error &&
                             (!templateListNotifier.templates || templateListNotifier.templates.length === 0)

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "\ue000" // error_outline
                        font.pixelSize: 64
                        color: "#F44336"
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: templateListNotifier ? templateListNotifier.error : ""
                        font.pixelSize: 14
                    }

                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Retry"
                        highlighted: true
                        onClicked: {
                            if (templateListNotifier)
                                templateListNotifier.refresh()
                        }
                    }
                }

                // Template grid
                GridView {
                    id: templateGrid
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    cellWidth: (width - 12) / 2
                    cellHeight: cellWidth / 0.65
                    interactive: false
                    visible: !root.isSearchActive && templateListNotifier &&
                             templateListNotifier.templates && templateListNotifier.templates.length > 0
                    model: !root.isSearchActive && templateListNotifier
                           ? templateListNotifier.templates : []

                    delegate: Item {
                        required property var modelData
                        required property int index
                        width: templateGrid.cellWidth
                        height: templateGrid.cellHeight

                        TemplateCard {
                            anchors.fill: parent
                            anchors.rightMargin: index % 2 === 0 ? 6 : 0
                            anchors.leftMargin: index % 2 === 1 ? 6 : 0
                            anchors.bottomMargin: 16
                            template: modelData
                            onClicked: router.push("/templates/" + modelData.id)
                        }
                    }
                }

                // Load more indicator
                BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 16
                    visible: !root.isSearchActive && templateListNotifier &&
                             templateListNotifier.isLoadingMore
                    running: visible
                    width: 32; height: 32
                }

                // End of list message
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    spacing: 4
                    visible: !root.isSearchActive && templateListNotifier &&
                             !templateListNotifier.hasMore &&
                             templateListNotifier.templates && templateListNotifier.templates.length > 0

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "\ue86c" // check_circle_outline
                        font.pixelSize: 20
                        color: palette.placeholderText
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "You've seen all templates"
                        font.pixelSize: 13
                        color: palette.placeholderText
                    }
                }

                // Bottom spacer
                Item { Layout.preferredHeight: 48 }
            }
        }
    }

    // Shimmer Grid Component
    Component {
        id: shimmerGridComponent

        Item {
            implicitHeight: shimmerGrid.implicitHeight

            GridLayout {
                id: shimmerGrid
                width: parent.width - 32
                anchors.horizontalCenter: parent.horizontalCenter
                columns: 2
                columnSpacing: 12
                rowSpacing: 16

                Repeater {
                    model: 6

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ShimmerLoading {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 180
                            radius: 12
                        }

                        ShimmerLoading {
                            implicitWidth: parent.width * 0.7
                            implicitHeight: 14
                            radius: 4
                        }

                        ShimmerLoading {
                            implicitWidth: parent.width * 0.4
                            implicitHeight: 12
                            radius: 4
                        }
                    }
                }
            }
        }
    }
}
