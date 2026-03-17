#pragma once

#include <QFont>
#include <QString>

namespace gopost::core {

namespace AppTypography {

inline const QString fontFamily = QStringLiteral("Inter");
inline const QString fontFamilyMono = QStringLiteral("JetBrains Mono");

struct TextStyle {
    QString family;
    int fontSize;
    int fontWeight;
    double lineHeight;
    double letterSpacing;
};

// Display
inline const TextStyle displayLarge{fontFamily, 57, QFont::Normal, 64.0 / 57.0, -0.25};
inline const TextStyle displayMedium{fontFamily, 45, QFont::Normal, 52.0 / 45.0, 0.0};
inline const TextStyle displaySmall{fontFamily, 36, QFont::Normal, 44.0 / 36.0, 0.0};

// Headline
inline const TextStyle headlineLarge{fontFamily, 32, QFont::DemiBold, 40.0 / 32.0, 0.0};
inline const TextStyle headlineMedium{fontFamily, 28, QFont::DemiBold, 36.0 / 28.0, 0.0};
inline const TextStyle headlineSmall{fontFamily, 24, QFont::DemiBold, 32.0 / 24.0, 0.0};

// Title
inline const TextStyle titleLarge{fontFamily, 22, QFont::Medium, 28.0 / 22.0, 0.0};
inline const TextStyle titleMedium{fontFamily, 16, QFont::Medium, 24.0 / 16.0, 0.15};

// Body
inline const TextStyle bodyLarge{fontFamily, 16, QFont::Normal, 24.0 / 16.0, 0.5};
inline const TextStyle bodyMedium{fontFamily, 14, QFont::Normal, 20.0 / 14.0, 0.25};
inline const TextStyle bodySmall{fontFamily, 12, QFont::Normal, 16.0 / 12.0, 0.4};

// Label
inline const TextStyle labelLarge{fontFamily, 14, QFont::Medium, 20.0 / 14.0, 0.1};
inline const TextStyle labelMedium{fontFamily, 12, QFont::Medium, 16.0 / 12.0, 0.5};
inline const TextStyle labelSmall{fontFamily, 11, QFont::Medium, 16.0 / 11.0, 0.5};

inline QFont toQFont(const TextStyle& style) {
    QFont font(style.family, style.fontSize);
    font.setWeight(static_cast<QFont::Weight>(style.fontWeight));
    font.setLetterSpacing(QFont::AbsoluteSpacing, style.letterSpacing);
    return font;
}

} // namespace AppTypography

} // namespace gopost::core
