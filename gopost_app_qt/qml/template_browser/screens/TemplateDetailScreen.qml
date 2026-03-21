import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

Page {
    id: root

    required property string templateId
    // templateDetailNotifier, downloadNotifier, authNotifier available via context properties

    Component.onCompleted: {
        if (templateDetailNotifier)
            templateDetailNotifier.load(templateId)
    }

    Connections {
        target: downloadNotifier
        enabled: downloadNotifier !== null

        function onStatusChanged() {
            if (!downloadNotifier || downloadNotifier.status !== "complete") return
            var tmpl = templateDetailNotifier ? templateDetailNotifier.template : null
            if (!tmpl) return
            var route
            if (tmpl.editableFields && tmpl.editableFields.length > 0) {
                route = "/editor/customize/" + tmpl.id
            } else {
                route = tmpl.isVideo ? "/editor/video2" : "/editor/image"
            }
            router.push(route)
            downloadNotifier.reset()
        }
    }

    // Loading state
    Loader {
        anchors.fill: parent
        active: templateDetailNotifier && templateDetailNotifier.isLoading
        visible: active
        sourceComponent: ColumnLayout {
            spacing: 16

            ShimmerLoading {
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                radius: 0
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 8

                ShimmerLoading { implicitWidth: 200; implicitHeight: 24 }
                ShimmerLoading { implicitWidth: 300; implicitHeight: 16 }
                Item { Layout.preferredHeight: 8 }
                ShimmerLoading {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                }
            }
        }
    }

    // Error state
    Loader {
        anchors.centerIn: parent
        active: templateDetailNotifier && templateDetailNotifier.error &&
                !templateDetailNotifier.template
        visible: active
        sourceComponent: ColumnLayout {
            spacing: 16

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "\ue000" // error_outline
                font.pixelSize: 64
                color: "#F44336"
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: templateDetailNotifier ? templateDetailNotifier.error : ""
                font.pixelSize: 14
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: "Retry"
                highlighted: true
                onClicked: {
                    if (templateDetailNotifier)
                        templateDetailNotifier.load(templateId)
                }
            }
        }
    }

    // Detail Content
    Flickable {
        id: detailFlickable
        visible: templateDetailNotifier && templateDetailNotifier.template ? true : false
        anchors.fill: parent
        anchors.bottomMargin: bottomBar.visible ? bottomBar.height : 0
        contentWidth: parent.width
        contentHeight: detailColumn.implicitHeight
        clip: true

        ColumnLayout {
            id: detailColumn
            width: parent.width
            spacing: 0

            // Preview area
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 400

                PreviewPlayer {
                    anchors.fill: parent
                    previewUrl: templateDetailNotifier && templateDetailNotifier.template
                                ? templateDetailNotifier.template.previewUrl || "" : ""
                    thumbnailUrl: templateDetailNotifier && templateDetailNotifier.template
                                  ? templateDetailNotifier.template.thumbnailUrl || "" : ""
                    isVideo: templateDetailNotifier && templateDetailNotifier.template
                             ? templateDetailNotifier.template.isVideo : false
                    videoWidth: templateDetailNotifier && templateDetailNotifier.template
                                ? templateDetailNotifier.template.width : 1080
                    videoHeight: templateDetailNotifier && templateDetailNotifier.template
                                 ? templateDetailNotifier.template.height : 1920
                }

                // Back button overlay
                Rectangle {
                    x: 16; y: 16
                    width: 40; height: 40
                    radius: 20
                    color: Qt.rgba(0, 0, 0, 0.4)

                    Label {
                        anchors.centerIn: parent
                        text: "\ue5c4" // arrow_back
                        font.pixelSize: 20
                        color: "white"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: router.pop()
                    }
                }

                // Share button overlay
                Rectangle {
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    y: 16
                    width: 40; height: 40
                    radius: 20
                    color: Qt.rgba(0, 0, 0, 0.4)

                    Label {
                        anchors.centerIn: parent
                        text: "\ue80d" // share
                        font.pixelSize: 20
                        color: "white"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // TODO: share functionality
                        }
                    }
                }
            }

            // Details section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                spacing: 16

                // Header: Name + info chips + PRO badge
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            Layout.fillWidth: true
                            text: templateDetailNotifier && templateDetailNotifier.template
                                  ? templateDetailNotifier.template.name : ""
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            wrapMode: Text.Wrap
                        }

                        RowLayout {
                            spacing: 8

                            Rectangle {
                                visible: templateDetailNotifier && templateDetailNotifier.template &&
                                         templateDetailNotifier.template.categoryName ? true : false
                                width: catNameLabel.implicitWidth + 16
                                height: catNameLabel.implicitHeight + 8
                                radius: 6
                                color: palette.alternateBase

                                Label {
                                    id: catNameLabel
                                    anchors.centerIn: parent
                                    text: templateDetailNotifier && templateDetailNotifier.template
                                          ? templateDetailNotifier.template.categoryName || "" : ""
                                    font.pixelSize: 11
                                    color: palette.placeholderText
                                }
                            }

                            Rectangle {
                                width: typeRow.implicitWidth + 16
                                height: typeRow.implicitHeight + 8
                                radius: 6
                                color: palette.alternateBase

                                RowLayout {
                                    id: typeRow
                                    anchors.centerIn: parent
                                    spacing: 4

                                    Label {
                                        text: templateDetailNotifier && templateDetailNotifier.template &&
                                              templateDetailNotifier.template.isVideo ? "\ue04b" : "\ue3f4"
                                        font.pixelSize: 12
                                        color: palette.placeholderText
                                    }

                                    Label {
                                        text: templateDetailNotifier && templateDetailNotifier.template
                                              ? (templateDetailNotifier.template.isVideo ? "video" : "image") : ""
                                        font.pixelSize: 11
                                        color: palette.placeholderText
                                    }
                                }
                            }
                        }
                    }

                    // PRO badge
                    Rectangle {
                        visible: templateDetailNotifier && templateDetailNotifier.template &&
                                 templateDetailNotifier.template.isPremium ? true : false
                        width: proBadge.implicitWidth + 24
                        height: proBadge.implicitHeight + 12
                        radius: 8
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: palette.highlight }
                            GradientStop { position: 1.0; color: Qt.darker(palette.highlight, 1.3) }
                        }

                        Label {
                            id: proBadge
                            anchors.centerIn: parent
                            text: "PRO"
                            color: "white"
                            font.weight: Font.Bold
                            font.pixelSize: 13
                        }
                    }
                }

                // Creator info
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    visible: templateDetailNotifier && templateDetailNotifier.template &&
                             templateDetailNotifier.template.creatorName ? true : false

                    Rectangle {
                        width: 36; height: 36
                        radius: 18
                        color: palette.alternateBase

                        Image {
                            anchors.fill: parent
                            source: templateDetailNotifier && templateDetailNotifier.template
                                    ? templateDetailNotifier.template.creatorAvatarUrl || "" : ""
                            fillMode: Image.PreserveAspectCrop
                            visible: source !== ""
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: !(templateDetailNotifier && templateDetailNotifier.template &&
                                       templateDetailNotifier.template.creatorAvatarUrl)
                            text: {
                                var name = templateDetailNotifier && templateDetailNotifier.template
                                           ? templateDetailNotifier.template.creatorName || "" : ""
                                return name.length > 0 ? name.charAt(0).toUpperCase() : ""
                            }
                            font.weight: Font.DemiBold
                            color: palette.highlight
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: templateDetailNotifier && templateDetailNotifier.template
                                  ? templateDetailNotifier.template.creatorName || "" : ""
                            font.weight: Font.DemiBold
                        }

                        Label {
                            text: "Creator"
                            font.pixelSize: 11
                            color: palette.placeholderText
                        }
                    }
                }

                // About section
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "About"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: templateDetailNotifier && templateDetailNotifier.template
                              ? templateDetailNotifier.template.description || "" : ""
                        wrapMode: Text.Wrap
                        color: palette.placeholderText
                        lineHeight: 1.5
                    }
                }

                // Editable Fields
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: templateDetailNotifier && templateDetailNotifier.template &&
                             templateDetailNotifier.template.editableFields &&
                             templateDetailNotifier.template.editableFields.length > 0

                    Label {
                        text: "Editable Fields"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Repeater {
                        model: templateDetailNotifier && templateDetailNotifier.template
                               ? templateDetailNotifier.template.editableFields : []

                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: {
                                    var icons = {
                                        "text": "\ue262",
                                        "image": "\ue3f4",
                                        "color": "\ue40a",
                                        "number": "\ue893"
                                    }
                                    return icons[modelData.fieldType] || "\ue262"
                                }
                                font.pixelSize: 18
                                color: palette.highlight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.label || ""
                            }

                            Rectangle {
                                width: ftLabel.implicitWidth + 12
                                height: ftLabel.implicitHeight + 4
                                radius: 4
                                color: palette.alternateBase

                                Label {
                                    id: ftLabel
                                    anchors.centerIn: parent
                                    text: modelData.fieldType || ""
                                    font.pixelSize: 11
                                    color: palette.placeholderText
                                }
                            }
                        }
                    }
                }

                // Metadata Details
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Details"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    MetaRow {
                        label: "Dimensions"
                        value: templateDetailNotifier && templateDetailNotifier.template
                               ? templateDetailNotifier.template.dimensions || "" : ""
                    }
                    MetaRow {
                        label: "Layers"
                        value: templateDetailNotifier && templateDetailNotifier.template
                               ? "" + (templateDetailNotifier.template.layerCount || 0) : ""
                    }
                    MetaRow {
                        label: "Duration"
                        value: templateDetailNotifier && templateDetailNotifier.template &&
                               templateDetailNotifier.template.durationSeconds
                               ? templateDetailNotifier.template.durationSeconds + "s" : ""
                        visible: value !== ""
                    }
                    MetaRow {
                        label: "Uses"
                        value: templateDetailNotifier && templateDetailNotifier.template
                               ? "" + (templateDetailNotifier.template.usageCount || 0) : ""
                    }
                    MetaRow {
                        label: "Version"
                        value: templateDetailNotifier && templateDetailNotifier.template
                               ? "v" + (templateDetailNotifier.template.version || "1") : ""
                    }
                }

                // Tags
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: templateDetailNotifier && templateDetailNotifier.template &&
                             templateDetailNotifier.template.tags &&
                             templateDetailNotifier.template.tags.length > 0

                    Label {
                        text: "Tags"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: templateDetailNotifier && templateDetailNotifier.template
                                   ? templateDetailNotifier.template.tags : []

                            delegate: Rectangle {
                                required property var modelData
                                width: tagLabel.implicitWidth + 16
                                height: tagLabel.implicitHeight + 8
                                radius: height / 2
                                color: palette.alternateBase

                                Label {
                                    id: tagLabel
                                    anchors.centerIn: parent
                                    text: "#" + modelData
                                    font.pixelSize: 11
                                    color: palette.placeholderText
                                }
                            }
                        }
                    }
                }

                // Bottom spacer for bottom bar
                Item { Layout.preferredHeight: 100 }
            }
        }
    }

    // Bottom Bar with Use Template button
    Rectangle {
        id: bottomBar
        visible: templateDetailNotifier && templateDetailNotifier.template ? true : false
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 80
        color: palette.window

        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 1
            color: Qt.rgba(palette.mid.r, palette.mid.g, palette.mid.b, 0.2)
        }

        Button {
            anchors.centerIn: parent
            width: parent.width - 32
            height: 52
            highlighted: true
            enabled: !downloadNotifier ||
                     (downloadNotifier.status !== "downloading" &&
                      downloadNotifier.status !== "loadingInEngine" &&
                      downloadNotifier.status !== "requestingAccess")

            contentItem: RowLayout {
                spacing: 8
                anchors.centerIn: parent

                BusyIndicator {
                    visible: downloadNotifier &&
                             (downloadNotifier.status === "downloading" ||
                              downloadNotifier.status === "loadingInEngine" ||
                              downloadNotifier.status === "requestingAccess")
                    running: visible
                    Layout.preferredWidth: 18; Layout.preferredHeight: 18
                }

                Label {
                    text: {
                        if (!downloadNotifier) return "Use Template"
                        switch (downloadNotifier.status) {
                        case "requestingAccess":
                            return "Requesting access..."
                        case "downloading":
                            return Math.round((downloadNotifier.progress || 0) * 100) + "%"
                        case "loadingInEngine":
                            return "Loading template..."
                        case "complete":
                            return "Open in Editor"
                        default:
                            var tmpl = templateDetailNotifier && templateDetailNotifier.template
                                       ? templateDetailNotifier.template : null
                            return tmpl && tmpl.isPremium ? "Use Template (PRO)" : "Use Template"
                        }
                    }
                    color: "white"
                    font.weight: Font.DemiBold
                }
            }

            onClicked: {
                if (authNotifier && authNotifier.isGuest) {
                    // TODO: show login required sheet
                    return
                }
                if (downloadNotifier)
                    downloadNotifier.download(templateId)
            }
        }
    }

    // MetaRow inline component
    component MetaRow: RowLayout {
        property string label
        property string value
        Layout.fillWidth: true

        Label {
            text: label
            font.pixelSize: 13
            color: palette.placeholderText
            Layout.fillWidth: true
        }

        Label {
            text: value
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }
    }
}
