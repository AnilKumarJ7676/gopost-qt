#if !defined(__APPLE__) && defined(GOPOST_HAS_FFMPEG)
#include "hw_video_decoder.hpp"
#include "gopost/engine.h"
#include "gopost/error.h"

extern "C" {
#include <libavutil/opt.h>
}

#include <cmath>
#include <cstring>
#include <algorithm>

namespace gopost {
namespace video {

// ============================================================================
// Platform hw device type selection
// ============================================================================

static AVHWDeviceType preferred_hw_type() {
#if defined(_WIN32) || defined(GOPOST_PLATFORM_WINDOWS)
    return AV_HWDEVICE_TYPE_D3D11VA;
#elif defined(__ANDROID__) || defined(GOPOST_PLATFORM_ANDROID)
    return AV_HWDEVICE_TYPE_MEDIACODEC;
#elif defined(__linux__) || defined(GOPOST_PLATFORM_LINUX)
    return AV_HWDEVICE_TYPE_VAAPI;
#else
    return AV_HWDEVICE_TYPE_NONE;
#endif
}

static AVHWDeviceType fallback_hw_type() {
#if defined(__linux__) || defined(GOPOST_PLATFORM_LINUX)
    return AV_HWDEVICE_TYPE_VDPAU;
#elif defined(_WIN32) || defined(GOPOST_PLATFORM_WINDOWS)
    return AV_HWDEVICE_TYPE_DXVA2;
#else
    return AV_HWDEVICE_TYPE_NONE;
#endif
}

// ============================================================================
// probe_hw_decoder_caps
// ============================================================================

HwDecoderCaps probe_hw_decoder_caps() {
    HwDecoderCaps caps;
    AVBufferRef* device_ref = nullptr;

    AVHWDeviceType type = preferred_hw_type();
    if (type == AV_HWDEVICE_TYPE_NONE) return caps;

    int err = av_hwdevice_ctx_create(&device_ref, type, nullptr, nullptr, 0);
    if (err < 0) {
        type = fallback_hw_type();
        if (type == AV_HWDEVICE_TYPE_NONE) return caps;
        err = av_hwdevice_ctx_create(&device_ref, type, nullptr, nullptr, 0);
    }

    if (err < 0 || !device_ref) return caps;

    caps.available = true;
    caps.device_type = type;
    caps.device_name = av_hwdevice_get_type_name(type);

    av_buffer_unref(&device_ref);
    return caps;
}

// ============================================================================
// HwVideoDecoder
// ============================================================================

HwVideoDecoder::HwVideoDecoder(GopostEngine* engine)
    : engine_(engine)
    , owns_hw_device_(true) {
    packet_ = av_packet_alloc();
    frame_ = av_frame_alloc();
    sw_frame_ = av_frame_alloc();
}

HwVideoDecoder::HwVideoDecoder(GopostEngine* engine, AVBufferRef* shared_hw_device)
    : engine_(engine)
    , owns_hw_device_(false) {
    packet_ = av_packet_alloc();
    frame_ = av_frame_alloc();
    sw_frame_ = av_frame_alloc();
    if (shared_hw_device) {
        hw_device_ctx_ = av_buffer_ref(shared_hw_device);
    }
}

HwVideoDecoder::~HwVideoDecoder() {
    close();
    av_packet_free(&packet_);
    av_frame_free(&frame_);
    av_frame_free(&sw_frame_);
}

// FFmpeg callback: pick the hw pixel format that matches our device
AVPixelFormat HwVideoDecoder::get_hw_format_cb(
        AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
    auto* self = static_cast<HwVideoDecoder*>(ctx->opaque);
    for (const AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == self->hw_pix_fmt_) {
            return *p;
        }
    }
    // hw format not offered — return first sw format as fallback
    return pix_fmts[0];
}

bool HwVideoDecoder::init_hw_codec(const AVCodec* codec) {
    // Find hw config for this codec + our device type
    AVHWDeviceType type = preferred_hw_type();
    const AVCodecHWConfig* hw_config = nullptr;

    for (int i = 0;; i++) {
        const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
        if (!config) break;
        if (config->device_type == type &&
            (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)) {
            hw_config = config;
            break;
        }
    }

    // Try fallback device type
    if (!hw_config) {
        type = fallback_hw_type();
        if (type != AV_HWDEVICE_TYPE_NONE) {
            for (int i = 0;; i++) {
                const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
                if (!config) break;
                if (config->device_type == type &&
                    (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)) {
                    hw_config = config;
                    break;
                }
            }
        }
    }

    if (!hw_config) return false;

    hw_pix_fmt_ = hw_config->pix_fmt;

    // Create hw device context if we don't have one (shared or own)
    if (!hw_device_ctx_) {
        int err = av_hwdevice_ctx_create(&hw_device_ctx_, type, nullptr, nullptr, 0);
        if (err < 0) {
            return false;
        }
        owns_hw_device_ = true;
    }

    // Create codec context
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) return false;

    avcodec_parameters_to_context(codec_ctx_,
        fmt_ctx_->streams[video_stream_idx_]->codecpar);

    codec_ctx_->hw_device_ctx = av_buffer_ref(hw_device_ctx_);
    codec_ctx_->opaque = this;
    codec_ctx_->get_format = &HwVideoDecoder::get_hw_format_cb;
    codec_ctx_->thread_count = 0;

    int err = avcodec_open2(codec_ctx_, codec, nullptr);
    if (err < 0) {
        avcodec_free_context(&codec_ctx_);
        return false;
    }

    using_hw_ = true;
    return true;
}

bool HwVideoDecoder::init_sw_codec(const AVCodec* codec) {
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) return false;

    avcodec_parameters_to_context(codec_ctx_,
        fmt_ctx_->streams[video_stream_idx_]->codecpar);
    codec_ctx_->thread_count = 0;

    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx_);
        return false;
    }

    using_hw_ = false;
    return true;
}

bool HwVideoDecoder::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Close previous file if any
    if (fmt_ctx_) close();

    if (path.empty() || !engine_) return false;

    if (avformat_open_input(&fmt_ctx_, path.c_str(), nullptr, nullptr) < 0)
        return false;

    if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
        avformat_close_input(&fmt_ctx_);
        return false;
    }

    video_stream_idx_ = av_find_best_stream(
        fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx_ < 0) {
        avformat_close_input(&fmt_ctx_);
        return false;
    }

    AVStream* stream = fmt_ctx_->streams[video_stream_idx_];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        avformat_close_input(&fmt_ctx_);
        return false;
    }

    // Try hardware decode, fall back to software transparently
    if (!init_hw_codec(codec)) {
        if (!init_sw_codec(codec)) {
            avformat_close_input(&fmt_ctx_);
            return false;
        }
    }

    // Fill stream info
    info_.width = codec_ctx_->width;
    info_.height = codec_ctx_->height;

    if (stream->avg_frame_rate.den > 0 && stream->avg_frame_rate.num > 0) {
        info_.frame_rate = av_q2d(stream->avg_frame_rate);
    } else if (stream->r_frame_rate.den > 0 && stream->r_frame_rate.num > 0) {
        info_.frame_rate = av_q2d(stream->r_frame_rate);
    } else {
        info_.frame_rate = 30.0;
    }

    if (fmt_ctx_->duration > 0) {
        info_.duration_seconds = (double)fmt_ctx_->duration / AV_TIME_BASE;
    } else if (stream->duration > 0 && stream->time_base.den > 0) {
        info_.duration_seconds = (double)stream->duration * av_q2d(stream->time_base);
    } else {
        info_.duration_seconds = 0;
    }

    info_.frame_count = (int64_t)(info_.duration_seconds * info_.frame_rate);

    // New metadata fields
    info_.codec_name = codec->name ? codec->name : "";
    info_.bitrate    = stream->codecpar->bit_rate > 0
                         ? stream->codecpar->bit_rate
                         : fmt_ctx_->bit_rate;

    // Extract rotation from display matrix side data
    {
        const uint8_t* matrix_data = nullptr;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(60, 0, 0)
        const AVPacketSideData* sd = av_packet_side_data_get(
            stream->codecpar->coded_side_data,
            stream->codecpar->nb_coded_side_data,
            AV_PKT_DATA_DISPLAYMATRIX);
        if (sd) matrix_data = sd->data;
#else
        size_t sd_size = 0;
        matrix_data = av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX,
                                              reinterpret_cast<int*>(&sd_size));
#endif
        if (matrix_data) {
            double angle = av_display_rotation_get(
                reinterpret_cast<const int32_t*>(matrix_data));
            int degrees = -static_cast<int>(std::round(angle));
            degrees = ((degrees % 360) + 360) % 360;
            switch (degrees) {
                case 90:  info_.rotation = Rotation::CW90; break;
                case 180: info_.rotation = Rotation::CW180; break;
                case 270: info_.rotation = Rotation::CW270; break;
                default:  info_.rotation = Rotation::None; break;
            }
        }
    }

    // Pre-create swscale context for the expected source format.
    // For hw decode the actual pixel format is determined after the first
    // decode + transfer, so sws_ctx_ will be (re-)created lazily.
    if (!using_hw_) {
        sws_ctx_ = sws_getContext(
            info_.width, info_.height, codec_ctx_->pix_fmt,
            info_.width, info_.height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    eof_ = false;
    path_ = path;
    return true;
}

void HwVideoDecoder::close() {
    if (sws_ctx_) { sws_freeContext(sws_ctx_); sws_ctx_ = nullptr; }
    if (codec_ctx_) { avcodec_free_context(&codec_ctx_); }

    // Only release hw_device_ctx_ if we own it (not shared)
    if (hw_device_ctx_ && owns_hw_device_) {
        av_buffer_unref(&hw_device_ctx_);
    }
    hw_device_ctx_ = nullptr;

    if (fmt_ctx_) { avformat_close_input(&fmt_ctx_); }

    video_stream_idx_ = -1;
    using_hw_ = false;
    eof_ = false;
    hw_pix_fmt_ = AV_PIX_FMT_NONE;
    path_.clear();
    info_ = {};
}

bool HwVideoDecoder::is_open() const {
    return fmt_ctx_ != nullptr && codec_ctx_ != nullptr;
}

VideoStreamInfo HwVideoDecoder::info() const { return info_; }

GopostFrame* HwVideoDecoder::transfer_hw_and_convert() {
    // Step 1: GPU surface → CPU (NV12 / P010 / etc.)
    av_frame_unref(sw_frame_);
    int err = av_hwframe_transfer_data(sw_frame_, frame_, 0);
    if (err < 0) {
        av_frame_unref(frame_);
        return nullptr;
    }
    sw_frame_->width = frame_->width;
    sw_frame_->height = frame_->height;

    // Step 2: Lazily create or recreate sws for the transferred format
    AVPixelFormat src_fmt = static_cast<AVPixelFormat>(sw_frame_->format);
    if (!sws_ctx_) {
        sws_ctx_ = sws_getContext(
            sw_frame_->width, sw_frame_->height, src_fmt,
            sw_frame_->width, sw_frame_->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx_) {
            av_frame_unref(sw_frame_);
            av_frame_unref(frame_);
            return nullptr;
        }
    }

    // Step 3: Convert to RGBA into engine frame pool
    GopostFrame* out = nullptr;
    if (gopost_frame_acquire(engine_, &out,
                             (uint32_t)sw_frame_->width,
                             (uint32_t)sw_frame_->height,
                             GOPOST_PIXEL_FORMAT_RGBA8) != GOPOST_OK) {
        av_frame_unref(sw_frame_);
        av_frame_unref(frame_);
        return nullptr;
    }
    if (!out || !out->data) {
        if (out) gopost_frame_release(engine_, out);
        av_frame_unref(sw_frame_);
        av_frame_unref(frame_);
        return nullptr;
    }

    uint8_t* dst_data[1] = { out->data };
    int dst_linesize[1] = { (int)(out->width * 4) };

    sws_scale(sws_ctx_,
              sw_frame_->data, sw_frame_->linesize,
              0, sw_frame_->height,
              dst_data, dst_linesize);

    av_frame_unref(sw_frame_);
    av_frame_unref(frame_);
    return out;
}

GopostFrame* HwVideoDecoder::convert_frame_to_rgba() {
    GopostFrame* out = nullptr;
    if (gopost_frame_acquire(engine_, &out,
                             (uint32_t)frame_->width,
                             (uint32_t)frame_->height,
                             GOPOST_PIXEL_FORMAT_RGBA8) != GOPOST_OK) {
        av_frame_unref(frame_);
        return nullptr;
    }
    if (!out || !out->data) {
        if (out) gopost_frame_release(engine_, out);
        av_frame_unref(frame_);
        return nullptr;
    }

    uint8_t* dst_data[1] = { out->data };
    int dst_linesize[1] = { (int)(out->width * 4) };

    sws_scale(sws_ctx_,
              frame_->data, frame_->linesize,
              0, frame_->height,
              dst_data, dst_linesize);

    av_frame_unref(frame_);
    return out;
}

GopostFrame* HwVideoDecoder::decode_frame_at(double source_time_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_open() || !engine_) return nullptr;

    double clamped = source_time_seconds;
    if (clamped < 0) clamped = 0;
    if (info_.duration_seconds > 0 && clamped > info_.duration_seconds)
        clamped = info_.duration_seconds;

    AVStream* stream = fmt_ctx_->streams[video_stream_idx_];
    int64_t target_ts = (int64_t)(clamped / av_q2d(stream->time_base));

    avcodec_flush_buffers(codec_ctx_);
    if (av_seek_frame(fmt_ctx_, video_stream_idx_, target_ts,
                      AVSEEK_FLAG_BACKWARD) < 0) {
        av_seek_frame(fmt_ctx_, video_stream_idx_, 0, AVSEEK_FLAG_BACKWARD);
    }
    avcodec_flush_buffers(codec_ctx_);
    eof_ = false;

    // Decode forward until we reach the target timestamp
    while (av_read_frame(fmt_ctx_, packet_) >= 0) {
        if (packet_->stream_index != video_stream_idx_) {
            av_packet_unref(packet_);
            continue;
        }

        int ret = avcodec_send_packet(codec_ctx_, packet_);
        av_packet_unref(packet_);
        if (ret < 0) continue;

        ret = avcodec_receive_frame(codec_ctx_, frame_);
        if (ret == AVERROR(EAGAIN)) continue;
        if (ret < 0) return nullptr;

        double frame_pts = 0;
        if (frame_->pts != AV_NOPTS_VALUE) {
            frame_pts = (double)frame_->pts * av_q2d(stream->time_base);
        }

        // Skip frames before our target (common after backward seek)
        if (frame_pts < clamped - 1.0 / info_.frame_rate) {
            av_frame_unref(frame_);
            continue;
        }

        // frame_ now contains the decoded frame (hw or sw surface)
        if (using_hw_) {
            return transfer_hw_and_convert();
        } else {
            return convert_frame_to_rgba();
        }
    }

    // Flush decoder for remaining buffered frames
    avcodec_send_packet(codec_ctx_, nullptr);
    if (avcodec_receive_frame(codec_ctx_, frame_) == 0) {
        if (using_hw_) {
            return transfer_hw_and_convert();
        } else {
            return convert_frame_to_rgba();
        }
    }

    eof_ = true;
    return nullptr;
}

// ============================================================================
// Sequential decode: seek_to, decode_next_frame, is_eof
// ============================================================================

bool HwVideoDecoder::is_eof() const {
    return eof_;
}

bool HwVideoDecoder::seek_to(double timestamp_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_open()) return false;

    double clamped = timestamp_seconds;
    if (clamped < 0) clamped = 0;
    if (info_.duration_seconds > 0 && clamped > info_.duration_seconds)
        clamped = info_.duration_seconds;

    AVStream* stream = fmt_ctx_->streams[video_stream_idx_];
    int64_t target_ts = (int64_t)(clamped / av_q2d(stream->time_base));

    avcodec_flush_buffers(codec_ctx_);
    int ret = av_seek_frame(fmt_ctx_, video_stream_idx_, target_ts,
                            AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        ret = av_seek_frame(fmt_ctx_, video_stream_idx_, 0, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) return false;
    }
    avcodec_flush_buffers(codec_ctx_);
    eof_ = false;
    return true;
}

double HwVideoDecoder::frame_pts_seconds() const {
    if (!fmt_ctx_ || video_stream_idx_ < 0) return 0;
    AVStream* stream = fmt_ctx_->streams[video_stream_idx_];
    if (frame_->pts != AV_NOPTS_VALUE)
        return (double)frame_->pts * av_q2d(stream->time_base);
    return 0;
}

std::optional<DecodedFrame> HwVideoDecoder::build_decoded_frame() {
    const uint32_t w = static_cast<uint32_t>(frame_->width);
    const uint32_t h = static_cast<uint32_t>(frame_->height);
    const size_t byte_count = static_cast<size_t>(w) * h * 4;

    DecodedFrame df;
    df.width  = w;
    df.height = h;
    df.format = GOPOST_PIXEL_FORMAT_RGBA8;
    df.pts    = frame_->pts != AV_NOPTS_VALUE ? frame_->pts : 0;
    df.timestamp_seconds = frame_pts_seconds();
    df.pixels.resize(byte_count);

    if (using_hw_) {
        // Transfer from GPU surface to CPU
        av_frame_unref(sw_frame_);
        int err = av_hwframe_transfer_data(sw_frame_, frame_, 0);
        if (err < 0) {
            av_frame_unref(frame_);
            return std::nullopt;
        }
        sw_frame_->width  = frame_->width;
        sw_frame_->height = frame_->height;

        AVPixelFormat src_fmt = static_cast<AVPixelFormat>(sw_frame_->format);
        if (!sws_ctx_) {
            sws_ctx_ = sws_getContext(
                sw_frame_->width, sw_frame_->height, src_fmt,
                sw_frame_->width, sw_frame_->height, AV_PIX_FMT_RGBA,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!sws_ctx_) {
                av_frame_unref(sw_frame_);
                av_frame_unref(frame_);
                return std::nullopt;
            }
        }

        uint8_t* dst_data[1] = { df.pixels.data() };
        int dst_linesize[1]  = { static_cast<int>(w * 4) };
        sws_scale(sws_ctx_,
                  sw_frame_->data, sw_frame_->linesize,
                  0, sw_frame_->height,
                  dst_data, dst_linesize);
        av_frame_unref(sw_frame_);
    } else {
        uint8_t* dst_data[1] = { df.pixels.data() };
        int dst_linesize[1]  = { static_cast<int>(w * 4) };
        sws_scale(sws_ctx_,
                  frame_->data, frame_->linesize,
                  0, frame_->height,
                  dst_data, dst_linesize);
    }

    av_frame_unref(frame_);
    return df;
}

std::optional<DecodedFrame> HwVideoDecoder::decode_next_frame() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_open() || eof_) return std::nullopt;

    while (av_read_frame(fmt_ctx_, packet_) >= 0) {
        if (packet_->stream_index != video_stream_idx_) {
            av_packet_unref(packet_);
            continue;
        }

        int ret = avcodec_send_packet(codec_ctx_, packet_);
        av_packet_unref(packet_);
        if (ret < 0 && ret != AVERROR(EAGAIN)) continue;

        ret = avcodec_receive_frame(codec_ctx_, frame_);
        if (ret == AVERROR(EAGAIN)) continue;
        if (ret == AVERROR_EOF) { eof_ = true; return std::nullopt; }
        if (ret < 0) return std::nullopt;

        return build_decoded_frame();
    }

    // Flush buffered frames
    avcodec_send_packet(codec_ctx_, nullptr);
    int ret = avcodec_receive_frame(codec_ctx_, frame_);
    if (ret == 0) return build_decoded_frame();

    eof_ = true;
    return std::nullopt;
}

// ============================================================================
// Factory override: return HwVideoDecoder when GOPOST_HAS_FFMPEG is set
// ============================================================================

// Note: The existing create_video_decoder in video_decoder_ffmpeg.cpp must be
// guarded with #if !defined(GOPOST_USE_HW_DECODER) so only one factory is
// active. When GOPOST_USE_HW_DECODER is defined, this factory takes over.

#if defined(GOPOST_USE_HW_DECODER)
std::unique_ptr<IVideoDecoder> create_video_decoder(GopostEngine* engine) {
    return std::make_unique<HwVideoDecoder>(engine);
}
#endif

}  // namespace video
}  // namespace gopost
#endif  // !__APPLE__ && GOPOST_HAS_FFMPEG
