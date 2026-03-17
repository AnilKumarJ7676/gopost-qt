#pragma once

#include <QObject>
#include <QQmlEngine>
#include <memory>

#include "image_editor/presentation/models/canvas_notifier.h"
#include "image_editor/presentation/models/adjustment_notifier.h"
#include "image_editor/presentation/models/filter_notifier.h"
#include "image_editor/presentation/models/photo_import_notifier.h"
#include "image_editor/presentation/models/text_notifier.h"
#include "image_editor/presentation/models/tool_notifier.h"
#include "image_editor/presentation/models/undo_redo_notifier.h"
#include "rendering_bridge/engine_api.h"

namespace gopost::image_editor {

// -------------------------------------------------------------------------
// EditorProviders — factory/wiring class that creates all notifiers
// with proper dependencies and registers them for QML access
// -------------------------------------------------------------------------

class EditorProviders : public QObject {
    Q_OBJECT

    Q_PROPERTY(CanvasNotifier* canvas READ canvas CONSTANT)
    Q_PROPERTY(AdjustmentNotifier* adjustment READ adjustment CONSTANT)
    Q_PROPERTY(FilterNotifier* filter READ filter CONSTANT)
    Q_PROPERTY(PhotoImportNotifier* photoImport READ photoImport CONSTANT)
    Q_PROPERTY(TextNotifier* text READ text CONSTANT)
    Q_PROPERTY(ToolNotifier* tool READ tool CONSTANT)
    Q_PROPERTY(UndoRedoNotifier* undoRedo READ undoRedo CONSTANT)

public:
    explicit EditorProviders(rendering::ImageEditorEngine* engine,
                             QObject* parent = nullptr);
    ~EditorProviders() override;

    CanvasNotifier* canvas() const { return m_canvas; }
    AdjustmentNotifier* adjustment() const { return m_adjustment; }
    FilterNotifier* filter() const { return m_filter; }
    PhotoImportNotifier* photoImport() const { return m_photoImport; }
    TextNotifier* text() const { return m_text; }
    ToolNotifier* tool() const { return m_tool; }
    UndoRedoNotifier* undoRedo() const { return m_undoRedo; }

    /// Register all QML types under "GopostApp 1.0"
    static void registerQmlTypes();

private:
    rendering::ImageEditorEngine* m_engine = nullptr;

    CanvasNotifier* m_canvas = nullptr;
    AdjustmentNotifier* m_adjustment = nullptr;
    FilterNotifier* m_filter = nullptr;
    PhotoImportNotifier* m_photoImport = nullptr;
    TextNotifier* m_text = nullptr;
    ToolNotifier* m_tool = nullptr;
    UndoRedoNotifier* m_undoRedo = nullptr;
};

} // namespace gopost::image_editor
