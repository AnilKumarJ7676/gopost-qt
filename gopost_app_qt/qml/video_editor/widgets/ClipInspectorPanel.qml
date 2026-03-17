import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GopostApp

/// Shows clip timing, properties, opacity, blend mode, effects, transitions, actions.
Item {
    id: root

    readonly property var clip: timelineNotifier ? timelineNotifier.selectedClip : ({})
    readonly property var blendModes: [
        "Normal", "Multiply", "Screen", "Overlay", "Soft Light", "Hard Light",
        "Color Dodge", "Color Burn", "Darken", "Lighten", "Difference",
        "Exclusion", "Hue", "Saturation", "Color", "Luminosity"
    ]

    // Empty state
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 8
        visible: !root.clip

        Label {
            text: "\u24D8"
            font.pixelSize: 38
            color: "#404060"
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: "Select a clip to inspect its properties"
            font.pixelSize: 14
            color: "#6B6B88"
            Layout.alignment: Qt.AlignHCenter
        }
    }

    // Inspector content
    ListView {
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        visible: !!root.clip
        spacing: 0

        model: ListModel { id: dummyModel; ListElement { dummy: true } }

        delegate: ColumnLayout {
            width: ListView.view.width
            spacing: 4

            // Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: headerRow.implicitHeight + 20
                radius: 8
                color: Qt.rgba(0.149, 0.776, 0.855, 0.08)
                border.color: Qt.rgba(0.149, 0.776, 0.855, 0.2)

                RowLayout {
                    id: headerRow
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Label {
                        text: root.clip ? root.clip.typeIcon : "\uD83C\uDFA5"
                        font.pixelSize: 24
                        color: "#26C6DA"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            text: root.clip ? root.clip.displayName : ""
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: "#E0E0F0"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: root.clip ? ("ID: " + root.clip.id + "  \u2022  " + root.clip.sourceType) : ""
                            font.pixelSize: 12
                            color: Qt.rgba(0.149, 0.776, 0.855, 0.8)
                        }
                    }
                }
            }

            Item { height: 14 }

            // Timing
            Label { text: "Timing"; font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0" }
            Item { height: 6 }
            Repeater {
                model: root.clip ? [
                    { label: "In", value: internal.formatTime(root.clip.timelineIn) },
                    { label: "Out", value: internal.formatTime(root.clip.timelineOut) },
                    { label: "Duration", value: internal.formatDuration(root.clip.duration) },
                    { label: "Source In", value: internal.formatTime(root.clip.sourceIn) },
                    { label: "Source Out", value: internal.formatTime(root.clip.sourceOut) }
                ] : []
                delegate: RowLayout {
                    Layout.fillWidth: true
                    Label { text: modelData.label; font.pixelSize: 13; color: "#6B6B88"; Layout.preferredWidth: 72 }
                    Item { Layout.fillWidth: true }
                    Label { text: modelData.value; font.pixelSize: 13; font.family: Theme.fontFamilyMono; color: "#D0D0E8" }
                }
            }

            Item { height: 14 }

            // Properties
            Label { text: "Properties"; font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0" }
            Item { height: 6 }
            Repeater {
                model: root.clip ? [
                    { label: "Speed", value: root.clip.speed + "x" },
                    { label: "Track", value: "" + root.clip.trackIndex },
                    { label: "Source", value: root.clip.sourceType }
                ] : []
                delegate: RowLayout {
                    Layout.fillWidth: true
                    Label { text: modelData.label; font.pixelSize: 13; color: "#6B6B88"; Layout.preferredWidth: 72 }
                    Item { Layout.fillWidth: true }
                    Label { text: modelData.value; font.pixelSize: 13; font.family: Theme.fontFamilyMono; color: "#D0D0E8" }
                }
            }

            Item { height: 14 }

            // Opacity
            Label { text: "Opacity"; font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0" }
            Item { height: 6 }
            RowLayout {
                Layout.fillWidth: true
                Slider {
                    Layout.fillWidth: true
                    from: 0; to: 1.0
                    value: root.clip ? root.clip.opacity : 1.0
                    onMoved: timelineNotifier.setClipOpacity(root.clip.id, value)
                }
                Label {
                    text: root.clip ? (Math.round(root.clip.opacity * 100) + "%") : "100%"
                    font.pixelSize: 13
                    font.family: Theme.fontFamilyMono
                    color: "#B0B0C8"
                    Layout.preferredWidth: 40
                    horizontalAlignment: Text.AlignRight
                }
            }

            Item { height: 14 }

            // Blend Mode
            Label { text: "Blend Mode"; font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0" }
            Item { height: 6 }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                radius: 8
                color: "#1A1A34"
                border.color: "#303050"

                ComboBox {
                    anchors.fill: parent
                    anchors.margins: 2
                    model: root.blendModes
                    currentIndex: root.clip ? root.clip.blendMode : 0
                    onCurrentIndexChanged: {
                        if (root.clip) timelineNotifier.setClipBlendMode(root.clip.id, currentIndex)
                    }
                }
            }

            Item { height: 14 }

            // Actions
            Label { text: "Actions"; font.pixelSize: 13; font.weight: Font.DemiBold; color: "#8888A0" }
            Item { height: 6 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: 8
                    color: Qt.rgba(0.424, 0.388, 1.0, 0.1)

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 4
                        Label { text: "\uD83D\uDCCB"; font.pixelSize: 14; color: "#6C63FF" }
                        Label { text: "Duplicate"; font.pixelSize: 13; font.weight: Font.DemiBold; color: "#6C63FF" }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { if (root.clip) timelineNotifier.duplicateClip(root.clip.id) }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: 8
                    color: Qt.rgba(0.937, 0.325, 0.314, 0.1)

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 4
                        Label { text: "\uD83D\uDDD1"; font.pixelSize: 14; color: "#EF5350" }
                        Label { text: "Delete"; font.pixelSize: 13; font.weight: Font.DemiBold; color: "#EF5350" }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { if (root.clip) timelineNotifier.removeClip(root.clip.id) }
                    }
                }
            }
        }
    }

    QtObject {
        id: internal

        function formatTime(s) {
            if (s === undefined || s === null) return "00:00.00"
            var m = Math.floor(s / 60)
            var sec = Math.floor(s % 60)
            var ms = Math.floor((s % 1) * 100)
            return String(m).padStart(2, '0') + ":" + String(sec).padStart(2, '0') + "." + String(ms).padStart(2, '0')
        }

        function formatDuration(s) {
            if (s === undefined || s === null) return "0s"
            if (s < 60) return s.toFixed(2) + "s"
            return Math.floor(s / 60) + "m " + (s % 60).toFixed(1) + "s"
        }
    }
}
