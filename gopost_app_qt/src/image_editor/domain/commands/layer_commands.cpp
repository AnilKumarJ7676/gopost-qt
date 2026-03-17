#include "image_editor/domain/commands/layer_commands.h"

// CanvasNotifier is defined in the presentation layer.  The header that
// provides it will be included by the build unit that actually instantiates
// these commands.  For the command implementations we only need the
// minimal interface declared below.  A full include would create a
// circular dependency between domain ↔ presentation, so we rely on the
// forward-declared class and define a thin abstract interface here that
// CanvasNotifier must satisfy.
//
// In practice, the presentation/providers/canvas_notifier.h header is
// included by translation units that create command objects, so the
// linker resolves all calls.

#include "image_editor/presentation/models/canvas_notifier_interface.h"

namespace gopost::image_editor {

// =========================================================================
// AddSolidLayerCommand
// =========================================================================

AddSolidLayerCommand::AddSolidLayerCommand(CanvasNotifier& notifier,
                                           double r, double g, double b, double a)
    : EditCommand(QStringLiteral("Add solid layer"))
    , m_notifier(notifier)
    , m_r(r), m_g(g), m_b(b), m_a(a) {}

void AddSolidLayerCommand::execute() {
    m_addedLayerId = m_notifier.addSolidLayer(m_r, m_g, m_b, m_a);
}

void AddSolidLayerCommand::undo() {
    if (m_addedLayerId.has_value()) {
        m_notifier.removeLayer(m_addedLayerId.value());
        m_addedLayerId.reset();
    }
}

// =========================================================================
// AddImageLayerCommand
// =========================================================================

AddImageLayerCommand::AddImageLayerCommand(CanvasNotifier& notifier,
                                           const QByteArray& pixels,
                                           int width, int height)
    : EditCommand(QStringLiteral("Add image layer"))
    , m_notifier(notifier)
    , m_pixels(pixels)
    , m_width(width)
    , m_height(height) {}

void AddImageLayerCommand::execute() {
    m_addedLayerId = m_notifier.addImageLayer(m_pixels, m_width, m_height);
}

void AddImageLayerCommand::undo() {
    if (m_addedLayerId.has_value()) {
        m_notifier.removeLayer(m_addedLayerId.value());
        m_addedLayerId.reset();
    }
}

// =========================================================================
// RemoveLayerCommand
// =========================================================================

RemoveLayerCommand::RemoveLayerCommand(CanvasNotifier& notifier, int layerId)
    : EditCommand(QStringLiteral("Remove layer"))
    , m_notifier(notifier)
    , m_layerId(layerId) {}

void RemoveLayerCommand::execute() {
    m_notifier.removeLayer(m_layerId);
}

void RemoveLayerCommand::undo() {
    // Full undo would require storing layer content; not implemented.
}

// =========================================================================
// ReorderLayerCommand
// =========================================================================

ReorderLayerCommand::ReorderLayerCommand(CanvasNotifier& notifier,
                                         int layerId, int oldIndex, int newIndex)
    : EditCommand(QStringLiteral("Reorder layer"))
    , m_notifier(notifier)
    , m_layerId(layerId)
    , m_oldIndex(oldIndex)
    , m_newIndex(newIndex) {}

void ReorderLayerCommand::execute() {
    m_notifier.reorderLayer(m_layerId, m_newIndex);
}

void ReorderLayerCommand::undo() {
    m_notifier.reorderLayer(m_layerId, m_oldIndex);
}

// =========================================================================
// SetLayerOpacityCommand
// =========================================================================

SetLayerOpacityCommand::SetLayerOpacityCommand(CanvasNotifier& notifier,
                                               int layerId,
                                               double previousOpacity,
                                               double newOpacity)
    : EditCommand(QStringLiteral("Set opacity"))
    , m_notifier(notifier)
    , m_layerId(layerId)
    , m_previousOpacity(previousOpacity)
    , m_newOpacity(newOpacity) {}

void SetLayerOpacityCommand::execute() {
    m_notifier.setLayerOpacity(m_layerId, m_newOpacity);
}

void SetLayerOpacityCommand::undo() {
    m_notifier.setLayerOpacity(m_layerId, m_previousOpacity);
}

// =========================================================================
// SetLayerVisibleCommand
// =========================================================================

SetLayerVisibleCommand::SetLayerVisibleCommand(CanvasNotifier& notifier,
                                               int layerId,
                                               bool previousVisible,
                                               bool newVisible)
    : EditCommand(QStringLiteral("Set visibility"))
    , m_notifier(notifier)
    , m_layerId(layerId)
    , m_previousVisible(previousVisible)
    , m_newVisible(newVisible) {}

void SetLayerVisibleCommand::execute() {
    m_notifier.setLayerVisible(m_layerId, m_newVisible);
}

void SetLayerVisibleCommand::undo() {
    m_notifier.setLayerVisible(m_layerId, m_previousVisible);
}

// =========================================================================
// SetLayerBlendModeCommand
// =========================================================================

SetLayerBlendModeCommand::SetLayerBlendModeCommand(CanvasNotifier& notifier,
                                                   int layerId,
                                                   rendering::BlendMode previousMode,
                                                   rendering::BlendMode newMode)
    : EditCommand(QStringLiteral("Set blend mode"))
    , m_notifier(notifier)
    , m_layerId(layerId)
    , m_previousMode(previousMode)
    , m_newMode(newMode) {}

void SetLayerBlendModeCommand::execute() {
    m_notifier.setLayerBlendMode(m_layerId, m_newMode);
}

void SetLayerBlendModeCommand::undo() {
    m_notifier.setLayerBlendMode(m_layerId, m_previousMode);
}

} // namespace gopost::image_editor
