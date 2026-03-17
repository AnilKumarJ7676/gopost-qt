#if !defined(__APPLE__)
#include "media_probe.hpp"
#include <algorithm>
#include <cstring>

namespace gopost {
namespace video {

bool probe_media(const std::string& path, MediaProbeInfo& out) {
    if (path.empty()) return false;

    // Without FFmpeg or platform media APIs, return best-effort defaults.
    // Real probing will be available when FFmpeg is integrated.
    auto ends_with = [](const std::string& s, const std::string& suffix) {
        if (suffix.size() > s.size()) return false;
        return std::equal(suffix.rbegin(), suffix.rend(), s.rbegin(),
            [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    };

    bool is_image = ends_with(path, ".jpg") || ends_with(path, ".jpeg") ||
                    ends_with(path, ".png") || ends_with(path, ".webp") ||
                    ends_with(path, ".bmp") || ends_with(path, ".gif");

    if (is_image) {
        out.width = 1920;
        out.height = 1080;
        out.duration_seconds = 5.0;
        out.frame_rate = 1.0;
        out.frame_count = 1;
        out.has_audio = false;
        return true;
    }

    // Assume video with reasonable defaults.
    out.width = 1920;
    out.height = 1080;
    out.frame_rate = 30.0;
    out.duration_seconds = 10.0;
    out.frame_count = 300;
    out.has_audio = true;
    out.audio_sample_rate = 48000;
    out.audio_channels = 2;
    out.audio_duration_seconds = 10.0;
    return true;
}

}  // namespace video
}  // namespace gopost
#endif
