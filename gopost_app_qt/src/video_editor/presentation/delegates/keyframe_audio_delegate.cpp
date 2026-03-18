#include "video_editor/presentation/delegates/keyframe_audio_delegate.h"

#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

namespace gopost::video_editor {

KeyframeAudioDelegate::KeyframeAudioDelegate(TimelineOperations* ops) : ops_(ops) {}
KeyframeAudioDelegate::~KeyframeAudioDelegate() = default;

// ---------------------------------------------------------------------------
// Keyframes
// ---------------------------------------------------------------------------

void KeyframeAudioDelegate::addKeyframe(int clipId, KeyframeProperty prop,
                                        const Keyframe& kf) {
    ops_->updateClip(clipId, [&](const VideoClip& c) {
        VideoClip u = c;
        const auto* existing = u.keyframes.trackFor(prop);
        KeyframeTrack track = existing ? *existing
                                       : KeyframeTrack{.property = prop, .keyframes = {}};
        track = track.addKeyframe(kf);
        u.keyframes = u.keyframes.updateTrack(track);
        return u;
    });
    ops_->debouncedRenderFrame();
}

void KeyframeAudioDelegate::removeKeyframe(int clipId, KeyframeProperty prop,
                                           double time) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [prop, time](const VideoClip& c) {
        VideoClip u = c;
        const auto* existing = u.keyframes.trackFor(prop);
        if (!existing) return u;
        auto track = existing->removeKeyframeAt(time);
        u.keyframes = u.keyframes.updateTrack(track);
        return u;
    });
    ops_->pushUndo(before);
}

void KeyframeAudioDelegate::moveKeyframe(int clipId, KeyframeProperty prop,
                                         double oldTime, double newTime) {
    ops_->updateClip(clipId, [prop, oldTime, newTime](const VideoClip& c) {
        VideoClip u = c;
        const auto* existing = u.keyframes.trackFor(prop);
        if (!existing) return u;
        // moveKeyframe expects (oldTime, newTime, newValue) — preserve the original value
        double oldValue = keyframePropertyDefaultValue(prop);
        for (const auto& kf : existing->keyframes) {
            if (std::abs(kf.time - oldTime) < 0.001) {
                oldValue = kf.value;
                break;
            }
        }
        auto track = existing->moveKeyframe(oldTime, newTime, oldValue);
        u.keyframes = u.keyframes.updateTrack(track);
        return u;
    });
}

void KeyframeAudioDelegate::commitKeyframe(int clipId, const VideoProject& before) {
    ops_->pushUndo(before);
}

void KeyframeAudioDelegate::clearKeyframes(int clipId, KeyframeProperty prop) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [prop](const VideoClip& c) {
        VideoClip u = c;
        u.keyframes = u.keyframes.removeTrack(prop);
        return u;
    });
    ops_->pushUndo(before);
}

// ---------------------------------------------------------------------------
// Per-clip audio
// ---------------------------------------------------------------------------

void KeyframeAudioDelegate::setClipVolume(int clipId, double volume) {
    ops_->updateClip(clipId, [volume](const VideoClip& c) {
        VideoClip u = c;
        u.audio.volume = std::clamp(volume, 0.0, 2.0);
        return u;
    });
    syncAudioToEngine(clipId);
}

void KeyframeAudioDelegate::setClipPan(int clipId, double pan) {
    ops_->updateClip(clipId, [pan](const VideoClip& c) {
        VideoClip u = c;
        u.audio.pan = std::clamp(pan, -1.0, 1.0);
        return u;
    });
    syncAudioToEngine(clipId);
}

void KeyframeAudioDelegate::setClipFadeIn(int clipId, double seconds) {
    ops_->updateClip(clipId, [seconds](const VideoClip& c) {
        VideoClip u = c;
        u.audio.fadeInSeconds = std::max(0.0, seconds);
        return u;
    });
}

void KeyframeAudioDelegate::setClipFadeOut(int clipId, double seconds) {
    ops_->updateClip(clipId, [seconds](const VideoClip& c) {
        VideoClip u = c;
        u.audio.fadeOutSeconds = std::max(0.0, seconds);
        return u;
    });
}

void KeyframeAudioDelegate::setClipMuted(int clipId, bool muted) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto before = *state.project;

    ops_->updateClip(clipId, [muted](const VideoClip& c) {
        VideoClip u = c;
        u.audio.isMuted = muted;
        return u;
    });
    ops_->pushUndo(before);
    syncAudioToEngine(clipId);
}

// ---------------------------------------------------------------------------
// Per-track audio
// ---------------------------------------------------------------------------

void KeyframeAudioDelegate::setTrackVolume(int trackIndex, double volume) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    tracks[trackIndex].audioSettings.volume = std::clamp(volume, 0.0, 2.0);
    state.trackGeneration++;
    ops_->setState(std::move(state));
}

void KeyframeAudioDelegate::setTrackPan(int trackIndex, double pan) {
    auto state = ops_->currentState();
    if (!state.project) return;
    auto& tracks = state.project->tracks;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    tracks[trackIndex].audioSettings.pan = std::clamp(pan, -1.0, 1.0);
    state.trackGeneration++;
    ops_->setState(std::move(state));
}

// ---------------------------------------------------------------------------
// Audio sync to engine
// ---------------------------------------------------------------------------

void KeyframeAudioDelegate::syncAudioToEngine(int clipId) {
    auto* engine = ops_->engine();
    if (!engine) return;
    // Engine audio sync would go here — delegated to native engine API
    Q_UNUSED(clipId);
}

// ---------------------------------------------------------------------------
// Media probe via ffprobe (no engine dependency)
// ---------------------------------------------------------------------------

static std::optional<MediaProbeResult> runFfprobe(const QString& path, int timeoutMs) {
    if (path.isEmpty() || !QFileInfo::exists(path)) return std::nullopt;

    QProcess proc;
    proc.start(QStringLiteral("ffprobe"), {
        QStringLiteral("-v"), QStringLiteral("quiet"),
        QStringLiteral("-print_format"), QStringLiteral("flat"),
        QStringLiteral("-show_format"),
        QStringLiteral("-show_streams"),
        path
    });

    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        qWarning() << "[MediaProbe] ffprobe timed out for:" << path;
        return std::nullopt;
    }

    if (proc.exitCode() != 0) {
        qWarning() << "[MediaProbe] ffprobe failed for:" << path;
        return std::nullopt;
    }

    const auto output = QString::fromUtf8(proc.readAllStandardOutput());
    MediaProbeResult result;

    // Parse duration
    static const QRegularExpression durRe(
        QString::fromLatin1("format\\.duration=\"([^\"]+)\""));
    auto durMatch = durRe.match(output);
    if (durMatch.hasMatch())
        result.durationSeconds = durMatch.captured(1).toDouble();

    // Parse video stream info
    static const QRegularExpression widthRe(
        QString::fromLatin1("streams\\.stream\\.\\d+\\.width=(\\d+)"));
    static const QRegularExpression heightRe(
        QString::fromLatin1("streams\\.stream\\.\\d+\\.height=(\\d+)"));
    static const QRegularExpression fpsRe(
        QString::fromLatin1("streams\\.stream\\.\\d+\\.r_frame_rate=\"(\\d+)/(\\d+)\""));
    static const QRegularExpression codecRe(
        QString::fromLatin1("streams\\.stream\\.\\d+\\.codec_name=\"([^\"]+)\""));
    static const QRegularExpression sampleRateRe(
        QString::fromLatin1("streams\\.stream\\.\\d+\\.sample_rate=\"(\\d+)\""));
    static const QRegularExpression channelsRe(
        QString::fromLatin1("streams\\.stream\\.\\d+\\.channels=(\\d+)"));

    auto wMatch = widthRe.match(output);
    if (wMatch.hasMatch()) result.width = wMatch.captured(1).toInt();

    auto hMatch = heightRe.match(output);
    if (hMatch.hasMatch()) result.height = hMatch.captured(1).toInt();

    auto fpsMatch = fpsRe.match(output);
    if (fpsMatch.hasMatch()) {
        int num = fpsMatch.captured(1).toInt();
        int den = fpsMatch.captured(2).toInt();
        if (den > 0) result.fps = static_cast<double>(num) / den;
    }

    auto codecMatch = codecRe.match(output);
    if (codecMatch.hasMatch()) result.codec = codecMatch.captured(1);

    auto srMatch = sampleRateRe.match(output);
    if (srMatch.hasMatch()) result.sampleRate = srMatch.captured(1).toInt();

    auto chMatch = channelsRe.match(output);
    if (chMatch.hasMatch()) result.channels = chMatch.captured(1).toInt();

    return result;
}

std::optional<MediaProbeResult> KeyframeAudioDelegate::probeMedia(const QString& path) const {
    return runFfprobe(path, 10000); // 10 second timeout for full probe
}

std::optional<MediaProbeResult> KeyframeAudioDelegate::probeMediaFast(const QString& path) const {
    return runFfprobe(path, 3000); // 3 second timeout for fast probe
}

} // namespace gopost::video_editor
