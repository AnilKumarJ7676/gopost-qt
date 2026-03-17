#ifndef GOPOST_VIDEO_AUDIO_DECODER_INTERFACE_HPP
#define GOPOST_VIDEO_AUDIO_DECODER_INTERFACE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct GopostEngine;

namespace gopost {
namespace video {

struct AudioStreamInfo {
    int32_t sample_rate = 0;
    int32_t channels = 0;
    double duration_seconds = 0;
    int64_t total_samples = 0;
};

struct AudioBuffer {
    std::vector<float> data;
    int32_t sample_rate = 48000;
    int32_t channels = 2;
    int32_t frame_count = 0;
};

class IAudioDecoder {
public:
    virtual ~IAudioDecoder() = default;
    virtual bool open(const std::string& path) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;
    virtual AudioStreamInfo info() const = 0;

    /**
     * Decode audio starting at time_seconds into interleaved float32 PCM.
     * Returns the number of frames actually decoded (may be < max_frames at EOF).
     * Output is interleaved: [L R L R ...] for stereo.
     */
    virtual int32_t decode_audio_at(double time_seconds,
                                     float* buffer,
                                     int32_t max_frames,
                                     int32_t target_sample_rate = 48000) = 0;
};

std::unique_ptr<IAudioDecoder> create_audio_decoder();

}  // namespace video
}  // namespace gopost

#endif
