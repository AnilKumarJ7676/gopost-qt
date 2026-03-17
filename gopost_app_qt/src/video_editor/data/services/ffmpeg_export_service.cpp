#include "video_editor/data/services/ffmpeg_export_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace gopost::video_editor {

FFmpegExportService::FFmpegExportService(std::shared_ptr<FfmpegRunner> ffmpeg)
    : ffmpeg_(ffmpeg ? std::move(ffmpeg) : FfmpegRunner::create()) {}

void FFmpegExportService::exportProject(
    const VideoProject& project,
    const ExportPreset& preset,
    const QString& outputPath,
    std::function<void(double percent)> onProgress)
{
    const auto clips = sortedVideoClips(project);
    if (clips.isEmpty())
        throw std::runtime_error("No video clips to export");

    const double totalDuration = outputDuration(clips);
    if (totalDuration <= 0)
        throw std::runtime_error("Project has zero duration");

    if (clips.size() == 1) {
        exportSingleClip(clips.first(), preset, outputPath, totalDuration, onProgress);
    } else {
        exportMultiClip(clips, preset, outputPath, totalDuration, onProgress);
    }

    QFileInfo outInfo(outputPath);
    if (!outInfo.exists() || outInfo.size() == 0)
        throw std::runtime_error("Export produced an empty file");
}

// ── Single-clip export ──────────────────────────────────────────────────────

void FFmpegExportService::exportSingleClip(
    const VideoClip& clip, const ExportPreset& preset,
    const QString& outputPath, double totalDuration,
    std::function<void(double)> onProgress)
{
    const auto cmd = buildClipCommand(clip, preset, outputPath);
    runFFmpeg(cmd, totalDuration, onProgress);
}

// ── Multi-clip export ───────────────────────────────────────────────────────

void FFmpegExportService::exportMultiClip(
    const QList<VideoClip>& clips, const ExportPreset& preset,
    const QString& outputPath, double totalDuration,
    std::function<void(double)> onProgress)
{
    const auto tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const auto sessionDir = QStringLiteral("%1/gopost_export_%2")
        .arg(tmpDir).arg(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(sessionDir);

    QStringList segmentPaths;
    double completedDuration = 0;

    try {
        for (int i = 0; i < clips.size(); ++i) {
            const auto& clip = clips[i];
            const auto segPath = QStringLiteral("%1/seg_%2.mp4").arg(sessionDir).arg(i);
            segmentPaths.append(segPath);
            const double clipDur = clipOutputDuration(clip);

            const auto cmd = buildClipCommand(clip, preset, segPath);
            runFFmpeg(cmd, clipDur, [&](double p) {
                const double overall =
                    (completedDuration + clipDur * (p / 100.0)) / totalDuration * 100.0;
                onProgress(std::clamp(overall, 0.0, 99.0));
            });

            completedDuration += clipDur;
            onProgress(std::clamp(completedDuration / totalDuration * 100.0, 0.0, 99.0));
        }

        concatSegments(segmentPaths, outputPath);
        onProgress(100.0);
    } catch (...) {
        QDir(sessionDir).removeRecursively();
        throw;
    }
    QDir(sessionDir).removeRecursively();
}

void FFmpegExportService::concatSegments(const QStringList& segmentPaths, const QString& outputPath) {
    const auto segDir = QFileInfo(segmentPaths.first()).path();
    const auto listFilePath = QStringLiteral("%1/concat.txt").arg(segDir);

    QFile listFile(listFilePath);
    if (listFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        for (const auto& p : segmentPaths) {
            auto normalized = p;
            normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
            listFile.write(QStringLiteral("file '%1'\n").arg(normalized).toUtf8());
        }
        listFile.close();
    }

    const auto cmd = QStringLiteral(R"(-y -f concat -safe 0 -i "%1" -c copy "%2")")
        .arg(listFilePath, outputPath);
    const auto result = ffmpeg_->execute(cmd);
    if (!result.success)
        throw std::runtime_error(
            QStringLiteral("Concat failed: %1").arg(result.output).toStdString());
}

// ── FFmpeg command builder ──────────────────────────────────────────────────

QString FFmpegExportService::buildClipCommand(
    const VideoClip& clip, const ExportPreset& preset, const QString& outputPath) const
{
    QString cmd = QStringLiteral("-y ");

    const double trimStart = clip.sourceIn;
    const double trimDuration = clip.sourceOut - clip.sourceIn;
    cmd += QStringLiteral("-ss %1 ").arg(trimStart, 0, 'f', 4);
    cmd += QStringLiteral("-t %1 ").arg(trimDuration, 0, 'f', 4);
    cmd += QStringLiteral(R"(-i "%1" )").arg(clip.sourcePath);

    const auto vf = videoFilterChain(clip, preset);
    if (!vf.isEmpty()) cmd += QStringLiteral(R"(-vf "%1" )").arg(vf);

    const auto af = audioFilterChain(clip);
    if (!af.isEmpty()) cmd += QStringLiteral(R"(-af "%1" )").arg(af);

    cmd += QStringLiteral("-c:v %1 ").arg(videoEncoder(preset.videoCodec));
    cmd += QStringLiteral("-b:v %1M ").arg(preset.videoBitrateMbps);
    cmd += QStringLiteral("-preset medium ");
    cmd += QStringLiteral("-c:a %1 ").arg(audioEncoder(preset.audioCodec));
    cmd += QStringLiteral("-b:a %1k ").arg(preset.audioBitrateKbps);
    cmd += QStringLiteral("-r %1 ").arg(static_cast<int>(preset.frameRate));
    cmd += QStringLiteral("-pix_fmt yuv420p ");
    cmd += QStringLiteral(R"("%1")").arg(outputPath);

    return cmd;
}

// ── Video filter chain ──────────────────────────────────────────────────────

QString FFmpegExportService::videoFilterChain(const VideoClip& clip, const ExportPreset& preset) const {
    QStringList filters;

    if (clip.speed != 1.0)
        filters.append(QStringLiteral("setpts=PTS/%1").arg(clip.speed, 0, 'f', 4));

    filters.append(QStringLiteral("scale=%1:%2").arg(preset.width).arg(preset.height));

    const auto eq = eqFilter(clip);
    if (!eq.isEmpty()) filters.append(eq);

    if (clip.colorGrading.hue != 0)
        filters.append(QStringLiteral("hue=h=%1").arg(clip.colorGrading.hue, 0, 'f', 2));

    for (const auto& effect : clip.effects) {
        if (!effect.enabled) continue;
        const auto f = effectFilter(effect);
        if (!f.isEmpty()) filters.append(f);
    }

    if (clip.opacity < 1.0) {
        const double alpha = std::clamp(clip.opacity, 0.0, 1.0);
        filters.append(QStringLiteral("colorchannelmixer=aa=%1").arg(alpha, 0, 'f', 3));
    }

    return filters.join(QLatin1Char(','));
}

QString FFmpegExportService::eqFilter(const VideoClip& clip) const {
    const auto& cg = clip.colorGrading;
    QStringList parts;

    if (cg.brightness != 0)
        parts.append(QStringLiteral("brightness=%1").arg(cg.brightness / 100.0, 0, 'f', 4));
    if (cg.contrast != 0)
        parts.append(QStringLiteral("contrast=%1").arg(1.0 + cg.contrast / 100.0, 0, 'f', 4));
    if (cg.saturation != 0)
        parts.append(QStringLiteral("saturation=%1").arg(1.0 + cg.saturation / 100.0, 0, 'f', 4));
    if (cg.exposure != 0)
        parts.append(QStringLiteral("gamma=%1").arg(std::pow(2.0, cg.exposure), 0, 'f', 4));

    if (parts.isEmpty()) return {};
    return QStringLiteral("eq=%1").arg(parts.join(QLatin1Char(':')));
}

QString FFmpegExportService::effectFilter(const VideoEffect& effect) const {
    const double v = effect.value;
    const double mix = effect.mix;
    const auto info = effectTypeInfo(effect.type);
    if (v == info.defaultValue) return {};

    switch (effect.type) {
        case EffectType::Brightness:
            return QStringLiteral("eq=brightness=%1").arg(v / 100.0 * mix, 0, 'f', 4);
        case EffectType::Contrast:
            return QStringLiteral("eq=contrast=%1").arg(1.0 + v / 100.0 * mix, 0, 'f', 4);
        case EffectType::Saturation:
            return QStringLiteral("eq=saturation=%1").arg(1.0 + v / 100.0 * mix, 0, 'f', 4);
        case EffectType::Exposure:
            return QStringLiteral("eq=gamma=%1").arg(std::pow(2.0, v * mix), 0, 'f', 4);
        case EffectType::HueRotate:
            return QStringLiteral("hue=h=%1").arg(v * mix, 0, 'f', 2);
        case EffectType::GaussianBlur: {
            const double radius = std::clamp(v / 10.0 * mix, 0.0, 20.0);
            return QStringLiteral("boxblur=%1:%1").arg(radius, 0, 'f', 1);
        }
        case EffectType::Sharpen: {
            const double amount = std::clamp(v / 25.0 * mix, 0.0, 10.0);
            return QStringLiteral("unsharp=5:5:%1").arg(amount, 0, 'f', 2);
        }
        case EffectType::Vignette: {
            if (v <= 0) return {};
            const double angle = M_PI / 4.0 * (v / 100.0) * mix;
            return QStringLiteral("vignette=a=%1").arg(angle, 0, 'f', 4);
        }
        case EffectType::Sepia: {
            if (v <= 0) return {};
            const double s = std::clamp(v / 100.0 * mix, 0.0, 1.0);
            return QStringLiteral("colorchannelmixer="
                "%1:%2:%3:0:%4:%5:%6:0:%7:%8:%9:0")
                .arg(0.393 * s + (1.0 - s), 0, 'f', 3)
                .arg(0.769 * s, 0, 'f', 3)
                .arg(0.189 * s, 0, 'f', 3)
                .arg(0.349 * s, 0, 'f', 3)
                .arg(0.686 * s + (1.0 - s), 0, 'f', 3)
                .arg(0.168 * s, 0, 'f', 3)
                .arg(0.272 * s, 0, 'f', 3)
                .arg(0.534 * s, 0, 'f', 3)
                .arg(0.131 * s + (1.0 - s), 0, 'f', 3);
        }
        case EffectType::Invert:
            if (v <= 0) return {};
            return QStringLiteral("negate");
        case EffectType::Grain: {
            if (v <= 0) return {};
            const int strength = static_cast<int>(std::clamp(v * mix / 2.0, 0.0, 50.0));
            return QStringLiteral("noise=alls=%1:allf=t+u").arg(strength);
        }
        case EffectType::Pixelate: {
            const int block = static_cast<int>(std::clamp(v * mix, 1.0, 50.0));
            const double s = 1.0 / block;
            return QStringLiteral("scale=iw*%1:ih*%1,scale=iw*%2:ih*%2:flags=neighbor")
                .arg(s, 0, 'f', 6).arg(block);
        }
        // Unsupported in FFmpeg filter chain
        case EffectType::Temperature:
        case EffectType::Tint:
        case EffectType::Highlights:
        case EffectType::Shadows:
        case EffectType::Vibrance:
        case EffectType::RadialBlur:
        case EffectType::TiltShift:
        case EffectType::Glitch:
        case EffectType::Chromatic:
        case EffectType::Posterize:
            return {};
    }
    return {};
}

// ── Audio filter chain ──────────────────────────────────────────────────────

QString FFmpegExportService::audioFilterChain(const VideoClip& clip) const {
    const auto& audio = clip.audio;

    if (audio.isMuted) return QStringLiteral("volume=0");

    QStringList filters;

    if (clip.speed != 1.0)
        filters.append(atempoChain(clip.speed));

    if (audio.volume != 1.0)
        filters.append(QStringLiteral("volume=%1").arg(audio.volume, 0, 'f', 3));

    const double clipDur = clipOutputDuration(clip);
    if (audio.fadeInSeconds > 0)
        filters.append(QStringLiteral("afade=t=in:d=%1").arg(audio.fadeInSeconds, 0, 'f', 3));
    if (audio.fadeOutSeconds > 0 && clipDur > audio.fadeOutSeconds) {
        const double st = clipDur - audio.fadeOutSeconds;
        filters.append(QStringLiteral("afade=t=out:st=%1:d=%2")
            .arg(st, 0, 'f', 3).arg(audio.fadeOutSeconds, 0, 'f', 3));
    }

    return filters.join(QLatin1Char(','));
}

QString FFmpegExportService::atempoChain(double speed) {
    if (speed >= 0.5 && speed <= 100.0)
        return QStringLiteral("atempo=%1").arg(speed, 0, 'f', 4);

    QStringList parts;
    double remaining = speed;
    if (speed < 0.5) {
        while (remaining < 0.5) {
            parts.append(QStringLiteral("atempo=0.5"));
            remaining /= 0.5;
        }
        parts.append(QStringLiteral("atempo=%1").arg(remaining, 0, 'f', 4));
    } else {
        while (remaining > 100.0) {
            parts.append(QStringLiteral("atempo=100.0"));
            remaining /= 100.0;
        }
        parts.append(QStringLiteral("atempo=%1").arg(remaining, 0, 'f', 4));
    }
    return parts.join(QLatin1Char(','));
}

// ── FFmpeg execution with progress ──────────────────────────────────────────

void FFmpegExportService::runFFmpeg(
    const QString& command, double expectedDurationSec,
    std::function<void(double)> onProgress)
{
    const int expectedMs = static_cast<int>(expectedDurationSec * 1000.0);

    ffmpeg_->onStatistics([&](int timeMs) {
        if (expectedMs > 0 && timeMs > 0) {
            const double pct = std::clamp(
                static_cast<double>(timeMs) / expectedMs * 100.0, 0.0, 99.0);
            onProgress(pct);
        }
    });

    const auto result = ffmpeg_->execute(command);

    if (!result.success) {
        throw std::runtime_error(
            QStringLiteral("FFmpeg failed (rc=%1): %2")
                .arg(result.returnCode).arg(result.output).toStdString());
    }

    onProgress(100.0);
}

void FFmpegExportService::cancel() {
    ffmpeg_->cancel();
}

// ── Helpers ─────────────────────────────────────────────────────────────────

QString FFmpegExportService::videoEncoder(VideoCodec codec) {
    switch (codec) {
        case VideoCodec::H264: return QStringLiteral("libx264");
        case VideoCodec::H265: return QStringLiteral("libx265");
        case VideoCodec::VP9:  return QStringLiteral("libvpx-vp9");
    }
    return QStringLiteral("libx264");
}

QString FFmpegExportService::audioEncoder(AudioCodec codec) {
    switch (codec) {
        case AudioCodec::AAC:  return QStringLiteral("aac");
        case AudioCodec::Opus: return QStringLiteral("libopus");
    }
    return QStringLiteral("aac");
}

QList<VideoClip> FFmpegExportService::sortedVideoClips(const VideoProject& project) {
    QList<VideoClip> clips;
    for (const auto& track : project.tracks) {
        for (const auto& clip : track.clips) {
            if (clip.sourceType == ClipSourceType::Video && !clip.sourcePath.isEmpty())
                clips.append(clip);
        }
    }
    std::sort(clips.begin(), clips.end(),
        [](const VideoClip& a, const VideoClip& b) { return a.timelineIn < b.timelineIn; });
    return clips;
}

double FFmpegExportService::clipOutputDuration(const VideoClip& clip) {
    return (clip.sourceOut - clip.sourceIn) / clip.speed;
}

double FFmpegExportService::outputDuration(const QList<VideoClip>& clips) {
    double total = 0.0;
    for (const auto& c : clips) total += clipOutputDuration(c);
    return total;
}

} // namespace gopost::video_editor
