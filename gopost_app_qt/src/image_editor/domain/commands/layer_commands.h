#pragma once

#include "image_editor/domain/entities/edit_command.h"
#include "rendering_bridge/engine_types.h"

#include <QByteArray>

#include <optional>

namespace gopost::image_editor {

// Forward declaration — the presentation layer provides the concrete type.
class CanvasNotifier;

// -------------------------------------------------------------------------
// AddSolidLayerCommand
// -------------------------------------------------------------------------

/// Command: add solid color layer. Undo = remove that layer.
class AddSolidLayerCommand : public EditCommand {
public:
    AddSolidLayerCommand(CanvasNotifier& notifier,
                         double r, double g, double b, double a);

    void execute() override;
    void undo() override;

private:
    CanvasNotifier& m_notifier;
    double m_r, m_g, m_b, m_a;
    std::optional<int> m_addedLayerId;
};

// -------------------------------------------------------------------------
// AddImageLayerCommand
// -------------------------------------------------------------------------

/// Command: add image layer. Undo = remove that layer.
class AddImageLayerCommand : public EditCommand {
public:
    AddImageLayerCommand(CanvasNotifier& notifier,
                         const QByteArray& pixels, int width, int height);

    void execute() override;
    void undo() override;

private:
    CanvasNotifier& m_notifier;
    QByteArray m_pixels;
    int m_width;
    int m_height;
    std::optional<int> m_addedLayerId;
};

// -------------------------------------------------------------------------
// RemoveLayerCommand
// -------------------------------------------------------------------------

/// Command: remove layer. Undo = no-op (full restore would need stored pixels).
class RemoveLayerCommand : public EditCommand {
public:
    RemoveLayerCommand(CanvasNotifier& notifier, int layerId);

    void execute() override;
    void undo() override;

private:
    CanvasNotifier& m_notifier;
    int m_layerId;
};

// -------------------------------------------------------------------------
// ReorderLayerCommand
// -------------------------------------------------------------------------

/// Command: reorder layer. Undo = reorder back.
class ReorderLayerCommand : public EditCommand {
public:
    ReorderLayerCommand(CanvasNotifier& notifier,
                        int layerId, int oldIndex, int newIndex);

    void execute() override;
    void undo() override;

private:
    CanvasNotifier& m_notifier;
    int m_layerId;
    int m_oldIndex;
    int m_newIndex;
};

// -------------------------------------------------------------------------
// SetLayerOpacityCommand
// -------------------------------------------------------------------------

/// Command: set layer opacity. Undo = restore previous opacity.
class SetLayerOpacityCommand : public EditCommand {
public:
    SetLayerOpacityCommand(CanvasNotifier& notifier,
                           int layerId, double previousOpacity, double newOpacity);

    void execute() override;
    void undo() override;

private:
    CanvasNotifier& m_notifier;
    int m_layerId;
    double m_previousOpacity;
    double m_newOpacity;
};

// -------------------------------------------------------------------------
// SetLayerVisibleCommand
// -------------------------------------------------------------------------

/// Command: set layer visible. Undo = restore previous visible.
class SetLayerVisibleCommand : public EditCommand {
public:
    SetLayerVisibleCommand(CanvasNotifier& notifier,
                           int layerId, bool previousVisible, bool newVisible);

    void execute() override;
    void undo() override;

private:
    CanvasNotifier& m_notifier;
    int m_layerId;
    bool m_previousVisible;
    bool m_newVisible;
};

// -------------------------------------------------------------------------
// SetLayerBlendModeCommand
// -------------------------------------------------------------------------

/// Command: set layer blend mode. Undo = restore previous blend mode.
class SetLayerBlendModeCommand : public EditCommand {
public:
    SetLayerBlendModeCommand(CanvasNotifier& notifier,
                             int layerId,
                             rendering::BlendMode previousMode,
                             rendering::BlendMode newMode);

    void execute() override;
    void undo() override;

private:
    CanvasNotifier& m_notifier;
    int m_layerId;
    rendering::BlendMode m_previousMode;
    rendering::BlendMode m_newMode;
};

} // namespace gopost::image_editor
