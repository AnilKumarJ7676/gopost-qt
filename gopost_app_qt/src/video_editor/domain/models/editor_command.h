#pragma once

#include <QString>
#include <QList>
#include <optional>

#include "video_editor/domain/models/video_project.h"

namespace gopost::video_editor {

/// Snapshot-based undo/redo: each command stores the full project state
/// before and after the operation. Simple and robust for timeline editing.
struct EditorCommand {
    QString description;
    VideoProject stateBefore;
    VideoProject stateAfter;
};

class UndoRedoStack {
public:
    static constexpr int maxUndoLevels = 100;

    [[nodiscard]] bool canUndo() const { return !undoStack_.isEmpty(); }
    [[nodiscard]] bool canRedo() const { return !redoStack_.isEmpty(); }

    [[nodiscard]] std::optional<QString> undoDescription() const {
        return undoStack_.isEmpty() ? std::nullopt
            : std::optional<QString>(undoStack_.last().description);
    }

    [[nodiscard]] std::optional<QString> redoDescription() const {
        return redoStack_.isEmpty() ? std::nullopt
            : std::optional<QString>(redoStack_.last().description);
    }

    void push(const EditorCommand& command) {
        undoStack_.append(command);
        redoStack_.clear();
        if (undoStack_.size() > maxUndoLevels) {
            undoStack_.removeFirst();
        }
    }

    std::optional<EditorCommand> undo() {
        if (undoStack_.isEmpty()) return std::nullopt;
        auto cmd = undoStack_.takeLast();
        redoStack_.append(cmd);
        return cmd;
    }

    std::optional<EditorCommand> redo() {
        if (redoStack_.isEmpty()) return std::nullopt;
        auto cmd = redoStack_.takeLast();
        undoStack_.append(cmd);
        return cmd;
    }

    void clear() {
        undoStack_.clear();
        redoStack_.clear();
    }

private:
    QList<EditorCommand> undoStack_;
    QList<EditorCommand> redoStack_;
};

} // namespace gopost::video_editor
