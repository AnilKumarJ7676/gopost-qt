#pragma once

#include <QDateTime>
#include <QString>

#include <memory>

namespace gopost::image_editor {

/// Base class for all undoable editing operations.
class EditCommand {
public:
    explicit EditCommand(const QString& description)
        : m_description(description)
        , m_timestamp(QDateTime::currentDateTime()) {}

    virtual ~EditCommand() = default;

    EditCommand(const EditCommand&) = delete;
    EditCommand& operator=(const EditCommand&) = delete;

    /// Execute the command (do/redo).
    virtual void execute() = 0;

    /// Reverse the command (undo).
    virtual void undo() = 0;

    [[nodiscard]] const QString& description() const { return m_description; }
    [[nodiscard]] const QDateTime& timestamp() const { return m_timestamp; }

private:
    QString m_description;
    QDateTime m_timestamp;
};

using EditCommandPtr = std::unique_ptr<EditCommand>;

} // namespace gopost::image_editor
