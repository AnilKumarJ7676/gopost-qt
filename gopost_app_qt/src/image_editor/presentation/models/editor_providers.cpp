#include "image_editor/presentation/models/editor_providers.h"
#include "image_editor/presentation/models/canvas_notifier.h"
#include "image_editor/presentation/models/adjustment_notifier.h"
#include "image_editor/presentation/models/filter_notifier.h"
#include "image_editor/presentation/models/photo_import_notifier.h"
#include "image_editor/presentation/models/text_notifier.h"
#include "image_editor/presentation/models/tool_notifier.h"
#include "image_editor/presentation/models/undo_redo_notifier.h"
#include "rendering_bridge/engine_api.h"

#include <QQmlEngine>

namespace gopost::image_editor {

EditorProviders::EditorProviders(rendering::ImageEditorEngine* engine,
                                 QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
    // Create all notifiers with proper dependencies (DIP)
    m_canvas = new CanvasNotifier(engine, this);
    m_adjustment = new AdjustmentNotifier(engine, this);
    m_filter = new FilterNotifier(engine, m_canvas, this);
    m_photoImport = new PhotoImportNotifier(engine, this);
    m_text = new TextNotifier(engine, this);
    m_tool = new ToolNotifier(this);
    m_undoRedo = new UndoRedoNotifier(this);

    // Prevent QML engine from taking ownership
    QQmlEngine::setObjectOwnership(m_canvas, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_adjustment, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_filter, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_photoImport, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_text, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_tool, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_undoRedo, QQmlEngine::CppOwnership);
}

EditorProviders::~EditorProviders() = default;

void EditorProviders::registerQmlTypes()
{
    qmlRegisterUncreatableType<CanvasNotifier>(
        "GopostApp", 1, 0, "CanvasNotifier",
        QStringLiteral("Created by EditorProviders"));
    qmlRegisterUncreatableType<AdjustmentNotifier>(
        "GopostApp", 1, 0, "AdjustmentNotifier",
        QStringLiteral("Created by EditorProviders"));
    qmlRegisterUncreatableType<FilterNotifier>(
        "GopostApp", 1, 0, "FilterNotifier",
        QStringLiteral("Created by EditorProviders"));
    qmlRegisterUncreatableType<PhotoImportNotifier>(
        "GopostApp", 1, 0, "PhotoImportNotifier",
        QStringLiteral("Created by EditorProviders"));
    qmlRegisterUncreatableType<TextNotifier>(
        "GopostApp", 1, 0, "TextNotifier",
        QStringLiteral("Created by EditorProviders"));
    qmlRegisterUncreatableType<ToolNotifier>(
        "GopostApp", 1, 0, "ToolNotifier",
        QStringLiteral("Created by EditorProviders"));
    qmlRegisterUncreatableType<UndoRedoNotifier>(
        "GopostApp", 1, 0, "UndoRedoNotifier",
        QStringLiteral("Created by EditorProviders"));
    qmlRegisterType<EditorProviders>(
        "GopostApp", 1, 0, "EditorProviders");
}

} // namespace gopost::image_editor
