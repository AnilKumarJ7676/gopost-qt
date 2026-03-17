#pragma once

#include "rendering_bridge/engine_types.h"

#include <QString>

#include <optional>

namespace gopost::image_editor {

/// Domain entity for a text layer's editable configuration.
class TextLayerConfig {
public:
    QString text;
    QString fontFamily = QStringLiteral("System");
    rendering::FontStyle style = rendering::FontStyle::Normal;
    double fontSize = 32.0;
    double colorR = 1.0, colorG = 1.0, colorB = 1.0, colorA = 1.0;
    rendering::TextAlignment alignment = rendering::TextAlignment::Center;
    double lineHeight = 1.2;
    double letterSpacing = 0.0;
    bool hasShadow = false;
    double shadowOffsetX = 2.0, shadowOffsetY = 2.0, shadowBlur = 4.0;
    double shadowColorR = 0.0, shadowColorG = 0.0, shadowColorB = 0.0, shadowColorA = 0.5;
    bool hasOutline = false;
    double outlineWidth = 1.0;
    double outlineColorR = 0.0, outlineColorG = 0.0, outlineColorB = 0.0, outlineColorA = 1.0;

    TextLayerConfig() = default;

    struct CopyWithArgs {
        std::optional<QString> text;
        std::optional<QString> fontFamily;
        std::optional<rendering::FontStyle> style;
        std::optional<double> fontSize;
        std::optional<double> colorR, colorG, colorB, colorA;
        std::optional<rendering::TextAlignment> alignment;
        std::optional<double> lineHeight;
        std::optional<double> letterSpacing;
        std::optional<bool> hasShadow;
        std::optional<bool> hasOutline;
        std::optional<double> outlineWidth;
    };

    [[nodiscard]] TextLayerConfig copyWith(const CopyWithArgs& args) const {
        TextLayerConfig c;
        c.text = args.text.value_or(text);
        c.fontFamily = args.fontFamily.value_or(fontFamily);
        c.style = args.style.value_or(style);
        c.fontSize = args.fontSize.value_or(fontSize);
        c.colorR = args.colorR.value_or(colorR);
        c.colorG = args.colorG.value_or(colorG);
        c.colorB = args.colorB.value_or(colorB);
        c.colorA = args.colorA.value_or(colorA);
        c.alignment = args.alignment.value_or(alignment);
        c.lineHeight = args.lineHeight.value_or(lineHeight);
        c.letterSpacing = args.letterSpacing.value_or(letterSpacing);
        c.hasShadow = args.hasShadow.value_or(hasShadow);
        c.shadowOffsetX = shadowOffsetX;
        c.shadowOffsetY = shadowOffsetY;
        c.shadowBlur = shadowBlur;
        c.shadowColorR = shadowColorR;
        c.shadowColorG = shadowColorG;
        c.shadowColorB = shadowColorB;
        c.shadowColorA = shadowColorA;
        c.hasOutline = args.hasOutline.value_or(hasOutline);
        c.outlineWidth = args.outlineWidth.value_or(outlineWidth);
        c.outlineColorR = outlineColorR;
        c.outlineColorG = outlineColorG;
        c.outlineColorB = outlineColorB;
        c.outlineColorA = outlineColorA;
        return c;
    }

    [[nodiscard]] rendering::TextConfig toEngineConfig() const {
        rendering::TextConfig cfg;
        cfg.text = text;
        cfg.fontFamily = fontFamily;
        cfg.style = style;
        cfg.fontSize = fontSize;
        cfg.colorR = colorR;
        cfg.colorG = colorG;
        cfg.colorB = colorB;
        cfg.colorA = colorA;
        cfg.alignment = alignment;
        cfg.lineHeight = lineHeight;
        cfg.letterSpacing = letterSpacing;
        cfg.hasShadow = hasShadow;
        cfg.shadowColorR = shadowColorR;
        cfg.shadowColorG = shadowColorG;
        cfg.shadowColorB = shadowColorB;
        cfg.shadowColorA = shadowColorA;
        cfg.shadowOffsetX = shadowOffsetX;
        cfg.shadowOffsetY = shadowOffsetY;
        cfg.shadowBlur = shadowBlur;
        cfg.hasOutline = hasOutline;
        cfg.outlineColorR = outlineColorR;
        cfg.outlineColorG = outlineColorG;
        cfg.outlineColorB = outlineColorB;
        cfg.outlineColorA = outlineColorA;
        cfg.outlineWidth = outlineWidth;
        return cfg;
    }
};

} // namespace gopost::image_editor
