#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <cstdint>
#include <optional>

namespace gopost::video_editor {

// ── EffectCategory ──────────────────────────────────────────────────────────

enum class EffectCategory : int {
    ColorTone = 0,
    BlurSharpen,
    Distort,
    Stylize
};

// ── EffectType ──────────────────────────────────────────────────────────────

enum class EffectType : int {
    Brightness = 0,
    Contrast,
    Saturation,
    Exposure,
    Temperature,
    Tint,
    Highlights,
    Shadows,
    Vibrance,
    HueRotate,
    GaussianBlur,
    RadialBlur,
    TiltShift,
    Sharpen,
    Pixelate,
    Glitch,
    Chromatic,
    Vignette,
    Grain,
    Sepia,
    Invert,
    Posterize
};

struct EffectTypeInfo {
    EffectCategory category;
    QString label;
    double min;
    double max;
    double defaultValue;
};

inline EffectTypeInfo effectTypeInfo(EffectType t) {
    switch (t) {
        case EffectType::Brightness:   return {EffectCategory::ColorTone,   QStringLiteral("Brightness"),    -100, 100, 0};
        case EffectType::Contrast:     return {EffectCategory::ColorTone,   QStringLiteral("Contrast"),      -100, 100, 0};
        case EffectType::Saturation:   return {EffectCategory::ColorTone,   QStringLiteral("Saturation"),    -100, 100, 0};
        case EffectType::Exposure:     return {EffectCategory::ColorTone,   QStringLiteral("Exposure"),      -2.0, 2.0, 0};
        case EffectType::Temperature:  return {EffectCategory::ColorTone,   QStringLiteral("Temperature"),   -100, 100, 0};
        case EffectType::Tint:         return {EffectCategory::ColorTone,   QStringLiteral("Tint"),          -100, 100, 0};
        case EffectType::Highlights:   return {EffectCategory::ColorTone,   QStringLiteral("Highlights"),    -100, 100, 0};
        case EffectType::Shadows:      return {EffectCategory::ColorTone,   QStringLiteral("Shadows"),       -100, 100, 0};
        case EffectType::Vibrance:     return {EffectCategory::ColorTone,   QStringLiteral("Vibrance"),      -100, 100, 0};
        case EffectType::HueRotate:    return {EffectCategory::ColorTone,   QStringLiteral("Hue"),           -180, 180, 0};
        case EffectType::GaussianBlur: return {EffectCategory::BlurSharpen, QStringLiteral("Gaussian Blur"), 0, 100, 0};
        case EffectType::RadialBlur:   return {EffectCategory::BlurSharpen, QStringLiteral("Radial Blur"),   0, 100, 0};
        case EffectType::TiltShift:    return {EffectCategory::BlurSharpen, QStringLiteral("Tilt Shift"),    0, 100, 0};
        case EffectType::Sharpen:      return {EffectCategory::BlurSharpen, QStringLiteral("Sharpen"),       0, 100, 0};
        case EffectType::Pixelate:     return {EffectCategory::Distort,     QStringLiteral("Pixelate"),      1, 50, 1};
        case EffectType::Glitch:       return {EffectCategory::Distort,     QStringLiteral("Glitch"),        0, 100, 0};
        case EffectType::Chromatic:    return {EffectCategory::Distort,     QStringLiteral("Chromatic"),     0, 100, 0};
        case EffectType::Vignette:     return {EffectCategory::Stylize,     QStringLiteral("Vignette"),      0, 100, 0};
        case EffectType::Grain:        return {EffectCategory::Stylize,     QStringLiteral("Film Grain"),    0, 100, 0};
        case EffectType::Sepia:        return {EffectCategory::Stylize,     QStringLiteral("Sepia"),         0, 100, 0};
        case EffectType::Invert:       return {EffectCategory::Stylize,     QStringLiteral("Invert"),        0, 100, 0};
        case EffectType::Posterize:    return {EffectCategory::Stylize,     QStringLiteral("Posterize"),     2, 16, 16};
    }
    return {EffectCategory::ColorTone, {}, 0, 0, 0};
}

// ── VideoEffect ─────────────────────────────────────────────────────────────

struct VideoEffect {
    EffectType type{EffectType::Brightness};
    double value{0.0};
    bool enabled{true};
    double mix{1.0};

    [[nodiscard]] VideoEffect copyWith(
        std::optional<double> value_ = std::nullopt,
        std::optional<bool> enabled_ = std::nullopt,
        std::optional<double> mix_ = std::nullopt
    ) const {
        return {
            .type = type,
            .value = value_.value_or(value),
            .enabled = enabled_.value_or(enabled),
            .mix = mix_.value_or(mix),
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        return {
            {QStringLiteral("type"), static_cast<int>(type)},
            {QStringLiteral("value"), value},
            {QStringLiteral("enabled"), enabled},
            {QStringLiteral("mix"), mix},
        };
    }

    [[nodiscard]] static VideoEffect fromMap(const QJsonObject& m) {
        return {
            .type = static_cast<EffectType>(m.value(QStringLiteral("type")).toInt()),
            .value = m.value(QStringLiteral("value")).toDouble(),
            .enabled = m.value(QStringLiteral("enabled")).toBool(true),
            .mix = m.value(QStringLiteral("mix")).toDouble(1.0),
        };
    }

    bool operator==(const VideoEffect&) const = default;
};

// ── PresetFilterId ──────────────────────────────────────────────────────────

enum class PresetFilterId : int {
    None = 0,
    Natural,
    Daylight,
    GoldenHour,
    Overcast,
    Portrait,
    SoftSkin,
    Studio,
    WarmPortrait,
    Vintage,
    Polaroid,
    Kodachrome,
    Retro,
    Cinematic,
    TealOrange,
    Noir,
    Desaturated,
    BWClassic,
    BWHigh,
    BWSelenium,
    BWInfrared,
    // Extended presets (used by effect_color_delegate)
    WarmSunset,
    CoolBlue,
    HighContrast,
    SoftPastel,
    FilmNoir,
    BleachBypass,
    CrossProcess,
    OrangeTeal,
    Moody,
    Dreamy,
    Retro70s,
    Hdr,
    Matte,
    Cyberpunk,
    Golden,
    Arctic
};

inline QString presetFilterLabel(PresetFilterId id) {
    switch (id) {
        case PresetFilterId::None:         return QStringLiteral("None");
        case PresetFilterId::Natural:      return QStringLiteral("Natural");
        case PresetFilterId::Daylight:     return QStringLiteral("Daylight");
        case PresetFilterId::GoldenHour:   return QStringLiteral("Golden Hour");
        case PresetFilterId::Overcast:     return QStringLiteral("Overcast");
        case PresetFilterId::Portrait:     return QStringLiteral("Portrait");
        case PresetFilterId::SoftSkin:     return QStringLiteral("Soft Skin");
        case PresetFilterId::Studio:       return QStringLiteral("Studio");
        case PresetFilterId::WarmPortrait: return QStringLiteral("Warm Portrait");
        case PresetFilterId::Vintage:      return QStringLiteral("Vintage");
        case PresetFilterId::Polaroid:     return QStringLiteral("Polaroid");
        case PresetFilterId::Kodachrome:   return QStringLiteral("Kodachrome");
        case PresetFilterId::Retro:        return QStringLiteral("Retro");
        case PresetFilterId::Cinematic:    return QStringLiteral("Cinematic");
        case PresetFilterId::TealOrange:   return QStringLiteral("Teal & Orange");
        case PresetFilterId::Noir:         return QStringLiteral("Noir");
        case PresetFilterId::Desaturated:  return QStringLiteral("Desaturated");
        case PresetFilterId::BWClassic:    return QStringLiteral("B&W Classic");
        case PresetFilterId::BWHigh:       return QStringLiteral("B&W High Contrast");
        case PresetFilterId::BWSelenium:   return QStringLiteral("B&W Selenium");
        case PresetFilterId::BWInfrared:   return QStringLiteral("B&W Infrared");
    }
    return {};
}

// ── AdjustmentClipStyle ─────────────────────────────────────────────────────

enum class AdjustmentClipStyle : int {
    ColorWave = 0,
    BlurPulse,
    DistortScan,
    StylizeDots,
    PresetStrip
};

// ── ColorGrading ────────────────────────────────────────────────────────────

struct ColorGrading {
    double brightness{0};
    double contrast{0};
    double saturation{0};
    double exposure{0};
    double temperature{0};
    double tint{0};
    double highlights{0};
    double shadows{0};
    double vibrance{0};
    double hue{0};
    double fade{0};
    double vignette{0};

    [[nodiscard]] bool isDefault() const {
        return brightness == 0 && contrast == 0 && saturation == 0 &&
               exposure == 0 && temperature == 0 && tint == 0 &&
               highlights == 0 && shadows == 0 && vibrance == 0 && hue == 0 &&
               fade == 0 && vignette == 0;
    }

    [[nodiscard]] ColorGrading copyWith(
        std::optional<double> brightness_ = std::nullopt,
        std::optional<double> contrast_ = std::nullopt,
        std::optional<double> saturation_ = std::nullopt,
        std::optional<double> exposure_ = std::nullopt,
        std::optional<double> temperature_ = std::nullopt,
        std::optional<double> tint_ = std::nullopt,
        std::optional<double> highlights_ = std::nullopt,
        std::optional<double> shadows_ = std::nullopt,
        std::optional<double> vibrance_ = std::nullopt,
        std::optional<double> hue_ = std::nullopt,
        std::optional<double> fade_ = std::nullopt,
        std::optional<double> vignette_ = std::nullopt
    ) const {
        return {
            .brightness = brightness_.value_or(brightness),
            .contrast = contrast_.value_or(contrast),
            .saturation = saturation_.value_or(saturation),
            .exposure = exposure_.value_or(exposure),
            .temperature = temperature_.value_or(temperature),
            .tint = tint_.value_or(tint),
            .highlights = highlights_.value_or(highlights),
            .shadows = shadows_.value_or(shadows),
            .vibrance = vibrance_.value_or(vibrance),
            .hue = hue_.value_or(hue),
            .fade = fade_.value_or(fade),
            .vignette = vignette_.value_or(vignette),
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        return {
            {QStringLiteral("brightness"), brightness},
            {QStringLiteral("contrast"), contrast},
            {QStringLiteral("saturation"), saturation},
            {QStringLiteral("exposure"), exposure},
            {QStringLiteral("temperature"), temperature},
            {QStringLiteral("tint"), tint},
            {QStringLiteral("highlights"), highlights},
            {QStringLiteral("shadows"), shadows},
            {QStringLiteral("vibrance"), vibrance},
            {QStringLiteral("hue"), hue},
            {QStringLiteral("fade"), fade},
            {QStringLiteral("vignette"), vignette},
        };
    }

    [[nodiscard]] static ColorGrading fromMap(const QJsonObject& m) {
        return {
            .brightness = m.value(QStringLiteral("brightness")).toDouble(0),
            .contrast = m.value(QStringLiteral("contrast")).toDouble(0),
            .saturation = m.value(QStringLiteral("saturation")).toDouble(0),
            .exposure = m.value(QStringLiteral("exposure")).toDouble(0),
            .temperature = m.value(QStringLiteral("temperature")).toDouble(0),
            .tint = m.value(QStringLiteral("tint")).toDouble(0),
            .highlights = m.value(QStringLiteral("highlights")).toDouble(0),
            .shadows = m.value(QStringLiteral("shadows")).toDouble(0),
            .vibrance = m.value(QStringLiteral("vibrance")).toDouble(0),
            .hue = m.value(QStringLiteral("hue")).toDouble(0),
            .fade = m.value(QStringLiteral("fade")).toDouble(0),
            .vignette = m.value(QStringLiteral("vignette")).toDouble(0),
        };
    }

    bool operator==(const ColorGrading&) const = default;
};

// ── AdjustmentClipData ──────────────────────────────────────────────────────

struct AdjustmentClipData {
    QList<VideoEffect> effects;
    ColorGrading colorGrading;
    PresetFilterId preset{PresetFilterId::None};
    AdjustmentClipStyle style{AdjustmentClipStyle::ColorWave};
    QString label{QStringLiteral("Adjustment")};
    uint32_t colorValue{0xFF6C63FF};

    [[nodiscard]] bool isEmpty_() const {
        return effects.isEmpty() && colorGrading.isDefault() && preset == PresetFilterId::None;
    }
    [[nodiscard]] bool hasEffects() const {
        return std::any_of(effects.begin(), effects.end(),
            [](const VideoEffect& e) { return e.enabled; });
    }
    [[nodiscard]] bool hasColorGrading() const { return !colorGrading.isDefault(); }
    [[nodiscard]] bool hasPreset() const { return preset != PresetFilterId::None; }

    [[nodiscard]] AdjustmentClipData copyWith(
        std::optional<QList<VideoEffect>> effects_ = std::nullopt,
        std::optional<ColorGrading> colorGrading_ = std::nullopt,
        std::optional<PresetFilterId> preset_ = std::nullopt,
        std::optional<AdjustmentClipStyle> style_ = std::nullopt,
        std::optional<QString> label_ = std::nullopt,
        std::optional<uint32_t> colorValue_ = std::nullopt
    ) const {
        return {
            .effects = effects_.value_or(effects),
            .colorGrading = colorGrading_.value_or(colorGrading),
            .preset = preset_.value_or(preset),
            .style = style_.value_or(style),
            .label = label_.value_or(label),
            .colorValue = colorValue_.value_or(colorValue),
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        QJsonArray effectsArr;
        for (const auto& e : effects) effectsArr.append(e.toMap());
        return {
            {QStringLiteral("effects"), effectsArr},
            {QStringLiteral("colorGrading"), colorGrading.toMap()},
            {QStringLiteral("preset"), static_cast<int>(preset)},
            {QStringLiteral("style"), static_cast<int>(style)},
            {QStringLiteral("label"), label},
            {QStringLiteral("colorValue"), static_cast<qint64>(colorValue)},
        };
    }

    [[nodiscard]] static AdjustmentClipData fromMap(const QJsonObject& m) {
        AdjustmentClipData result;
        const auto arr = m.value(QStringLiteral("effects")).toArray();
        for (const auto& v : arr)
            result.effects.append(VideoEffect::fromMap(v.toObject()));
        if (m.contains(QStringLiteral("colorGrading")))
            result.colorGrading = ColorGrading::fromMap(m.value(QStringLiteral("colorGrading")).toObject());
        result.preset = static_cast<PresetFilterId>(m.value(QStringLiteral("preset")).toInt(0));
        result.style = static_cast<AdjustmentClipStyle>(m.value(QStringLiteral("style")).toInt(0));
        result.label = m.value(QStringLiteral("label")).toString(QStringLiteral("Adjustment"));
        result.colorValue = static_cast<uint32_t>(m.value(QStringLiteral("colorValue")).toInteger(0xFF6C63FF));
        return result;
    }

    /// Infer the best visual style from the current effect configuration.
    [[nodiscard]] static AdjustmentClipStyle inferStyle(const QList<VideoEffect>& effects, PresetFilterId preset) {
        if (preset != PresetFilterId::None) return AdjustmentClipStyle::PresetStrip;
        if (effects.isEmpty()) return AdjustmentClipStyle::ColorWave;
        const auto cat = effectTypeInfo(effects.first().type).category;
        switch (cat) {
            case EffectCategory::ColorTone:   return AdjustmentClipStyle::ColorWave;
            case EffectCategory::BlurSharpen:  return AdjustmentClipStyle::BlurPulse;
            case EffectCategory::Distort:      return AdjustmentClipStyle::DistortScan;
            case EffectCategory::Stylize:      return AdjustmentClipStyle::StylizeDots;
        }
        return AdjustmentClipStyle::ColorWave;
    }

    /// Build a display label from effects.
    [[nodiscard]] static QString buildLabel(const QList<VideoEffect>& effects, PresetFilterId preset) {
        if (preset != PresetFilterId::None) return presetFilterLabel(preset);
        if (effects.isEmpty()) return QStringLiteral("Adjustment");
        if (effects.size() == 1) {
            const auto& e = effects.first();
            return QStringLiteral("%1 %2%3")
                .arg(effectTypeInfo(e.type).label)
                .arg(e.value >= 0 ? QStringLiteral("+") : QString())
                .arg(static_cast<int>(e.value));
        }
        return QStringLiteral("%1 Effects").arg(effects.size());
    }

    /// Pick a signature color based on the dominant effect category.
    [[nodiscard]] static uint32_t pickColor(const QList<VideoEffect>& effects, PresetFilterId preset) {
        if (preset != PresetFilterId::None) return 0xFFAB47BC;
        if (effects.isEmpty()) return 0xFF6C63FF;
        switch (effectTypeInfo(effects.first().type).category) {
            case EffectCategory::ColorTone:   return 0xFFFF7043;
            case EffectCategory::BlurSharpen:  return 0xFF26C6DA;
            case EffectCategory::Distort:      return 0xFFEF5350;
            case EffectCategory::Stylize:      return 0xFFAB47BC;
        }
        return 0xFF6C63FF;
    }

    bool operator==(const AdjustmentClipData&) const = default;
};

} // namespace gopost::video_editor
