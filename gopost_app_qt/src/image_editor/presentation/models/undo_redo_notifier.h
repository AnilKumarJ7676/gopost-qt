#pragma once

#include <QObject>
#include <QString>
#include <QList>

#include <functional>
#include <memory>

namespace gopost::image_editor {

// -------------------------------------------------------------------------
// EditCommand — abstract command for undo/redo
// -------------------------------------------------------------------------

class EditCommand {
public:
    virtual ~EditCommand() = default;

    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual QString description() const = 0;
};

using EditCommandPtr = std::shared_ptr<EditCommand>;

// -------------------------------------------------------------------------
// UndoRedoNotifier — manages undo/redo stacks
// -------------------------------------------------------------------------

class UndoRedoNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool canUndo READ canUndo NOTIFY stateChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY stateChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY stateChanged)
    Q_PROPERTY(QString lastActionName READ lastActionName NOTIFY stateChanged)
    Q_PROPERTY(QString nextRedoName READ nextRedoName NOTIFY stateChanged)

public:
    static constexpr int MaxUndoLevels = 50;

    explicit UndoRedoNotifier(QObject* parent = nullptr);

    bool canUndo() const { return !m_undoStack.isEmpty() && !m_isProcessing; }
    bool canRedo() const { return !m_redoStack.isEmpty() && !m_isProcessing; }
    bool isProcessing() const { return m_isProcessing; }

    QString lastActionName() const {
        return m_undoStack.isEmpty() ? QString()
                                     : m_undoStack.last()->description();
    }

    QString nextRedoName() const {
        return m_redoStack.isEmpty() ? QString()
                                      : m_redoStack.last()->description();
    }

    Q_INVOKABLE void execute(EditCommandPtr command);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clear();

signals:
    void stateChanged();

private:
    QList<EditCommandPtr> m_undoStack;
    QList<EditCommandPtr> m_redoStack;
    bool m_isProcessing = false;
};

} // namespace gopost::image_editor
