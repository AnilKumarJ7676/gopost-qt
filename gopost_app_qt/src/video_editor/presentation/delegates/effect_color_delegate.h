#pragma once

#include <optional>
#include <QString>
#include <vector>

#include "video_editor/domain/models/video_effect.h"
#include "video_editor/domain/models/video_transition.h"
#include "video_editor/presentation/delegates/timeline_operations.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// EffectColorDelegate — effects, color grading, transitions, blend modes,
//                        adjustment layers
//
// Converted 1:1 from effect_color_delegate.dart.
// Plain C++ class (not QObject).
// ---------------------------------------------------------------------------
class EffectColorDelegate {
public:
    explicit EffectColorDelegate(TimelineOperations* ops);
    ~EffectColorDelegate();

    // ---- per-clip effects --------------------------------------------------
    void addEffect(int clipId, const VideoEffect& effect);
    void removeEffect(int clipId, EffectType type);
    void toggleEffect(int clipId, EffectType type);
    void updateEffect(int clipId, EffectType type, double value);
    void clearEffects(int clipId);

    // ---- color grading (live preview + commit) ----------------------------
    void setColorGrading(int clipId, const ColorGrading& grading);
    void commitColorGrading(int clipId, const VideoProject& before);
    void resetColorGrading(int clipId);

    // ---- preset filters ---------------------------------------------------
    void applyPresetFilter(int clipId, PresetFilterId preset);
    static const ColorGrading& presetGrading(PresetFilterId preset);

    // ---- transitions -------------------------------------------------------
    void setTransitionIn(int clipId, const ClipTransition& t);
    void setTransitionOut(int clipId, const ClipTransition& t);
    void removeTransitionIn(int clipId);
    void removeTransitionOut(int clipId);

    // ---- opacity / blend mode ----------------------------------------------
    void setClipOpacity(int clipId, double opacity);
    void commitClipOpacity(int clipId, const VideoProject& before);
    void setClipBlendMode(int clipId, int blendMode);

    // ---- adjustment layers -------------------------------------------------
    void createAdjustmentClip(const AdjustmentClipData& data, double atTime);
    void updateAdjustmentClip(int clipId, const AdjustmentClipData& data);

private:
    TimelineOperations* ops_;

    // Static preset map (lazy-initialised)
    static std::vector<std::pair<PresetFilterId, ColorGrading>> buildPresetMap();
};

} // namespace gopost::video_editor
