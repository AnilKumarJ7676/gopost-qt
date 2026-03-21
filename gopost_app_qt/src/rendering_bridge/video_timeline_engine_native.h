#pragma once

#include "rendering_bridge/engine_api.h"

#include <cstdint>
#include <map>

// Forward-declare additional C engine API types for video timeline.
extern "C" {
struct GopostEngine;
struct GopostFrame;
struct GopostTimeline;
struct GopostTimelineConfig;
struct GopostClipDescriptor;
struct GopostTimelineRange;
struct GopostSourceRange;
struct GopostMediaInfo;
struct GopostMaskDesc;
struct GopostMaskPoint;
struct GopostTextLayerDesc;
struct GopostShapeDesc;
struct GopostCameraAngle;

int gopost_timeline_create(GopostEngine* engine,
                           const GopostTimelineConfig* config,
                           GopostTimeline** out);
int gopost_timeline_destroy(GopostTimeline* tl);
int gopost_timeline_get_config(GopostTimeline* tl,
                               GopostTimelineConfig* out);
int gopost_timeline_get_duration(GopostTimeline* tl, double* out);
int gopost_timeline_add_track(GopostTimeline* tl, int type, int* outIndex);
int gopost_timeline_remove_track(GopostTimeline* tl, int trackIndex);
int gopost_timeline_get_track_count(GopostTimeline* tl, int* out);
int gopost_timeline_add_clip(GopostTimeline* tl,
                             const GopostClipDescriptor* desc, int* outClipId);
int gopost_timeline_remove_clip(GopostTimeline* tl, int clipId);
int gopost_timeline_trim_clip(GopostTimeline* tl, int clipId,
                              const GopostTimelineRange* range,
                              const GopostSourceRange* source);
int gopost_timeline_move_clip(GopostTimeline* tl, int clipId,
                              int newTrackIndex, double newInTime);
int gopost_timeline_split_clip(GopostTimeline* tl, int clipId,
                               double splitTime, int* outNewId);
int gopost_timeline_ripple_delete(GopostTimeline* tl, int trackIndex,
                                  double rangeStart, double rangeEnd);
int gopost_timeline_seek(GopostTimeline* tl, double pos);
int gopost_timeline_render_frame(GopostTimeline* tl, GopostFrame** outFrame);
int gopost_timeline_get_position(GopostTimeline* tl, double* out);
int gopost_timeline_set_frame_cache_size_bytes(GopostTimeline* tl, int max);
int gopost_timeline_invalidate_frame_cache(GopostTimeline* tl);
int gopost_media_probe(const char* path, GopostMediaInfo* out);
int gopost_timeline_set_clip_volume(GopostTimeline* tl, int clipId,
                                    double volume);
int gopost_timeline_get_clip_volume(GopostTimeline* tl, int clipId,
                                    float* out);
int gopost_timeline_set_clip_transition_in(GopostTimeline* tl, int clipId,
                                           int type, double dur, int easing);
int gopost_timeline_set_clip_transition_out(GopostTimeline* tl, int clipId,
                                            int type, double dur, int easing);
int gopost_timeline_set_clip_keyframe(GopostTimeline* tl, int clipId,
                                      int prop, double time, double value,
                                      int interp);
int gopost_timeline_remove_clip_keyframe(GopostTimeline* tl, int clipId,
                                         int prop, double time);
int gopost_timeline_clear_clip_keyframes(GopostTimeline* tl, int clipId,
                                         int prop);
int gopost_timeline_set_clip_color_grading(GopostTimeline* tl, int clipId,
    double brightness, double contrast, double saturation, double exposure,
    double temperature, double tint, double highlights, double shadows,
    double vibrance, double hue);
int gopost_timeline_clear_clip_effects(GopostTimeline* tl, int clipId);
int gopost_timeline_set_clip_pan(GopostTimeline* tl, int clipId, double pan);
int gopost_timeline_set_clip_fade_in(GopostTimeline* tl, int clipId, double s);
int gopost_timeline_set_clip_fade_out(GopostTimeline* tl, int clipId, double s);
int gopost_timeline_set_track_volume(GopostTimeline* tl, int idx, double vol);
int gopost_timeline_set_track_pan(GopostTimeline* tl, int idx, double pan);
int gopost_timeline_set_track_mute(GopostTimeline* tl, int idx, int mute);
int gopost_timeline_set_track_solo(GopostTimeline* tl, int idx, int solo);
int gopost_timeline_start_export(GopostTimeline* tl, int w, int h, double fps,
    int vCodec, int vBitrate, int aCodec, int aBitrate, int container,
    const char* path);
double gopost_export_get_progress(int jobId);
int gopost_export_cancel(int jobId);
int gopost_timeline_insert_edit(GopostTimeline* tl, int trackIndex,
    double atTime, const GopostClipDescriptor* desc, int* outId);
int gopost_timeline_overwrite_edit(GopostTimeline* tl, int trackIndex,
    double atTime, const GopostClipDescriptor* desc, int* outId);
int gopost_timeline_roll_edit(GopostTimeline* tl, int clipId, double delta);
int gopost_timeline_slip_edit(GopostTimeline* tl, int clipId, double delta);
int gopost_timeline_slide_edit(GopostTimeline* tl, int clipId, double delta);
int gopost_timeline_rate_stretch(GopostTimeline* tl, int clipId, double dur);
int gopost_timeline_duplicate_clip(GopostTimeline* tl, int clipId, int* outId);
int gopost_timeline_get_snap_points(GopostTimeline* tl, double time,
    double threshold, double* outPts, int maxPts, int* outCount);
int gopost_timeline_reorder_tracks(GopostTimeline* tl, const int* order,
    int count);
int gopost_timeline_add_clip_effect(GopostTimeline* tl, int clipId,
    const char* defId, int* outId);
int gopost_timeline_remove_clip_effect(GopostTimeline* tl, int clipId, int id);
int gopost_timeline_set_clip_effect_param(GopostTimeline* tl, int clipId,
    int effectId, const char* paramId, double value);
int gopost_timeline_set_clip_effect_enabled(GopostTimeline* tl, int clipId,
    int effectId, int enabled);
int gopost_timeline_set_clip_effect_mix(GopostTimeline* tl, int clipId,
    int effectId, double mix);
int gopost_timeline_add_clip_mask(GopostTimeline* tl, int clipId,
    const GopostMaskDesc* desc, const GopostMaskPoint* pts, int* outId);
int gopost_timeline_update_clip_mask(GopostTimeline* tl, int clipId,
    int maskId, const GopostMaskDesc* desc, const GopostMaskPoint* pts);
int gopost_timeline_remove_clip_mask(GopostTimeline* tl, int clipId, int id);
int gopost_timeline_start_tracking(GopostTimeline* tl, int clipId,
    double x, double y, double time, int* outId);
int gopost_timeline_stabilize_clip(GopostTimeline* tl, int clipId,
    int method, double smoothness, int crop);
int gopost_timeline_set_clip_text(GopostTimeline* tl, int clipId,
    const GopostTextLayerDesc* desc);
int gopost_timeline_add_clip_shape(GopostTimeline* tl, int clipId,
    const GopostShapeDesc* desc, int* outId);
int gopost_timeline_update_clip_shape(GopostTimeline* tl, int clipId,
    int shapeId, const GopostShapeDesc* desc);
int gopost_timeline_remove_clip_shape(GopostTimeline* tl, int clipId, int id);
int gopost_timeline_add_audio_effect(GopostTimeline* tl, int clipId,
    const char* defId, int* outId);
int gopost_timeline_set_audio_effect_param(GopostTimeline* tl, int clipId,
    int effectId, const char* paramId, double value);
int gopost_timeline_remove_audio_effect(GopostTimeline* tl, int clipId, int id);
int gopost_timeline_start_ai_segmentation(GopostTimeline* tl, int clipId,
    int type, double feather, int refine, int* outId);
double gopost_ai_segmentation_get_progress(int jobId);
int gopost_ai_segmentation_cancel(int jobId);
int gopost_timeline_enable_proxy_mode(GopostTimeline* tl, int resolution,
    int codec, int bitrate);
int gopost_timeline_disable_proxy_mode(GopostTimeline* tl);
int gopost_timeline_is_proxy_active(GopostTimeline* tl, int* out);
int gopost_timeline_create_multicam_clip(GopostTimeline* tl, int trackIndex,
    const char* name, const GopostCameraAngle* angles, int count,
    double duration, int* outId);
int gopost_timeline_switch_multicam_angle(GopostTimeline* tl, int clipId,
    int angle, double atTime);
int gopost_timeline_flatten_multicam(GopostTimeline* tl, int clipId);
int gopost_timeline_move_multiple_clips(GopostTimeline* tl, const int* ids,
    int count, double deltaTime, int deltaTrack);
int gopost_timeline_swap_clips(GopostTimeline* tl, int a, int b);
int gopost_timeline_split_all_tracks(GopostTimeline* tl, double time,
    int* outCount);
int gopost_timeline_lift_delete(GopostTimeline* tl, int track, double start,
    double end);
int gopost_timeline_check_overlap(GopostTimeline* tl, int track, double inT,
    double outT, int excludeId, int* outResult);
int gopost_timeline_get_overlapping_clips(GopostTimeline* tl, int track,
    double inT, double outT, int* outIds, int maxIds, int* outCount);
int gopost_timeline_set_track_sync_lock(GopostTimeline* tl, int track,
    int locked);
int gopost_timeline_set_track_height(GopostTimeline* tl, int track,
    double height);
int gopost_timeline_get_track_height(GopostTimeline* tl, int track,
    float* out);
int gopost_texture_bridge_create(GopostEngine* engine, int w, int h,
    void** outBridge, int64_t* outId);
int gopost_texture_bridge_destroy(void* bridge);
int gopost_texture_bridge_update_frame(void* bridge, GopostFrame* frame);
int gopost_texture_bridge_resize(void* bridge, int w, int h);
int gopost_texture_bridge_get_pixels(void* bridge, uint8_t** outData,
    int* outW, int* outH, int64_t* outCounter);
int gopost_frame_release(GopostEngine* engine, GopostFrame* frame);
const char* gopost_error_string(int err);
} // extern "C"

namespace gopost::rendering {

/// Native C-engine-backed VideoTimelineEngine.
/// Directly calls the gopost_engine C API (linked at compile time).
class GopostVideoTimelineEngineNative final : public VideoTimelineEngine {
public:
    /// Construct with a shared engine pointer (from GopostImageEditorEngineNative).
    explicit GopostVideoTimelineEngineNative(GopostEngine* enginePtr);
    ~GopostVideoTimelineEngineNative() override;

    // --- TimelineLifecycle ---
    int createTimeline(const TimelineConfig& config) override;
    void destroyTimeline(int timelineId) override;
    TimelineConfig getTimelineConfig(int timelineId) override;
    double getDuration(int timelineId) override;

    // --- TimelineTrackOps ---
    int addTrack(int timelineId, VideoTrackType type) override;
    void removeTrack(int timelineId, int trackIndex) override;
    int getTrackCount(int timelineId) override;
    void reorderTracks(int timelineId, const std::vector<int>& newOrder) override;
    void setTrackSyncLock(int timelineId, int trackIndex, bool locked) override;
    void setTrackHeight(int timelineId, int trackIndex, double heightPx) override;
    double getTrackHeight(int timelineId, int trackIndex) override;

    // --- TimelineClipOps ---
    int addClip(int timelineId, const ClipDescriptor& descriptor) override;
    void removeClip(int timelineId, int clipId) override;
    void trimClip(int timelineId, int clipId, const TimelineRange& newRange,
                  const SourceRange& newSource) override;
    void moveClip(int timelineId, int clipId, int newTrackIndex,
                  double newInTime) override;
    std::optional<int> splitClip(int timelineId, int clipId,
                                  double splitTimeSeconds) override;
    void rippleDelete(int timelineId, int trackIndex, double rangeStart,
                      double rangeEnd) override;
    void moveMultipleClips(int timelineId, const std::vector<int>& clipIds,
                           double deltaTime, int deltaTrack) override;
    void swapClips(int timelineId, int clipIdA, int clipIdB) override;
    int splitAllTracks(int timelineId, double splitTimeSeconds) override;
    void liftDelete(int timelineId, int trackIndex, double rangeStart,
                    double rangeEnd) override;
    int checkOverlap(int timelineId, int trackIndex, double inTime,
                     double outTime, int excludeClipId) override;
    std::vector<int> getOverlappingClips(int timelineId, int trackIndex,
                                          double inTime, double outTime) override;

    // --- TimelinePlayback ---
    void seek(int timelineId, double positionSeconds) override;
    std::optional<DecodedImage> renderFrame(int timelineId) override;
    double getPosition(int timelineId) override;
    void setFrameCacheSizeBytes(int timelineId, int maxBytes) override;
    void invalidateFrameCache(int timelineId) override;

    // --- TimelineNleEdits ---
    int insertEdit(int timelineId, int trackIndex, double atTime,
                   const ClipDescriptor& clip) override;
    int overwriteEdit(int timelineId, int trackIndex, double atTime,
                      const ClipDescriptor& clip) override;
    void rollEdit(int timelineId, int clipId, double deltaSec) override;
    void slipEdit(int timelineId, int clipId, double deltaSec) override;
    void slideEdit(int timelineId, int clipId, double deltaSec) override;
    void rateStretch(int timelineId, int clipId, double newDurationSec) override;
    int duplicateClip(int timelineId, int clipId) override;
    std::vector<double> getSnapPoints(int timelineId, double timeSec,
                                       double thresholdSec) override;

    // --- TimelineMediaProbe ---
    std::optional<MediaInfo> probeMedia(const QString& filePath) override;
    std::optional<MediaInfo> probeMediaFast(const QString& filePath) override;

    // --- VideoTimelineEngine extended ---
    void setClipVolume(int timelineId, int clipId, double volume) override;
    double getClipVolume(int timelineId, int clipId) override;
    void setClipTransitionIn(int timelineId, int clipId, int transitionType,
                             double durationSeconds, int easingCurve) override;
    void setClipTransitionOut(int timelineId, int clipId, int transitionType,
                              double durationSeconds, int easingCurve) override;
    void setClipKeyframe(int timelineId, int clipId, int property, double time,
                         double value, int interpolation) override;
    void removeClipKeyframe(int timelineId, int clipId, int property,
                            double time) override;
    void clearClipKeyframes(int timelineId, int clipId, int property) override;
    void setClipColorGrading(int timelineId, int clipId, double brightness,
        double contrast, double saturation, double exposure,
        double temperature, double tint, double highlights, double shadows,
        double vibrance, double hue, double fade, double vignette) override;
    void clearClipEffects(int timelineId, int clipId) override;
    void setClipPan(int timelineId, int clipId, double pan) override;
    void setClipFadeIn(int timelineId, int clipId, double seconds) override;
    void setClipFadeOut(int timelineId, int clipId, double seconds) override;
    void setTrackVolume(int timelineId, int trackIndex, double volume) override;
    void setTrackPan(int timelineId, int trackIndex, double pan) override;
    void setTrackMute(int timelineId, int trackIndex, bool mute) override;
    void setTrackSolo(int timelineId, int trackIndex, bool solo) override;
    int startExport(int timelineId, const VideoExportConfig& config) override;
    double getExportProgress(int exportJobId) override;
    void cancelExport(int exportJobId) override;
    bool supportsHardwareEncoding() const override;
    std::vector<EngineEffectDef> listEffects(const QString& category) override;
    int addClipEffect(int timelineId, int clipId, const QString& effectDefId) override;
    void removeClipEffect(int timelineId, int clipId, int effectInstanceId) override;
    void reorderClipEffects(int timelineId, int clipId,
                            const std::vector<int>& instanceIds) override;
    void setEffectParam(int timelineId, int clipId, int effectInstanceId,
                        const QString& paramId, double value) override;
    void setEffectEnabled(int timelineId, int clipId, int effectInstanceId,
                          bool enabled) override;
    void setEffectMix(int timelineId, int clipId, int effectInstanceId,
                      double mix) override;
    int addClipMask(int timelineId, int clipId, const MaskData& mask) override;
    void updateClipMask(int timelineId, int clipId, int maskId,
                        const MaskData& mask) override;
    void removeClipMask(int timelineId, int clipId, int maskId) override;
    int startTracking(int timelineId, int clipId, double x, double y,
                      double timeSec) override;
    std::vector<TrackPoint> getTrackingData(int timelineId, int trackerId) override;
    void stabilizeClip(int timelineId, int clipId,
                       const StabilizationConfig& config) override;
    void setClipText(int timelineId, int clipId, const TextLayerData& textData) override;
    int addClipShape(int timelineId, int clipId, const ShapeData& shape) override;
    void updateClipShape(int timelineId, int clipId, int shapeId,
                         const ShapeData& shape) override;
    void removeClipShape(int timelineId, int clipId, int shapeId) override;
    int addAudioEffect(int timelineId, int clipId,
                       const QString& audioEffectDefId) override;
    void setAudioEffectParam(int timelineId, int clipId, int effectInstanceId,
                             const QString& paramId, double value) override;
    void removeAudioEffect(int timelineId, int clipId, int effectInstanceId) override;
    std::vector<AudioEffectDef> listAudioEffects() override;
    int startAiSegmentation(int timelineId, int clipId,
                            const AiSegmentationConfig& config) override;
    double getAiSegmentationProgress(int jobId) override;
    void cancelAiSegmentation(int jobId) override;
    void enableProxyMode(int timelineId, const ProxyConfig& config) override;
    void disableProxyMode(int timelineId) override;
    bool isProxyModeActive(int timelineId) override;
    int createMultiCamClip(int timelineId, int trackIndex,
                           const MultiCamConfig& config) override;
    void switchMultiCamAngle(int timelineId, int clipId, int angleIndex,
                             double atTimeSec) override;
    void flattenMultiCam(int timelineId, int clipId) override;
    int createTextureBridge(int width, int height) override;
    void destroyTextureBridge() override;
    bool renderToTextureBridge(int timelineId) override;
    void resizeTextureBridge(int width, int height) override;
    std::optional<QByteArray> getTextureBridgePixels() override;

private:
    void checkErr(int err);
    GopostTimeline* getTimeline(int id);
    static int toTrackType(VideoTrackType t);
    static int toClipSourceType(VideoClipSourceType t);
    void fillDescriptor(GopostClipDescriptor* desc, const ClipDescriptor& clip);

    GopostEngine* enginePtr_ = nullptr;
    int nextTimelineId_ = 1;
    std::map<int, GopostTimeline*> timelines_;
    void* textureBridge_ = nullptr;
    int64_t textureBridgeId_ = -1;
};

} // namespace gopost::rendering
