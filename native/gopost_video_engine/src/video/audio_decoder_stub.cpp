#if !defined(__APPLE__)
#include "audio_decoder_interface.hpp"
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace gopost {
namespace video {

class StubAudioDecoder : public IAudioDecoder {
public:
    bool open(const std::string& path) override {
        if (path.empty()) return false;
        path_ = path;
        info_.sample_rate = 48000;
        info_.channels = 2;
        info_.duration_seconds = 10.0;
        info_.total_samples = 480000;
        return true;
    }

    void close() override { path_.clear(); info_ = {}; }
    bool is_open() const override { return !path_.empty(); }
    AudioStreamInfo info() const override { return info_; }

    int32_t decode_audio_at(double time_seconds, float* buffer,
                            int32_t max_frames, int32_t target_sample_rate) override {
        if (!is_open() || !buffer || max_frames <= 0) return 0;
        if (time_seconds >= info_.duration_seconds) return 0;

        // Generate a 440Hz sine tone for testing.
        const int32_t channels = 2;
        const double freq = 440.0;
        for (int32_t i = 0; i < max_frames; ++i) {
            double t = time_seconds + (double)i / (double)target_sample_rate;
            float sample = 0.2f * (float)std::sin(2.0 * M_PI * freq * t);
            buffer[i * channels] = sample;
            buffer[i * channels + 1] = sample;
        }
        return max_frames;
    }

private:
    std::string path_;
    AudioStreamInfo info_;
};

std::unique_ptr<IAudioDecoder> create_audio_decoder() {
    return std::make_unique<StubAudioDecoder>();
}

}  // namespace video
}  // namespace gopost
#endif
