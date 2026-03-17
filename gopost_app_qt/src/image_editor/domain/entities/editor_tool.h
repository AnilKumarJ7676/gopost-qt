#pragma once

#include <QString>

#include <array>
#include <cstdint>

namespace gopost::image_editor {

/// Active tool in the image editor toolbar.
enum class EditorTool : uint8_t {
    Select,
    Move,
    Layers,
    AddImage,
    AddText,
    AddShape,
    Filter,
    Adjust,
    Crop,
    Mask,
    Sticker,
    Draw,
    Eraser,
    Export,
    Count_ // sentinel for iteration
};

/// Display metadata for toolbar rendering.
struct EditorToolInfo {
    static QString label(EditorTool tool) {
        switch (tool) {
        case EditorTool::Select:   return QStringLiteral("Select");
        case EditorTool::Move:     return QStringLiteral("Move");
        case EditorTool::Layers:   return QStringLiteral("Layers");
        case EditorTool::AddImage: return QStringLiteral("Image");
        case EditorTool::AddText:  return QStringLiteral("Text");
        case EditorTool::AddShape: return QStringLiteral("Shape");
        case EditorTool::Filter:   return QStringLiteral("Filter");
        case EditorTool::Adjust:   return QStringLiteral("Adjust");
        case EditorTool::Crop:     return QStringLiteral("Crop");
        case EditorTool::Mask:     return QStringLiteral("Mask");
        case EditorTool::Sticker:  return QStringLiteral("Sticker");
        case EditorTool::Draw:     return QStringLiteral("Draw");
        case EditorTool::Eraser:   return QStringLiteral("Eraser");
        case EditorTool::Export:   return QStringLiteral("Export");
        default:                   return {};
        }
    }

    static QString iconName(EditorTool tool) {
        switch (tool) {
        case EditorTool::Select:   return QStringLiteral("touch_app");
        case EditorTool::Move:     return QStringLiteral("open_with");
        case EditorTool::Layers:   return QStringLiteral("layers");
        case EditorTool::AddImage: return QStringLiteral("add_photo_alternate");
        case EditorTool::AddText:  return QStringLiteral("text_fields");
        case EditorTool::AddShape: return QStringLiteral("category");
        case EditorTool::Filter:   return QStringLiteral("auto_fix_high");
        case EditorTool::Adjust:   return QStringLiteral("tune");
        case EditorTool::Crop:     return QStringLiteral("crop");
        case EditorTool::Mask:     return QStringLiteral("gradient");
        case EditorTool::Sticker:  return QStringLiteral("emoji_emotions");
        case EditorTool::Draw:     return QStringLiteral("brush");
        case EditorTool::Eraser:   return QStringLiteral("auto_fix_off");
        case EditorTool::Export:   return QStringLiteral("save_alt");
        default:                   return {};
        }
    }
};

} // namespace gopost::image_editor
