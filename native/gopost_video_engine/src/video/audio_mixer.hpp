#ifndef GOPOST_VIDEO_AUDIO_MIXER_HPP
#define GOPOST_VIDEO_AUDIO_MIXER_HPP

#include "audio_decoder_interface.hpp"
#include "timeline_model.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace gopost {
namespace video {

class AudioMixer {
public:
    AudioMixer() = default;
    ~AudioMixer() = default;

    int32_t render_audio(const TimelineModel* model,
                         double position_seconds,
                         float* buffer,
                         int32_t num_frames,
                         int32_t num_channels,
                         int32_t sample_rate);

    IAudioDecoder* get_decoder_for(const std::string& path);

    void set_clip_volume(int32_t clip_id, float volume);
    float get_clip_volume(int32_t clip_id) const;

    void set_clip_pan(int32_t clip_id, float pan);
    float get_clip_pan(int32_t clip_id) const;

    void set_clip_fade_in(int32_t clip_id, float seconds);
    void set_clip_fade_out(int32_t clip_id, float seconds);

    void set_track_volume(int32_t track_index, float volume);
    void set_track_pan(int32_t track_index, float pan);
    void set_track_mute(int32_t track_index, bool mute);
    void set_track_solo(int32_t track_index, bool solo);

private:
    float compute_fade(const Clip& clip, double position_seconds) const;
    void apply_pan(float* left, float* right, float pan) const;

    std::unordered_map<std::string, std::unique_ptr<IAudioDecoder>> decoders_;
    std::unordered_map<int32_t, float> clip_volumes_;
    std::unordered_map<int32_t, float> clip_pans_;
    std::unordered_map<int32_t, float> clip_fade_ins_;
    std::unordered_map<int32_t, float> clip_fade_outs_;
    std::unordered_map<int32_t, float> track_volumes_;
    std::unordered_map<int32_t, float> track_pans_;
    std::unordered_map<int32_t, bool>  track_mutes_;
    std::unordered_map<int32_t, bool>  track_solos_;
};

}  // namespace video
}  // namespace gopost

#endif
