#if !defined(__APPLE__) && defined(GOPOST_HAS_FFMPEG)
#include "audio_decoder_interface.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

#include <string>
#include <cstring>
#include <algorithm>

namespace gopost {
namespace video {

class FFmpegAudioDecoder : public IAudioDecoder {
public:
    ~FFmpegAudioDecoder() override { close(); }

    bool open(const std::string& path) override {
        close();
        if (path.empty()) return false;

        if (avformat_open_input(&fmt_ctx_, path.c_str(), nullptr, nullptr) < 0)
            return false;

        if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
            close();
            return false;
        }

        audio_stream_idx_ = av_find_best_stream(
            fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audio_stream_idx_ < 0) {
            close();
            return false;
        }

        AVStream* stream = fmt_ctx_->streams[audio_stream_idx_];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            close();
            return false;
        }

        codec_ctx_ = avcodec_alloc_context3(codec);
        if (!codec_ctx_ || avcodec_parameters_to_context(codec_ctx_, stream->codecpar) < 0) {
            close();
            return false;
        }

        if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
            close();
            return false;
        }

        info_.sample_rate = codec_ctx_->sample_rate;
        info_.channels = codec_ctx_->ch_layout.nb_channels;
        if (info_.channels <= 0) info_.channels = 2;

        if (fmt_ctx_->duration > 0) {
            info_.duration_seconds = (double)fmt_ctx_->duration / AV_TIME_BASE;
        } else if (stream->duration > 0 && stream->time_base.den > 0) {
            info_.duration_seconds = (double)stream->duration * av_q2d(stream->time_base);
        }
        info_.total_samples = (int64_t)(info_.duration_seconds * info_.sample_rate);

        frame_ = av_frame_alloc();
        packet_ = av_packet_alloc();
        if (!frame_ || !packet_) {
            close();
            return false;
        }

        path_ = path;
        return true;
    }

    void close() override {
        if (swr_ctx_) { swr_free(&swr_ctx_); }
        if (frame_) { av_frame_free(&frame_); }
        if (packet_) { av_packet_free(&packet_); }
        if (codec_ctx_) { avcodec_free_context(&codec_ctx_); }
        if (fmt_ctx_) { avformat_close_input(&fmt_ctx_); }
        audio_stream_idx_ = -1;
        path_.clear();
        info_ = {};
    }

    bool is_open() const override { return fmt_ctx_ != nullptr && codec_ctx_ != nullptr; }
    AudioStreamInfo info() const override { return info_; }

    int32_t decode_audio_at(double time_seconds, float* buffer,
                            int32_t max_frames, int32_t target_sample_rate) override {
        if (!is_open() || !buffer || max_frames <= 0) return 0;

        double clamped = time_seconds;
        if (clamped < 0) clamped = 0;
        if (clamped >= info_.duration_seconds) return 0;

        if (!ensure_resampler(target_sample_rate)) return 0;

        AVStream* stream = fmt_ctx_->streams[audio_stream_idx_];
        int64_t target_ts = (int64_t)(clamped / av_q2d(stream->time_base));

        avcodec_flush_buffers(codec_ctx_);
        av_seek_frame(fmt_ctx_, audio_stream_idx_, target_ts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codec_ctx_);
        if (swr_ctx_) swr_init(swr_ctx_);

        const int32_t out_channels = 2;
        int32_t frames_written = 0;

        while (frames_written < max_frames) {
            int ret = av_read_frame(fmt_ctx_, packet_);
            if (ret < 0) break;

            if (packet_->stream_index != audio_stream_idx_) {
                av_packet_unref(packet_);
                continue;
            }

            ret = avcodec_send_packet(codec_ctx_, packet_);
            av_packet_unref(packet_);
            if (ret < 0) continue;

            while (avcodec_receive_frame(codec_ctx_, frame_) == 0) {
                double frame_pts = 0;
                if (frame_->pts != AV_NOPTS_VALUE) {
                    frame_pts = (double)frame_->pts * av_q2d(stream->time_base);
                }

                if (frame_pts + (double)frame_->nb_samples / codec_ctx_->sample_rate < clamped) {
                    av_frame_unref(frame_);
                    continue;
                }

                int32_t space = max_frames - frames_written;
                if (space <= 0) {
                    av_frame_unref(frame_);
                    return frames_written;
                }

                uint8_t* out_buf = reinterpret_cast<uint8_t*>(
                    buffer + frames_written * out_channels);
                const uint8_t* const* in_buf =
                    const_cast<const uint8_t* const*>(frame_->extended_data);

                int converted = swr_convert(swr_ctx_,
                    &out_buf, space,
                    in_buf, frame_->nb_samples);

                if (converted > 0) frames_written += converted;
                av_frame_unref(frame_);
            }
        }

        return frames_written;
    }

private:
    bool ensure_resampler(int32_t target_sample_rate) {
        if (swr_ctx_ && last_target_rate_ == target_sample_rate)
            return true;

        if (swr_ctx_) swr_free(&swr_ctx_);

        AVChannelLayout out_layout = AV_CHANNEL_LAYOUT_STEREO;
        swr_alloc_set_opts2(&swr_ctx_,
            &out_layout, AV_SAMPLE_FMT_FLT, target_sample_rate,
            &codec_ctx_->ch_layout, codec_ctx_->sample_fmt, codec_ctx_->sample_rate,
            0, nullptr);

        if (!swr_ctx_ || swr_init(swr_ctx_) < 0) {
            if (swr_ctx_) swr_free(&swr_ctx_);
            return false;
        }

        last_target_rate_ = target_sample_rate;
        return true;
    }

    std::string path_;
    AudioStreamInfo info_;

    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    SwrContext* swr_ctx_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;
    int audio_stream_idx_ = -1;
    int32_t last_target_rate_ = 0;
};

std::unique_ptr<IAudioDecoder> create_audio_decoder() {
    return std::make_unique<FFmpegAudioDecoder>();
}

}  // namespace video
}  // namespace gopost
#endif
