#include "image_editor/presentation/models/tool_notifier.h"

namespace gopost::image_editor {

ToolNotifier::ToolNotifier(QObject* parent)
    : QObject(parent)
{
}

void ToolNotifier::selectTool(int tool)
{
    if (m_activeTool != tool) {
        m_activeTool = tool;
        emit toolChanged(tool);
    }
}

} // namespace gopost::image_editor
