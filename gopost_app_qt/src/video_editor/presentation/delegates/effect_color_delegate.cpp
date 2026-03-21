#include "video_editor/presentation/delegates/effect_color_delegate.h"

#include <algorithm>

namespace gopost::video_editor {

EffectColorDelegate::EffectColorDelegate(TimelineOperations* ops) : ops_(ops) {}
EffectColorDelegate::~EffectColorDelegate() = default;

// ---------------------------------------------------------------------------
// Per-clip effects
// ---------------------------------------------------------------------------

void EffectColorDelegate::addEffect(int clipId, const VideoEffect& effect) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&](const VideoClip& c) {
        VideoClip updated = c;
        // Replace if same type already exists
        auto it = std::find_if(updated.effects.begin(), updated.effects.end(),
                               [&](const VideoEffect& e) { return e.type == effect.type; });
        if (it != updated.effects.end()) *it = effect;
        else updated.effects.push_back(effect);
        return updated;
    });
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
}

void EffectColorDelegate::removeEffect(int clipId, EffectType type) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [type](const VideoClip& c) {
        VideoClip updated = c;
        updated.effects.erase(
            std::remove_if(updated.effects.begin(), updated.effects.end(),
                           [type](const VideoEffect& e) { return e.type == type; }),
            updated.effects.end());
        return updated;
    });
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
}

void EffectColorDelegate::toggleEffect(int clipId, EffectType type) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [type](const VideoClip& c) {
        VideoClip updated = c;
        for (auto& e : updated.effects) {
            if (e.type == type) { e.enabled = !e.enabled; break; }
        }
        return updated;
    });
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
}

void EffectColorDelegate::updateEffect(int clipId, EffectType type, double value) {
    ops_->updateClip(clipId, [type, value](const VideoClip& c) {
        VideoClip updated = c;
        for (auto& e : updated.effects) {
            if (e.type == type) { e.value = value; break; }
        }
        return updated;
    });
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void EffectColorDelegate::clearEffects(int clipId) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [](const VideoClip& c) {
        VideoClip updated = c;
        updated.effects.clear();
        return updated;
    });
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
}

// ---------------------------------------------------------------------------
// Color grading
// ---------------------------------------------------------------------------

void EffectColorDelegate::setColorGrading(int clipId, const ColorGrading& grading) {
    ops_->updateClip(clipId, [&grading](const VideoClip& c) {
        VideoClip updated = c;
        updated.colorGrading = grading;
        return updated;
    });
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void EffectColorDelegate::commitColorGrading(int clipId, const VideoProject& before) {
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
}

void EffectColorDelegate::resetColorGrading(int clipId) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [](const VideoClip& c) {
        VideoClip updated = c;
        updated.colorGrading = ColorGrading{};
        return updated;
    });
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
}

// ---------------------------------------------------------------------------
// Preset filters (static mapping)
// ---------------------------------------------------------------------------

std::vector<std::pair<PresetFilterId, ColorGrading>> EffectColorDelegate::buildPresetMap() {
    std::vector<std::pair<PresetFilterId, ColorGrading>> map;

    auto make = [](double bright, double contrast, double sat, double temp,
                   double tint, double shadows, double highlights, double vibrance,
                   double fade, double vignette) {
        ColorGrading g;
        g.brightness  = bright;   g.contrast   = contrast;
        g.saturation  = sat;      g.temperature = temp;
        g.tint        = tint;     g.shadows     = shadows;
        g.highlights  = highlights; g.vibrance  = vibrance;
        g.fade        = fade;     g.vignette    = vignette;
        return g;
    };

    // Lighting & Scene
    map.emplace_back(PresetFilterId::Natural,      make(   2,   3,   5,    0,  0,    0,   0,   5,  0,  0));
    map.emplace_back(PresetFilterId::Daylight,     make(   5,   5,   8,   10, -3,    5,   5,   8,  0,  0));
    map.emplace_back(PresetFilterId::GoldenHour,   make(  10,   5,  10,   35, 10,   -5,  10,  12,  0,  8));
    map.emplace_back(PresetFilterId::Overcast,     make(  -3,  -5,  -8,  -10,  0,    5,  -5,  -5,  3,  0));
    map.emplace_back(PresetFilterId::Studio,       make(   0,  10,   0,    0,  0,   -5,   5,   0,  0,  5));

    // Portrait
    map.emplace_back(PresetFilterId::Portrait,     make(   5,  -5,   5,    5,  3,    5,   5,   5,  0,  8));
    map.emplace_back(PresetFilterId::SoftSkin,     make(   8, -10,  -5,    8,  5,   10,  10,   0, 10, 10));
    map.emplace_back(PresetFilterId::WarmPortrait, make(   8,  -3,   5,   18,  8,    5,   8,   5,  5, 10));

    // Film & Vintage
    map.emplace_back(PresetFilterId::Vintage,      make(   5, -10, -20,   15, 10,  -10,  10,   0, 20, 15));
    map.emplace_back(PresetFilterId::Polaroid,     make(  10, -10, -10,   12,  8,    5,  15,   0, 20, 10));
    map.emplace_back(PresetFilterId::Kodachrome,   make(   5,  15,  15,   10,  5,  -10,  10,  10,  0, 12));
    map.emplace_back(PresetFilterId::Retro,        make(   8, -10, -15,   12, 10,   -8,   5,  -5, 15, 10));
    map.emplace_back(PresetFilterId::Retro70s,     make(   8,  -5, -15,   20, 15,   -5,  10,   0, 15, 12));

    // Cinematic
    map.emplace_back(PresetFilterId::Cinematic,    make(  -5,  10,  -8,  -10,  0,  -15,  -5,   0,  5, 20));
    map.emplace_back(PresetFilterId::TealOrange,   make(   0,  12,   8,  -15,-10,  -15,   5,  15,  0, 10));
    map.emplace_back(PresetFilterId::Noir,         make( -15,  30, -70,    0,  0,  -25, -10,   0,  5, 35));
    map.emplace_back(PresetFilterId::FilmNoir,     make( -10,  25, -60,    0,  0,  -20,  -5,   0, 10, 30));
    map.emplace_back(PresetFilterId::Moody,        make( -10,  15, -12,   -5,  5,  -20, -10,   0, 10, 25));

    // Black & White
    map.emplace_back(PresetFilterId::Desaturated,  make(   0,   5, -40,    0,  0,    0,   0, -20,  0,  0));
    map.emplace_back(PresetFilterId::BWClassic,    make(   0,  10, -100,   0,  0,    0,   0,   0,  0,  0));
    map.emplace_back(PresetFilterId::BWHigh,       make(   0,  30, -100,   0,  0,  -15,  10,   0,  0,  5));
    map.emplace_back(PresetFilterId::BWSelenium,   make(   5,  15, -100,  12,  8,   -5,   5,   0,  5,  8));
    map.emplace_back(PresetFilterId::BWInfrared,   make(  15,  25, -100,   0,  0,  -20,  20,   0,  0, 10));

    // Creative
    map.emplace_back(PresetFilterId::WarmSunset,   make(   8,   5,  10,   30,  5,    0,   5,  10,  0, 10));
    map.emplace_back(PresetFilterId::CoolBlue,     make(  -3,   8,  -5,  -25,  0,   -5,  -8,   0,  0,  5));
    map.emplace_back(PresetFilterId::HighContrast, make(   0,  30,   5,    0,  0,  -10,  10,   5,  0,  0));
    map.emplace_back(PresetFilterId::SoftPastel,   make(  10, -15, -10,    5,  5,   10,  15,   0, 15,  5));
    map.emplace_back(PresetFilterId::BleachBypass, make(  -5,  20, -25,    0,  0,  -15,   5,   0,  5, 10));
    map.emplace_back(PresetFilterId::CrossProcess, make(   5,  10,  15,   10,-10,  -10,  10,  10,  0,  5));
    map.emplace_back(PresetFilterId::OrangeTeal,   make(   0,  10,  10,   20,-15,  -15,   5,  15,  0,  8));
    map.emplace_back(PresetFilterId::Dreamy,       make(  15, -20,  -5,   10, 10,   15,  20,   0, 25, 15));
    map.emplace_back(PresetFilterId::Hdr,          make(   0,  25,  10,    0,  0,  -20,  20,  15,  0,  5));
    map.emplace_back(PresetFilterId::Matte,        make(   5,  -5, -10,    0,  0,   15, -10,   0, 30,  8));
    map.emplace_back(PresetFilterId::Cyberpunk,    make( -10,  20,  20,  -20,-10,  -15,   5,  20,  0, 20));
    map.emplace_back(PresetFilterId::Golden,       make(  12,   5,   5,   25, 10,    0,  10,   8,  0,  5));
    map.emplace_back(PresetFilterId::Arctic,       make(   5,  10,  -8,  -30, -5,    5, -10,   0,  5,  0));
    return map;
}

const ColorGrading& EffectColorDelegate::presetGrading(PresetFilterId preset) {
    static const auto map = buildPresetMap();
    static const ColorGrading kDefault{};
    for (const auto& [id, grading] : map)
        if (id == preset) return grading;
    return kDefault;
}

void EffectColorDelegate::applyPresetFilter(int clipId, PresetFilterId preset) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    setColorGrading(clipId, presetGrading(preset));
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
}

// ---------------------------------------------------------------------------
// Transitions
// ---------------------------------------------------------------------------

void EffectColorDelegate::setTransitionIn(int clipId, const ClipTransition& t) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&t](const VideoClip& c) {
        VideoClip u = c; u.transitionIn = t; return u;
    });
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void EffectColorDelegate::setTransitionOut(int clipId, const ClipTransition& t) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&t](const VideoClip& c) {
        VideoClip u = c; u.transitionOut = t; return u;
    });
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void EffectColorDelegate::removeTransitionIn(int clipId) {
    setTransitionIn(clipId, ClipTransition{});
}

void EffectColorDelegate::removeTransitionOut(int clipId) {
    setTransitionOut(clipId, ClipTransition{});
}

// ---------------------------------------------------------------------------
// Opacity / Blend mode
// ---------------------------------------------------------------------------

void EffectColorDelegate::setClipOpacity(int clipId, double opacity) {
    ops_->updateClip(clipId, [opacity](const VideoClip& c) {
        VideoClip u = c; u.opacity = std::clamp(opacity, 0.0, 1.0); return u;
    });
    ops_->syncEffectsToEngine(clipId);
    ops_->debouncedRenderFrame();
}

void EffectColorDelegate::commitClipOpacity(int clipId, const VideoProject& before) {
    ops_->pushUndo(before);
}

void EffectColorDelegate::setClipBlendMode(int clipId, int blendMode) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [blendMode](const VideoClip& c) {
        VideoClip u = c; u.blendMode = blendMode; return u;
    });
    ops_->pushUndo(before);
}

// ---------------------------------------------------------------------------
// Adjustment layers
// ---------------------------------------------------------------------------

void EffectColorDelegate::createAdjustmentClip(const AdjustmentClipData& data, double atTime) {
    auto state = ops_->currentState();
    if (!state.project) return;

    // Find or create an effect track
    int effectTrackIdx = -1;
    for (const auto& t : state.project->tracks) {
        if (t.type == TrackType::Effect) { effectTrackIdx = t.index; break; }
    }

    if (effectTrackIdx < 0) {
        VideoTrack et;
        et.type  = TrackType::Effect;
        et.index = static_cast<int>(state.project->tracks.size());
        et.label = QStringLiteral("Effect 1");
        state.project->tracks.push_back(std::move(et));
        effectTrackIdx = et.index;
        ops_->setState(std::move(state));
        state = ops_->currentState();
    }

    auto before = *state.project;

    VideoClip clip;
    clip.id          = 0; // will be assigned by addClip logic below
    clip.trackIndex  = effectTrackIdx;
    clip.sourceType  = ClipSourceType::Adjustment;
    clip.displayName = QStringLiteral("Adjustment");
    clip.timelineIn  = atTime;
    clip.timelineOut = atTime + 5.0;
    clip.effects     = data.effects;

    // Assign id
    int maxId = 0;
    for (const auto& t : state.project->tracks)
        for (const auto& c : t.clips)
            maxId = std::max(maxId, c.id);
    clip.id = maxId + 1;

    state.project->tracks[effectTrackIdx].clips.push_back(std::move(clip));
    ops_->setState(std::move(state));
    ops_->pushUndo(before);
    ops_->syncNativeToProject();
}

void EffectColorDelegate::updateAdjustmentClip(int clipId, const AdjustmentClipData& data) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [&data](const VideoClip& c) {
        VideoClip u = c;
        u.effects = data.effects;
        return u;
    });
    ops_->pushUndo(before);
    ops_->syncEffectsToEngine(clipId);
}

} // namespace gopost::video_editor
