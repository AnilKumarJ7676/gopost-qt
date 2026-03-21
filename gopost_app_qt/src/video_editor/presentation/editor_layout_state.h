#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMap>
#include <QList>
#include <optional>

namespace gopost::video_editor {

// ── LayoutPreset ────────────────────────────────────────────────────────────

enum class LayoutPreset : int { Edit = 0, Color, Audio, Custom };

// ── EditorLayoutState ───────────────────────────────────────────────────────

struct EditorLayoutState {
    /// Fraction of total height for the upper area (preview + sidebar).
    double upperFraction{0.556};

    /// Width of the sidebar panel in pixels. 0 = collapsed.
    double sidebarWidth{300.0};

    /// Per-track heights keyed by track index.
    QMap<int, double> trackHeights;

    /// Default height for tracks without a per-track override.
    double defaultTrackHeight{68.0};

    /// Currently active layout preset.
    LayoutPreset activePreset{LayoutPreset::Edit};

    /// Whether a panel is maximized (header double-click). Empty = none.
    std::optional<QString> maximizedPanelId;

    [[nodiscard]] double timelineFraction() const { return 1.0 - upperFraction; }

    [[nodiscard]] double trackHeight(int trackIndex) const {
        auto it = trackHeights.find(trackIndex);
        return (it != trackHeights.end()) ? *it : defaultTrackHeight;
    }

    struct CopyWithOpts {
        std::optional<double> upperFraction;
        std::optional<double> sidebarWidth;
        std::optional<QMap<int, double>> trackHeights;
        std::optional<double> defaultTrackHeight;
        std::optional<LayoutPreset> activePreset;
        std::optional<QString> maximizedPanelId;
        bool clearMaximized{false};
    };

    [[nodiscard]] EditorLayoutState copyWith(const CopyWithOpts& o) const {
        return {
            .upperFraction = o.upperFraction.value_or(upperFraction),
            .sidebarWidth = o.sidebarWidth.value_or(sidebarWidth),
            .trackHeights = o.trackHeights.value_or(trackHeights),
            .defaultTrackHeight = o.defaultTrackHeight.value_or(defaultTrackHeight),
            .activePreset = o.activePreset.value_or(activePreset),
            .maximizedPanelId = o.clearMaximized ? std::nullopt :
                (o.maximizedPanelId.has_value() ? o.maximizedPanelId : maximizedPanelId),
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        QJsonObject heights;
        for (auto it = trackHeights.begin(); it != trackHeights.end(); ++it)
            heights[QString::number(it.key())] = it.value();
        return {
            {QStringLiteral("upperFraction"), upperFraction},
            {QStringLiteral("sidebarWidth"), sidebarWidth},
            {QStringLiteral("trackHeights"), heights},
            {QStringLiteral("defaultTrackHeight"), defaultTrackHeight},
            {QStringLiteral("activePreset"), static_cast<int>(activePreset)},
        };
    }

    [[nodiscard]] static EditorLayoutState fromMap(const QJsonObject& m) {
        EditorLayoutState s;
        s.upperFraction = m.value(QStringLiteral("upperFraction")).toDouble(0.556);
        s.sidebarWidth = m.value(QStringLiteral("sidebarWidth")).toDouble(300.0);
        s.defaultTrackHeight = m.value(QStringLiteral("defaultTrackHeight")).toDouble(68.0);
        s.activePreset = static_cast<LayoutPreset>(m.value(QStringLiteral("activePreset")).toInt(0));

        const auto heights = m.value(QStringLiteral("trackHeights")).toObject();
        for (auto it = heights.begin(); it != heights.end(); ++it)
            s.trackHeights[it.key().toInt()] = it.value().toDouble();
        return s;
    }

    [[nodiscard]] QString toJson() const {
        return QString::fromUtf8(QJsonDocument(toMap()).toJson(QJsonDocument::Compact));
    }

    [[nodiscard]] static EditorLayoutState fromJson(const QString& json) {
        return fromMap(QJsonDocument::fromJson(json.toUtf8()).object());
    }

    // ── Preset defaults ─────────────────────────────────────────────────

    static EditorLayoutState editPreset() {
        return {.upperFraction = 0.556, .sidebarWidth = 300, .defaultTrackHeight = 68,
                .activePreset = LayoutPreset::Edit};
    }

    static EditorLayoutState colorPreset() {
        return {.upperFraction = 0.50, .sidebarWidth = 340, .defaultTrackHeight = 50,
                .activePreset = LayoutPreset::Color};
    }

    static EditorLayoutState audioPreset() {
        return {.upperFraction = 0.35, .sidebarWidth = 260, .defaultTrackHeight = 90,
                .activePreset = LayoutPreset::Audio};
    }

    [[nodiscard]] static EditorLayoutState forPreset(LayoutPreset preset) {
        switch (preset) {
            case LayoutPreset::Edit:   return editPreset();
            case LayoutPreset::Color:  return colorPreset();
            case LayoutPreset::Audio:  return audioPreset();
            case LayoutPreset::Custom: return editPreset();
        }
        return editPreset();
    }
};

// ── FloatingPanelState ──────────────────────────────────────────────────────

struct FloatingPanelState {
    QString id;
    QString title;
    double x{100.0};
    double y{100.0};
    double width{320.0};
    double height{400.0};

    [[nodiscard]] FloatingPanelState copyWith(
        std::optional<double> x_ = std::nullopt,
        std::optional<double> y_ = std::nullopt,
        std::optional<double> width_ = std::nullopt,
        std::optional<double> height_ = std::nullopt
    ) const {
        return {
            .id = id,
            .title = title,
            .x = x_.value_or(x),
            .y = y_.value_or(y),
            .width = width_.value_or(width),
            .height = height_.value_or(height),
        };
    }
};

// ── TabGroupState ───────────────────────────────────────────────────────────

struct TabGroupState {
    QString groupId;
    QList<QString> tabIds;
    int activeIndex{0};

    [[nodiscard]] TabGroupState copyWith(
        std::optional<QList<QString>> tabIds_ = std::nullopt,
        std::optional<int> activeIndex_ = std::nullopt
    ) const {
        return {
            .groupId = groupId,
            .tabIds = tabIds_.value_or(tabIds),
            .activeIndex = activeIndex_.value_or(activeIndex),
        };
    }
};

} // namespace gopost::video_editor
