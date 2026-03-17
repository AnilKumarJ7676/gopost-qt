#include "audio_mixer.hpp"
#include "keyframe_evaluator.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace gopost {
namespace video {

IAudioDecoder* AudioMixer::get_decoder_for(const std::string& path) {
    if (path.empty()) return nullptr;
    auto it = decoders_.find(path);
    if (it != decoders_.end()) return it->second.get();
    auto dec = create_audio_decoder();
    if (!dec || !dec->open(path)) return nullptr;
    IAudioDecoder* p = dec.get();
    decoders_[path] = std::move(dec);
    return p;
}

void AudioMixer::set_clip_volume(int32_t clip_id, float volume) {
    clip_volumes_[clip_id] = std::clamp(volume, 0.0f, 2.0f);
}
float AudioMixer::get_clip_volume(int32_t clip_id) const {
    auto it = clip_volumes_.find(clip_id);
    return (it != clip_volumes_.end()) ? it->second : 1.0f;
}

void AudioMixer::set_clip_pan(int32_t clip_id, float pan) {
    clip_pans_[clip_id] = std::clamp(pan, -1.0f, 1.0f);
}
float AudioMixer::get_clip_pan(int32_t clip_id) const {
    auto it = clip_pans_.find(clip_id);
    return (it != clip_pans_.end()) ? it->second : 0.0f;
}

void AudioMixer::set_clip_fade_in(int32_t clip_id, float seconds) {
    clip_fade_ins_[clip_id] = std::max(0.0f, seconds);
}
void AudioMixer::set_clip_fade_out(int32_t clip_id, float seconds) {
    clip_fade_outs_[clip_id] = std::max(0.0f, seconds);
}

void AudioMixer::set_track_volume(int32_t track_index, float volume) {
    track_volumes_[track_index] = std::clamp(volume, 0.0f, 2.0f);
}
void AudioMixer::set_track_pan(int32_t track_index, float pan) {
    track_pans_[track_index] = std::clamp(pan, -1.0f, 1.0f);
}
void AudioMixer::set_track_mute(int32_t track_index, bool mute) {
    track_mutes_[track_index] = mute;
}
void AudioMixer::set_track_solo(int32_t track_index, bool solo) {
    track_solos_[track_index] = solo;
}

float AudioMixer::compute_fade(const Clip& clip, double position_seconds) const {
    double clip_local = position_seconds - clip.timeline_range.in_time;
    double clip_duration = clip.timeline_range.out_time - clip.timeline_range.in_time;
    float fade = 1.0f;

    // Fade in
    float fade_in = clip.audio.fade_in_seconds;
    auto fi_it = clip_fade_ins_.find(clip.id);
    if (fi_it != clip_fade_ins_.end()) fade_in = fi_it->second;
    if (fade_in > 0 && clip_local < fade_in) {
        fade *= static_cast<float>(clip_local / fade_in);
    }

    // Fade out
    float fade_out = clip.audio.fade_out_seconds;
    auto fo_it = clip_fade_outs_.find(clip.id);
    if (fo_it != clip_fade_outs_.end()) fade_out = fo_it->second;
    if (fade_out > 0 && (clip_duration - clip_local) < fade_out) {
        fade *= static_cast<float>((clip_duration - clip_local) / fade_out);
    }

    return std::clamp(fade, 0.0f, 1.0f);
}

void AudioMixer::apply_pan(float* left, float* right, float pan) const {
    // Equal-power panning
    float angle = (pan + 1.0f) * 0.25f * 3.14159265f;
    float l_gain = std::cos(angle);
    float r_gain = std::sin(angle);
    float mono = (*left + *right) * 0.5f;
    *left = mono * l_gain;
    *right = mono * r_gain;
}

int32_t AudioMixer::render_audio(const TimelineModel* model,
                                  double position_seconds,
                                  float* buffer,
                                  int32_t num_frames,
                                  int32_t num_channels,
                                  int32_t sample_rate) {
    if (!model || !buffer || num_frames <= 0 || num_channels <= 0) return 0;

    std::memset(buffer, 0, num_frames * num_channels * sizeof(float));

    // Check if any track has solo enabled
    bool any_solo = false;
    const auto& tracks = model->tracks();
    for (size_t ti = 0; ti < tracks.size(); ++ti) {
        auto s_it = track_solos_.find(static_cast<int32_t>(ti));
        if (s_it != track_solos_.end() && s_it->second) { any_solo = true; break; }
        if (tracks[ti].audio.is_solo) { any_solo = true; break; }
    }

    for (size_t ti = 0; ti < tracks.size(); ++ti) {
        const auto& track = tracks[ti];
        int32_t track_idx = static_cast<int32_t>(ti);

        // Track mute/solo
        auto m_it = track_mutes_.find(track_idx);
        bool track_muted = (m_it != track_mutes_.end()) ? m_it->second : track.audio.is_muted;
        if (track_muted) continue;
        if (any_solo) {
            auto s_it = track_solos_.find(track_idx);
            bool track_solo = (s_it != track_solos_.end()) ? s_it->second : track.audio.is_solo;
            if (!track_solo) continue;
        }

        // Track volume and pan
        auto tv_it = track_volumes_.find(track_idx);
        float track_vol = (tv_it != track_volumes_.end()) ? tv_it->second : track.audio.volume;
        auto tp_it = track_pans_.find(track_idx);
        float track_pan = (tp_it != track_pans_.end()) ? tp_it->second : track.audio.pan;

        for (const auto& clip : track.clips) {
            if (clip.audio.is_muted) continue;
            if (clip.source_type != ClipSourceType::Video &&
                track.type != TrackType::Audio) {
                if (track.type != TrackType::Video) continue;
            }

            if (position_seconds < clip.timeline_range.in_time ||
                position_seconds >= clip.timeline_range.out_time) {
                continue;
            }

            IAudioDecoder* dec = get_decoder_for(clip.source_path);
            if (!dec) continue;

            double clip_local = position_seconds - clip.timeline_range.in_time;
            double source_time = clip.source_range.source_in + clip_local * clip.speed;

            float clip_vol = get_clip_volume(clip.id) * clip.audio.volume;
            float clip_pan_val = get_clip_pan(clip.id);
            if (clip_pan_val == 0) clip_pan_val = clip.audio.pan;
            float fade = compute_fade(clip, position_seconds);

            // Keyframe-animated volume
            float kf_volume = 1.0f;
            if (!clip.keyframes.tracks.empty()) {
                EvaluatedProperties kf = evaluate_keyframes(clip.keyframes, clip_local);
                kf_volume = static_cast<float>(kf.volume);
            }

            float final_vol = clip_vol * track_vol * fade * kf_volume;

            std::vector<float> clip_buf(num_frames * 2);
            int32_t decoded = dec->decode_audio_at(source_time, clip_buf.data(),
                                                    num_frames, sample_rate);
            if (decoded <= 0) continue;

            // Mix with volume, pan
            float combined_pan = std::clamp(clip_pan_val + track_pan, -1.0f, 1.0f);
            for (int32_t f = 0; f < decoded; ++f) {
                float left = clip_buf[f * 2] * final_vol;
                float right = clip_buf[f * 2 + 1] * final_vol;
                if (combined_pan != 0) apply_pan(&left, &right, combined_pan);

                if (num_channels >= 2) {
                    buffer[f * num_channels] += left;
                    buffer[f * num_channels + 1] += right;
                } else {
                    buffer[f] += (left + right) * 0.5f;
                }
            }
        }
    }

    // Clamp output
    for (int32_t i = 0; i < num_frames * num_channels; ++i) {
        buffer[i] = std::clamp(buffer[i], -1.0f, 1.0f);
    }

    return num_frames;
}

}  // namespace video
}  // namespace gopost
