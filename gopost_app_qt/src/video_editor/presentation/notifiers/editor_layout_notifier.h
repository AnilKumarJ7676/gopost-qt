#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>

#include "video_editor/presentation/editor_layout_state.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------------------
inline constexpr double kMinSidebarWidth    = 0.0;
inline constexpr double kMaxSidebarWidth    = 600.0;
inline constexpr double kMinUpperFraction   = 0.15;
inline constexpr double kMaxUpperFraction   = 0.85;

// ---------------------------------------------------------------------------
// EditorLayoutNotifier -- splitter positions, per-track heights, persistence
//
// Converted 1:1 from editor_layout_notifier.dart.
// Uses EditorLayoutState from the domain model.
// ---------------------------------------------------------------------------
class EditorLayoutNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(double sidebarWidth   READ sidebarWidth   NOTIFY stateChanged)
    Q_PROPERTY(double upperFraction  READ upperFraction   NOTIFY stateChanged)
    Q_PROPERTY(QString maximizedPanelId READ maximizedPanelId NOTIFY stateChanged)
    Q_PROPERTY(int    activePreset   READ activePresetInt NOTIFY stateChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames CONSTANT)

public:
    explicit EditorLayoutNotifier(QObject* parent = nullptr);
    ~EditorLayoutNotifier() override;

    double  sidebarWidth()     const { return state_.sidebarWidth; }
    double  upperFraction()    const { return state_.upperFraction; }
    QString maximizedPanelId() const { return state_.maximizedPanelId.value_or(QString()); }
    int     activePresetInt()  const { return static_cast<int>(state_.activePreset); }
    QStringList presetNames()  const {
        return {QStringLiteral("Edit"), QStringLiteral("Color"),
                QStringLiteral("Audio"), QStringLiteral("Custom")};
    }

    const EditorLayoutState& state() const { return state_; }

    // ---- splitter controls -------------------------------------------------
    Q_INVOKABLE void setSidebarWidth(double width);
    Q_INVOKABLE void setUpperFraction(double fraction);

    // ---- per-track height --------------------------------------------------
    Q_INVOKABLE double trackHeight(int trackIndex) const;
    Q_INVOKABLE void   setTrackHeight(int trackIndex, double height);
    Q_INVOKABLE void   resetTrackHeight(int trackIndex);

    // ---- maximize / restore ------------------------------------------------
    Q_INVOKABLE void toggleMaximize(const QString& panelId);

    // ---- presets -----------------------------------------------------------
    Q_INVOKABLE void applyPreset(int preset);
    Q_INVOKABLE void applyPreset(const QString& presetName);
    Q_INVOKABLE void saveCustomPreset();
    Q_INVOKABLE void resetVerticalSplitter();

    // ---- persistence (QSettings) -------------------------------------------
    void saveLayout();
    void restoreLayout();

signals:
    void stateChanged();

private:
    EditorLayoutState state_;
    QSettings settings_;
};

} // namespace gopost::video_editor
