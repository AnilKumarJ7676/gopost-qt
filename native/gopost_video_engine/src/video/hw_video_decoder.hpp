#ifndef GOPOST_HW_VIDEO_DECODER_HPP
#define GOPOST_HW_VIDEO_DECODER_HPP

#include "decoder_interface.hpp"
#include <string>
#include <mutex>

#if defined(GOPOST_HAS_FFMPEG)

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/display.h>
}

namespace gopost {
namespace video {

// ============================================================================
// Runtime GPU decoder capability info
// ============================================================================

struct HwDecoderCaps {
    bool available = false;
    AVHWDeviceType device_type = AV_HWDEVICE_TYPE_NONE;
    std::string device_name;
};

/// Probe whether hardware decoding is available on this machine.
/// Safe to call from any thread; internally creates and tears down a
/// temporary hw device context.
HwDecoderCaps probe_hw_decoder_caps();

// ============================================================================
// HwVideoDecoder — drop-in replacement for FFmpegVideoDecoder
// ============================================================================

/// Hardware-accelerated video decoder.
///
/// On open() it tries the platform-preferred hw device (D3D11VA on Windows,
/// VideoToolbox on macOS/iOS, VAAPI on Linux, MediaCodec on Android).
/// If hw init fails for any reason the decoder falls back transparently to
/// multi-threaded software decode — the caller never needs to care.
///
/// The returned GopostFrame* from decode_frame_at() is always RGBA8 from the
/// engine frame pool, regardless of whether hw or sw path was used.
class HwVideoDecoder : public IVideoDecoder {
public:
    explicit HwVideoDecoder(GopostEngine* engine);
    /// Construct with a shared hw device — avoids creating one per decoder.
    HwVideoDecoder(GopostEngine* engine, AVBufferRef* shared_hw_device);
    ~HwVideoDecoder() override;

    bool open(const std::string& path) override;
    void close() override;
    bool is_open() const override;
    VideoStreamInfo info() const override;
    GopostFrame* decode_frame_at(double source_time_seconds) override;
    bool seek_to(double timestamp_seconds) override;
    std::optional<DecodedFrame> decode_next_frame() override;
    bool is_eof() const override;

    /// True if hardware decode path is active for the current file.
    bool is_hw_active() const { return using_hw_; }

private:
    bool init_hw_codec(const AVCodec* codec);
    bool init_sw_codec(const AVCodec* codec);

    /// FFmpeg get_format callback — selects the hw pixel format.
    static AVPixelFormat get_hw_format_cb(AVCodecContext* ctx,
                                           const AVPixelFormat* pix_fmts);

    GopostFrame* convert_frame_to_rgba();
    GopostFrame* transfer_hw_and_convert();
    std::optional<DecodedFrame> build_decoded_frame();
    double frame_pts_seconds() const;

    GopostEngine* engine_ = nullptr;
    std::string path_;
    VideoStreamInfo info_{};

    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    AVBufferRef* hw_device_ctx_ = nullptr;  // Our ref (may be shared)
    bool owns_hw_device_ = false;           // Did we create it, or is it shared?
    SwsContext* sws_ctx_ = nullptr;
    AVFrame* frame_ = nullptr;              // Decoded frame (hw or sw surface)
    AVFrame* sw_frame_ = nullptr;           // CPU-side transfer target
    AVPacket* packet_ = nullptr;
    int video_stream_idx_ = -1;
    bool using_hw_ = false;
    bool eof_ = false;
    AVPixelFormat hw_pix_fmt_ = AV_PIX_FMT_NONE;
    std::mutex mutex_;
};

}  // namespace video
}  // namespace gopost

#endif  // GOPOST_HAS_FFMPEG
#endif  // GOPOST_HW_VIDEO_DECODER_HPP
