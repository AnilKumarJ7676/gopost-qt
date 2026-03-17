#include "rendering_bridge/stub_video_timeline_engine.h"

#include <QDebug>
#include <QFileInfo>
#include <QPainter>
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
        auto dt = std::make_unique<RenderDecodeThread>(8);
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
                double progress = (positionSeconds - clip.timelineRange.inTime) / clipDuration;
                double sourceDuration = clip.sourceRange.sourceOut - clip.sourceRange.sourceIn;
                double sourceTime = clip.sourceRange.sourceIn + progress * sourceDuration;

                // Only seek if source time changed significantly from last seek
                auto lastIt = lastSeekTime_.find(clip.id);
                if (lastIt == lastSeekTime_.end() || std::abs(sourceTime - lastIt->second) > 0.3) {
                    decIt->second->seekTo(sourceTime);
                    lastSeekTime_[clip.id] = sourceTime;
                }
            }
        }
    }
}

std::optional<DecodedImage> StubVideoTimelineEngine::renderFrame(
    int timelineId) {
    checkTimeline(timelineId);

    // Get timeline config for dimensions
    auto cfg = configs_[timelineId];
    int w = cfg.width  > 0 ? cfg.width  : 1920;
    int h = cfg.height > 0 ? cfg.height : 1080;
    double pos = positions_[timelineId];

    // Find clips visible at current position
    auto trackIt = tracks_.find(timelineId);
    if (trackIt == tracks_.end() || trackIt->second.empty()) {
        return std::nullopt;  // no tracks → no frame
    }

    // Collect visible clips across all tracks
    struct VisibleClip {
        const StubClip* clip;
        int trackIdx;
    };
    std::vector<VisibleClip> visible;
    for (int ti = 0; ti < static_cast<int>(trackIt->second.size()); ++ti) {
        for (const auto& clip : trackIt->second[ti].clips) {
            if (pos >= clip.timelineRange.inTime - 0.001 &&
                pos < clip.timelineRange.outTime + 0.001) {
                visible.push_back({&clip, ti});
            }
        }
    }

    if (visible.empty()) {
        // Position is outside all clips — render black frame
        QImage img(w, h, QImage::Format_RGBA8888);
        img.fill(QColor(13, 13, 26));  // #0D0D1A

        DecodedImage decoded;
        decoded.width  = w;
        decoded.height = h;
        decoded.pixels = QByteArray(reinterpret_cast<const char*>(img.constBits()),
                                    img.sizeInBytes());
        return decoded;
    }

    // Render a composited preview frame
    QImage img(w, h, QImage::Format_RGBA8888);
    img.fill(QColor(13, 13, 26));

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw from bottom track (highest index) to top track (lowest index)
    for (auto it = visible.rbegin(); it != visible.rend(); ++it) {
        const auto* clip = it->clip;
        double clipProgress = (pos - clip->timelineRange.inTime)
                            / std::max(0.001, clip->timelineRange.outTime - clip->timelineRange.inTime);
        clipProgress = std::clamp(clipProgress, 0.0, 1.0);

        // Try to render actual decoded video frame for video clips
        auto decIt = decoders_.find(clip->id);
        if (decIt != decoders_.end() && decIt->second->isOpen()) {
            int buffered = decIt->second->bufferedFrameCount();
            // Pop available frame from decode buffer
            auto decodedFrame = decIt->second->popFrame();
            if (decodedFrame.has_value()) {
                // Log only the first decoded frame per clip to avoid spam
                if (cachedFrames_.find(clip->id) == cachedFrames_.end()) {
                    qDebug() << "[StubEngine] renderFrame: first decoded frame for clip" << clip->id
                             << "ts:" << decodedFrame->timestamp_seconds
                             << "size:" << decodedFrame->width << "x" << decodedFrame->height;
                }
                cachedFrames_[clip->id] = CachedFrame{std::move(*decodedFrame), pos};
            }

            // Use cached frame if available
            auto cacheIt = cachedFrames_.find(clip->id);
            if (cacheIt != cachedFrames_.end() && !cacheIt->second.frame.pixels.empty()) {
                const auto& cf = cacheIt->second.frame;
                QImage frameImg(cf.pixels.data(), cf.width, cf.height,
                                cf.width * 4, QImage::Format_RGBA8888);

                // Scale decoded frame to output dimensions
                painter.drawImage(QRect(0, 0, w, h), frameImg);

                // Overlay timecode
                painter.setPen(QColor(255, 255, 255, 200));
                QFont tcFont("monospace", 12);
                painter.setFont(tcFont);
                int mins = static_cast<int>(pos) / 60;
                int secs = static_cast<int>(pos) % 60;
                int frames = static_cast<int>((pos - std::floor(pos)) * 30);
                QString timecode = QString("%1:%2:%3")
                    .arg(mins, 2, 10, QChar('0'))
                    .arg(secs, 2, 10, QChar('0'))
                    .arg(frames, 2, 10, QChar('0'));
                painter.drawText(QRect(w - 120, h - 30, 110, 25),
                                 Qt::AlignRight | Qt::AlignVCenter, timecode);

                break;  // rendered actual frame
            }
        }

        // Fallback: colored placeholder for non-video clips or when no frame decoded yet
        QColor clipColor;
        switch (clip->sourceType) {
        case VideoClipSourceType::Video:  clipColor = QColor(38, 198, 218); break; // #26C6DA
        case VideoClipSourceType::Image:  clipColor = QColor(255, 112, 67); break; // #FF7043
        case VideoClipSourceType::Title:  clipColor = QColor(171, 71, 188); break; // #AB47BC
        case VideoClipSourceType::Color:  clipColor = QColor(102, 187, 106); break; // #66BB6A
        default:                          clipColor = QColor(108, 99, 255);  break; // #6C63FF
        }

        // Fill background with clip's colour (dimmed)
        painter.fillRect(0, 0, w, h, QColor(clipColor.red() / 4, clipColor.green() / 4,
                                             clipColor.blue() / 4));

        // Draw a gradient bar across the frame to show clip progress
        int barY = h / 2 - 40;
        int barH = 80;
        painter.fillRect(0, barY, static_cast<int>(w * clipProgress), barH, clipColor);
        painter.fillRect(static_cast<int>(w * clipProgress), barY,
                         w - static_cast<int>(w * clipProgress), barH,
                         QColor(clipColor.red() / 2, clipColor.green() / 2, clipColor.blue() / 2));

        // Draw clip name and timecode
        painter.setPen(Qt::white);
        QFont font("sans-serif", 16);
        font.setBold(true);
        painter.setFont(font);

        QString sourceLabel;
        switch (clip->sourceType) {
        case VideoClipSourceType::Video:  sourceLabel = "VIDEO"; break;
        case VideoClipSourceType::Image:  sourceLabel = "IMAGE"; break;
        case VideoClipSourceType::Title:  sourceLabel = "TITLE"; break;
        case VideoClipSourceType::Color:  sourceLabel = "COLOR"; break;
        default:                          sourceLabel = "CLIP";  break;
        }

        // Source file name
        QString fileName = clip->sourcePath;
        if (!fileName.isEmpty()) {
            int lastSlash = fileName.lastIndexOf('/');
            if (lastSlash < 0) lastSlash = fileName.lastIndexOf('\\');
            if (lastSlash >= 0) fileName = fileName.mid(lastSlash + 1);
        }

        painter.drawText(QRect(0, barY - 60, w, 50), Qt::AlignCenter,
                         sourceLabel + ": " + fileName);

        // Timecode
        int mins = static_cast<int>(pos) / 60;
        int secs = static_cast<int>(pos) % 60;
        int frames = static_cast<int>((pos - std::floor(pos)) * 30);
        QString timecode = QString("%1:%2:%3")
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'))
            .arg(frames, 2, 10, QChar('0'));

        QFont tcFont("monospace", 14);
        painter.setFont(tcFont);
        painter.drawText(QRect(0, barY + barH + 10, w, 40), Qt::AlignCenter, timecode);

        break;  // only render topmost visible clip for now
    }

    painter.end();

    DecodedImage decoded;
    decoded.width  = w;
    decoded.height = h;
    decoded.pixels = QByteArray(reinterpret_cast<const char*>(img.constBits()),
                                img.sizeInBytes());
    return decoded;
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
// Stubs for all remaining VideoTimelineEngine extended methods
// =========================================================================

void StubVideoTimelineEngine::setClipVolume(int tl, int, double) { checkTimeline(tl); }
double StubVideoTimelineEngine::getClipVolume(int tl, int) { checkTimeline(tl); return 1.0; }
void StubVideoTimelineEngine::setClipTransitionIn(int tl, int, int, double, int) { checkTimeline(tl); }
void StubVideoTimelineEngine::setClipTransitionOut(int tl, int, int, double, int) { checkTimeline(tl); }
void StubVideoTimelineEngine::setClipKeyframe(int tl, int, int, double, double, int) { checkTimeline(tl); }
void StubVideoTimelineEngine::removeClipKeyframe(int tl, int, int, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::clearClipKeyframes(int tl, int, int) { checkTimeline(tl); }
void StubVideoTimelineEngine::setClipColorGrading(int tl, int, double, double, double, double, double, double, double, double, double, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::clearClipEffects(int tl, int) { checkTimeline(tl); }
void StubVideoTimelineEngine::setClipPan(int tl, int, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::setClipFadeIn(int tl, int, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::setClipFadeOut(int tl, int, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::setTrackVolume(int tl, int, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::setTrackPan(int tl, int, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::setTrackMute(int tl, int, bool) { checkTimeline(tl); }
void StubVideoTimelineEngine::setTrackSolo(int tl, int, bool) { checkTimeline(tl); }

int StubVideoTimelineEngine::startExport(int timelineId, const VideoExportConfig&) {
    checkTimeline(timelineId);
    int jobId = nextExportId_++;
    exportProgress_[jobId] = 0.0;
    // Simulate completion
    exportProgress_[jobId] = 1.0;
    return jobId;
}

double StubVideoTimelineEngine::getExportProgress(int exportJobId) {
    auto it = exportProgress_.find(exportJobId);
    return it != exportProgress_.end() ? it->second : -1.0;
}

void StubVideoTimelineEngine::cancelExport(int exportJobId) {
    exportProgress_.erase(exportJobId);
}

bool StubVideoTimelineEngine::supportsHardwareEncoding() const { return false; }

std::vector<EngineEffectDef> StubVideoTimelineEngine::listEffects(const QString&) { return {}; }
int StubVideoTimelineEngine::addClipEffect(int tl, int, const QString&) { checkTimeline(tl); return 0; }
void StubVideoTimelineEngine::removeClipEffect(int tl, int, int) { checkTimeline(tl); }
void StubVideoTimelineEngine::reorderClipEffects(int tl, int, const std::vector<int>&) { checkTimeline(tl); }
void StubVideoTimelineEngine::setEffectParam(int tl, int, int, const QString&, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::setEffectEnabled(int tl, int, int, bool) { checkTimeline(tl); }
void StubVideoTimelineEngine::setEffectMix(int tl, int, int, double) { checkTimeline(tl); }

int StubVideoTimelineEngine::addClipMask(int tl, int, const MaskData&) { checkTimeline(tl); return 0; }
void StubVideoTimelineEngine::updateClipMask(int tl, int, int, const MaskData&) { checkTimeline(tl); }
void StubVideoTimelineEngine::removeClipMask(int tl, int, int) { checkTimeline(tl); }
int StubVideoTimelineEngine::startTracking(int tl, int, double, double, double) { checkTimeline(tl); return 0; }
std::vector<TrackPoint> StubVideoTimelineEngine::getTrackingData(int tl, int) { checkTimeline(tl); return {}; }
void StubVideoTimelineEngine::stabilizeClip(int tl, int, const StabilizationConfig&) { checkTimeline(tl); }

void StubVideoTimelineEngine::setClipText(int tl, int, const TextLayerData&) { checkTimeline(tl); }
int StubVideoTimelineEngine::addClipShape(int tl, int, const ShapeData&) { checkTimeline(tl); return 0; }
void StubVideoTimelineEngine::updateClipShape(int tl, int, int, const ShapeData&) { checkTimeline(tl); }
void StubVideoTimelineEngine::removeClipShape(int tl, int, int) { checkTimeline(tl); }
int StubVideoTimelineEngine::addAudioEffect(int tl, int, const QString&) { checkTimeline(tl); return 0; }
void StubVideoTimelineEngine::setAudioEffectParam(int tl, int, int, const QString&, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::removeAudioEffect(int tl, int, int) { checkTimeline(tl); }
std::vector<AudioEffectDef> StubVideoTimelineEngine::listAudioEffects() { return {}; }

int StubVideoTimelineEngine::startAiSegmentation(int tl, int, const AiSegmentationConfig&) { checkTimeline(tl); return 0; }
double StubVideoTimelineEngine::getAiSegmentationProgress(int) { return -1.0; }
void StubVideoTimelineEngine::cancelAiSegmentation(int) {}
void StubVideoTimelineEngine::enableProxyMode(int tl, const ProxyConfig&) { checkTimeline(tl); }
void StubVideoTimelineEngine::disableProxyMode(int tl) { checkTimeline(tl); }
bool StubVideoTimelineEngine::isProxyModeActive(int tl) { checkTimeline(tl); return false; }
int StubVideoTimelineEngine::createMultiCamClip(int tl, int, const MultiCamConfig&) { checkTimeline(tl); return nextClipId_++; }
void StubVideoTimelineEngine::switchMultiCamAngle(int tl, int, int, double) { checkTimeline(tl); }
void StubVideoTimelineEngine::flattenMultiCam(int tl, int) { checkTimeline(tl); }

int StubVideoTimelineEngine::createTextureBridge(int, int) { return -1; }
void StubVideoTimelineEngine::destroyTextureBridge() {}
bool StubVideoTimelineEngine::renderToTextureBridge(int) { return false; }
void StubVideoTimelineEngine::resizeTextureBridge(int, int) {}
std::optional<QByteArray> StubVideoTimelineEngine::getTextureBridgePixels() { return std::nullopt; }

} // namespace gopost::rendering
