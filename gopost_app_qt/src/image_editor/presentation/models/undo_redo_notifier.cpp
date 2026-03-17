#include "image_editor/presentation/models/undo_redo_notifier.h"

#include <QDebug>

namespace gopost::image_editor {

UndoRedoNotifier::UndoRedoNotifier(QObject* parent)
    : QObject(parent)
{
}

void UndoRedoNotifier::execute(EditCommandPtr command)
{
    m_isProcessing = true;
    emit stateChanged();

    try {
        command->execute();

        m_undoStack.append(command);
        if (m_undoStack.size() > MaxUndoLevels) {
            m_undoStack.removeFirst();
        }

        // Clear redo stack on new action
        m_redoStack.clear();

        m_isProcessing = false;
        emit stateChanged();
    } catch (const std::exception& e) {
        m_isProcessing = false;
        emit stateChanged();
        qWarning() << "UndoRedoNotifier::execute failed:" << e.what();
    }
}

void UndoRedoNotifier::undo()
{
    if (!canUndo()) return;

    m_isProcessing = true;
    emit stateChanged();

    try {
        auto command = m_undoStack.last();
        command->undo();

        m_undoStack.removeLast();
        m_redoStack.append(command);

        m_isProcessing = false;
        emit stateChanged();
    } catch (const std::exception& e) {
        m_isProcessing = false;
        emit stateChanged();
        qWarning() << "UndoRedoNotifier::undo failed:" << e.what();
    }
}

void UndoRedoNotifier::redo()
{
    if (!canRedo()) return;

    m_isProcessing = true;
    emit stateChanged();

    try {
        auto command = m_redoStack.last();
        command->execute();

        m_redoStack.removeLast();
        m_undoStack.append(command);

        m_isProcessing = false;
        emit stateChanged();
    } catch (const std::exception& e) {
        m_isProcessing = false;
        emit stateChanged();
        qWarning() << "UndoRedoNotifier::redo failed:" << e.what();
    }
}

void UndoRedoNotifier::clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
    m_isProcessing = false;
    emit stateChanged();
}

} // namespace gopost::image_editor
