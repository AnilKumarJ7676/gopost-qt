#pragma once

#include <functional>
#include <memory>

#include "video_editor/domain/models/video_project.h"
#include "video_editor/presentation/notifiers/timeline_state.h"

namespace gopost::video_editor {

class NativeVideoEngine;
class ProxyGenerationService;
class UndoRedoStack;

// ---------------------------------------------------------------------------
// TimelineOperations — abstract interface shared by all delegates
//
// Mirrors the Dart TimelineOperations mixin.  Each delegate receives a
// non-owning pointer to the concrete TimelineNotifier that implements this
// interface, so delegates never depend on each other directly.
// ---------------------------------------------------------------------------
class TimelineOperations {
public:
    virtual ~TimelineOperations() = default;

    // ---- state access ------------------------------------------------------
    virtual const TimelineState& currentState() const = 0;
    virtual void              setState(TimelineState state) = 0;

    // ---- collaborators -----------------------------------------------------
    virtual NativeVideoEngine*      engine()       = 0;
    virtual ProxyGenerationService* proxyService() = 0;
    virtual UndoRedoStack*          undoRedo()     = 0;

    // ---- lifecycle ---------------------------------------------------------
    virtual bool isMounted() const = 0;

    // ---- undo helpers ------------------------------------------------------
    virtual void pushUndo(const VideoProject& before) = 0;
    virtual void syncUndoRedoState()                   = 0;

    // ---- clip helpers ------------------------------------------------------
    virtual void updateClip(int clipId,
                            std::function<VideoClip(const VideoClip&)> mutator) = 0;

    // ---- render pipeline ---------------------------------------------------
    virtual void renderCurrentFrame()      = 0;
    virtual void debouncedRenderFrame()    = 0;
    virtual void throttledRenderFrame()    = 0;

    // ---- engine sync -------------------------------------------------------
    virtual void updateActiveVideo()       = 0;
    virtual QString resolvePlaybackPath(const VideoClip& clip) const = 0;
    virtual int toEngineSourceType(ClipSourceType t) const = 0;
    virtual void syncNativeToProject()     = 0;
    virtual void restoreClipS10State(const VideoClip& clip) = 0;
    virtual void syncEffectsToEngine(int clipId) = 0;
};

} // namespace gopost::video_editor
