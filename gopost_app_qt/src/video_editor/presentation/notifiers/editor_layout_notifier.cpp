#include "video_editor/presentation/notifiers/editor_layout_notifier.h"

#include <algorithm>

namespace gopost::video_editor {

EditorLayoutNotifier::EditorLayoutNotifier(QObject* parent)
    : QObject(parent)
    , settings_(QStringLiteral("GoPost"), QStringLiteral("VideoEditor"))
{
    restoreLayout();
}

EditorLayoutNotifier::~EditorLayoutNotifier() {
    saveLayout();
}

// ---------------------------------------------------------------------------
// Splitter controls
// ---------------------------------------------------------------------------

void EditorLayoutNotifier::setSidebarWidth(double width) {
    state_.sidebarWidth = std::clamp(width, kMinSidebarWidth, kMaxSidebarWidth);
    state_.activePreset = LayoutPreset::Custom;
    emit stateChanged();
}

void EditorLayoutNotifier::setUpperFraction(double fraction) {
    state_.upperFraction = std::clamp(fraction, kMinUpperFraction, kMaxUpperFraction);
    state_.activePreset  = LayoutPreset::Custom;
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Per-track height
// ---------------------------------------------------------------------------

double EditorLayoutNotifier::trackHeight(int trackIndex) const {
    return state_.trackHeight(trackIndex);
}

void EditorLayoutNotifier::setTrackHeight(int trackIndex, double height) {
    state_.trackHeights[trackIndex] = std::clamp(height, 24.0, 200.0);
    emit stateChanged();
}

void EditorLayoutNotifier::resetTrackHeight(int trackIndex) {
    state_.trackHeights.remove(trackIndex);
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Maximize / restore
// ---------------------------------------------------------------------------

void EditorLayoutNotifier::toggleMaximize(const QString& panelId) {
    if (state_.maximizedPanelId.has_value() && *state_.maximizedPanelId == panelId)
        state_.maximizedPanelId.reset();
    else
        state_.maximizedPanelId = panelId;
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

void EditorLayoutNotifier::applyPreset(int preset) {
    state_ = EditorLayoutState::forPreset(static_cast<LayoutPreset>(preset));
    emit stateChanged();
}

void EditorLayoutNotifier::applyPreset(const QString& presetName) {
    if (presetName == QStringLiteral("Edit"))       applyPreset(0);
    else if (presetName == QStringLiteral("Color"))  applyPreset(1);
    else if (presetName == QStringLiteral("Audio"))  applyPreset(2);
    else if (presetName == QStringLiteral("Custom")) applyPreset(3);
}

void EditorLayoutNotifier::saveCustomPreset() {
    state_.activePreset = LayoutPreset::Custom;
    saveLayout();
}

void EditorLayoutNotifier::resetVerticalSplitter() {
    state_.sidebarWidth = 300.0;
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void EditorLayoutNotifier::saveLayout() {
    settings_.beginGroup(QStringLiteral("Layout"));
    settings_.setValue(QStringLiteral("json"), state_.toJson());
    settings_.endGroup();
}

void EditorLayoutNotifier::restoreLayout() {
    settings_.beginGroup(QStringLiteral("Layout"));
    const auto json = settings_.value(QStringLiteral("json")).toString();
    settings_.endGroup();

    if (!json.isEmpty()) {
        state_ = EditorLayoutState::fromJson(json);
    } else {
        state_ = EditorLayoutState::editPreset();
    }
}

} // namespace gopost::video_editor
