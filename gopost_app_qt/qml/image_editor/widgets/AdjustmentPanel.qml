import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import GopostApp 1.0

/// Panel with individual adjustment sliders (brightness, contrast, etc.).
/// Binds to adjustmentNotifier and canvasNotifier global context properties.
Rectangle {
    id: root

    height: 350
    color: "#1E1E2E"

    Rectangle {
        anchors.top: parent.top
        width: parent.width; height: 0.5
        color: Qt.rgba(1, 1, 1, 0.1)
    }

    Component.onCompleted: {
        if (adjustmentNotifier) adjustmentNotifier.loadAdjustments()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Reset All button (visible when changes exist)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: resetRow.visible ? 36 : 0
            color: "transparent"

            RowLayout {
                id: resetRow
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12
                visible: adjustmentNotifier ? adjustmentNotifier.hasChanges : false

                Item { Layout.fillWidth: true }

                ToolButton {
                    icon.name: "view-refresh"
                    icon.color: "#6C63FF"
                    ToolTip.text: "Reset All"
                    ToolTip.visible: hovered
                    onClicked: {
                        if (adjustmentNotifier && canvasNotifier
                                && canvasNotifier.canvasId >= 0
                                && canvasNotifier.selectedLayerId >= 0) {
                            adjustmentNotifier.resetAll(
                                canvasNotifier.canvasId,
                                canvasNotifier.selectedLayerId)
                        }
                    }
                }

                Label {
                    text: "Reset All"
                    color: "#6C63FF"
                    font.pixelSize: 13; font.weight: Font.Medium
                }
            }
        }

        // Loading state
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            running: adjustmentNotifier ? adjustmentNotifier.isLoading : false
            visible: running
        }

        // Empty state
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: adjustmentNotifier
                     && !adjustmentNotifier.isLoading
                     && adjustmentNotifier.adjustmentCount === 0
            text: "No adjustments available"
            color: Qt.rgba(1, 1, 1, 0.38)
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
        }

        // Adjustment list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: adjustmentNotifier
                     && !adjustmentNotifier.isLoading
                     && adjustmentNotifier.adjustmentCount > 0
            clip: true
            spacing: 4

            model: adjustmentNotifier ? adjustmentNotifier.adjustments : []

            delegate: ColumnLayout {
                width: parent ? parent.width - 24 : 200
                x: 12
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        text: modelData.displayName
                        color: Qt.rgba(1, 1, 1, 0.7)
                        font.pixelSize: 13
                    }

                    Label {
                        text: internal.formatValue(modelData.value)
                        color: "#6C63FF"
                        font.pixelSize: 12; font.weight: Font.Medium
                    }

                    ToolButton {
                        enabled: !modelData.isDefault
                        icon.name: "view-refresh"
                        icon.color: enabled ? "#6C63FF" : Qt.rgba(1, 1, 1, 0.24)
                        implicitWidth: 32; implicitHeight: 32
                        onClicked: {
                            if (adjustmentNotifier && canvasNotifier
                                    && canvasNotifier.canvasId >= 0
                                    && canvasNotifier.selectedLayerId >= 0) {
                                adjustmentNotifier.resetAdjustment(
                                    canvasNotifier.canvasId,
                                    canvasNotifier.selectedLayerId,
                                    modelData.effectId)
                            }
                        }
                    }
                }

                Slider {
                    Layout.fillWidth: true
                    from: 0; to: 1
                    value: {
                        var range = modelData.maxValue - modelData.minValue
                        if (range <= 0) return 0.5
                        return (modelData.value - modelData.minValue) / range
                    }

                    onMoved: {
                        var actual = modelData.minValue + value * (modelData.maxValue - modelData.minValue)
                        if (adjustmentNotifier && canvasNotifier
                                && canvasNotifier.canvasId >= 0
                                && canvasNotifier.selectedLayerId >= 0) {
                            adjustmentNotifier.setAdjustment(
                                canvasNotifier.canvasId,
                                canvasNotifier.selectedLayerId,
                                modelData.effectId,
                                actual)
                        }
                    }

                    onPressedChanged: {
                        if (!pressed && canvasNotifier) {
                            canvasNotifier.refreshPreview()
                        }
                    }
                }
            }
        }
    }

    QtObject {
        id: internal

        function formatValue(value) {
            if (Math.abs(value) < 0.01) return "0"
            if (Math.abs(value) >= 100) return Math.round(value).toString()
            if (value === Math.floor(value)) return value.toFixed(0)
            return value.toFixed(1)
        }
    }
}
