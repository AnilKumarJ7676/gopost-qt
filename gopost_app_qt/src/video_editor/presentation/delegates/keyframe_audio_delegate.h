#pragma once

#include <optional>
#include <QString>

#include "video_editor/domain/models/video_keyframe.h"
#include "video_editor/presentation/delegates/timeline_operations.h"

namespace gopost::video_editor {

struct MediaProbeResult {
    double durationSeconds = 0.0;
    int    width           = 0;
    int    height          = 0;
    double fps             = 0.0;
    int    sampleRate      = 0;
    int    channels        = 0;
    QString codec;
};

// ---------------------------------------------------------------------------
// KeyframeAudioDelegate — keyframe animation + per-clip & per-track audio
//
// Converted 1:1 from keyframe_audio_delegate.dart.
// Plain C++ class (not QObject).
// ---------------------------------------------------------------------------
class KeyframeAudioDelegate {
public:
    explicit KeyframeAudioDelegate(TimelineOperations* ops);
    ~KeyframeAudioDelegate();

    // ---- keyframes ---------------------------------------------------------
    void addKeyframe(int clipId, KeyframeProperty prop, const Keyframe& kf);
    void removeKeyframe(int clipId, KeyframeProperty prop, double time);
    void moveKeyframe(int clipId, KeyframeProperty prop,
                      double oldTime, double newTime);
    void commitKeyframe(int clipId, const VideoProject& before);
    void clearKeyframes(int clipId, KeyframeProperty prop);

    // ---- per-clip audio ----------------------------------------------------
    void setClipVolume(int clipId, double volume);
    void setClipPan(int clipId, double pan);
    void setClipFadeIn(int clipId, double seconds);
    void setClipFadeOut(int clipId, double seconds);
    void setClipMuted(int clipId, bool muted);

    // ---- per-track audio ---------------------------------------------------
    void setTrackVolume(int trackIndex, double volume);
    void setTrackPan(int trackIndex, double pan);

    // ---- audio sync to engine ----------------------------------------------
    void syncAudioToEngine(int clipId);

    // ---- media probe -------------------------------------------------------
    /// Async probe returning full metadata.
    std::optional<MediaProbeResult> probeMedia(const QString& path) const;
    /// Fast probe (duration only, < 100 ms).
    std::optional<MediaProbeResult> probeMediaFast(const QString& path) const;

private:
    TimelineOperations* ops_;
};

} // namespace gopost::video_editor
