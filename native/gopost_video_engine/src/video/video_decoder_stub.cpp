#if !defined(__APPLE__)
#include "decoder_interface.hpp"
#include "gopost/engine.h"
#include "gopost/error.h"
#include <cstring>
#include <cmath>

namespace gopost {
namespace video {

class StubVideoDecoder : public IVideoDecoder {
public:
    explicit StubVideoDecoder(GopostEngine* engine) : engine_(engine) {}

    bool open(const std::string& path) override {
        path_ = path;
        if (path.empty()) return false;
        info_.width = 1920;
        info_.height = 1080;
        info_.frame_rate = 30.0;
        info_.duration_seconds = 10.0;
        info_.frame_count = 300;
        info_.codec_name = "stub";
        info_.bitrate = 0;
        info_.rotation = Rotation::None;
        current_time_ = 0;
        eof_ = false;
        return true;
    }

    void close() override {
        path_.clear();
        info_ = {};
        current_time_ = 0;
        eof_ = false;
    }

    bool is_open() const override { return !path_.empty(); }
    VideoStreamInfo info() const override { return info_; }
    bool is_eof() const override { return eof_; }

    GopostFrame* decode_frame_at(double source_time_seconds) override {
        if (!engine_ || !is_open()) return nullptr;
        GopostFrame* frame = nullptr;
        if (gopost_frame_acquire(engine_, &frame, (uint32_t)info_.width, (uint32_t)info_.height,
                                 GOPOST_PIXEL_FORMAT_RGBA8) != GOPOST_OK) {
            return nullptr;
        }
        if (!frame || !frame->data) return nullptr;
        fill_test_pattern(frame->data, frame->width, frame->height, source_time_seconds);
        return frame;
    }

    bool seek_to(double timestamp_seconds) override {
        if (!is_open()) return false;
        current_time_ = timestamp_seconds;
        if (current_time_ < 0) current_time_ = 0;
        if (current_time_ > info_.duration_seconds) current_time_ = info_.duration_seconds;
        eof_ = false;
        return true;
    }

    std::optional<DecodedFrame> decode_next_frame() override {
        if (!is_open() || eof_) return std::nullopt;
        if (current_time_ >= info_.duration_seconds) {
            eof_ = true;
            return std::nullopt;
        }

        DecodedFrame df;
        df.width  = static_cast<uint32_t>(info_.width);
        df.height = static_cast<uint32_t>(info_.height);
        df.format = GOPOST_PIXEL_FORMAT_RGBA8;
        df.timestamp_seconds = current_time_;
        df.pts = static_cast<int64_t>(current_time_ * info_.frame_rate);

        const size_t n = static_cast<size_t>(df.width) * df.height * 4;
        df.pixels.resize(n);
        fill_test_pattern(df.pixels.data(), df.width, df.height, current_time_);

        current_time_ += 1.0 / info_.frame_rate;
        if (current_time_ >= info_.duration_seconds) eof_ = true;

        return df;
    }

private:
    static void fill_test_pattern(uint8_t* data, uint32_t w, uint32_t h, double t) {
        const size_t n = static_cast<size_t>(w) * h * 4;
        const double frac = std::fmod(t, 1.0);
        uint8_t r = static_cast<uint8_t>(frac * 255);
        uint8_t g = 80;
        uint8_t b = 180;
        uint8_t a = 255;
        for (size_t i = 0; i < n; i += 4) {
            data[i]     = r;
            data[i + 1] = g;
            data[i + 2] = b;
            data[i + 3] = a;
        }
    }

    GopostEngine* engine_ = nullptr;
    std::string path_;
    VideoStreamInfo info_;
    double current_time_ = 0;
    bool eof_ = false;
};

std::unique_ptr<IVideoDecoder> create_video_decoder(GopostEngine* engine) {
    return std::make_unique<StubVideoDecoder>(engine);
}

}  // namespace video
}  // namespace gopost
#endif
