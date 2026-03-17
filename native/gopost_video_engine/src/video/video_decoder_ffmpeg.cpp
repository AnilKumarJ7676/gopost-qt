#if !defined(__APPLE__) && defined(GOPOST_HAS_FFMPEG)
#include "decoder_interface.hpp"
#include "gopost/engine.h"
#include "gopost/error.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/display.h>
}

#include <string>
#include <cstring>
#include <cmath>

namespace gopost {
namespace video {

// ============================================================================
// Helper: extract rotation from side-data / display matrix
// ============================================================================

static Rotation rotation_from_stream(const AVStream* stream) {
    // FFmpeg >= 5.x: side data on codecpar; older: on stream
    const uint8_t* matrix_data = nullptr;

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(60, 0, 0)
    // FFmpeg 6+: use av_packet_side_data_get on stream codecpar
    const AVPacketSideData* sd = av_packet_side_data_get(
        stream->codecpar->coded_side_data,
        stream->codecpar->nb_coded_side_data,
        AV_PKT_DATA_DISPLAYMATRIX);
    if (sd) matrix_data = sd->data;
#else
    // FFmpeg 4.x / 5.x: use av_stream_get_side_data
    size_t sd_size = 0;
    matrix_data = av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX,
                                          reinterpret_cast<int*>(&sd_size));
#endif

    if (!matrix_data) return Rotation::None;

    double angle = av_display_rotation_get(reinterpret_cast<const int32_t*>(matrix_data));
    // av_display_rotation_get returns negative clockwise angle
    int degrees = -static_cast<int>(std::round(angle));
    degrees = ((degrees % 360) + 360) % 360;  // normalise to 0..359

    switch (degrees) {
        case 90:  return Rotation::CW90;
        case 180: return Rotation::CW180;
        case 270: return Rotation::CW270;
        default:  return Rotation::None;
    }
}

// ============================================================================
// FFmpegVideoDecoder
// ============================================================================

class FFmpegVideoDecoder : public IVideoDecoder {
public:
    explicit FFmpegVideoDecoder(GopostEngine* engine) : engine_(engine) {}

    ~FFmpegVideoDecoder() override { close(); }

    // --- lifecycle ---

    bool open(const std::string& path) override {
        close();
        if (path.empty() || !engine_) return false;

        if (avformat_open_input(&fmt_ctx_, path.c_str(), nullptr, nullptr) < 0)
            return false;

        if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
            close();
            return false;
        }

        video_stream_idx_ = av_find_best_stream(
            fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (video_stream_idx_ < 0) {
            close();
            return false;
        }

        AVStream* stream = fmt_ctx_->streams[video_stream_idx_];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            close();
            return false;
        }

        codec_ctx_ = avcodec_alloc_context3(codec);
        if (!codec_ctx_) {
            close();
            return false;
        }

        if (avcodec_parameters_to_context(codec_ctx_, stream->codecpar) < 0) {
            close();
            return false;
        }

        codec_ctx_->thread_count = 0;
        if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
            close();
            return false;
        }

        // --- populate VideoStreamInfo ---
        info_.width  = codec_ctx_->width;
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
        info_.rotation   = rotation_from_stream(stream);

        // --- swscale context ---
        sws_ctx_ = sws_getContext(
            info_.width, info_.height, codec_ctx_->pix_fmt,
            info_.width, info_.height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx_) {
            close();
            return false;
        }

        frame_  = av_frame_alloc();
        packet_ = av_packet_alloc();
        if (!frame_ || !packet_) {
            close();
            return false;
        }

        eof_   = false;
        path_  = path;
        return true;
    }

    void close() override {
        if (sws_ctx_) { sws_freeContext(sws_ctx_); sws_ctx_ = nullptr; }
        if (frame_)   { av_frame_free(&frame_); }
        if (packet_)  { av_packet_free(&packet_); }
        if (codec_ctx_) { avcodec_free_context(&codec_ctx_); }
        if (fmt_ctx_) { avformat_close_input(&fmt_ctx_); }
        video_stream_idx_ = -1;
        eof_ = false;
        path_.clear();
        info_ = {};
    }

    bool is_open() const override { return fmt_ctx_ != nullptr && codec_ctx_ != nullptr; }
    VideoStreamInfo info() const override { return info_; }
    bool is_eof() const override { return eof_; }

    // --- random-access decode (existing) ---

    GopostFrame* decode_frame_at(double source_time_seconds) override {
        if (!is_open() || !engine_) return nullptr;

        double clamped = clamp_time(source_time_seconds);
        AVStream* stream = fmt_ctx_->streams[video_stream_idx_];
        int64_t target_ts = (int64_t)(clamped / av_q2d(stream->time_base));

        avcodec_flush_buffers(codec_ctx_);
        if (av_seek_frame(fmt_ctx_, video_stream_idx_, target_ts,
                          AVSEEK_FLAG_BACKWARD) < 0) {
            av_seek_frame(fmt_ctx_, video_stream_idx_, 0, AVSEEK_FLAG_BACKWARD);
        }
        avcodec_flush_buffers(codec_ctx_);
        eof_ = false;

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

            double frame_pts = frame_pts_seconds();
            if (frame_pts < clamped - 1.0 / info_.frame_rate) {
                av_frame_unref(frame_);
                continue;
            }

            return convert_frame_to_rgba();
        }

        avcodec_send_packet(codec_ctx_, nullptr);
        if (avcodec_receive_frame(codec_ctx_, frame_) == 0) {
            return convert_frame_to_rgba();
        }

        eof_ = true;
        return nullptr;
    }

    // --- sequential decode (new) ---

    bool seek_to(double timestamp_seconds) override {
        if (!is_open()) return false;

        double clamped = clamp_time(timestamp_seconds);
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

    std::optional<DecodedFrame> decode_next_frame() override {
        if (!is_open() || eof_) return std::nullopt;

        // Read packets until we get a decoded frame
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
        if (ret == 0) {
            return build_decoded_frame();
        }

        eof_ = true;
        return std::nullopt;
    }

private:
    double clamp_time(double t) const {
        if (t < 0) t = 0;
        if (info_.duration_seconds > 0 && t > info_.duration_seconds)
            t = info_.duration_seconds;
        return t;
    }

    double frame_pts_seconds() const {
        if (!fmt_ctx_ || video_stream_idx_ < 0) return 0;
        AVStream* stream = fmt_ctx_->streams[video_stream_idx_];
        if (frame_->pts != AV_NOPTS_VALUE)
            return (double)frame_->pts * av_q2d(stream->time_base);
        return 0;
    }

    GopostFrame* convert_frame_to_rgba() {
        GopostFrame* out = nullptr;
        if (gopost_frame_acquire(engine_, &out,
                                 (uint32_t)frame_->width, (uint32_t)frame_->height,
                                 GOPOST_PIXEL_FORMAT_RGBA8) != GOPOST_OK) {
            av_frame_unref(frame_);
            return nullptr;
        }
        if (!out || !out->data) {
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

    std::optional<DecodedFrame> build_decoded_frame() {
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

        uint8_t* dst_data[1] = { df.pixels.data() };
        int dst_linesize[1]  = { static_cast<int>(w * 4) };

        sws_scale(sws_ctx_,
                  frame_->data, frame_->linesize,
                  0, frame_->height,
                  dst_data, dst_linesize);

        av_frame_unref(frame_);
        return df;
    }

    GopostEngine*    engine_  = nullptr;
    std::string      path_;
    VideoStreamInfo  info_;
    bool             eof_ = false;

    AVFormatContext*  fmt_ctx_   = nullptr;
    AVCodecContext*   codec_ctx_ = nullptr;
    SwsContext*       sws_ctx_   = nullptr;
    AVFrame*          frame_     = nullptr;
    AVPacket*         packet_    = nullptr;
    int video_stream_idx_ = -1;
};

std::unique_ptr<IVideoDecoder> create_video_decoder(GopostEngine* engine) {
    return std::make_unique<FFmpegVideoDecoder>(engine);
}

}  // namespace video
}  // namespace gopost
#endif
