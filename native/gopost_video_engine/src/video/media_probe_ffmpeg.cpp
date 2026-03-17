#if !defined(__APPLE__) && defined(GOPOST_HAS_FFMPEG)
#include "media_probe.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace gopost {
namespace video {

bool probe_media(const std::string& path, MediaProbeInfo& out) {
    if (path.empty()) return false;

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr) < 0)
        return false;

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        return false;
    }

    if (fmt_ctx->duration > 0) {
        out.duration_seconds = (double)fmt_ctx->duration / AV_TIME_BASE;
    }

    int video_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_idx >= 0) {
        AVStream* vs = fmt_ctx->streams[video_idx];
        AVCodecParameters* par = vs->codecpar;

        out.width = par->width;
        out.height = par->height;

        if (vs->avg_frame_rate.den > 0 && vs->avg_frame_rate.num > 0) {
            out.frame_rate = av_q2d(vs->avg_frame_rate);
        } else if (vs->r_frame_rate.den > 0 && vs->r_frame_rate.num > 0) {
            out.frame_rate = av_q2d(vs->r_frame_rate);
        } else {
            out.frame_rate = 30.0;
        }

        if (vs->nb_frames > 0) {
            out.frame_count = vs->nb_frames;
        } else if (out.duration_seconds > 0 && out.frame_rate > 0) {
            out.frame_count = (int64_t)(out.duration_seconds * out.frame_rate);
        }

        if (out.duration_seconds <= 0 && vs->duration > 0 && vs->time_base.den > 0) {
            out.duration_seconds = (double)vs->duration * av_q2d(vs->time_base);
        }
    }

    int audio_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_idx >= 0) {
        out.has_audio = true;
        AVStream* as = fmt_ctx->streams[audio_idx];
        AVCodecParameters* par = as->codecpar;

        out.audio_sample_rate = par->sample_rate;
        out.audio_channels = par->ch_layout.nb_channels;
        if (out.audio_channels <= 0) out.audio_channels = 2;

        if (as->duration > 0 && as->time_base.den > 0) {
            out.audio_duration_seconds = (double)as->duration * av_q2d(as->time_base);
        } else {
            out.audio_duration_seconds = out.duration_seconds;
        }
    }

    avformat_close_input(&fmt_ctx);
    return (video_idx >= 0 || audio_idx >= 0);
}

}  // namespace video
}  // namespace gopost
#endif
