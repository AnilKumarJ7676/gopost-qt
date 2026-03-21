#pragma once

#include <QColor>

namespace gopost::core {

namespace AppColors {

// Brand - Primary
inline constexpr QColor brandPrimary{0x6C, 0x5C, 0xE7};
inline constexpr QColor brandPrimaryDark{0xA2, 0x9B, 0xFE};
inline constexpr QColor brandPrimaryContainerLight{0xE8, 0xE5, 0xFD};
inline constexpr QColor brandPrimaryContainerDark{0x2D, 0x2A, 0x5E};

// Brand - Secondary
inline constexpr QColor brandSecondary{0x00, 0xB8, 0x94};
inline constexpr QColor brandSecondaryDark{0x55, 0xEF, 0xC4};
inline constexpr QColor brandSecondaryContainerLight{0xD5, 0xF5, 0xED};
inline constexpr QColor brandSecondaryContainerDark{0x1A, 0x4A, 0x3A};

// Brand - Tertiary
inline constexpr QColor brandTertiary{0xFD, 0x79, 0xA8};
inline constexpr QColor brandTertiaryDark{0xFA, 0xB1, 0xC8};
inline constexpr QColor brandTertiaryContainerLight{0xFD, 0xE8, 0xF0};
inline constexpr QColor brandTertiaryContainerDark{0x5E, 0x2A, 0x3A};

// Neutral
inline constexpr QColor neutralBackgroundLight{0xFF, 0xFF, 0xFF};
inline constexpr QColor neutralBackgroundDark{0x12, 0x12, 0x12};
inline constexpr QColor neutralSurfaceLight{0xFA, 0xFA, 0xFA};
inline constexpr QColor neutralSurfaceDark{0x1E, 0x1E, 0x1E};
inline constexpr QColor neutralSurfaceVariantLight{0xF0, 0xF0, 0xF0};
inline constexpr QColor neutralSurfaceVariantDark{0x2C, 0x2C, 0x2C};
inline constexpr QColor neutralOutlineLight{0xD1, 0xD1, 0xD6};
inline constexpr QColor neutralOutlineDark{0x3A, 0x3A, 0x3C};

// Text
inline constexpr QColor textPrimaryLight{0x1A, 0x1A, 0x2E};
inline constexpr QColor textPrimaryDark{0xF5, 0xF5, 0xF7};
inline constexpr QColor textSecondaryLight{0x63, 0x63, 0x66};
inline constexpr QColor textSecondaryDark{0xAE, 0xAE, 0xB2};

// Semantic
inline constexpr QColor semanticError{0xE7, 0x4C, 0x3C};
inline constexpr QColor semanticErrorDark{0xFF, 0x6B, 0x6B};
inline constexpr QColor semanticWarning{0xF3, 0x9C, 0x12};
inline constexpr QColor semanticWarningDark{0xFD, 0xCB, 0x6E};
inline constexpr QColor semanticSuccess{0x27, 0xAE, 0x60};
inline constexpr QColor semanticSuccessDark{0x55, 0xEF, 0xC4};
inline constexpr QColor semanticInfo{0x34, 0x98, 0xDB};
inline constexpr QColor semanticInfoDark{0x74, 0xB9, 0xFF};

// Editor (always dark theme)
inline constexpr QColor editorBackground{0x0D, 0x0D, 0x0D};
inline constexpr QColor editorSurface{0x1A, 0x1A, 0x1A};
inline constexpr QColor editorTimeline{0x14, 0x14, 0x14};
inline constexpr QColor editorPlayhead{0xFF, 0x3B, 0x30};
inline const QColor editorSelection = QColor(0x6C, 0x5C, 0xE7, 0x4D);

} // namespace AppColors

} // namespace gopost::core
