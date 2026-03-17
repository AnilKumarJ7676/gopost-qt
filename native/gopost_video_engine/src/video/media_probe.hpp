#ifndef GOPOST_VIDEO_MEDIA_PROBE_HPP
#define GOPOST_VIDEO_MEDIA_PROBE_HPP

#include <cstdint>
#include <string>

namespace gopost {
namespace video {

struct MediaProbeInfo {
    double duration_seconds = 0;
    int32_t width = 0;
    int32_t height = 0;
    double frame_rate = 0;
    int64_t frame_count = 0;
    bool has_audio = false;
    int32_t audio_sample_rate = 0;
    int32_t audio_channels = 0;
    double audio_duration_seconds = 0;
};

bool probe_media(const std::string& path, MediaProbeInfo& out);

}  // namespace video
}  // namespace gopost

#endif
