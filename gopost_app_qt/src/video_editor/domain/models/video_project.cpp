#include "video_editor/domain/models/video_project.h"

#include <cmath>

namespace gopost::video_editor {

// ── ClipAudioSettings ───────────────────────────────────────────────────────

QJsonObject ClipAudioSettings::toMap() const {
    return {
        {QStringLiteral("volume"), volume},
        {QStringLiteral("pan"), pan},
        {QStringLiteral("fadeInSeconds"), fadeInSeconds},
        {QStringLiteral("fadeOutSeconds"), fadeOutSeconds},
        {QStringLiteral("isMuted"), isMuted},
    };
}

ClipAudioSettings ClipAudioSettings::fromMap(const QJsonObject& m) {
    return {
        .volume = m.value(QStringLiteral("volume")).toDouble(1.0),
        .pan = m.value(QStringLiteral("pan")).toDouble(0.0),
        .fadeInSeconds = m.value(QStringLiteral("fadeInSeconds")).toDouble(0.0),
        .fadeOutSeconds = m.value(QStringLiteral("fadeOutSeconds")).toDouble(0.0),
        .isMuted = m.value(QStringLiteral("isMuted")).toBool(false),
    };
}

// ── TrackAudioSettings ──────────────────────────────────────────────────────

QJsonObject TrackAudioSettings::toMap() const {
    return {
        {QStringLiteral("volume"), volume},
        {QStringLiteral("pan"), pan},
    };
}

TrackAudioSettings TrackAudioSettings::fromMap(const QJsonObject& m) {
    return {
        .volume = m.value(QStringLiteral("volume")).toDouble(1.0),
        .pan = m.value(QStringLiteral("pan")).toDouble(0.0),
    };
}

// ── VideoClip ───────────────────────────────────────────────────────────────

bool VideoClip::hasEffects() const {
    return std::any_of(effects.begin(), effects.end(), [](const VideoEffect& e) {
        return e.enabled && e.value != effectTypeInfo(e.type).defaultValue;
    });
}

bool VideoClip::hasColorGrading() const {
    return !colorGrading.isDefault() || presetFilter != PresetFilterId::None;
}

bool VideoClip::hasTransition() const {
    return !transitionIn.isNone() || !transitionOut.isNone();
}

bool VideoClip::hasSpeedChange() const {
    return std::abs(speed - 1.0) > 0.01 || isReversed || hasSpeedRamp();
}

bool VideoClip::hasSpeedRamp() const {
    const auto* speedTrack = keyframes.trackFor(KeyframeProperty::Speed);
    return speedTrack && speedTrack->keyframes.size() >= 2;
}

bool VideoClip::hasKeyframes() const {
    return !keyframes.isEmpty();
}

bool VideoClip::hasAudioMod() const {
    return audio.isMuted || audio.volume != 1.0 ||
           audio.fadeInSeconds > 0 || audio.fadeOutSeconds > 0;
}

bool VideoClip::hasProxy() const {
    return proxyStatus == ProxyStatus::Ready && proxyPath.has_value();
}

QList<AppliedBadge> VideoClip::appliedBadges() const {
    QList<AppliedBadge> badges;
    if (hasEffects())      badges.append(AppliedBadge::Effects);
    if (hasColorGrading()) badges.append(AppliedBadge::Color);
    if (hasTransition())   badges.append(AppliedBadge::Transition);
    if (hasSpeedChange())  badges.append(AppliedBadge::Speed);
    if (hasKeyframes())    badges.append(AppliedBadge::Keyframes);
    if (hasAudioMod())     badges.append(AppliedBadge::Audio);
    return badges;
}

VideoClip VideoClip::copyWith(const CopyWithOpts& o) const {
    return {
        .id = o.id.value_or(id),
        .trackIndex = o.trackIndex.value_or(trackIndex),
        .sourceType = sourceType,
        .sourcePath = sourcePath,
        .proxyPath = o.clearProxyPath ? std::nullopt : (o.proxyPath.has_value() ? o.proxyPath : proxyPath),
        .proxyStatus = o.proxyStatus.value_or(proxyStatus),
        .displayName = displayName,
        .timelineIn = o.timelineIn.value_or(timelineIn),
        .timelineOut = o.timelineOut.value_or(timelineOut),
        .sourceIn = o.sourceIn.value_or(sourceIn),
        .sourceOut = o.sourceOut.value_or(sourceOut),
        .speed = o.speed.value_or(speed),
        .isReversed = o.isReversed.value_or(isReversed),
        .opacity = o.opacity.value_or(opacity),
        .blendMode = o.blendMode.value_or(blendMode),
        .effectHash = effectHash,
        .effects = o.effects.value_or(effects),
        .colorGrading = o.colorGrading.value_or(colorGrading),
        .presetFilter = o.presetFilter.value_or(presetFilter),
        .transitionIn = o.transitionIn.value_or(transitionIn),
        .transitionOut = o.transitionOut.value_or(transitionOut),
        .keyframes = o.keyframes.value_or(keyframes),
        .audio = o.audio.value_or(audio),
        .adjustmentData = o.adjustmentData.has_value() ? o.adjustmentData : adjustmentData,
        .linkedClipId = o.clearLinkedClipId ? std::nullopt : (o.linkedClipId.has_value() ? o.linkedClipId : linkedClipId),
        .colorTag = o.clearColorTag ? std::nullopt : (o.colorTag.has_value() ? o.colorTag : colorTag),
        .customLabel = o.customLabel.value_or(customLabel),
    };
}

QJsonObject VideoClip::toMap() const {
    QJsonObject obj;
    obj[QStringLiteral("id")] = id;
    obj[QStringLiteral("trackIndex")] = trackIndex;
    obj[QStringLiteral("sourceType")] = static_cast<int>(sourceType);
    obj[QStringLiteral("sourcePath")] = sourcePath;
    if (proxyPath.has_value())
        obj[QStringLiteral("proxyPath")] = *proxyPath;
    if (proxyStatus != ProxyStatus::None)
        obj[QStringLiteral("proxyStatus")] = static_cast<int>(proxyStatus);
    obj[QStringLiteral("displayName")] = displayName;
    obj[QStringLiteral("timelineIn")] = timelineIn;
    obj[QStringLiteral("timelineOut")] = timelineOut;
    obj[QStringLiteral("sourceIn")] = sourceIn;
    obj[QStringLiteral("sourceOut")] = sourceOut;
    obj[QStringLiteral("speed")] = speed;
    if (isReversed)
        obj[QStringLiteral("isReversed")] = true;
    obj[QStringLiteral("opacity")] = opacity;
    obj[QStringLiteral("blendMode")] = blendMode;
    obj[QStringLiteral("effectHash")] = effectHash;

    QJsonArray effectsArr;
    for (const auto& e : effects) effectsArr.append(e.toMap());
    obj[QStringLiteral("effects")] = effectsArr;

    obj[QStringLiteral("colorGrading")] = colorGrading.toMap();
    obj[QStringLiteral("presetFilter")] = static_cast<int>(presetFilter);
    obj[QStringLiteral("transitionIn")] = transitionIn.toMap();
    obj[QStringLiteral("transitionOut")] = transitionOut.toMap();
    obj[QStringLiteral("keyframes")] = keyframes.toMap();
    obj[QStringLiteral("audio")] = audio.toMap();
    if (adjustmentData.has_value())
        obj[QStringLiteral("adjustmentData")] = adjustmentData->toMap();
    if (linkedClipId.has_value())
        obj[QStringLiteral("linkedClipId")] = *linkedClipId;
    if (colorTag.has_value())
        obj[QStringLiteral("colorTag")] = *colorTag;
    if (!customLabel.isEmpty())
        obj[QStringLiteral("customLabel")] = customLabel;
    return obj;
}

VideoClip VideoClip::fromMap(const QJsonObject& m) {
    VideoClip clip;
    clip.id = m.value(QStringLiteral("id")).toInt();
    clip.trackIndex = m.value(QStringLiteral("trackIndex")).toInt();
    clip.sourceType = static_cast<ClipSourceType>(m.value(QStringLiteral("sourceType")).toInt());
    clip.sourcePath = m.value(QStringLiteral("sourcePath")).toString();
    if (m.contains(QStringLiteral("proxyPath")))
        clip.proxyPath = m.value(QStringLiteral("proxyPath")).toString();
    clip.proxyStatus = static_cast<ProxyStatus>(m.value(QStringLiteral("proxyStatus")).toInt(0));
    clip.displayName = m.value(QStringLiteral("displayName")).toString();
    clip.timelineIn = m.value(QStringLiteral("timelineIn")).toDouble();
    clip.timelineOut = m.value(QStringLiteral("timelineOut")).toDouble();
    clip.sourceIn = m.value(QStringLiteral("sourceIn")).toDouble();
    clip.sourceOut = m.value(QStringLiteral("sourceOut")).toDouble();
    clip.speed = m.value(QStringLiteral("speed")).toDouble(1.0);
    clip.isReversed = m.value(QStringLiteral("isReversed")).toBool(false);
    clip.opacity = m.value(QStringLiteral("opacity")).toDouble(1.0);
    clip.blendMode = m.value(QStringLiteral("blendMode")).toInt(0);
    clip.effectHash = m.value(QStringLiteral("effectHash")).toInt(0);

    const auto effectsArr = m.value(QStringLiteral("effects")).toArray();
    for (const auto& v : effectsArr)
        clip.effects.append(VideoEffect::fromMap(v.toObject()));

    if (m.contains(QStringLiteral("colorGrading")))
        clip.colorGrading = ColorGrading::fromMap(m.value(QStringLiteral("colorGrading")).toObject());
    clip.presetFilter = static_cast<PresetFilterId>(m.value(QStringLiteral("presetFilter")).toInt(0));

    if (m.contains(QStringLiteral("transitionIn")))
        clip.transitionIn = ClipTransition::fromMap(m.value(QStringLiteral("transitionIn")).toObject());
    if (m.contains(QStringLiteral("transitionOut")))
        clip.transitionOut = ClipTransition::fromMap(m.value(QStringLiteral("transitionOut")).toObject());
    if (m.contains(QStringLiteral("keyframes")))
        clip.keyframes = ClipKeyframes::fromMap(m.value(QStringLiteral("keyframes")).toObject());
    if (m.contains(QStringLiteral("audio")))
        clip.audio = ClipAudioSettings::fromMap(m.value(QStringLiteral("audio")).toObject());
    if (m.contains(QStringLiteral("adjustmentData")))
        clip.adjustmentData = AdjustmentClipData::fromMap(m.value(QStringLiteral("adjustmentData")).toObject());
    if (m.contains(QStringLiteral("linkedClipId")))
        clip.linkedClipId = m.value(QStringLiteral("linkedClipId")).toInt();
    if (m.contains(QStringLiteral("colorTag")))
        clip.colorTag = m.value(QStringLiteral("colorTag")).toString();
    clip.customLabel = m.value(QStringLiteral("customLabel")).toString();
    return clip;
}

// ── VideoTrack ──────────────────────────────────────────────────────────────

VideoTrack VideoTrack::copyWith(const CopyWithOpts& o) const {
    return {
        .index = index,
        .type = type,
        .label = o.label.value_or(label),
        .isVisible = o.isVisible.value_or(isVisible),
        .isLocked = o.isLocked.value_or(isLocked),
        .isMuted = o.isMuted.value_or(isMuted),
        .isSolo = o.isSolo.value_or(isSolo),
        .clips = o.clips.value_or(clips),
        .audioSettings = o.audioSettings.value_or(audioSettings),
        .color = o.clearColor ? std::nullopt : (o.color.has_value() ? o.color : color),
    };
}

QJsonObject VideoTrack::toMap() const {
    QJsonArray clipsArr;
    for (const auto& c : clips) clipsArr.append(c.toMap());
    QJsonObject obj{
        {QStringLiteral("index"), index},
        {QStringLiteral("type"), static_cast<int>(type)},
        {QStringLiteral("label"), label},
        {QStringLiteral("isVisible"), isVisible},
        {QStringLiteral("isLocked"), isLocked},
        {QStringLiteral("isMuted"), isMuted},
        {QStringLiteral("isSolo"), isSolo},
        {QStringLiteral("clips"), clipsArr},
        {QStringLiteral("audioSettings"), audioSettings.toMap()},
    };
    if (color.has_value())
        obj[QStringLiteral("color")] = *color;
    return obj;
}

VideoTrack VideoTrack::fromMap(const QJsonObject& m) {
    VideoTrack track;
    track.index = m.value(QStringLiteral("index")).toInt();
    track.type = static_cast<TrackType>(m.value(QStringLiteral("type")).toInt());
    track.label = m.value(QStringLiteral("label")).toString();
    track.isVisible = m.value(QStringLiteral("isVisible")).toBool(true);
    track.isLocked = m.value(QStringLiteral("isLocked")).toBool(false);
    track.isMuted = m.value(QStringLiteral("isMuted")).toBool(false);
    track.isSolo = m.value(QStringLiteral("isSolo")).toBool(false);

    const auto clipsArr = m.value(QStringLiteral("clips")).toArray();
    for (const auto& v : clipsArr)
        track.clips.append(VideoClip::fromMap(v.toObject()));

    if (m.contains(QStringLiteral("audioSettings")))
        track.audioSettings = TrackAudioSettings::fromMap(m.value(QStringLiteral("audioSettings")).toObject());
    if (m.contains(QStringLiteral("color")))
        track.color = m.value(QStringLiteral("color")).toString();
    return track;
}

// ── TimelineMarker ──────────────────────────────────────────────────────────

QJsonObject TimelineMarker::toMap() const {
    QJsonObject obj;
    obj[QStringLiteral("id")] = id;
    obj[QStringLiteral("positionSeconds")] = positionSeconds;
    obj[QStringLiteral("type")] = static_cast<int>(type);
    obj[QStringLiteral("label")] = label;
    if (color.has_value())
        obj[QStringLiteral("color")] = *color;
    if (!notes.isEmpty())
        obj[QStringLiteral("notes")] = notes;
    if (endPositionSeconds.has_value())
        obj[QStringLiteral("endPositionSeconds")] = *endPositionSeconds;
    if (clipId.has_value())
        obj[QStringLiteral("clipId")] = *clipId;
    return obj;
}

TimelineMarker TimelineMarker::fromMap(const QJsonObject& m) {
    TimelineMarker marker;
    marker.id = m.value(QStringLiteral("id")).toInt();
    marker.positionSeconds = m.value(QStringLiteral("positionSeconds")).toDouble();
    marker.type = static_cast<MarkerType>(m.value(QStringLiteral("type")).toInt(0));
    marker.label = m.value(QStringLiteral("label")).toString();
    if (m.contains(QStringLiteral("color")))
        marker.color = m.value(QStringLiteral("color")).toString();
    marker.notes = m.value(QStringLiteral("notes")).toString();
    if (m.contains(QStringLiteral("endPositionSeconds")))
        marker.endPositionSeconds = m.value(QStringLiteral("endPositionSeconds")).toDouble();
    if (m.contains(QStringLiteral("clipId")))
        marker.clipId = m.value(QStringLiteral("clipId")).toInt();
    return marker;
}

// ── TimelineTransition ──────────────────────────────────────────────────────

QJsonObject TimelineTransition::toMap() const {
    return {
        {QStringLiteral("clipAId"), clipAId},
        {QStringLiteral("clipBId"), clipBId},
        {QStringLiteral("duration"), duration},
        {QStringLiteral("type"), static_cast<int>(type)},
    };
}

TimelineTransition TimelineTransition::fromMap(const QJsonObject& m) {
    TimelineTransition t;
    t.clipAId  = m.value(QStringLiteral("clipAId")).toInt();
    t.clipBId  = m.value(QStringLiteral("clipBId")).toInt();
    t.duration = m.value(QStringLiteral("duration")).toDouble();
    t.type     = static_cast<CrossfadeType>(m.value(QStringLiteral("type")).toInt(0));
    return t;
}

// ── VideoProject ────────────────────────────────────────────────────────────

double VideoProject::duration() const {
    double end = 0.0;
    for (const auto& track : tracks) {
        for (const auto& clip : track.clips) {
            if (clip.timelineOut > end) end = clip.timelineOut;
        }
    }
    return end;
}

QList<VideoClip> VideoProject::allClips() const {
    QList<VideoClip> result;
    for (const auto& track : tracks) {
        result.append(track.clips);
    }
    return result;
}

const VideoClip* VideoProject::findClip(int clipId) const {
    for (const auto& track : tracks) {
        for (const auto& clip : track.clips) {
            if (clip.id == clipId) return &clip;
        }
    }
    return nullptr;
}

VideoProject VideoProject::copyWith(const CopyWithOpts& o) const {
    return {
        .timelineId = timelineId,
        .frameRate = frameRate,
        .width = o.width.value_or(width),
        .height = o.height.value_or(height),
        .tracks = o.tracks.value_or(tracks),
        .markers = o.markers.value_or(markers),
        .transitions = o.transitions.value_or(transitions),
    };
}

QJsonObject VideoProject::toMap() const {
    QJsonArray tracksArr;
    for (const auto& t : tracks) tracksArr.append(t.toMap());
    QJsonArray markersArr;
    for (const auto& m : markers) markersArr.append(m.toMap());
    QJsonArray transArr;
    for (const auto& t : transitions) transArr.append(t.toMap());
    return {
        {QStringLiteral("timelineId"), timelineId},
        {QStringLiteral("frameRate"), frameRate},
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), height},
        {QStringLiteral("tracks"), tracksArr},
        {QStringLiteral("markers"), markersArr},
        {QStringLiteral("transitions"), transArr},
    };
}

VideoProject VideoProject::fromMap(const QJsonObject& m) {
    VideoProject proj;
    proj.timelineId = m.value(QStringLiteral("timelineId")).toInt();
    proj.frameRate = m.value(QStringLiteral("frameRate")).toDouble(30.0);
    proj.width = m.value(QStringLiteral("width")).toInt(1920);
    proj.height = m.value(QStringLiteral("height")).toInt(1080);

    const auto tracksArr = m.value(QStringLiteral("tracks")).toArray();
    for (const auto& v : tracksArr)
        proj.tracks.append(VideoTrack::fromMap(v.toObject()));

    const auto markersArr = m.value(QStringLiteral("markers")).toArray();
    for (const auto& v : markersArr)
        proj.markers.append(TimelineMarker::fromMap(v.toObject()));

    const auto transArr = m.value(QStringLiteral("transitions")).toArray();
    for (const auto& v : transArr)
        proj.transitions.append(TimelineTransition::fromMap(v.toObject()));
    return proj;
}

} // namespace gopost::video_editor
