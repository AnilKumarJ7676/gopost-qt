#include "rendering_bridge/stub_video_timeline_engine.h"

#include <QDebug>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QColor>
#include <QFont>
#include <QRect>

#include <algorithm>
#include <cmath>
#include <set>

namespace gopost::rendering {

// =========================================================================
// Helpers
// =========================================================================

void StubVideoTimelineEngine::checkTimeline(int id) {
    if (configs_.find(id) == configs_.end())
        throw EngineException("Unknown timeline");
}

double StubVideoTimelineEngine::computeDuration(int timelineId) {
    double end = 0;
    auto it = tracks_.find(timelineId);
    if (it == tracks_.end()) return 0;
    for (const auto& track : it->second) {
        for (const auto& clip : track.clips) {
            if (clip.timelineRange.outTime > end)
                end = clip.timelineRange.outTime;
        }
    }
    return end;
}

StubVideoTimelineEngine::StubClip* StubVideoTimelineEngine::findClip(
    int timelineId, int clipId) {
    auto it = tracks_.find(timelineId);
    if (it == tracks_.end()) return nullptr;
    for (auto& track : it->second) {
        for (auto& clip : track.clips) {
            if (clip.id == clipId) return &clip;
        }
    }
    return nullptr;
}

// =========================================================================
// TimelineLifecycle
// =========================================================================

int StubVideoTimelineEngine::createTimeline(const TimelineConfig& config) {
    int id = nextId_++;
    configs_[id] = config;
    tracks_[id] = {};
    positions_[id] = 0.0;
    return id;
}

void StubVideoTimelineEngine::destroyTimeline(int timelineId) {
    // Clean up decode threads for all clips in this timeline
    auto trackIt = tracks_.find(timelineId);
    if (trackIt != tracks_.end()) {
        for (const auto& track : trackIt->second) {
            for (const auto& clip : track.clips) {
                decoders_.erase(clip.id);
                cachedFrames_.erase(clip.id);
                lastSeekTime_.erase(clip.id);
            }
        }
    }
    configs_.erase(timelineId);
    tracks_.erase(timelineId);
    positions_.erase(timelineId);
}

TimelineConfig StubVideoTimelineEngine::getTimelineConfig(int timelineId) {
    checkTimeline(timelineId);
    return configs_[timelineId];
}

double StubVideoTimelineEngine::getDuration(int timelineId) {
    checkTimeline(timelineId);
    return computeDuration(timelineId);
}

// =========================================================================
// TimelineTrackOps
// =========================================================================

int StubVideoTimelineEngine::addTrack(int timelineId, VideoTrackType type) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    StubTrack t;
    t.type = type;
    tracks.push_back(std::move(t));
    return static_cast<int>(tracks.size()) - 1;
}

void StubVideoTimelineEngine::removeTrack(int timelineId, int trackIndex) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size())) {
        tracks.erase(tracks.begin() + trackIndex);
    }
}

int StubVideoTimelineEngine::getTrackCount(int timelineId) {
    checkTimeline(timelineId);
    return static_cast<int>(tracks_[timelineId].size());
}

void StubVideoTimelineEngine::reorderTracks(int timelineId,
                                              const std::vector<int>& newOrder) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    if (newOrder.size() != tracks.size()) return;
    std::vector<StubTrack> reordered;
    reordered.reserve(tracks.size());
    for (int idx : newOrder) {
        if (idx < 0 || idx >= static_cast<int>(tracks.size())) return;
        reordered.push_back(std::move(tracks[idx]));
    }
    tracks = std::move(reordered);
    for (int i = 0; i < static_cast<int>(tracks_[timelineId].size()); i++) {
        for (auto& clip : tracks_[timelineId][i].clips) {
            clip.trackIndex = i;
        }
    }
}

void StubVideoTimelineEngine::setTrackSyncLock(int timelineId, int, bool) {
    checkTimeline(timelineId);
}

void StubVideoTimelineEngine::setTrackHeight(int timelineId, int, double) {
    checkTimeline(timelineId);
}

double StubVideoTimelineEngine::getTrackHeight(int timelineId, int) {
    checkTimeline(timelineId);
    return 68.0;
}

// =========================================================================
// TimelineClipOps
// =========================================================================

int StubVideoTimelineEngine::addClip(int timelineId,
                                      const ClipDescriptor& descriptor) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    while (static_cast<int>(tracks.size()) <= descriptor.trackIndex) {
        StubTrack t;
        t.type = VideoTrackType::Video;
        tracks.push_back(std::move(t));
    }
    int clipId = nextClipId_++;
    StubClip clip;
    clip.id = clipId;
    clip.trackIndex = descriptor.trackIndex;
    clip.sourceType = descriptor.sourceType;
    clip.sourcePath = descriptor.sourcePath;
    clip.timelineRange = descriptor.timelineRange;
    clip.sourceRange = descriptor.sourceRange;
    clip.speed = descriptor.speed;
    clip.opacity = descriptor.opacity;
    clip.blendMode = descriptor.blendMode;
    clip.effectHash = descriptor.effectHash;
    tracks[descriptor.trackIndex].clips.push_back(std::move(clip));

    // Start a background decode thread for video clips (only when decode is enabled)
    if (decodeEnabled_ &&
        descriptor.sourceType == VideoClipSourceType::Video &&
        !descriptor.sourcePath.isEmpty()) {
        qDebug() << "[StubEngine] addClip: starting decode thread for clip" << clipId
                 << "path:" << descriptor.sourcePath;
        auto dt = std::make_unique<RenderDecodeThread>(24);
        if (dt->open(descriptor.sourcePath)) {
            qDebug() << "[StubEngine] decode thread opened:" << dt->width() << "x" << dt->height()
                     << "fps:" << dt->frameRate() << "duration:" << dt->duration();
            // Seek to source-in so we start decoding from the clip's start
            if (descriptor.sourceRange.sourceIn > 0.01) {
                dt->seekTo(descriptor.sourceRange.sourceIn);
            }
            decoders_[clipId] = std::move(dt);
        } else {
            qWarning() << "[StubEngine] FAILED to open decode thread for:" << descriptor.sourcePath;
        }
    }

    return clipId;
}

void StubVideoTimelineEngine::removeClip(int timelineId, int clipId) {
    checkTimeline(timelineId);
    for (auto& track : tracks_[timelineId]) {
        track.clips.erase(
            std::remove_if(track.clips.begin(), track.clips.end(),
                           [clipId](const StubClip& c) {
                               return c.id == clipId;
                           }),
            track.clips.end());
    }
    decoders_.erase(clipId);
    cachedFrames_.erase(clipId);
    lastSeekTime_.erase(clipId);
}

void StubVideoTimelineEngine::trimClip(int timelineId, int clipId,
                                        const TimelineRange& newRange,
                                        const SourceRange& newSource) {
    checkTimeline(timelineId);
    auto* clip = findClip(timelineId, clipId);
    if (clip) {
        clip->timelineRange = newRange;
        clip->sourceRange = newSource;
    }
}

void StubVideoTimelineEngine::moveClip(int timelineId, int clipId,
                                        int newTrackIndex, double newInTime) {
    checkTimeline(timelineId);
    auto* clip = findClip(timelineId, clipId);
    if (!clip) return;
    double duration =
        clip->timelineRange.outTime - clip->timelineRange.inTime;

    // Remove from old track
    for (auto& track : tracks_[timelineId]) {
        track.clips.erase(
            std::remove_if(track.clips.begin(), track.clips.end(),
                           [clipId](const StubClip& c) {
                               return c.id == clipId;
                           }),
            track.clips.end());
    }

    auto& tracks = tracks_[timelineId];
    while (static_cast<int>(tracks.size()) <= newTrackIndex) {
        StubTrack t;
        t.type = VideoTrackType::Video;
        tracks.push_back(std::move(t));
    }

    // We need a fresh lookup since we erased
    StubClip newClip;
    newClip.id = clipId;
    newClip.trackIndex = newTrackIndex;
    newClip.timelineRange = {newInTime, newInTime + duration};
    tracks[newTrackIndex].clips.push_back(std::move(newClip));
}

std::optional<int> StubVideoTimelineEngine::splitClip(int timelineId,
                                                       int clipId,
                                                       double splitTime) {
    checkTimeline(timelineId);
    auto* clip = findClip(timelineId, clipId);
    if (!clip) return std::nullopt;
    double inT = clip->timelineRange.inTime;
    double outT = clip->timelineRange.outTime;
    if (splitTime <= inT || splitTime >= outT) return std::nullopt;

    double clipDuration = outT - inT;
    double sourceDuration =
        clip->sourceRange.sourceOut - clip->sourceRange.sourceIn;
    double tRatio =
        clipDuration > 0 ? (splitTime - inT) / clipDuration : 0.0;
    double sourceAtSplit = clip->sourceRange.sourceIn + tRatio * sourceDuration;

    int newId = nextClipId_++;
    StubClip right;
    right.id = newId;
    right.trackIndex = clip->trackIndex;
    right.sourceType = clip->sourceType;
    right.sourcePath = clip->sourcePath;
    right.timelineRange = {splitTime, outT};
    right.sourceRange = {sourceAtSplit, clip->sourceRange.sourceOut};
    right.speed = clip->speed;
    right.opacity = clip->opacity;
    right.blendMode = clip->blendMode;
    right.effectHash = clip->effectHash;

    clip->timelineRange = {inT, splitTime};
    clip->sourceRange = {clip->sourceRange.sourceIn, sourceAtSplit};

    auto& tracks = tracks_[timelineId];
    if (clip->trackIndex < static_cast<int>(tracks.size())) {
        tracks[clip->trackIndex].clips.push_back(std::move(right));
    }
    return newId;
}

void StubVideoTimelineEngine::rippleDelete(int timelineId, int trackIndex,
                                            double rangeStart,
                                            double rangeEnd) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) return;
    auto& track = tracks[trackIndex];
    double deleteDuration = rangeEnd - rangeStart;
    if (deleteDuration <= 0) return;

    std::vector<StubClip> kept;
    for (auto& clip : track.clips) {
        bool overlaps = clip.timelineRange.inTime < rangeEnd &&
                        clip.timelineRange.outTime > rangeStart;
        if (overlaps) continue;
        if (clip.timelineRange.outTime > rangeEnd) {
            clip.timelineRange.inTime -= deleteDuration;
            clip.timelineRange.outTime -= deleteDuration;
        }
        kept.push_back(std::move(clip));
    }
    track.clips = std::move(kept);
}

void StubVideoTimelineEngine::moveMultipleClips(
    int timelineId, const std::vector<int>& clipIds, double deltaTime, int) {
    checkTimeline(timelineId);
    for (int id : clipIds) {
        auto* clip = findClip(timelineId, id);
        if (clip) {
            clip->timelineRange.inTime += deltaTime;
            clip->timelineRange.outTime += deltaTime;
        }
    }
}

void StubVideoTimelineEngine::swapClips(int timelineId, int clipIdA,
                                          int clipIdB) {
    checkTimeline(timelineId);
    auto* a = findClip(timelineId, clipIdA);
    auto* b = findClip(timelineId, clipIdB);
    if (a && b) {
        std::swap(a->timelineRange, b->timelineRange);
    }
}

int StubVideoTimelineEngine::splitAllTracks(int timelineId,
                                             double splitTimeSeconds) {
    checkTimeline(timelineId);
    int count = 0;
    auto& tracks = tracks_[timelineId];
    for (auto& track : tracks) {
        std::vector<int> toSplit;
        for (const auto& clip : track.clips) {
            if (splitTimeSeconds > clip.timelineRange.inTime + 0.001 &&
                splitTimeSeconds < clip.timelineRange.outTime - 0.001) {
                toSplit.push_back(clip.id);
            }
        }
        for (int id : toSplit) {
            auto result = splitClip(timelineId, id, splitTimeSeconds);
            if (result.has_value()) count++;
        }
    }
    return count;
}

void StubVideoTimelineEngine::liftDelete(int timelineId, int trackIndex,
                                           double rangeStart,
                                           double rangeEnd) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size())) {
        auto& clips = tracks[trackIndex].clips;
        clips.erase(
            std::remove_if(clips.begin(), clips.end(),
                           [rangeStart, rangeEnd](const StubClip& c) {
                               return c.timelineRange.inTime >= rangeStart &&
                                      c.timelineRange.outTime <= rangeEnd;
                           }),
            clips.end());
    }
}

int StubVideoTimelineEngine::checkOverlap(int timelineId, int trackIndex,
                                            double inTime, double outTime,
                                            int excludeClipId) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size()))
        return 0;
    bool hasAdjacent = false;
    for (const auto& clip : tracks[trackIndex].clips) {
        if (clip.id == excludeClipId) continue;
        if (clip.timelineRange.inTime < outTime - 0.001 &&
            clip.timelineRange.outTime > inTime + 0.001) {
            return 1; // OVERLAP
        }
        if (std::abs(clip.timelineRange.inTime - outTime) < 0.001 ||
            std::abs(clip.timelineRange.outTime - inTime) < 0.001) {
            hasAdjacent = true;
        }
    }
    return hasAdjacent ? 2 : 0;
}

std::vector<int> StubVideoTimelineEngine::getOverlappingClips(
    int timelineId, int trackIndex, double inTime, double outTime) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size()))
        return {};
    std::vector<int> result;
    for (const auto& clip : tracks[trackIndex].clips) {
        if (clip.timelineRange.inTime < outTime - 0.001 &&
            clip.timelineRange.outTime > inTime + 0.001) {
            result.push_back(clip.id);
        }
    }
    return result;
}

// =========================================================================
// TimelinePlayback
// =========================================================================

void StubVideoTimelineEngine::seek(int timelineId, double positionSeconds) {
    checkTimeline(timelineId);
    double prevPos = positions_[timelineId];
    positions_[timelineId] = positionSeconds;

    // Only seek decode threads if the position jumped significantly.
    // Small sequential advances (< 0.5s) let the decode thread keep
    // streaming frames sequentially — far cheaper than kill/restart ffmpeg.
    double jumpThreshold = 0.5;
    bool isJump = std::abs(positionSeconds - prevPos) > jumpThreshold;

    if (!isJump) return;  // Sequential playback — no need to restart ffmpeg

    auto trackIt = tracks_.find(timelineId);
    if (trackIt == tracks_.end()) return;

    for (const auto& track : trackIt->second) {
        for (const auto& clip : track.clips) {
            auto decIt = decoders_.find(clip.id);
            if (decIt == decoders_.end()) continue;

            double clipDuration = clip.timelineRange.outTime - clip.timelineRange.inTime;
            if (clipDuration <= 0) continue;

            if (positionSeconds >= clip.timelineRange.inTime &&
                positionSeconds < clip.timelineRange.outTime) {
                // Active clip — seek decoder to the correct source time
                double progress = (positionSeconds - clip.timelineRange.inTime) / clipDuration;
                double sourceDuration = clip.sourceRange.sourceOut - clip.sourceRange.sourceIn;
                double sourceTime = clip.sourceRange.sourceIn + progress * sourceDuration;

                auto lastIt = lastSeekTime_.find(clip.id);
                if (lastIt == lastSeekTime_.end() || std::abs(sourceTime - lastIt->second) > 0.3) {
                    decIt->second->seekTo(sourceTime);
                    lastSeekTime_[clip.id] = sourceTime;
                }
            } else {
                // Pre-warm: if clip starts within 1 second, pre-seek its
                // decoder so frames are buffered before the transition.
                double distToStart = clip.timelineRange.inTime - positionSeconds;
                if (distToStart > 0 && distToStart < 1.0) {
                    auto lastIt = lastSeekTime_.find(clip.id);
                    if (lastIt == lastSeekTime_.end() ||
                        std::abs(clip.sourceRange.sourceIn - lastIt->second) > 0.3) {
                        decIt->second->seekTo(clip.sourceRange.sourceIn);
                        lastSeekTime_[clip.id] = clip.sourceRange.sourceIn;
                    }
                }
            }
        }
    }
}

std::optional<DecodedImage> StubVideoTimelineEngine::renderFrame(
    int timelineId) {
    auto cfg = configs_[timelineId];
    int w = cfg.width  > 0 ? cfg.width  : 1920;
    int h = cfg.height > 0 ? cfg.height : 1080;
    return renderFrame(timelineId, w, h);
}

std::optional<DecodedImage> StubVideoTimelineEngine::renderFrame(
    int timelineId, int previewW, int previewH) {
    checkTimeline(timelineId);

    int w = previewW > 0 ? previewW : 640;
    int h = previewH > 0 ? previewH : 360;
    double pos = positions_[timelineId];

    // Find clips visible at current position
    auto trackIt = tracks_.find(timelineId);
    if (trackIt == tracks_.end() || trackIt->second.empty()) {
        return std::nullopt;  // no tracks → no frame
    }

    // Check if any track is soloed
    bool anySolo = false;
    for (const auto& [idx, params] : trackAudioParams_) {
        if (params.solo) { anySolo = true; break; }
    }

    // Collect visible clips across all tracks (bottom-to-top order)
    struct VisibleClip {
        const StubClip* clip;
        int trackIdx;
    };
    std::vector<VisibleClip> visible;
    for (int ti = static_cast<int>(trackIt->second.size()) - 1; ti >= 0; --ti) {
        // Solo filter: if any track is soloed, skip non-soloed tracks
        if (anySolo) {
            auto paramIt = trackAudioParams_.find(ti);
            if (paramIt == trackAudioParams_.end() || !paramIt->second.solo)
                continue;
        }
        for (const auto& clip : trackIt->second[ti].clips) {
            if (pos >= clip.timelineRange.inTime - 0.001 &&
                pos < clip.timelineRange.outTime + 0.001) {
                visible.push_back({&clip, ti});
            }
        }
    }

    // Base canvas (dark background)
    QImage img(w, h, QImage::Format_RGBA8888);
    img.fill(QColor(13, 13, 26));

    if (visible.empty()) {
        DecodedImage decoded;
        decoded.width = w; decoded.height = h;
        decoded.pixels = QByteArray(reinterpret_cast<const char*>(img.constBits()),
                                    img.sizeInBytes());
        return decoded;
    }

    // ---- Multi-track compositing: render each clip then composite ----
    for (const auto& vc : visible) {
        const auto* clip = vc.clip;
        double localTime = pos - clip->timelineRange.inTime;

        // Render clip content to its own frame
        QImage clipFrame(w, h, QImage::Format_RGBA8888);
        clipFrame.fill(Qt::transparent);

        bool hasContent = false;

        // Check for transition-in: blend with previous clip on same track
        auto trInIt = clipTransitionIn_.find(clip->id);
        if (trInIt != clipTransitionIn_.end() && trInIt->second.type > 0 &&
            localTime < trInIt->second.durationSec) {
            // Find outgoing clip (previous clip on same track)
            const StubClip* outClip = nullptr;
            for (const auto& c : trackIt->second[vc.trackIdx].clips) {
                if (std::abs(c.timelineRange.outTime - clip->timelineRange.inTime) < 0.01) {
                    outClip = &c; break;
                }
            }
            if (outClip) {
                QImage outFrame(w, h, QImage::Format_RGBA8888);
                outFrame.fill(Qt::transparent);
                renderClipContent(outFrame, outClip, pos, w, h);

                QImage inFrame(w, h, QImage::Format_RGBA8888);
                inFrame.fill(Qt::transparent);
                renderClipContent(inFrame, clip, pos, w, h);

                double progress = localTime / std::max(0.001, trInIt->second.durationSec);
                renderTransition(clipFrame, outFrame, inFrame, progress,
                                 trInIt->second.type, trInIt->second.easing, w, h);
                hasContent = true;
            }
        }

        if (!hasContent) {
            renderClipContent(clipFrame, clip, pos, w, h);
        }

        // Apply color grading
        applyColorGrading(clipFrame, clip->id);

        // Apply effects
        applyEffects(clipFrame, clip->id);

        // Apply keyframe transforms
        applyTransform(clipFrame, clip->id, localTime, w, h);

        // Evaluate per-clip opacity (base + keyframe-animated)
        double opacity = clip->opacity;
        auto kfIt = clipKeyframes_.find(clip->id);
        if (kfIt != clipKeyframes_.end()) {
            auto pIt = kfIt->second.find(4); // KeyframeProperty::Opacity
            if (pIt != kfIt->second.end() && !pIt->second.empty())
                opacity = evaluateKeyframes(pIt->second, localTime);
        }

        // Composite onto main canvas with blend mode
        compositeFrame(img, clipFrame, opacity, clip->blendMode);
    }

    DecodedImage decoded;
    decoded.width  = w;
    decoded.height = h;
    decoded.pixels = QByteArray(reinterpret_cast<const char*>(img.constBits()),
                                img.sizeInBytes());
    return decoded;
}

// =========================================================================
// Render clip content (video frame, placeholder, title text, color solid)
// =========================================================================

void StubVideoTimelineEngine::renderClipContent(
    QImage& clipFrame, const StubClip* clip, double pos, int w, int h) {
    double clipProgress = (pos - clip->timelineRange.inTime)
                        / std::max(0.001, clip->timelineRange.outTime - clip->timelineRange.inTime);
    clipProgress = std::clamp(clipProgress, 0.0, 1.0);

    // Title clips: render text
    if (clip->sourceType == VideoClipSourceType::Title) {
        auto textIt = clipTextData_.find(clip->id);
        if (textIt != clipTextData_.end()) {
            renderTextLayer(clipFrame, textIt->second, w, h);
        } else {
            // Fallback: render displayName as centered text
            QPainter tp(&clipFrame);
            tp.setRenderHint(QPainter::Antialiasing);
            tp.fillRect(0, 0, w, h, QColor(20, 10, 40, 200));
            tp.setPen(Qt::white);
            QFont f(QStringLiteral("Inter"), 36);
            f.setBold(true);
            tp.setFont(f);
            tp.drawText(QRect(0, 0, w, h), Qt::AlignCenter, clip->displayName);
            tp.end();
        }
        return;
    }

    // Color clips: solid color fill
    if (clip->sourceType == VideoClipSourceType::Color) {
        clipFrame.fill(QColor(102, 187, 106)); // default green
        // Render shapes if any
        auto shIt = clipShapes_.find(clip->id);
        if (shIt != clipShapes_.end()) {
            QPainter sp(&clipFrame);
            sp.setRenderHint(QPainter::Antialiasing);
            for (const auto& [shapeId, sd] : shIt->second) {
                double cx = sd.x * w, cy = sd.y * h;
                double sw = sd.width * w, sh = sd.height * h;
                sp.save();
                sp.translate(cx, cy);
                if (sd.rotation != 0) sp.rotate(sd.rotation);
                QColor fill(sd.fillColor >> 16 & 0xFF, sd.fillColor >> 8 & 0xFF,
                            sd.fillColor & 0xFF, sd.fillColor >> 24 & 0xFF);
                QColor stroke(sd.strokeColor >> 16 & 0xFF, sd.strokeColor >> 8 & 0xFF,
                              sd.strokeColor & 0xFF, sd.strokeColor >> 24 & 0xFF);
                if (sd.fillEnabled) sp.setBrush(fill); else sp.setBrush(Qt::NoBrush);
                if (sd.strokeEnabled) sp.setPen(QPen(stroke, sd.strokeWidth)); else sp.setPen(Qt::NoPen);
                if (sd.type == ShapeType::Ellipse)
                    sp.drawEllipse(QRectF(-sw/2, -sh/2, sw, sh));
                else
                    sp.drawRoundedRect(QRectF(-sw/2, -sh/2, sw, sh), sd.cornerRadius, sd.cornerRadius);
                sp.restore();
            }
            sp.end();
        }
        return;
    }

    // Video/Image clips: try decoded frame, fallback to placeholder
    auto decIt = decoders_.find(clip->id);
    if (decIt != decoders_.end() && decIt->second->isOpen()) {
        auto decodedFrame = decIt->second->popFrame();
        if (decodedFrame.has_value()) {
            if (cachedFrames_.find(clip->id) == cachedFrames_.end()) {
                qDebug() << "[StubEngine] first decoded frame for clip" << clip->id;
            }
            cachedFrames_[clip->id] = CachedFrame{std::move(*decodedFrame), pos};
        }
        auto cacheIt = cachedFrames_.find(clip->id);
        if (cacheIt != cachedFrames_.end() && !cacheIt->second.frame.pixels.empty()) {
            const auto& cf = cacheIt->second.frame;
            QImage frameImg(cf.pixels.data(), cf.width, cf.height,
                            cf.width * 4, QImage::Format_RGBA8888);
            QPainter fp(&clipFrame);
            fp.drawImage(QRect(0, 0, w, h), frameImg);
            fp.end();
            return;
        }
    }

    // Fallback: colored placeholder
    QColor clipColor;
    switch (clip->sourceType) {
    case VideoClipSourceType::Video: clipColor = QColor(38, 198, 218); break;
    case VideoClipSourceType::Image: clipColor = QColor(255, 112, 67); break;
    default:                         clipColor = QColor(108, 99, 255); break;
    }

    QPainter pp(&clipFrame);
    pp.fillRect(0, 0, w, h, QColor(clipColor.red()/4, clipColor.green()/4, clipColor.blue()/4));
    int barY = h/2 - 30, barH = 60;
    pp.fillRect(0, barY, static_cast<int>(w * clipProgress), barH, clipColor);
    pp.fillRect(static_cast<int>(w*clipProgress), barY, w-static_cast<int>(w*clipProgress), barH,
                QColor(clipColor.red()/2, clipColor.green()/2, clipColor.blue()/2));
    pp.setPen(Qt::white);
    QFont f(QStringLiteral("sans-serif"), 14); f.setBold(true); pp.setFont(f);
    QString name = clip->sourcePath;
    int sl = name.lastIndexOf('/'); if (sl < 0) sl = name.lastIndexOf('\\');
    if (sl >= 0) name = name.mid(sl + 1);
    pp.drawText(QRect(0, barY - 40, w, 30), Qt::AlignCenter, name);
    // Timecode
    int mins = static_cast<int>(pos)/60, secs = static_cast<int>(pos)%60;
    int fr = static_cast<int>((pos - std::floor(pos)) * 30);
    pp.drawText(QRect(0, barY+barH+5, w, 25), Qt::AlignCenter,
                QString("%1:%2:%3").arg(mins,2,10,QChar('0')).arg(secs,2,10,QChar('0')).arg(fr,2,10,QChar('0')));
    pp.end();
}

double StubVideoTimelineEngine::getPosition(int timelineId) {
    checkTimeline(timelineId);
    return positions_[timelineId];
}

void StubVideoTimelineEngine::setFrameCacheSizeBytes(int timelineId, int) {
    checkTimeline(timelineId);
}

void StubVideoTimelineEngine::invalidateFrameCache(int timelineId) {
    checkTimeline(timelineId);
}

// =========================================================================
// TimelineNleEdits
// =========================================================================

int StubVideoTimelineEngine::insertEdit(int timelineId, int trackIndex,
                                          double atTime,
                                          const ClipDescriptor& clip) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    while (static_cast<int>(tracks.size()) <= trackIndex) {
        StubTrack t;
        t.type = VideoTrackType::Video;
        tracks.push_back(std::move(t));
    }
    double duration = clip.timelineRange.outTime - clip.timelineRange.inTime;
    if (duration <= 0) return -1;

    // Push existing clips right
    for (auto& c : tracks[trackIndex].clips) {
        if (c.timelineRange.inTime >= atTime) {
            c.timelineRange.inTime += duration;
            c.timelineRange.outTime += duration;
        }
    }

    int clipId = nextClipId_++;
    StubClip sc;
    sc.id = clipId;
    sc.trackIndex = trackIndex;
    sc.sourceType = clip.sourceType;
    sc.sourcePath = clip.sourcePath;
    sc.timelineRange = {atTime, atTime + duration};
    sc.sourceRange = clip.sourceRange;
    sc.speed = clip.speed;
    sc.opacity = clip.opacity;
    sc.blendMode = clip.blendMode;
    sc.effectHash = clip.effectHash;
    tracks[trackIndex].clips.push_back(std::move(sc));
    return clipId;
}

int StubVideoTimelineEngine::overwriteEdit(int timelineId, int trackIndex,
                                             double atTime,
                                             const ClipDescriptor& clip) {
    checkTimeline(timelineId);
    auto& tracks = tracks_[timelineId];
    while (static_cast<int>(tracks.size()) <= trackIndex) {
        StubTrack t;
        t.type = VideoTrackType::Video;
        tracks.push_back(std::move(t));
    }
    double duration = clip.timelineRange.outTime - clip.timelineRange.inTime;
    if (duration <= 0) return -1;

    double rangeEnd = atTime + duration;
    auto& clips = tracks[trackIndex].clips;
    clips.erase(std::remove_if(clips.begin(), clips.end(),
                                [atTime, rangeEnd](const StubClip& c) {
                                    return c.timelineRange.inTime < rangeEnd &&
                                           c.timelineRange.outTime > atTime;
                                }),
                 clips.end());

    int clipId = nextClipId_++;
    StubClip sc;
    sc.id = clipId;
    sc.trackIndex = trackIndex;
    sc.sourceType = clip.sourceType;
    sc.sourcePath = clip.sourcePath;
    sc.timelineRange = {atTime, rangeEnd};
    sc.sourceRange = clip.sourceRange;
    sc.speed = clip.speed;
    sc.opacity = clip.opacity;
    sc.blendMode = clip.blendMode;
    sc.effectHash = clip.effectHash;
    clips.push_back(std::move(sc));
    return clipId;
}

void StubVideoTimelineEngine::rollEdit(int timelineId, int, double) {
    checkTimeline(timelineId);
}

void StubVideoTimelineEngine::slipEdit(int timelineId, int clipId,
                                         double deltaSec) {
    checkTimeline(timelineId);
    auto* clip = findClip(timelineId, clipId);
    if (!clip) return;
    double newIn = clip->sourceRange.sourceIn + deltaSec;
    double newOut = clip->sourceRange.sourceOut + deltaSec;
    if (newIn < 0 || newOut <= newIn) return;
    clip->sourceRange = {newIn, newOut};
}

void StubVideoTimelineEngine::slideEdit(int timelineId, int clipId,
                                          double deltaSec) {
    checkTimeline(timelineId);
    auto* clip = findClip(timelineId, clipId);
    if (!clip) return;
    double duration =
        clip->timelineRange.outTime - clip->timelineRange.inTime;
    double newIn = clip->timelineRange.inTime + deltaSec;
    if (newIn < 0) return;
    clip->timelineRange = {newIn, newIn + duration};
}

void StubVideoTimelineEngine::rateStretch(int timelineId, int clipId,
                                            double newDurationSec) {
    checkTimeline(timelineId);
    auto* clip = findClip(timelineId, clipId);
    if (!clip) return;
    double oldDuration =
        clip->timelineRange.outTime - clip->timelineRange.inTime;
    if (oldDuration <= 0 || newDurationSec <= 0) return;
    clip->speed = clip->speed * oldDuration / newDurationSec;
    clip->timelineRange.outTime =
        clip->timelineRange.inTime + newDurationSec;
}

int StubVideoTimelineEngine::duplicateClip(int timelineId, int clipId) {
    checkTimeline(timelineId);
    auto* clip = findClip(timelineId, clipId);
    if (!clip) return -1;
    double duration =
        clip->timelineRange.outTime - clip->timelineRange.inTime;
    int newId = nextClipId_++;
    auto& tracks = tracks_[timelineId];
    if (clip->trackIndex < static_cast<int>(tracks.size())) {
        StubClip copy = *clip;
        copy.id = newId;
        copy.timelineRange = {clip->timelineRange.outTime,
                              clip->timelineRange.outTime + duration};
        tracks[clip->trackIndex].clips.push_back(std::move(copy));
    }
    return newId;
}

std::vector<double> StubVideoTimelineEngine::getSnapPoints(int timelineId,
                                                             double timeSec,
                                                             double threshold) {
    checkTimeline(timelineId);
    std::set<double> points;
    for (const auto& track : tracks_[timelineId]) {
        for (const auto& clip : track.clips) {
            if (std::abs(clip.timelineRange.inTime - timeSec) <= threshold)
                points.insert(clip.timelineRange.inTime);
            if (std::abs(clip.timelineRange.outTime - timeSec) <= threshold)
                points.insert(clip.timelineRange.outTime);
        }
    }
    return {points.begin(), points.end()};
}

// =========================================================================
// TimelineMediaProbe
// =========================================================================

std::optional<MediaInfo> StubVideoTimelineEngine::probeMedia(
    const QString& filePath) {
    QString lower = filePath.toLower();
    bool isImage = lower.endsWith(QStringLiteral(".jpg")) ||
                   lower.endsWith(QStringLiteral(".jpeg")) ||
                   lower.endsWith(QStringLiteral(".png")) ||
                   lower.endsWith(QStringLiteral(".webp")) ||
                   lower.endsWith(QStringLiteral(".bmp")) ||
                   lower.endsWith(QStringLiteral(".gif"));

    if (isImage) {
        return MediaInfo{5.0, 1920, 1080, 1.0, 1, false, 0, 0, 0.0};
    }

    // Try real probe via QProcessVideoDecoder for video files
    {
        QProcessVideoDecoder probe;
        if (probe.open(filePath)) {
            double dur = probe.duration();
            double fps = probe.frameRate();
            int pw = probe.width();
            int ph = probe.height();
            probe.close();

            if (dur > 0 && pw > 0 && ph > 0) {
                return MediaInfo{
                    dur, pw, ph, fps,
                    static_cast<int>(dur * fps),
                    true, 48000, 2, dur
                };
            }
        }
    }

    // File-size heuristic fallback
    QFileInfo fi(filePath);
    if (fi.exists()) {
        double est = fi.size() * 8.0 / (8.0 * 1000.0 * 1000.0);
        est = std::clamp(est, 1.0, 360000.0);
        return MediaInfo{est,   1920,
                         1080,  30.0,
                         static_cast<int>(est * 30),
                         true,  48000,
                         2,     est};
    }

    return MediaInfo{10.0, 1920, 1080, 30.0, 300, true, 48000, 2, 10.0};
}

std::optional<MediaInfo> StubVideoTimelineEngine::probeMediaFast(
    const QString& filePath) {
    return probeMedia(filePath);
}

// =========================================================================
// Data-storing implementations for all extended methods
// =========================================================================

// ---- Audio per-clip ----
void StubVideoTimelineEngine::setClipVolume(int tl, int clipId, double vol) {
    checkTimeline(tl); clipAudioParams_[clipId].volume = vol;
}
double StubVideoTimelineEngine::getClipVolume(int tl, int clipId) {
    checkTimeline(tl);
    auto it = clipAudioParams_.find(clipId);
    return it != clipAudioParams_.end() ? it->second.volume : 1.0;
}
void StubVideoTimelineEngine::setClipPan(int tl, int clipId, double pan) {
    checkTimeline(tl); clipAudioParams_[clipId].pan = pan;
}
void StubVideoTimelineEngine::setClipFadeIn(int tl, int clipId, double sec) {
    checkTimeline(tl); clipAudioParams_[clipId].fadeIn = sec;
}
void StubVideoTimelineEngine::setClipFadeOut(int tl, int clipId, double sec) {
    checkTimeline(tl); clipAudioParams_[clipId].fadeOut = sec;
}

// ---- Audio per-track ----
void StubVideoTimelineEngine::setTrackVolume(int tl, int ti, double vol) {
    checkTimeline(tl); trackAudioParams_[ti].volume = vol;
}
void StubVideoTimelineEngine::setTrackPan(int tl, int ti, double pan) {
    checkTimeline(tl); trackAudioParams_[ti].pan = pan;
}
void StubVideoTimelineEngine::setTrackMute(int tl, int ti, bool m) {
    checkTimeline(tl); trackAudioParams_[ti].mute = m;
}
void StubVideoTimelineEngine::setTrackSolo(int tl, int ti, bool s) {
    checkTimeline(tl); trackAudioParams_[ti].solo = s;
}

// ---- Transitions ----
void StubVideoTimelineEngine::setClipTransitionIn(int tl, int clipId, int type, double dur, int easing) {
    checkTimeline(tl);
    clipTransitionIn_[clipId] = {type, dur, easing};
}
void StubVideoTimelineEngine::setClipTransitionOut(int tl, int clipId, int type, double dur, int easing) {
    checkTimeline(tl);
    clipTransitionOut_[clipId] = {type, dur, easing};
}

// ---- Keyframes ----
void StubVideoTimelineEngine::setClipKeyframe(int tl, int clipId, int prop, double time, double value, int interp) {
    checkTimeline(tl);
    auto& kfs = clipKeyframes_[clipId][prop];
    auto it = std::lower_bound(kfs.begin(), kfs.end(), time,
        [](const KeyframeData& k, double t) { return k.time < t; });
    if (it != kfs.end() && std::abs(it->time - time) < 0.0001)
        *it = {time, value, interp};
    else
        kfs.insert(it, {time, value, interp});
}
void StubVideoTimelineEngine::removeClipKeyframe(int tl, int clipId, int prop, double time) {
    checkTimeline(tl);
    auto cIt = clipKeyframes_.find(clipId);
    if (cIt == clipKeyframes_.end()) return;
    auto pIt = cIt->second.find(prop);
    if (pIt == cIt->second.end()) return;
    pIt->second.erase(
        std::remove_if(pIt->second.begin(), pIt->second.end(),
            [time](const KeyframeData& k) { return std::abs(k.time - time) < 0.0001; }),
        pIt->second.end());
}
void StubVideoTimelineEngine::clearClipKeyframes(int tl, int clipId, int prop) {
    checkTimeline(tl);
    auto cIt = clipKeyframes_.find(clipId);
    if (cIt != clipKeyframes_.end()) cIt->second.erase(prop);
}

// ---- Color grading ----
void StubVideoTimelineEngine::setClipColorGrading(int tl, int clipId,
    double brightness, double contrast, double saturation, double exposure,
    double temperature, double tint, double highlights, double shadows,
    double vibrance, double hue, double fade, double vignette) {
    checkTimeline(tl);
    colorGradingParams_[clipId] = {brightness, contrast, saturation,
                                   exposure, temperature, tint,
                                   highlights, shadows, vibrance, hue,
                                   fade, vignette};
}
void StubVideoTimelineEngine::clearClipEffects(int tl, int clipId) {
    checkTimeline(tl);
    colorGradingParams_.erase(clipId);
    clipEffects_.erase(clipId);
}

// ---- Effect instances ----
int StubVideoTimelineEngine::addClipEffect(int tl, int clipId, const QString& effectDefId) {
    checkTimeline(tl);
    int instanceId = nextEffectInstanceId_++;
    EffectData fx;
    fx.type = effectDefId.toInt();
    fx.instanceId = instanceId;
    clipEffects_[clipId].push_back(fx);
    return instanceId;
}
void StubVideoTimelineEngine::removeClipEffect(int tl, int clipId, int instanceId) {
    checkTimeline(tl);
    auto it = clipEffects_.find(clipId);
    if (it == clipEffects_.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(),
        [instanceId](const EffectData& e) { return e.instanceId == instanceId; }), vec.end());
}
void StubVideoTimelineEngine::reorderClipEffects(int tl, int, const std::vector<int>&) { checkTimeline(tl); }
void StubVideoTimelineEngine::setEffectParam(int tl, int clipId, int instanceId, const QString&, double value) {
    checkTimeline(tl);
    auto it = clipEffects_.find(clipId);
    if (it == clipEffects_.end()) return;
    for (auto& fx : it->second) {
        if (fx.instanceId == instanceId) { fx.value = value; return; }
    }
}
void StubVideoTimelineEngine::setEffectEnabled(int tl, int clipId, int instanceId, bool enabled) {
    checkTimeline(tl);
    auto it = clipEffects_.find(clipId);
    if (it == clipEffects_.end()) return;
    for (auto& fx : it->second) {
        if (fx.instanceId == instanceId) { fx.enabled = enabled; return; }
    }
}
void StubVideoTimelineEngine::setEffectMix(int tl, int clipId, int instanceId, double mix) {
    checkTimeline(tl);
    auto it = clipEffects_.find(clipId);
    if (it == clipEffects_.end()) return;
    for (auto& fx : it->second) {
        if (fx.instanceId == instanceId) { fx.mix = mix; return; }
    }
}
std::vector<EngineEffectDef> StubVideoTimelineEngine::listEffects(const QString&) { return {}; }

// ---- Text & shapes ----
void StubVideoTimelineEngine::setClipText(int tl, int clipId, const TextLayerData& td) {
    checkTimeline(tl); clipTextData_[clipId] = td;
}
int StubVideoTimelineEngine::addClipShape(int tl, int clipId, const ShapeData& sd) {
    checkTimeline(tl);
    int shapeId = nextId_++;
    clipShapes_[clipId].push_back({shapeId, sd});
    return shapeId;
}
void StubVideoTimelineEngine::updateClipShape(int tl, int clipId, int shapeId, const ShapeData& sd) {
    checkTimeline(tl);
    auto it = clipShapes_.find(clipId);
    if (it == clipShapes_.end()) return;
    for (auto& p : it->second) {
        if (p.first == shapeId) { p.second = sd; return; }
    }
}
void StubVideoTimelineEngine::removeClipShape(int tl, int clipId, int shapeId) {
    checkTimeline(tl);
    auto it = clipShapes_.find(clipId);
    if (it == clipShapes_.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(),
        [shapeId](const std::pair<int, ShapeData>& p) { return p.first == shapeId; }), vec.end());
}

// ---- Masks, tracking, stabilization (store-only, no rendering yet) ----
int StubVideoTimelineEngine::addClipMask(int tl, int, const MaskData&) { checkTimeline(tl); return nextId_++; }
void StubVideoTimelineEngine::updateClipMask(int tl, int, int, const MaskData&) { checkTimeline(tl); }
void StubVideoTimelineEngine::removeClipMask(int tl, int, int) { checkTimeline(tl); }
int StubVideoTimelineEngine::startTracking(int tl, int, double, double, double) { checkTimeline(tl); return 0; }
std::vector<TrackPoint> StubVideoTimelineEngine::getTrackingData(int tl, int) { checkTimeline(tl); return {}; }
void StubVideoTimelineEngine::stabilizeClip(int tl, int, const StabilizationConfig&) { checkTimeline(tl); }

// ---- Audio effects (store-only) ----
int StubVideoTimelineEngine::addAudioEffect(int tl, int, const QString&) { checkTimeline(tl); return nextId_++; }
void StubVideoTimelineEngine::setAudioEffectParam(int tl, int, int, const QString&, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::removeAudioEffect(int tl, int, int) { checkTimeline(tl); }
std::vector<AudioEffectDef> StubVideoTimelineEngine::listAudioEffects() { return {}; }

// ---- Export ----
int StubVideoTimelineEngine::startExport(int timelineId, const VideoExportConfig&) {
    checkTimeline(timelineId);
    int jobId = nextExportId_++;
    exportProgress_[jobId] = 0.0;
    exportProgress_[jobId] = 1.0; // Real export goes through FFmpegExportService
    return jobId;
}
double StubVideoTimelineEngine::getExportProgress(int exportJobId) {
    auto it = exportProgress_.find(exportJobId);
    return it != exportProgress_.end() ? it->second : -1.0;
}
void StubVideoTimelineEngine::cancelExport(int exportJobId) { exportProgress_.erase(exportJobId); }
bool StubVideoTimelineEngine::supportsHardwareEncoding() const { return false; }

// ---- AI, proxy, multicam (keep as stubs — require ML/external deps) ----
int StubVideoTimelineEngine::startAiSegmentation(int tl, int, const AiSegmentationConfig&) { checkTimeline(tl); return 0; }
double StubVideoTimelineEngine::getAiSegmentationProgress(int) { return -1.0; }
void StubVideoTimelineEngine::cancelAiSegmentation(int) {}
void StubVideoTimelineEngine::enableProxyMode(int tl, const ProxyConfig&) { checkTimeline(tl); }
void StubVideoTimelineEngine::disableProxyMode(int tl) { checkTimeline(tl); }
bool StubVideoTimelineEngine::isProxyModeActive(int tl) { checkTimeline(tl); return false; }
int StubVideoTimelineEngine::createMultiCamClip(int tl, int, const MultiCamConfig&) { checkTimeline(tl); return nextClipId_++; }
void StubVideoTimelineEngine::switchMultiCamAngle(int tl, int, int, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::flattenMultiCam(int tl, int) { checkTimeline(tl); }

// ---- Texture bridge (requires GPU) ----
int StubVideoTimelineEngine::createTextureBridge(int, int) { return -1; }
void StubVideoTimelineEngine::destroyTextureBridge() {}
bool StubVideoTimelineEngine::renderToTextureBridge(int) { return false; }
void StubVideoTimelineEngine::resizeTextureBridge(int, int) {}
std::optional<QByteArray> StubVideoTimelineEngine::getTextureBridgePixels() { return std::nullopt; }

// =========================================================================
// Keyframe evaluation & easing
// =========================================================================

double StubVideoTimelineEngine::applyEasing(double t, int easing) {
    t = std::clamp(t, 0.0, 1.0);
    switch (easing) {
    case 0: return t;                                    // Linear
    case 1: return t * t * (3.0 - 2.0 * t);             // Bezier (smoothstep)
    case 2: return t < 1.0 ? 0.0 : 1.0;                 // Hold
    case 3: return t * t;                                // EaseIn
    case 4: return 1.0 - (1.0 - t) * (1.0 - t);         // EaseOut
    case 5: return t < 0.5 ? 2*t*t : 1-std::pow(-2*t+2,2)/2; // EaseInOut
    default: return t;
    }
}

double StubVideoTimelineEngine::evaluateKeyframes(
    const std::vector<KeyframeData>& kfs, double time) {
    if (kfs.empty()) return 0.0;
    if (kfs.size() == 1) return kfs[0].value;
    if (time <= kfs.front().time) return kfs.front().value;
    if (time >= kfs.back().time) return kfs.back().value;
    for (size_t i = 0; i + 1 < kfs.size(); ++i) {
        if (time >= kfs[i].time && time <= kfs[i + 1].time) {
            double span = kfs[i + 1].time - kfs[i].time;
            double frac = span > 0.0001 ? (time - kfs[i].time) / span : 0.0;
            frac = applyEasing(frac, kfs[i].interpolation);
            return kfs[i].value + (kfs[i + 1].value - kfs[i].value) * frac;
        }
    }
    return kfs.back().value;
}

// =========================================================================
// Compositing helper — Porter-Duff src-over with blend mode
// =========================================================================

void StubVideoTimelineEngine::compositeFrame(
    QImage& dst, const QImage& src, double opacity, int blendMode) {
    if (opacity < 0.001) return;
    const int pixels = dst.width() * dst.height();
    uint8_t* d = dst.bits();
    const uint8_t* s = src.constBits();
    for (int i = 0; i < pixels; ++i) {
        double sa = (s[3] / 255.0) * opacity;
        if (sa < 0.002) { d += 4; s += 4; continue; }
        double sr = s[0], sg = s[1], sb = s[2];
        double dr = d[0], dg = d[1], db = d[2];
        double rr, rg, rb;
        switch (blendMode) {
        default:
        case 0: rr=sr; rg=sg; rb=sb; break; // Normal
        case 1: rr=sr*dr/255; rg=sg*dg/255; rb=sb*db/255; break; // Multiply
        case 2: rr=255-(255-sr)*(255-dr)/255; rg=255-(255-sg)*(255-dg)/255; rb=255-(255-sb)*(255-db)/255; break; // Screen
        case 3: // Overlay
            rr = dr<128 ? 2*sr*dr/255 : 255-2*(255-sr)*(255-dr)/255;
            rg = dg<128 ? 2*sg*dg/255 : 255-2*(255-sg)*(255-dg)/255;
            rb = db<128 ? 2*sb*db/255 : 255-2*(255-sb)*(255-db)/255;
            break;
        }
        double da = d[3] / 255.0;
        double outA = sa + da * (1.0 - sa);
        if (outA > 0.001) {
            d[0] = static_cast<uint8_t>((rr*sa + dr*da*(1-sa)) / outA);
            d[1] = static_cast<uint8_t>((rg*sa + dg*da*(1-sa)) / outA);
            d[2] = static_cast<uint8_t>((rb*sa + db*da*(1-sa)) / outA);
            d[3] = static_cast<uint8_t>(outA * 255);
        }
        d += 4; s += 4;
    }
}

// =========================================================================
// Effect pixel operations
// =========================================================================

void StubVideoTimelineEngine::applyBoxBlur(QImage& img, double radius) {
    if (radius < 0.5) return;
    int r = std::min(static_cast<int>(radius), 20);
    int w = img.width(), h = img.height();
    QImage tmp = img.copy();
    uint8_t* src = tmp.bits();
    uint8_t* dst = img.bits();
    // Horizontal pass
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int rr=0,gg=0,bb=0,aa=0,cnt=0;
            for (int k = -r; k <= r; ++k) {
                int sx = std::clamp(x+k, 0, w-1);
                int idx = (y*w+sx)*4;
                rr+=src[idx]; gg+=src[idx+1]; bb+=src[idx+2]; aa+=src[idx+3]; ++cnt;
            }
            int idx = (y*w+x)*4;
            dst[idx]=rr/cnt; dst[idx+1]=gg/cnt; dst[idx+2]=bb/cnt; dst[idx+3]=aa/cnt;
        }
    }
    // Vertical pass
    tmp = img.copy(); src = tmp.bits(); dst = img.bits();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int rr=0,gg=0,bb=0,aa=0,cnt=0;
            for (int k = -r; k <= r; ++k) {
                int sy = std::clamp(y+k, 0, h-1);
                int idx = (sy*w+x)*4;
                rr+=src[idx]; gg+=src[idx+1]; bb+=src[idx+2]; aa+=src[idx+3]; ++cnt;
            }
            int idx = (y*w+x)*4;
            dst[idx]=rr/cnt; dst[idx+1]=gg/cnt; dst[idx+2]=bb/cnt; dst[idx+3]=aa/cnt;
        }
    }
}

void StubVideoTimelineEngine::applyPixelate(QImage& img, int blockSize) {
    if (blockSize < 2) return;
    int w = img.width(), h = img.height();
    uint8_t* data = img.bits();
    for (int by = 0; by < h; by += blockSize) {
        for (int bx = 0; bx < w; bx += blockSize) {
            // Sample center pixel of block
            int cx = std::min(bx + blockSize/2, w-1);
            int cy = std::min(by + blockSize/2, h-1);
            int ci = (cy*w+cx)*4;
            uint8_t r=data[ci], g=data[ci+1], b=data[ci+2], a=data[ci+3];
            // Fill block
            for (int y = by; y < std::min(by+blockSize, h); ++y)
                for (int x = bx; x < std::min(bx+blockSize, w); ++x) {
                    int i = (y*w+x)*4;
                    data[i]=r; data[i+1]=g; data[i+2]=b; data[i+3]=a;
                }
        }
    }
}

void StubVideoTimelineEngine::applyGlitch(QImage& img, double amount, int seed) {
    if (amount < 1) return;
    int w = img.width(), h = img.height();
    uint8_t* data = img.bits();
    int shift = static_cast<int>(amount * 0.3);
    for (int y = 0; y < h; ++y) {
        int hash = (y * 7919 + seed * 104729) & 0xFFFF;
        if ((hash % 5) != 0) continue; // Only glitch ~20% of scanlines
        int offset = (hash % (shift * 2 + 1)) - shift;
        if (offset == 0) continue;
        // Shift scanline horizontally
        std::vector<uint8_t> row(w * 4);
        memcpy(row.data(), data + y * w * 4, w * 4);
        for (int x = 0; x < w; ++x) {
            int sx = std::clamp(x - offset, 0, w - 1);
            memcpy(data + (y*w+x)*4, row.data() + sx*4, 4);
        }
    }
}

void StubVideoTimelineEngine::applyChromatic(QImage& img, double amount) {
    if (amount < 1) return;
    int shift = std::min(static_cast<int>(amount * 0.2), 20);
    int w = img.width(), h = img.height();
    QImage orig = img.copy();
    const uint8_t* src = orig.constBits();
    uint8_t* dst = img.bits();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int i = (y*w+x)*4;
            int rxSrc = std::clamp(x - shift, 0, w-1);
            int bxSrc = std::clamp(x + shift, 0, w-1);
            dst[i]   = src[(y*w+rxSrc)*4];       // Red from left
            dst[i+1] = src[i+1];                  // Green stays
            dst[i+2] = src[(y*w+bxSrc)*4+2];     // Blue from right
        }
    }
}

void StubVideoTimelineEngine::applyVignette(QImage& img, double amount) {
    if (amount < 0.01) return;
    int w = img.width(), h = img.height();
    uint8_t* data = img.bits();
    double cx = w * 0.5, cy = h * 0.5;
    double maxDist = std::sqrt(cx*cx + cy*cy);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double dx = x - cx, dy = y - cy;
            double dist = std::sqrt(dx*dx + dy*dy) / maxDist;
            double factor = 1.0 - dist * dist * amount;
            factor = std::clamp(factor, 0.0, 1.0);
            int i = (y*w+x)*4;
            data[i]   = static_cast<uint8_t>(data[i]   * factor);
            data[i+1] = static_cast<uint8_t>(data[i+1] * factor);
            data[i+2] = static_cast<uint8_t>(data[i+2] * factor);
        }
    }
}

void StubVideoTimelineEngine::applyGrain(QImage& img, double amount, int seed) {
    if (amount < 0.01) return;
    int pixels = img.width() * img.height();
    uint8_t* data = img.bits();
    int strength = static_cast<int>(amount * 50);
    uint32_t rng = static_cast<uint32_t>(seed * 2654435761u);
    for (int i = 0; i < pixels; ++i) {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; // xorshift32
        int noise = static_cast<int>(rng % (strength * 2 + 1)) - strength;
        data[0] = static_cast<uint8_t>(std::clamp(data[0]+noise, 0, 255));
        data[1] = static_cast<uint8_t>(std::clamp(data[1]+noise, 0, 255));
        data[2] = static_cast<uint8_t>(std::clamp(data[2]+noise, 0, 255));
        data += 4;
    }
}

void StubVideoTimelineEngine::applySepia(QImage& img, double amount) {
    if (amount < 0.01) return;
    double a = std::clamp(amount, 0.0, 1.0);
    int pixels = img.width() * img.height();
    uint8_t* data = img.bits();
    for (int i = 0; i < pixels; ++i) {
        double r = data[0], g = data[1], b = data[2];
        double sr = std::min(255.0, 0.393*r + 0.769*g + 0.189*b);
        double sg = std::min(255.0, 0.349*r + 0.686*g + 0.168*b);
        double sb = std::min(255.0, 0.272*r + 0.534*g + 0.131*b);
        data[0] = static_cast<uint8_t>(r + (sr - r) * a);
        data[1] = static_cast<uint8_t>(g + (sg - g) * a);
        data[2] = static_cast<uint8_t>(b + (sb - b) * a);
        data += 4;
    }
}

void StubVideoTimelineEngine::applyInvert(QImage& img) {
    int pixels = img.width() * img.height();
    uint8_t* data = img.bits();
    for (int i = 0; i < pixels; ++i) {
        data[0] = 255 - data[0];
        data[1] = 255 - data[1];
        data[2] = 255 - data[2];
        data += 4;
    }
}

void StubVideoTimelineEngine::applyPosterize(QImage& img, int levels) {
    if (levels < 2) levels = 2;
    int step = 256 / levels;
    int pixels = img.width() * img.height();
    uint8_t* data = img.bits();
    for (int i = 0; i < pixels; ++i) {
        data[0] = static_cast<uint8_t>((data[0] / step) * step);
        data[1] = static_cast<uint8_t>((data[1] / step) * step);
        data[2] = static_cast<uint8_t>((data[2] / step) * step);
        data += 4;
    }
}

// =========================================================================
// Color grading (extended from basic LUT to full 10-param)
// =========================================================================

void StubVideoTimelineEngine::applyColorGrading(QImage& img, int clipId) {
    auto cgIt = colorGradingParams_.find(clipId);
    if (cgIt == colorGradingParams_.end() || cgIt->second.isDefault()) return;
    const auto& cg = cgIt->second;

    // Build per-channel LUT for brightness/contrast/exposure/highlights/shadows
    uint8_t lutR[256], lutG[256], lutB[256];
    for (int i = 0; i < 256; ++i) {
        double v = i / 255.0;
        // Exposure (EV stops)
        v *= std::pow(2.0, cg.exposure * 0.01);
        // Brightness
        v += cg.brightness * 0.01;
        // Contrast
        v = 0.5 + (v - 0.5) * (1.0 + cg.contrast * 0.01);
        // Highlights (boost values > 0.5)
        if (cg.highlights != 0) {
            double h = cg.highlights * 0.01;
            if (v > 0.5) v += (v - 0.5) * h;
        }
        // Shadows (boost values < 0.5)
        if (cg.shadows != 0) {
            double s = cg.shadows * 0.01;
            if (v < 0.5) v += (0.5 - v) * s * -1;
        }
        v = std::clamp(v, 0.0, 1.0);
        // Temperature: shift R and B
        double rv = v + cg.temperature * 0.002;
        double gv = v;
        double bv = v - cg.temperature * 0.002;
        // Tint: shift G
        gv += cg.tint * 0.002;
        lutR[i] = static_cast<uint8_t>(std::clamp(rv * 255.0, 0.0, 255.0));
        lutG[i] = static_cast<uint8_t>(std::clamp(gv * 255.0, 0.0, 255.0));
        lutB[i] = static_cast<uint8_t>(std::clamp(bv * 255.0, 0.0, 255.0));
    }

    const double satFactor = 1.0 + cg.saturation * 0.01;
    const double vibFactor = cg.vibrance * 0.01;
    const double hueRad = cg.hue * M_PI / 180.0;
    const bool doSat = cg.saturation != 0;
    const bool doVib = cg.vibrance != 0;
    const bool doHue = std::abs(cg.hue) > 0.1;

    const int pixelCount = img.width() * img.height();
    uint8_t* data = img.bits();
    for (int i = 0; i < pixelCount; ++i) {
        double r = lutR[data[0]];
        double g = lutG[data[1]];
        double b = lutB[data[2]];

        if (doSat || doVib) {
            double gray = 0.299 * r + 0.587 * g + 0.114 * b;
            double sf = satFactor;
            if (doVib) {
                // Vibrance: boost less-saturated colors more
                double maxC = std::max({r, g, b});
                double minC = std::min({r, g, b});
                double curSat = maxC > 0 ? (maxC - minC) / maxC : 0;
                sf += vibFactor * (1.0 - curSat);
            }
            r = gray + (r - gray) * sf;
            g = gray + (g - gray) * sf;
            b = gray + (b - gray) * sf;
        }

        if (doHue) {
            // RGB hue rotation via rotation matrix
            double cosH = std::cos(hueRad), sinH = std::sin(hueRad);
            double nr = r*(cosH + (1-cosH)/3.0) + g*((1-cosH)/3.0 - std::sqrt(1/3.0)*sinH) + b*((1-cosH)/3.0 + std::sqrt(1/3.0)*sinH);
            double ng = r*((1-cosH)/3.0 + std::sqrt(1/3.0)*sinH) + g*(cosH + (1-cosH)/3.0) + b*((1-cosH)/3.0 - std::sqrt(1/3.0)*sinH);
            double nb = r*((1-cosH)/3.0 - std::sqrt(1/3.0)*sinH) + g*((1-cosH)/3.0 + std::sqrt(1/3.0)*sinH) + b*(cosH + (1-cosH)/3.0);
            r = nr; g = ng; b = nb;
        }

        // Fade: lift blacks toward mid-gray
        if (cg.fade > 0) {
            double fadeLift = cg.fade * 0.01 * 0.3; // max 30% lift
            r = r + (128.0 - r) * fadeLift;
            g = g + (128.0 - g) * fadeLift;
            b = b + (128.0 - b) * fadeLift;
        }

        data[0] = static_cast<uint8_t>(std::clamp(r, 0.0, 255.0));
        data[1] = static_cast<uint8_t>(std::clamp(g, 0.0, 255.0));
        data[2] = static_cast<uint8_t>(std::clamp(b, 0.0, 255.0));
        data += 4;
    }

    // Vignette: darken edges with radial falloff
    if (cg.vignette > 0) {
        const double vigStrength = cg.vignette * 0.01;
        const int w = img.width(), h = img.height();
        const double cx = w * 0.5, cy = h * 0.5;
        const double maxDist = std::sqrt(cx * cx + cy * cy);
        uint8_t* bits = img.bits();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                double dx = x - cx, dy = y - cy;
                double dist = std::sqrt(dx * dx + dy * dy) / maxDist;
                double factor = 1.0 - vigStrength * dist * dist;
                uint8_t* px = bits + (y * w + x) * 4;
                px[0] = static_cast<uint8_t>(std::clamp(px[0] * factor, 0.0, 255.0));
                px[1] = static_cast<uint8_t>(std::clamp(px[1] * factor, 0.0, 255.0));
                px[2] = static_cast<uint8_t>(std::clamp(px[2] * factor, 0.0, 255.0));
            }
        }
    }
}

// =========================================================================
// Effect dispatch
// =========================================================================

void StubVideoTimelineEngine::applyEffects(QImage& img, int clipId) {
    auto it = clipEffects_.find(clipId);
    if (it == clipEffects_.end()) return;
    int frameSeed = static_cast<int>(positions_.begin()->second * 30) + clipId;
    for (const auto& fx : it->second) {
        if (!fx.enabled || fx.mix < 0.01) continue;
        double v = fx.value * fx.mix;
        switch (fx.type) {
        case 10: applyBoxBlur(img, std::clamp(v*10, 0.0, 200.0)); break;   // GaussianBlur
        case 13: { // Sharpen (unsharp mask: blur copy, add difference)
            QImage blurred = img.copy();
            applyBoxBlur(blurred, std::clamp(v*2, 1.0, 10.0));
            uint8_t* d=img.bits(); const uint8_t* b=blurred.constBits();
            int n=img.width()*img.height();
            double amt = std::clamp(v*2, 0.0, 8.0);
            for(int i=0;i<n*4;i+=4) {
                d[i]  =static_cast<uint8_t>(std::clamp(d[i]  +(d[i]  -b[i]  )*amt,0.0,255.0));
                d[i+1]=static_cast<uint8_t>(std::clamp(d[i+1]+(d[i+1]-b[i+1])*amt,0.0,255.0));
                d[i+2]=static_cast<uint8_t>(std::clamp(d[i+2]+(d[i+2]-b[i+2])*amt,0.0,255.0));
            }
            break;
        }
        case 14: applyPixelate(img, std::max(2, static_cast<int>(v))); break; // Pixelate
        case 15: applyGlitch(img, v, frameSeed); break;   // Glitch
        case 16: applyChromatic(img, v); break;            // Chromatic
        case 17: applyVignette(img, v * 0.01); break;     // Vignette
        case 18: applyGrain(img, v * 0.01, frameSeed); break; // Grain
        case 19: applySepia(img, v * 0.01); break;        // Sepia
        case 20: applyInvert(img); break;                  // Invert
        case 21: applyPosterize(img, std::max(2, static_cast<int>(v))); break; // Posterize
        default: break; // ColorTone types handled by applyColorGrading
        }
    }
}

// =========================================================================
// Transform via keyframes
// =========================================================================

void StubVideoTimelineEngine::applyTransform(
    QImage& img, int clipId, double localTime, int w, int h) {
    auto kfIt = clipKeyframes_.find(clipId);
    if (kfIt == clipKeyframes_.end()) return;
    const auto& kfs = kfIt->second;

    double posX = 0, posY = 0, scale = 1.0, rotation = 0;
    auto eval = [&](int prop, double def) {
        auto pIt = kfs.find(prop);
        return (pIt != kfs.end() && !pIt->second.empty())
            ? evaluateKeyframes(pIt->second, localTime) : def;
    };
    posX     = eval(0, 0);  // PositionX
    posY     = eval(1, 0);  // PositionY
    scale    = eval(2, 1);  // Scale
    rotation = eval(3, 0);  // Rotation

    if (posX == 0 && posY == 0 && scale == 1.0 && rotation == 0) return;

    QImage transformed(w, h, QImage::Format_RGBA8888);
    transformed.fill(Qt::transparent);
    QPainter tp(&transformed);
    tp.setRenderHint(QPainter::SmoothPixmapTransform);
    tp.translate(w / 2.0 + posX, h / 2.0 + posY);
    tp.rotate(rotation);
    tp.scale(scale, scale);
    tp.drawImage(QRect(-w/2, -h/2, w, h), img);
    tp.end();
    img = std::move(transformed);
}

// =========================================================================
// Text layer rendering
// =========================================================================

void StubVideoTimelineEngine::renderTextLayer(
    QImage& img, const TextLayerData& td, int w, int h) {
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    QFont font(td.fontFamily.isEmpty() ? QStringLiteral("Inter") : td.fontFamily);
    font.setPixelSize(static_cast<int>(td.fontSize * h / 1080.0)); // scale to preview
    if (td.fontStyle.contains(QStringLiteral("Bold"))) font.setBold(true);
    if (td.fontStyle.contains(QStringLiteral("Italic"))) font.setItalic(true);
    p.setFont(font);

    QColor fill(td.fillColor >> 16 & 0xFF, td.fillColor >> 8 & 0xFF,
                td.fillColor & 0xFF, td.fillColor >> 24 & 0xFF);

    double px = td.positionX * w;
    double py = td.positionY * h;

    p.translate(px, py);
    if (td.rotation != 0) p.rotate(td.rotation);
    if (td.scaleX != 1.0 || td.scaleY != 1.0) p.scale(td.scaleX, td.scaleY);

    int flags = Qt::AlignCenter;
    if (td.alignment == TextAlignment::Left) flags = Qt::AlignLeft | Qt::AlignVCenter;
    else if (td.alignment == TextAlignment::Right) flags = Qt::AlignRight | Qt::AlignVCenter;

    QRectF textRect(-w * 0.4, -h * 0.2, w * 0.8, h * 0.4);

    if (td.strokeEnabled && td.strokeWidth > 0) {
        QColor stroke(td.strokeColor >> 16 & 0xFF, td.strokeColor >> 8 & 0xFF,
                      td.strokeColor & 0xFF, td.strokeColor >> 24 & 0xFF);
        QPainterPath path;
        path.addText(textRect.topLeft() + QPointF(0, font.pixelSize()), font, td.text);
        p.strokePath(path, QPen(stroke, td.strokeWidth));
        if (td.fillEnabled) p.fillPath(path, fill);
    } else {
        p.setPen(td.fillEnabled ? fill : Qt::NoPen);
        p.drawText(textRect, flags | Qt::TextWordWrap, td.text);
    }
    p.end();
}

// =========================================================================
// Transition rendering
// =========================================================================

void StubVideoTimelineEngine::renderTransition(
    QImage& dst, const QImage& outgoing, const QImage& incoming,
    double progress, int type, int easing, int w, int h) {
    progress = applyEasing(std::clamp(progress, 0.0, 1.0), easing);
    QPainter p(&dst);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    switch (type) {
    case 1: case 2: // Fade / Dissolve
        p.setOpacity(1.0 - progress);
        p.drawImage(0, 0, outgoing);
        p.setOpacity(progress);
        p.drawImage(0, 0, incoming);
        break;
    case 3: // SlideLeft
        p.drawImage(QRect(static_cast<int>(-w * progress), 0, w, h), outgoing);
        p.drawImage(QRect(static_cast<int>(w * (1 - progress)), 0, w, h), incoming);
        break;
    case 4: // SlideRight
        p.drawImage(QRect(static_cast<int>(w * progress), 0, w, h), outgoing);
        p.drawImage(QRect(static_cast<int>(-w * (1 - progress)), 0, w, h), incoming);
        break;
    case 5: // SlideUp
        p.drawImage(QRect(0, static_cast<int>(-h * progress), w, h), outgoing);
        p.drawImage(QRect(0, static_cast<int>(h * (1 - progress)), w, h), incoming);
        break;
    case 6: // SlideDown
        p.drawImage(QRect(0, static_cast<int>(h * progress), w, h), outgoing);
        p.drawImage(QRect(0, static_cast<int>(-h * (1 - progress)), w, h), incoming);
        break;
    case 7: // WipeLeft
        p.drawImage(0, 0, outgoing);
        p.setClipRect(0, 0, static_cast<int>(w * progress), h);
        p.drawImage(0, 0, incoming);
        break;
    case 8: // WipeRight
        p.drawImage(0, 0, outgoing);
        p.setClipRect(static_cast<int>(w * (1 - progress)), 0, w, h);
        p.drawImage(0, 0, incoming);
        break;
    case 9: // WipeUp
        p.drawImage(0, 0, outgoing);
        p.setClipRect(0, 0, w, static_cast<int>(h * progress));
        p.drawImage(0, 0, incoming);
        break;
    case 10: // WipeDown
        p.drawImage(0, 0, outgoing);
        p.setClipRect(0, static_cast<int>(h * (1 - progress)), w, h);
        p.drawImage(0, 0, incoming);
        break;
    case 11: { // Zoom
        p.drawImage(0, 0, outgoing);
        double s = progress;
        p.setOpacity(progress);
        p.translate(w/2.0, h/2.0);
        p.scale(s, s);
        p.drawImage(QRect(-w/2, -h/2, w, h), incoming);
        break;
    }
    default: // Fallback: dissolve
        p.setOpacity(1.0 - progress);
        p.drawImage(0, 0, outgoing);
        p.setOpacity(progress);
        p.drawImage(0, 0, incoming);
        break;
    }
    p.end();
}

} // namespace gopost::rendering
