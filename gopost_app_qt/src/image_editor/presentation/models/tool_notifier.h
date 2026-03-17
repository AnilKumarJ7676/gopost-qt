#pragma once

#include <QObject>

namespace gopost::image_editor {

// -------------------------------------------------------------------------
// ToolNotifier — manages the active tool selection in the editor
// -------------------------------------------------------------------------

class ToolNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(int activeTool READ activeTool WRITE selectTool NOTIFY toolChanged)

public:
    enum EditorTool {
        Select = 0,
        Move,
        Layers,
        AddImage,
        AddText,
        AddShape,
        Sticker,
        Filter,
        Adjust,
        Crop,
        Mask,
        Draw,
        Eraser,
        Export
    };
    Q_ENUM(EditorTool)

    explicit ToolNotifier(QObject* parent = nullptr);

    int activeTool() const { return m_activeTool; }

    Q_INVOKABLE void selectTool(int tool);

signals:
    void toolChanged(int tool);

private:
    int m_activeTool = Select;
};

} // namespace gopost::image_editor
