import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Screen listing saved video projects.
Page {
    id: root

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8; anchors.rightMargin: 8

            ToolButton {
                icon.name: "go-previous"
                onClicked: router.pop()
            }
            Label {
                text: "My Projects"
                font.pixelSize: 20
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            ToolButton {
                icon.name: "view-refresh"
                ToolTip.text: "Refresh"
                ToolTip.visible: hovered
                onClicked: projectListNotifier.loadProjects()
            }
        }
    }

    // Floating action button
    RoundButton {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        z: 10
        width: 56; height: 56
        icon.name: "list-add"
        icon.color: "white"

        background: Rectangle {
            radius: 28
            color: "#6C63FF"
        }

        onClicked: router.push("/editor/video")
    }

    // Content area
    Item {
        anchors.fill: parent

        // Loading state
        BusyIndicator {
            anchors.centerIn: parent
            running: projectListNotifier.isLoading
            visible: running
        }

        // Error state
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 12
            visible: !projectListNotifier.isLoading && projectListNotifier.error.length > 0

            Label {
                text: "\u26A0"
                font.pixelSize: 48
                color: Theme.semanticError
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: projectListNotifier.error
                color: Theme.semanticError
                Layout.alignment: Qt.AlignHCenter
            }
            Button {
                text: "Retry"
                Layout.alignment: Qt.AlignHCenter
                onClicked: projectListNotifier.loadProjects()
            }
        }

        // Empty state
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 16
            visible: !projectListNotifier.isLoading && projectListNotifier.error.length === 0 && projectListNotifier.projectCount === 0

            Label {
                text: "\uD83C\uDFA5"
                font.pixelSize: 64
                opacity: 0.5
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: "No saved projects"
                font.pixelSize: 18
                font.weight: Font.Medium
                opacity: 0.6
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: "Create a new video project to get started."
                font.pixelSize: 14
                opacity: 0.5
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // Project list
        ListView {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10
            visible: !projectListNotifier.isLoading && projectListNotifier.projectCount > 0
            model: projectListNotifier.projects
            clip: true

            delegate: Rectangle {
                width: ListView.view.width
                height: 80
                radius: 12
                color: Theme.editorSurface
                border.color: "#303050"
                border.width: 0.5

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: router.push("/editor/video?projectId=" + model.id)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 14

                    Rectangle {
                        width: 56; height: 56
                        radius: 10
                        color: "#26C6DA"

                        Label {
                            anchors.centerIn: parent
                            text: "\uD83C\uDFAC"
                            font.pixelSize: 24
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: model.name
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: model.trackCount + " tracks  \u2022  " + model.clipCount + " clips  \u2022  " + model.durationSeconds.toFixed(1) + "s"
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                        Label {
                            text: "Last edited: " + model.updatedAt
                            font.pixelSize: 11
                            opacity: 0.5
                        }
                    }

                    ToolButton {
                        icon.name: "overflow-menu"
                        onClicked: projectMenu.open()

                        Menu {
                            id: projectMenu
                            MenuItem {
                                text: "Delete"
                                onTriggered: deleteDialog.projectId = model.id
                            }
                        }
                    }
                }
            }
        }
    }

    // Delete confirmation dialog
    Dialog {
        id: deleteDialog
        property string projectId: ""
        anchors.centerIn: parent
        title: "Delete Project"
        modal: true
        standardButtons: Dialog.Cancel

        onProjectIdChanged: if (projectId.length > 0) open()

        Label {
            text: "Are you sure you want to delete this project? This cannot be undone."
            wrapMode: Text.Wrap
        }

        footer: DialogButtonBox {
            Button {
                text: "Cancel"
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            Button {
                text: "Delete"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

                background: Rectangle {
                    radius: 4
                    color: Theme.semanticError
                }
                contentItem: Label {
                    text: "Delete"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        onAccepted: {
            projectListNotifier.deleteProject(deleteDialog.projectId)
            deleteDialog.projectId = ""
        }
        onRejected: deleteDialog.projectId = ""
    }
}
