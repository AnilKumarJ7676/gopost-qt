#pragma once

#include <QProcess>
#include <QImage>
#include <QString>
#include <QFileInfo>

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gopost::rendering {

// ============================================================================
// Lightweight DecodedFrame — self-contained decoded video frame
// (Mirrors the native engine's DecodedFrame but uses Qt types)
// ============================================================================

struct RenderDecodedFrame {
    std::vector<uint8_t> pixels;       // RGBA8 pixel data
    uint32_t             width  = 0;
    uint32_t             height = 0;
    double               timestamp_seconds = 0.0;
    int64_t              pts    = 0;
};

// ============================================================================
// QProcessVideoDecoder — Decodes video frames using ffmpeg CLI via pipe.
//
// This decoder spawns `ffmpeg` as a child process and reads raw RGBA frames
// from stdout. It supports sequential decode (frame-by-frame) and seeking.
//
// Unlike the native FFmpegVideoDecoder (which links libav*), this works
// anywhere ffmpeg.exe is on PATH — no dev libraries needed.
//
// Thread safety: NOT thread-safe. Use from one thread at a time.
// Designed to be owned by a single decode thread.
// ============================================================================

class QProcessVideoDecoder {
public:
    QProcessVideoDecoder() = default;
    ~QProcessVideoDecoder();

    // Non-copyable
    QProcessVideoDecoder(const QProcessVideoDecoder&) = delete;
    QProcessVideoDecoder& operator=(const QProcessVideoDecoder&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────

    /// Open a video file. Probes metadata via ffprobe, then spawns ffmpeg
    /// to decode sequentially from the start.
    bool open(const QString& path);

    /// Close the decoder and kill the ffmpeg process.
    void close();

    bool isOpen() const { return open_; }

    // ── Metadata ──────────────────────────────────────────────────────

    int width() const { return width_; }
    int height() const { return height_; }
    double frameRate() const { return fps_; }
    double duration() const { return duration_; }

    // ── Decode ────────────────────────────────────────────────────────

    /// Seek to a timestamp. Kills the current ffmpeg process and restarts
    /// from the new position.
    bool seekTo(double timestampSeconds);

    /// Decode the next frame. Reads exactly (width * height * 4) bytes
    /// from ffmpeg's stdout pipe. Returns nullopt on EOF or error.
    std::optional<RenderDecodedFrame> decodeNextFrame();

    /// True if ffmpeg process exited (EOF or error).
    bool isEof() const { return eof_; }

private:
    bool probe(const QString& path);
    bool startDecodeProcess(double startTime = 0.0);
    void killProcess();

    QString path_;
    bool open_ = false;
    bool eof_ = false;

    int width_ = 0;
    int height_ = 0;
    double fps_ = 30.0;
    double duration_ = 0.0;

    double currentTime_ = 0.0;
    int64_t frameIndex_ = 0;

    // Decode dimensions (scaled down for preview performance)
    int decodeWidth_ = 0;
    int decodeHeight_ = 0;

    std::unique_ptr<QProcess> ffmpeg_;
};

} // namespace gopost::rendering
