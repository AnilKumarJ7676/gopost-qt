#include "rendering_bridge/video_timeline_engine_native.h"

#include <algorithm>
#include <cstring>

namespace gopost::rendering {

// =========================================================================
// Helpers
// =========================================================================

GopostVideoTimelineEngineNative::GopostVideoTimelineEngineNative(
    GopostEngine* enginePtr)
    : enginePtr_(enginePtr) {}

GopostVideoTimelineEngineNative::~GopostVideoTimelineEngineNative() {
    for (auto& [id, ptr] : timelines_) {
        gopost_timeline_destroy(ptr);
    }
    timelines_.clear();
    if (textureBridge_) {
        gopost_texture_bridge_destroy(textureBridge_);
        textureBridge_ = nullptr;
    }
}

void GopostVideoTimelineEngineNative::checkErr(int err) {
    if (err != 0) {
        const char* msg = gopost_error_string(err);
        throw EngineException(msg ? msg : "Unknown engine error");
    }
}

GopostTimeline* GopostVideoTimelineEngineNative::getTimeline(int id) {
    auto it = timelines_.find(id);
    if (it == timelines_.end())
        throw EngineException("Unknown timeline");
    return it->second;
}

int GopostVideoTimelineEngineNative::toTrackType(VideoTrackType t) {
    switch (t) {
    case VideoTrackType::Video:    return 0;
    case VideoTrackType::Audio:    return 1;
    case VideoTrackType::Title:    return 2;
    case VideoTrackType::Effect:   return 3;
    case VideoTrackType::Subtitle: return 4;
    }
    return 0;
}

int GopostVideoTimelineEngineNative::toClipSourceType(VideoClipSourceType t) {
    switch (t) {
    case VideoClipSourceType::Video: return 0;
    case VideoClipSourceType::Image: return 1;
    case VideoClipSourceType::Title: return 2;
    case VideoClipSourceType::Color: return 3;
    }
    return 0;
}

void GopostVideoTimelineEngineNative::fillDescriptor(
    GopostClipDescriptor* desc, const ClipDescriptor& clip) {
    desc->trackIndex = clip.trackIndex;
    desc->sourceType = toClipSourceType(clip.sourceType);
    auto pathUtf8 = clip.sourcePath.toUtf8();
    int pathLen = std::min(static_cast<int>(pathUtf8.size()), 1023);
    std::memcpy(desc->sourcePath, pathUtf8.constData(), pathLen);
    desc->sourcePath[pathLen] = '\0';
    desc->timelineInTime = clip.timelineRange.inTime;
    desc->timelineOutTime = clip.timelineRange.outTime;
    desc->sourceIn = clip.sourceRange.sourceIn;
    desc->sourceOut = clip.sourceRange.sourceOut;
    desc->speed = clip.speed;
    desc->opacity = clip.opacity;
    desc->blendMode = clip.blendMode;
    desc->effectHash = clip.effectHash;
}

// =========================================================================
// TimelineLifecycle
// =========================================================================

int GopostVideoTimelineEngineNative::createTimeline(
    const TimelineConfig& config) {
    GopostTimelineConfig nc{};
    nc.frameRate = config.frameRate;
    nc.width = config.width;
    nc.height = config.height;
    nc.colorSpace = config.colorSpace;

    GopostTimeline* ptr = nullptr;
    checkErr(gopost_timeline_create(enginePtr_, &nc, &ptr));
    if (!ptr) throw EngineException("timeline_create returned null");
    int id = nextTimelineId_++;
    timelines_[id] = ptr;
    return id;
}

void GopostVideoTimelineEngineNative::destroyTimeline(int timelineId) {
    auto it = timelines_.find(timelineId);
    if (it == timelines_.end()) return;
    gopost_timeline_destroy(it->second);
    timelines_.erase(it);
}

TimelineConfig GopostVideoTimelineEngineNative::getTimelineConfig(
    int timelineId) {
    auto* tl = getTimeline(timelineId);
    GopostTimelineConfig nc{};
    checkErr(gopost_timeline_get_config(tl, &nc));
    return {nc.frameRate, nc.width, nc.height, nc.colorSpace};
}

double GopostVideoTimelineEngineNative::getDuration(int timelineId) {
    auto* tl = getTimeline(timelineId);
    double out = 0;
    checkErr(gopost_timeline_get_duration(tl, &out));
    return out;
}

// =========================================================================
// TimelineTrackOps
// =========================================================================

int GopostVideoTimelineEngineNative::addTrack(int timelineId,
                                               VideoTrackType type) {
    auto* tl = getTimeline(timelineId);
    int outIndex = 0;
    checkErr(gopost_timeline_add_track(tl, toTrackType(type), &outIndex));
    return outIndex;
}

void GopostVideoTimelineEngineNative::removeTrack(int timelineId,
                                                    int trackIndex) {
    checkErr(gopost_timeline_remove_track(getTimeline(timelineId), trackIndex));
}

int GopostVideoTimelineEngineNative::getTrackCount(int timelineId) {
    int out = 0;
    checkErr(gopost_timeline_get_track_count(getTimeline(timelineId), &out));
    return out;
}

void GopostVideoTimelineEngineNative::reorderTracks(
    int timelineId, const std::vector<int>& newOrder) {
    checkErr(gopost_timeline_reorder_tracks(
        getTimeline(timelineId), newOrder.data(),
        static_cast<int>(newOrder.size())));
}

void GopostVideoTimelineEngineNative::setTrackSyncLock(int timelineId,
                                                         int trackIndex,
                                                         bool locked) {
    checkErr(gopost_timeline_set_track_sync_lock(getTimeline(timelineId),
                                                  trackIndex, locked ? 1 : 0));
}

void GopostVideoTimelineEngineNative::setTrackHeight(int timelineId,
                                                       int trackIndex,
                                                       double heightPx) {
    checkErr(gopost_timeline_set_track_height(getTimeline(timelineId),
                                               trackIndex, heightPx));
}

double GopostVideoTimelineEngineNative::getTrackHeight(int timelineId,
                                                         int trackIndex) {
    float out = 0;
    checkErr(gopost_timeline_get_track_height(getTimeline(timelineId),
                                               trackIndex, &out));
    return static_cast<double>(out);
}

// =========================================================================
// TimelineClipOps
// =========================================================================

int GopostVideoTimelineEngineNative::addClip(int timelineId,
                                              const ClipDescriptor& descriptor) {
    GopostClipDescriptor desc{};
    fillDescriptor(&desc, descriptor);
    int outClipId = 0;
    checkErr(gopost_timeline_add_clip(getTimeline(timelineId), &desc,
                                      &outClipId));
    return outClipId;
}

void GopostVideoTimelineEngineNative::removeClip(int timelineId, int clipId) {
    checkErr(gopost_timeline_remove_clip(getTimeline(timelineId), clipId));
}

void GopostVideoTimelineEngineNative::trimClip(int timelineId, int clipId,
                                                const TimelineRange& newRange,
                                                const SourceRange& newSource) {
    GopostTimelineRange r{newRange.inTime, newRange.outTime};
    GopostSourceRange s{newSource.sourceIn, newSource.sourceOut};
    checkErr(gopost_timeline_trim_clip(getTimeline(timelineId), clipId, &r, &s));
}

void GopostVideoTimelineEngineNative::moveClip(int timelineId, int clipId,
                                                int newTrackIndex,
                                                double newInTime) {
    checkErr(gopost_timeline_move_clip(getTimeline(timelineId), clipId,
                                       newTrackIndex, newInTime));
}

std::optional<int> GopostVideoTimelineEngineNative::splitClip(
    int timelineId, int clipId, double splitTimeSeconds) {
    int outId = 0;
    checkErr(gopost_timeline_split_clip(getTimeline(timelineId), clipId,
                                        splitTimeSeconds, &outId));
    return outId;
}

void GopostVideoTimelineEngineNative::rippleDelete(int timelineId,
                                                    int trackIndex,
                                                    double rangeStart,
                                                    double rangeEnd) {
    checkErr(gopost_timeline_ripple_delete(getTimeline(timelineId), trackIndex,
                                           rangeStart, rangeEnd));
}

void GopostVideoTimelineEngineNative::moveMultipleClips(
    int timelineId, const std::vector<int>& clipIds, double deltaTime,
    int deltaTrack) {
    checkErr(gopost_timeline_move_multiple_clips(
        getTimeline(timelineId), clipIds.data(),
        static_cast<int>(clipIds.size()), deltaTime, deltaTrack));
}

void GopostVideoTimelineEngineNative::swapClips(int timelineId, int clipIdA,
                                                 int clipIdB) {
    checkErr(gopost_timeline_swap_clips(getTimeline(timelineId), clipIdA,
                                        clipIdB));
}

int GopostVideoTimelineEngineNative::splitAllTracks(int timelineId,
                                                     double splitTimeSeconds) {
    int outCount = 0;
    checkErr(gopost_timeline_split_all_tracks(getTimeline(timelineId),
                                              splitTimeSeconds, &outCount));
    return outCount;
}

void GopostVideoTimelineEngineNative::liftDelete(int timelineId,
                                                   int trackIndex,
                                                   double rangeStart,
                                                   double rangeEnd) {
    checkErr(gopost_timeline_lift_delete(getTimeline(timelineId), trackIndex,
                                         rangeStart, rangeEnd));
}

int GopostVideoTimelineEngineNative::checkOverlap(int timelineId,
                                                    int trackIndex,
                                                    double inTime,
                                                    double outTime,
                                                    int excludeClipId) {
    int result = 0;
    checkErr(gopost_timeline_check_overlap(getTimeline(timelineId), trackIndex,
                                           inTime, outTime, excludeClipId,
                                           &result));
    return result;
}

std::vector<int> GopostVideoTimelineEngineNative::getOverlappingClips(
    int timelineId, int trackIndex, double inTime, double outTime) {
    constexpr int maxIds = 128;
    int outIds[maxIds] = {};
    int outCount = 0;
    checkErr(gopost_timeline_get_overlapping_clips(
        getTimeline(timelineId), trackIndex, inTime, outTime, outIds, maxIds,
        &outCount));
    return {outIds, outIds + outCount};
}

// =========================================================================
// TimelinePlayback
// =========================================================================

void GopostVideoTimelineEngineNative::seek(int timelineId,
                                            double positionSeconds) {
    checkErr(gopost_timeline_seek(getTimeline(timelineId), positionSeconds));
}

std::optional<DecodedImage> GopostVideoTimelineEngineNative::renderFrame(
    int timelineId) {
    GopostFrame* framePtr = nullptr;
    int err = gopost_timeline_render_frame(getTimeline(timelineId), &framePtr);
    if (err != 0 || !framePtr) return std::nullopt;

    int w = framePtr->width;
    int h = framePtr->height;
    if (w <= 0 || h <= 0 || !framePtr->data) {
        gopost_frame_release(enginePtr_, framePtr);
        return std::nullopt;
    }

    int size = w * h * 4;
    QByteArray pixels(size, '\0');
    std::memcpy(pixels.data(), framePtr->data, size);
    gopost_frame_release(enginePtr_, framePtr);

    return DecodedImage{w, h, pixels};
}

double GopostVideoTimelineEngineNative::getPosition(int timelineId) {
    double out = 0;
    checkErr(gopost_timeline_get_position(getTimeline(timelineId), &out));
    return out;
}

void GopostVideoTimelineEngineNative::setFrameCacheSizeBytes(int timelineId,
                                                              int maxBytes) {
    checkErr(gopost_timeline_set_frame_cache_size_bytes(
        getTimeline(timelineId), maxBytes));
}

void GopostVideoTimelineEngineNative::invalidateFrameCache(int timelineId) {
    checkErr(gopost_timeline_invalidate_frame_cache(getTimeline(timelineId)));
}

// =========================================================================
// TimelineNleEdits
// =========================================================================

int GopostVideoTimelineEngineNative::insertEdit(int timelineId,
                                                 int trackIndex, double atTime,
                                                 const ClipDescriptor& clip) {
    GopostClipDescriptor desc{};
    fillDescriptor(&desc, clip);
    int outId = 0;
    checkErr(gopost_timeline_insert_edit(getTimeline(timelineId), trackIndex,
                                         atTime, &desc, &outId));
    return outId;
}

int GopostVideoTimelineEngineNative::overwriteEdit(int timelineId,
                                                    int trackIndex,
                                                    double atTime,
                                                    const ClipDescriptor& clip) {
    GopostClipDescriptor desc{};
    fillDescriptor(&desc, clip);
    int outId = 0;
    checkErr(gopost_timeline_overwrite_edit(getTimeline(timelineId), trackIndex,
                                            atTime, &desc, &outId));
    return outId;
}

void GopostVideoTimelineEngineNative::rollEdit(int timelineId, int clipId,
                                                double deltaSec) {
    checkErr(gopost_timeline_roll_edit(getTimeline(timelineId), clipId,
                                       deltaSec));
}

void GopostVideoTimelineEngineNative::slipEdit(int timelineId, int clipId,
                                                double deltaSec) {
    checkErr(gopost_timeline_slip_edit(getTimeline(timelineId), clipId,
                                       deltaSec));
}

void GopostVideoTimelineEngineNative::slideEdit(int timelineId, int clipId,
                                                 double deltaSec) {
    checkErr(gopost_timeline_slide_edit(getTimeline(timelineId), clipId,
                                        deltaSec));
}

void GopostVideoTimelineEngineNative::rateStretch(int timelineId, int clipId,
                                                    double newDurationSec) {
    checkErr(gopost_timeline_rate_stretch(getTimeline(timelineId), clipId,
                                          newDurationSec));
}

int GopostVideoTimelineEngineNative::duplicateClip(int timelineId,
                                                    int clipId) {
    int outId = 0;
    checkErr(gopost_timeline_duplicate_clip(getTimeline(timelineId), clipId,
                                            &outId));
    return outId;
}

std::vector<double> GopostVideoTimelineEngineNative::getSnapPoints(
    int timelineId, double timeSec, double thresholdSec) {
    constexpr int maxPoints = 64;
    double outPoints[maxPoints] = {};
    int outCount = 0;
    checkErr(gopost_timeline_get_snap_points(getTimeline(timelineId), timeSec,
                                             thresholdSec, outPoints, maxPoints,
                                             &outCount));
    return {outPoints, outPoints + outCount};
}

// =========================================================================
// TimelineMediaProbe
// =========================================================================

std::optional<MediaInfo> GopostVideoTimelineEngineNative::probeMedia(
    const QString& filePath) {
    auto pathUtf8 = filePath.toUtf8();
    GopostMediaInfo native{};
    int err = gopost_media_probe(pathUtf8.constData(), &native);
    if (err != 0) return std::nullopt;

    return MediaInfo{
        .durationSeconds = native.durationSeconds,
        .width = native.width,
        .height = native.height,
        .frameRate = native.frameRate,
        .frameCount = native.frameCount,
        .hasAudio = native.hasAudio != 0,
        .audioSampleRate = native.audioSampleRate,
        .audioChannels = native.audioChannels,
        .audioDurationSeconds = native.audioDurationSeconds,
    };
}

std::optional<MediaInfo> GopostVideoTimelineEngineNative::probeMediaFast(
    const QString& filePath) {
    return probeMedia(filePath);
}

// =========================================================================
// VideoTimelineEngine extended methods
// =========================================================================

void GopostVideoTimelineEngineNative::setClipVolume(int timelineId,
                                                     int clipId,
                                                     double volume) {
    checkErr(gopost_timeline_set_clip_volume(getTimeline(timelineId), clipId,
                                             volume));
}

double GopostVideoTimelineEngineNative::getClipVolume(int timelineId,
                                                       int clipId) {
    float out = 0;
    checkErr(gopost_timeline_get_clip_volume(getTimeline(timelineId), clipId,
                                             &out));
    return static_cast<double>(out);
}

void GopostVideoTimelineEngineNative::setClipTransitionIn(
    int timelineId, int clipId, int transitionType, double durationSeconds,
    int easingCurve) {
    checkErr(gopost_timeline_set_clip_transition_in(
        getTimeline(timelineId), clipId, transitionType, durationSeconds,
        easingCurve));
}

void GopostVideoTimelineEngineNative::setClipTransitionOut(
    int timelineId, int clipId, int transitionType, double durationSeconds,
    int easingCurve) {
    checkErr(gopost_timeline_set_clip_transition_out(
        getTimeline(timelineId), clipId, transitionType, durationSeconds,
        easingCurve));
}

void GopostVideoTimelineEngineNative::setClipKeyframe(int timelineId,
                                                        int clipId,
                                                        int property,
                                                        double time,
                                                        double value,
                                                        int interpolation) {
    checkErr(gopost_timeline_set_clip_keyframe(getTimeline(timelineId), clipId,
                                               property, time, value,
                                               interpolation));
}

void GopostVideoTimelineEngineNative::removeClipKeyframe(int timelineId,
                                                           int clipId,
                                                           int property,
                                                           double time) {
    checkErr(gopost_timeline_remove_clip_keyframe(getTimeline(timelineId),
                                                   clipId, property, time));
}

void GopostVideoTimelineEngineNative::clearClipKeyframes(int timelineId,
                                                           int clipId,
                                                           int property) {
    checkErr(gopost_timeline_clear_clip_keyframes(getTimeline(timelineId),
                                                   clipId, property));
}

void GopostVideoTimelineEngineNative::setClipColorGrading(
    int timelineId, int clipId, double brightness, double contrast,
    double saturation, double exposure, double temperature, double tint,
    double highlights, double shadows, double vibrance, double hue,
    double fade, double vignette) {
    // Pass fade/vignette to native engine when the Go backend adds support;
    // for now they are applied at the Qt rendering layer.
    Q_UNUSED(fade);
    Q_UNUSED(vignette);
    checkErr(gopost_timeline_set_clip_color_grading(
        getTimeline(timelineId), clipId, brightness, contrast, saturation,
        exposure, temperature, tint, highlights, shadows, vibrance, hue));
}

void GopostVideoTimelineEngineNative::clearClipEffects(int timelineId,
                                                         int clipId) {
    checkErr(gopost_timeline_clear_clip_effects(getTimeline(timelineId),
                                                clipId));
}

void GopostVideoTimelineEngineNative::setClipPan(int timelineId, int clipId,
                                                   double pan) {
    checkErr(gopost_timeline_set_clip_pan(getTimeline(timelineId), clipId, pan));
}

void GopostVideoTimelineEngineNative::setClipFadeIn(int timelineId,
                                                      int clipId,
                                                      double seconds) {
    checkErr(gopost_timeline_set_clip_fade_in(getTimeline(timelineId), clipId,
                                              seconds));
}

void GopostVideoTimelineEngineNative::setClipFadeOut(int timelineId,
                                                       int clipId,
                                                       double seconds) {
    checkErr(gopost_timeline_set_clip_fade_out(getTimeline(timelineId), clipId,
                                               seconds));
}

void GopostVideoTimelineEngineNative::setTrackVolume(int timelineId,
                                                       int trackIndex,
                                                       double volume) {
    checkErr(gopost_timeline_set_track_volume(getTimeline(timelineId),
                                              trackIndex, volume));
}

void GopostVideoTimelineEngineNative::setTrackPan(int timelineId,
                                                    int trackIndex,
                                                    double pan) {
    checkErr(gopost_timeline_set_track_pan(getTimeline(timelineId), trackIndex,
                                           pan));
}

void GopostVideoTimelineEngineNative::setTrackMute(int timelineId,
                                                     int trackIndex,
                                                     bool mute) {
    checkErr(gopost_timeline_set_track_mute(getTimeline(timelineId), trackIndex,
                                            mute ? 1 : 0));
}

void GopostVideoTimelineEngineNative::setTrackSolo(int timelineId,
                                                     int trackIndex,
                                                     bool solo) {
    checkErr(gopost_timeline_set_track_solo(getTimeline(timelineId), trackIndex,
                                            solo ? 1 : 0));
}

int GopostVideoTimelineEngineNative::startExport(
    int timelineId, const VideoExportConfig& config) {
    auto pathUtf8 = config.outputPath.toUtf8();
    int jobId = gopost_timeline_start_export(
        getTimeline(timelineId), config.width, config.height, config.frameRate,
        config.videoCodec, config.videoBitrateBps, config.audioCodec,
        config.audioBitrateKbps, config.container, pathUtf8.constData());
    if (jobId < 0) throw EngineException("Failed to start export");
    return jobId;
}

double GopostVideoTimelineEngineNative::getExportProgress(int exportJobId) {
    return gopost_export_get_progress(exportJobId);
}

void GopostVideoTimelineEngineNative::cancelExport(int exportJobId) {
    gopost_export_cancel(exportJobId);
}

bool GopostVideoTimelineEngineNative::supportsHardwareEncoding() const {
    return false;
}

// =========================================================================
// Phase 3: Effect DAG
// =========================================================================

std::vector<EngineEffectDef> GopostVideoTimelineEngineNative::listEffects(
    const QString&) {
    return {};
}

int GopostVideoTimelineEngineNative::addClipEffect(int timelineId, int clipId,
                                                     const QString& effectDefId) {
    auto idUtf8 = effectDefId.toUtf8();
    int outId = 0;
    checkErr(gopost_timeline_add_clip_effect(getTimeline(timelineId), clipId,
                                             idUtf8.constData(), &outId));
    return outId;
}

void GopostVideoTimelineEngineNative::removeClipEffect(int timelineId,
                                                         int clipId,
                                                         int effectInstanceId) {
    checkErr(gopost_timeline_remove_clip_effect(getTimeline(timelineId), clipId,
                                                effectInstanceId));
}

void GopostVideoTimelineEngineNative::reorderClipEffects(
    int, int, const std::vector<int>&) {
    // No native function — client-side reorder.
}

void GopostVideoTimelineEngineNative::setEffectParam(int timelineId,
                                                       int clipId,
                                                       int effectInstanceId,
                                                       const QString& paramId,
                                                       double value) {
    auto idUtf8 = paramId.toUtf8();
    checkErr(gopost_timeline_set_clip_effect_param(
        getTimeline(timelineId), clipId, effectInstanceId, idUtf8.constData(),
        value));
}

void GopostVideoTimelineEngineNative::setEffectEnabled(int timelineId,
                                                         int clipId,
                                                         int effectInstanceId,
                                                         bool enabled) {
    checkErr(gopost_timeline_set_clip_effect_enabled(
        getTimeline(timelineId), clipId, effectInstanceId, enabled ? 1 : 0));
}

void GopostVideoTimelineEngineNative::setEffectMix(int timelineId, int clipId,
                                                     int effectInstanceId,
                                                     double mix) {
    checkErr(gopost_timeline_set_clip_effect_mix(getTimeline(timelineId),
                                                  clipId, effectInstanceId,
                                                  mix));
}

// =========================================================================
// Phase 4: Masking & Tracking
// =========================================================================

int GopostVideoTimelineEngineNative::addClipMask(int timelineId, int clipId,
                                                   const MaskData& mask) {
    GopostMaskDesc desc{};
    desc.type = static_cast<int>(mask.type);
    desc.feather = mask.feather;
    desc.opacity = mask.opacity;
    desc.inverted = mask.inverted ? 1 : 0;
    desc.expansion = mask.expansion;
    desc.pointCount = static_cast<int>(mask.points.size());

    std::vector<GopostMaskPoint> pts(mask.points.size());
    for (size_t i = 0; i < mask.points.size(); i++) {
        pts[i] = {mask.points[i].x, mask.points[i].y,
                   mask.points[i].handleInX, mask.points[i].handleInY,
                   mask.points[i].handleOutX, mask.points[i].handleOutY};
    }

    int outId = 0;
    checkErr(gopost_timeline_add_clip_mask(
        getTimeline(timelineId), clipId, &desc,
        pts.empty() ? nullptr : pts.data(), &outId));
    return outId;
}

void GopostVideoTimelineEngineNative::updateClipMask(int timelineId,
                                                       int clipId, int maskId,
                                                       const MaskData& mask) {
    GopostMaskDesc desc{};
    desc.type = static_cast<int>(mask.type);
    desc.feather = mask.feather;
    desc.opacity = mask.opacity;
    desc.inverted = mask.inverted ? 1 : 0;
    desc.expansion = mask.expansion;
    desc.pointCount = static_cast<int>(mask.points.size());

    std::vector<GopostMaskPoint> pts(mask.points.size());
    for (size_t i = 0; i < mask.points.size(); i++) {
        pts[i] = {mask.points[i].x, mask.points[i].y,
                   mask.points[i].handleInX, mask.points[i].handleInY,
                   mask.points[i].handleOutX, mask.points[i].handleOutY};
    }

    checkErr(gopost_timeline_update_clip_mask(
        getTimeline(timelineId), clipId, maskId, &desc,
        pts.empty() ? nullptr : pts.data()));
}

void GopostVideoTimelineEngineNative::removeClipMask(int timelineId,
                                                       int clipId, int maskId) {
    checkErr(gopost_timeline_remove_clip_mask(getTimeline(timelineId), clipId,
                                              maskId));
}

int GopostVideoTimelineEngineNative::startTracking(int timelineId, int clipId,
                                                     double x, double y,
                                                     double timeSec) {
    int outId = 0;
    checkErr(gopost_timeline_start_tracking(getTimeline(timelineId), clipId, x,
                                            y, timeSec, &outId));
    return outId;
}

std::vector<TrackPoint> GopostVideoTimelineEngineNative::getTrackingData(
    int, int) {
    return {};
}

void GopostVideoTimelineEngineNative::stabilizeClip(
    int timelineId, int clipId, const StabilizationConfig& config) {
    checkErr(gopost_timeline_stabilize_clip(
        getTimeline(timelineId), clipId, static_cast<int>(config.method),
        config.smoothness, config.cropToStable ? 1 : 0));
}

// =========================================================================
// Phase 5: Text, Shapes, Audio Effects
// =========================================================================

void GopostVideoTimelineEngineNative::setClipText(
    int timelineId, int clipId, const TextLayerData& textData) {
    GopostTextLayerDesc desc{};
    auto textUtf8 = textData.text.toUtf8();
    std::strncpy(desc.text, textUtf8.constData(), 511);
    auto fontUtf8 = textData.fontFamily.toUtf8();
    std::strncpy(desc.fontFamily, fontUtf8.constData(), 127);
    auto styleUtf8 = textData.fontStyle.toUtf8();
    std::strncpy(desc.fontStyle, styleUtf8.constData(), 63);
    desc.fontSize = textData.fontSize;
    desc.fillColor = textData.fillColor;
    desc.fillEnabled = textData.fillEnabled ? 1 : 0;
    desc.strokeColor = textData.strokeColor;
    desc.strokeWidth = textData.strokeWidth;
    desc.strokeEnabled = textData.strokeEnabled ? 1 : 0;
    desc.alignment = static_cast<int>(textData.alignment);
    desc.tracking = textData.tracking;
    desc.leading = textData.leading;
    desc.positionX = textData.positionX;
    desc.positionY = textData.positionY;
    desc.rotation = textData.rotation;
    desc.scaleX = textData.scaleX;
    desc.scaleY = textData.scaleY;
    checkErr(gopost_timeline_set_clip_text(getTimeline(timelineId), clipId,
                                           &desc));
}

int GopostVideoTimelineEngineNative::addClipShape(int timelineId, int clipId,
                                                    const ShapeData& shape) {
    GopostShapeDesc desc{};
    desc.type = static_cast<int>(shape.type);
    desc.x = shape.x;
    desc.y = shape.y;
    desc.width = shape.width;
    desc.height = shape.height;
    desc.rotation = shape.rotation;
    desc.fillColor = shape.fillColor;
    desc.fillEnabled = shape.fillEnabled ? 1 : 0;
    desc.strokeColor = shape.strokeColor;
    desc.strokeWidth = shape.strokeWidth;
    desc.strokeEnabled = shape.strokeEnabled ? 1 : 0;
    desc.cornerRadius = shape.cornerRadius;
    desc.sides = shape.sides;
    desc.innerRadius = shape.innerRadius;

    int outId = 0;
    checkErr(gopost_timeline_add_clip_shape(getTimeline(timelineId), clipId,
                                            &desc, &outId));
    return outId;
}

void GopostVideoTimelineEngineNative::updateClipShape(int timelineId,
                                                        int clipId,
                                                        int shapeId,
                                                        const ShapeData& shape) {
    GopostShapeDesc desc{};
    desc.type = static_cast<int>(shape.type);
    desc.x = shape.x;
    desc.y = shape.y;
    desc.width = shape.width;
    desc.height = shape.height;
    desc.rotation = shape.rotation;
    desc.fillColor = shape.fillColor;
    desc.fillEnabled = shape.fillEnabled ? 1 : 0;
    desc.strokeColor = shape.strokeColor;
    desc.strokeWidth = shape.strokeWidth;
    desc.strokeEnabled = shape.strokeEnabled ? 1 : 0;
    desc.cornerRadius = shape.cornerRadius;
    desc.sides = shape.sides;
    desc.innerRadius = shape.innerRadius;
    checkErr(gopost_timeline_update_clip_shape(getTimeline(timelineId), clipId,
                                               shapeId, &desc));
}

void GopostVideoTimelineEngineNative::removeClipShape(int timelineId,
                                                        int clipId,
                                                        int shapeId) {
    checkErr(gopost_timeline_remove_clip_shape(getTimeline(timelineId), clipId,
                                               shapeId));
}

int GopostVideoTimelineEngineNative::addAudioEffect(
    int timelineId, int clipId, const QString& audioEffectDefId) {
    auto idUtf8 = audioEffectDefId.toUtf8();
    int outId = 0;
    checkErr(gopost_timeline_add_audio_effect(getTimeline(timelineId), clipId,
                                              idUtf8.constData(), &outId));
    return outId;
}

void GopostVideoTimelineEngineNative::setAudioEffectParam(
    int timelineId, int clipId, int effectInstanceId, const QString& paramId,
    double value) {
    auto idUtf8 = paramId.toUtf8();
    checkErr(gopost_timeline_set_audio_effect_param(
        getTimeline(timelineId), clipId, effectInstanceId, idUtf8.constData(),
        value));
}

void GopostVideoTimelineEngineNative::removeAudioEffect(int timelineId,
                                                          int clipId,
                                                          int effectInstanceId) {
    checkErr(gopost_timeline_remove_audio_effect(getTimeline(timelineId),
                                                  clipId, effectInstanceId));
}

std::vector<AudioEffectDef> GopostVideoTimelineEngineNative::listAudioEffects() {
    return {};
}

// =========================================================================
// Phase 6: AI, Proxy, Multi-Cam
// =========================================================================

int GopostVideoTimelineEngineNative::startAiSegmentation(
    int timelineId, int clipId, const AiSegmentationConfig& config) {
    int outId = 0;
    checkErr(gopost_timeline_start_ai_segmentation(
        getTimeline(timelineId), clipId, static_cast<int>(config.type),
        config.edgeFeather, config.refineEdges ? 1 : 0, &outId));
    return outId;
}

double GopostVideoTimelineEngineNative::getAiSegmentationProgress(int jobId) {
    return gopost_ai_segmentation_get_progress(jobId);
}

void GopostVideoTimelineEngineNative::cancelAiSegmentation(int jobId) {
    gopost_ai_segmentation_cancel(jobId);
}

void GopostVideoTimelineEngineNative::enableProxyMode(
    int timelineId, const ProxyConfig& config) {
    checkErr(gopost_timeline_enable_proxy_mode(
        getTimeline(timelineId), static_cast<int>(config.resolution),
        config.videoCodec, config.bitrateBps));
}

void GopostVideoTimelineEngineNative::disableProxyMode(int timelineId) {
    checkErr(gopost_timeline_disable_proxy_mode(getTimeline(timelineId)));
}

bool GopostVideoTimelineEngineNative::isProxyModeActive(int timelineId) {
    int out = 0;
    checkErr(gopost_timeline_is_proxy_active(getTimeline(timelineId), &out));
    return out != 0;
}

int GopostVideoTimelineEngineNative::createMultiCamClip(
    int timelineId, int trackIndex, const MultiCamConfig& config) {
    auto nameUtf8 = config.name.toUtf8();
    std::vector<GopostCameraAngle> angles(config.angles.size());
    for (size_t i = 0; i < config.angles.size(); i++) {
        auto n = config.angles[i].name.toUtf8();
        std::strncpy(angles[i].name, n.constData(), 127);
        auto p = config.angles[i].sourcePath.toUtf8();
        std::strncpy(angles[i].sourcePath, p.constData(), 1023);
        angles[i].syncOffset = config.angles[i].syncOffset;
    }

    int outId = 0;
    checkErr(gopost_timeline_create_multicam_clip(
        getTimeline(timelineId), trackIndex, nameUtf8.constData(),
        angles.data(), static_cast<int>(angles.size()), config.durationSec,
        &outId));
    return outId;
}

void GopostVideoTimelineEngineNative::switchMultiCamAngle(int timelineId,
                                                            int clipId,
                                                            int angleIndex,
                                                            double atTimeSec) {
    checkErr(gopost_timeline_switch_multicam_angle(getTimeline(timelineId),
                                                    clipId, angleIndex,
                                                    atTimeSec));
}

void GopostVideoTimelineEngineNative::flattenMultiCam(int timelineId,
                                                        int clipId) {
    checkErr(gopost_timeline_flatten_multicam(getTimeline(timelineId), clipId));
}

// =========================================================================
// Texture Bridge
// =========================================================================

int GopostVideoTimelineEngineNative::createTextureBridge(int width,
                                                          int height) {
    if (textureBridge_) destroyTextureBridge();

    void* bridge = nullptr;
    int64_t id = -1;
    checkErr(gopost_texture_bridge_create(enginePtr_, width, height, &bridge,
                                          &id));
    textureBridge_ = bridge;
    textureBridgeId_ = id;
    return static_cast<int>(textureBridgeId_);
}

void GopostVideoTimelineEngineNative::destroyTextureBridge() {
    if (textureBridge_) {
        gopost_texture_bridge_destroy(textureBridge_);
        textureBridge_ = nullptr;
        textureBridgeId_ = -1;
    }
}

bool GopostVideoTimelineEngineNative::renderToTextureBridge(int timelineId) {
    auto* tl = timelines_.count(timelineId) ? timelines_[timelineId] : nullptr;
    if (!tl || !textureBridge_) return false;

    GopostFrame* framePtr = nullptr;
    int err = gopost_timeline_render_frame(tl, &framePtr);
    if (err != 0 || !framePtr) return false;

    err = gopost_texture_bridge_update_frame(textureBridge_, framePtr);
    gopost_frame_release(enginePtr_, framePtr);
    return err == 0;
}

void GopostVideoTimelineEngineNative::resizeTextureBridge(int width,
                                                            int height) {
    if (!textureBridge_) return;
    gopost_texture_bridge_resize(textureBridge_, width, height);
}

std::optional<QByteArray> GopostVideoTimelineEngineNative::getTextureBridgePixels() {
    if (!textureBridge_) return std::nullopt;

    uint8_t* data = nullptr;
    int w = 0, h = 0;
    int64_t counter = 0;
    int err = gopost_texture_bridge_get_pixels(textureBridge_, &data, &w, &h,
                                               &counter);
    if (err != 0 || !data || w <= 0 || h <= 0) return std::nullopt;

    int byteCount = w * h * 4;
    return QByteArray(reinterpret_cast<const char*>(data), byteCount);
}

} // namespace gopost::rendering
