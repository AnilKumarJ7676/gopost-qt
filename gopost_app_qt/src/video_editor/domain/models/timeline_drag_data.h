#pragma once

#include <QString>
#include <variant>

#include "video_editor/domain/models/media_asset.h"
#include "video_editor/domain/models/video_effect.h"
#include "video_editor/domain/models/video_transition.h"

namespace gopost::video_editor {

// ── Drag data types (sealed class hierarchy → std::variant) ─────────────────

struct EffectDragData {
    EffectType effectType{EffectType::Brightness};
    int iconCodePoint{0}; // Qt icon code point (replaces Flutter IconData)
};

struct TransitionDragData {
    TransitionType transitionType{TransitionType::None};
    bool isIn{true};
    int iconCodePoint{0};
};

/// Dragged from the AdjustmentLayerPanel onto an effect track.
struct AdjustmentClipDragData {
    AdjustmentClipData data;
    int iconCodePoint{0};
    QString label;
};

/// Dragged from the presets strip — shortcut for a preset-based adjustment clip.
struct PresetClipDragData {
    PresetFilterId preset{PresetFilterId::None};
    int iconCodePoint{0};
};

/// Dragged from the Media Pool onto the timeline.
struct MediaAssetDragData {
    MediaAsset asset;
};

/// The variant type covering all timeline drag data possibilities.
using TimelineDragData = std::variant<
    EffectDragData,
    TransitionDragData,
    AdjustmentClipDragData,
    PresetClipDragData,
    MediaAssetDragData
>;

} // namespace gopost::video_editor
