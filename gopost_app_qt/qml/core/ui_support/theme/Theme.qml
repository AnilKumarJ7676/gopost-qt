pragma Singleton
import QtQuick

QtObject {
    // Brand - Primary
    readonly property color brandPrimary: "#6C5CE7"
    readonly property color brandPrimaryDark: "#A29BFE"
    readonly property color brandPrimaryContainerLight: "#E8E5FD"
    readonly property color brandPrimaryContainerDark: "#2D2A5E"

    // Brand - Secondary
    readonly property color brandSecondary: "#00B894"
    readonly property color brandSecondaryDark: "#55EFC4"
    readonly property color brandSecondaryContainerLight: "#D5F5ED"
    readonly property color brandSecondaryContainerDark: "#1A4A3A"

    // Brand - Tertiary
    readonly property color brandTertiary: "#FD79A8"
    readonly property color brandTertiaryDark: "#FAB1C8"
    readonly property color brandTertiaryContainerLight: "#FDE8F0"
    readonly property color brandTertiaryContainerDark: "#5E2A3A"

    // Semantic
    readonly property color semanticError: "#E74C3C"
    readonly property color semanticWarning: "#F39C12"
    readonly property color semanticSuccess: "#27AE60"
    readonly property color semanticInfo: "#3498DB"

    // Editor (always dark)
    readonly property color editorBackground: "#0D0D0D"
    readonly property color editorSurface: "#1A1A1A"
    readonly property color editorTimeline: "#141414"
    readonly property color editorPlayhead: "#FF3B30"
    readonly property color editorSelection: Qt.rgba(0.424, 0.361, 0.906, 0.3)

    // Spacing
    readonly property real spacingXxs: 2
    readonly property real spacingXs: 4
    readonly property real spacingSm: 8
    readonly property real spacingMd: 12
    readonly property real spacingBase: 16
    readonly property real spacingLg: 20
    readonly property real spacingXl: 24
    readonly property real spacingXxl: 32
    readonly property real spacingXxxl: 48
    readonly property real spacingXxxxl: 64

    // Radius
    readonly property real radiusSm: 8
    readonly property real radiusMd: 12
    readonly property real radiusLg: 16
    readonly property real radiusXl: 24
    readonly property real radiusFull: 999

    // Durations (ms)
    readonly property int durationQuick: 150
    readonly property int durationNormal: 300
    readonly property int durationSlow: 500

    // Breakpoints
    readonly property real breakpointCompact: 600
    readonly property real breakpointMedium: 840
    readonly property real breakpointExpanded: 1200
    readonly property real breakpointLarge: 1600
    readonly property real breakpointXlarge: 1920

    // Typography
    readonly property string fontFamily: "Inter"
    readonly property string fontFamilyMono: "JetBrains Mono"
}
