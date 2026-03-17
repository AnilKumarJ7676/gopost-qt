#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>
#include <chrono>
#include <optional>
#include <cstdint>

namespace gopost::video_editor {

// ── VideoCodec ──────────────────────────────────────────────────────────────

enum class VideoCodec : int {
    H264 = 0,
    H265,
    VP9
};

inline QString videoCodecLabel(VideoCodec c) {
    switch (c) {
        case VideoCodec::H264: return QStringLiteral("H.264");
        case VideoCodec::H265: return QStringLiteral("H.265 / HEVC");
        case VideoCodec::VP9:  return QStringLiteral("VP9");
    }
    return {};
}

inline QString videoCodecFourcc(VideoCodec c) {
    switch (c) {
        case VideoCodec::H264: return QStringLiteral("avc1");
        case VideoCodec::H265: return QStringLiteral("hvc1");
        case VideoCodec::VP9:  return QStringLiteral("vp09");
    }
    return {};
}

// ── AudioCodec ──────────────────────────────────────────────────────────────

enum class AudioCodec : int {
    AAC = 0,
    Opus
};

inline QString audioCodecLabel(AudioCodec c) {
    switch (c) {
        case AudioCodec::AAC:  return QStringLiteral("AAC-LC");
        case AudioCodec::Opus: return QStringLiteral("Opus");
    }
    return {};
}

// ── ContainerFormat ─────────────────────────────────────────────────────────

enum class ContainerFormat : int {
    MP4 = 0,
    MOV,
    WebM
};

inline QString containerFormatLabel(ContainerFormat f) {
    switch (f) {
        case ContainerFormat::MP4:  return QStringLiteral("MP4");
        case ContainerFormat::MOV:  return QStringLiteral("MOV");
        case ContainerFormat::WebM: return QStringLiteral("WebM");
    }
    return {};
}

inline QString containerFormatExtension(ContainerFormat f) {
    switch (f) {
        case ContainerFormat::MP4:  return QStringLiteral(".mp4");
        case ContainerFormat::MOV:  return QStringLiteral(".mov");
        case ContainerFormat::WebM: return QStringLiteral(".webm");
    }
    return {};
}

// ── ExportPresetId ──────────────────────────────────────────────────────────

enum class ExportPresetId : int {
    InstagramReel = 0,
    TikTok,
    YouTube4K,
    YouTube1080p,
    GeneralHD,
    Custom
};

// ── ExportPreset ────────────────────────────────────────────────────────────

struct ExportPreset {
    ExportPresetId id{ExportPresetId::GeneralHD};
    QString name;
    QString description;
    int width{1920};
    int height{1080};
    double frameRate{30.0};
    VideoCodec videoCodec{VideoCodec::H264};
    int videoBitrateMbps{8};
    AudioCodec audioCodec{AudioCodec::AAC};
    int audioBitrateKbps{192};
    ContainerFormat container{ContainerFormat::MP4};

    [[nodiscard]] static const QList<ExportPreset>& all() {
        static const QList<ExportPreset> presets = {
            {ExportPresetId::InstagramReel, QStringLiteral("Instagram Reel"),
             QStringLiteral("9:16 vertical, optimized for Instagram"),
             1080, 1920, 30, VideoCodec::H264, 8, AudioCodec::AAC, 128, ContainerFormat::MP4},
            {ExportPresetId::TikTok, QStringLiteral("TikTok"),
             QStringLiteral("9:16 vertical, optimized for TikTok"),
             1080, 1920, 30, VideoCodec::H264, 6, AudioCodec::AAC, 128, ContainerFormat::MP4},
            {ExportPresetId::YouTube4K, QStringLiteral("YouTube 4K"),
             QStringLiteral("3840x2160, high quality"),
             3840, 2160, 30, VideoCodec::H265, 40, AudioCodec::AAC, 256, ContainerFormat::MP4},
            {ExportPresetId::YouTube1080p, QStringLiteral("YouTube 1080p"),
             QStringLiteral("1920x1080, standard HD"),
             1920, 1080, 30, VideoCodec::H264, 12, AudioCodec::AAC, 192, ContainerFormat::MP4},
            {ExportPresetId::GeneralHD, QStringLiteral("General HD"),
             QStringLiteral("1920x1080, balanced quality & size"),
             1920, 1080, 30, VideoCodec::H264, 8, AudioCodec::AAC, 192, ContainerFormat::MP4},
        };
        return presets;
    }

    [[nodiscard]] static ExportPreset byId(ExportPresetId id) {
        for (const auto& p : all()) {
            if (p.id == id) return p;
        }
        return all().last();
    }
};

// ── ExportConfig ────────────────────────────────────────────────────────────

struct ExportConfig {
    ExportPresetId presetId{ExportPresetId::GeneralHD};
    int width{1920};
    int height{1080};
    double frameRate{30.0};
    VideoCodec videoCodec{VideoCodec::H264};
    int videoBitrateMbps{8};
    AudioCodec audioCodec{AudioCodec::AAC};
    int audioBitrateKbps{192};
    ContainerFormat container{ContainerFormat::MP4};
    QString outputPath;

    [[nodiscard]] static ExportConfig fromPreset(const ExportPreset& preset, const QString& outputPath) {
        return {
            .presetId = preset.id,
            .width = preset.width,
            .height = preset.height,
            .frameRate = preset.frameRate,
            .videoCodec = preset.videoCodec,
            .videoBitrateMbps = preset.videoBitrateMbps,
            .audioCodec = preset.audioCodec,
            .audioBitrateKbps = preset.audioBitrateKbps,
            .container = preset.container,
            .outputPath = outputPath,
        };
    }

    [[nodiscard]] QJsonObject toMap() const {
        return {
            {QStringLiteral("presetId"), static_cast<int>(presetId)},
            {QStringLiteral("width"), width},
            {QStringLiteral("height"), height},
            {QStringLiteral("frameRate"), frameRate},
            {QStringLiteral("videoCodec"), static_cast<int>(videoCodec)},
            {QStringLiteral("videoBitrateMbps"), videoBitrateMbps},
            {QStringLiteral("audioCodec"), static_cast<int>(audioCodec)},
            {QStringLiteral("audioBitrateKbps"), audioBitrateKbps},
            {QStringLiteral("container"), static_cast<int>(container)},
            {QStringLiteral("outputPath"), outputPath},
        };
    }
};

// ── ExportPhase ─────────────────────────────────────────────────────────────

enum class ExportPhase : int {
    Preparing = 0,
    Encoding,
    Muxing,
    Finalizing,
    Done,
    Failed,
    Cancelled
};

// ── ExportProgress ──────────────────────────────────────────────────────────

struct ExportProgress {
    ExportPhase phase{ExportPhase::Preparing};
    double percent{0.0};
    std::chrono::milliseconds elapsed{0};
    std::optional<std::chrono::milliseconds> estimatedRemaining;
    std::optional<QString> message;

    [[nodiscard]] ExportProgress copyWith(
        std::optional<ExportPhase> phase_ = std::nullopt,
        std::optional<double> percent_ = std::nullopt,
        std::optional<std::chrono::milliseconds> elapsed_ = std::nullopt,
        std::optional<std::chrono::milliseconds> estimatedRemaining_ = std::nullopt,
        std::optional<QString> message_ = std::nullopt
    ) const {
        return {
            .phase = phase_.value_or(phase),
            .percent = percent_.value_or(percent),
            .elapsed = elapsed_.value_or(elapsed),
            .estimatedRemaining = estimatedRemaining_.has_value() ? estimatedRemaining_ : estimatedRemaining,
            .message = message_.has_value() ? message_ : message,
        };
    }
};

// ── ExportResult ────────────────────────────────────────────────────────────

struct ExportResult {
    QString filePath;
    int64_t fileSizeBytes{0};
    double durationSeconds{0.0};
    int width{0};
    int height{0};
    std::chrono::milliseconds exportTime{0};

    [[nodiscard]] QString fileSizeFormatted() const {
        if (fileSizeBytes < 1024) return QStringLiteral("%1 B").arg(fileSizeBytes);
        if (fileSizeBytes < 1024 * 1024)
            return QStringLiteral("%1 KB").arg(fileSizeBytes / 1024.0, 0, 'f', 1);
        if (fileSizeBytes < 1024LL * 1024 * 1024)
            return QStringLiteral("%1 MB").arg(fileSizeBytes / (1024.0 * 1024.0), 0, 'f', 1);
        return QStringLiteral("%1 GB").arg(fileSizeBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
};

} // namespace gopost::video_editor
