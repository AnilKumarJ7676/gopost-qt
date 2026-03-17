
## VE-11. Audio Engine

### 11.1 Audio Graph Architecture

```cpp
namespace gp::audio {

// Audio processing uses a node-based graph similar to Web Audio API

enum class AudioNodeType {
    Source,         // Decoded audio from media
    Gain,           // Volume control
    Pan,            // Stereo panning
    Mixer,          // Mix multiple inputs to one output
    Equalizer,      // Parametric EQ
    Compressor,     // Dynamic range compression
    Reverb,         // Convolution/algorithmic reverb
    Delay,          // Delay/echo
    Limiter,        // Peak limiter
    NoiseGate,      // Noise gate
    Chorus,         // Chorus/flanger
    Distortion,     // Overdrive/distortion
    PitchShift,     // Pitch correction
    TimeStretch,    // Time stretch without pitch change
    Analyzer,       // FFT spectrum analyzer
    Meter,          // Level meter (RMS, peak, LUFS)
    Output,         // Final output bus
};

struct AudioBuffer {
    float* data;                // Interleaved or planar float samples
    int32_t channel_count;
    int32_t sample_count;
    int32_t sample_rate;
    bool planar;                // Planar (per-channel) vs interleaved

    float peak() const;
    float rms() const;
    void apply_gain(float gain);
    void mix_into(AudioBuffer& dest, float gain = 1.0f) const;
};

class AudioNode {
public:
    virtual ~AudioNode() = default;
    virtual AudioNodeType type() const = 0;
    virtual void process(const AudioBuffer& input, AudioBuffer& output,
                        int32_t sample_count, Rational time) = 0;
    virtual void reset() = 0;

    std::vector<AudioNode*> inputs;
    std::vector<std::pair<std::string, ParameterValue>> parameters;
};

class AudioGraph {
public:
    int32_t add_node(std::unique_ptr<AudioNode> node);
    void connect(int32_t from, int32_t to, int32_t output_channel = 0, int32_t input_channel = 0);
    void disconnect(int32_t from, int32_t to);
    void remove_node(int32_t id);

    void process(AudioBuffer& output, int32_t sample_count, Rational time);

    // Real-time analysis
    float get_peak_level(int32_t node_id, int channel) const;
    float get_rms_level(int32_t node_id, int channel) const;
    float get_lufs(int32_t node_id) const;
    std::vector<float> get_spectrum(int32_t node_id, int fft_size = 2048) const;
    std::vector<float> get_waveform(int32_t node_id, int sample_count) const;

private:
    std::vector<std::unique_ptr<AudioNode>> nodes_;
    std::vector<AudioConnection> connections_;
    std::vector<int32_t> processing_order_;   // Topological sort
};

} // namespace gp::audio
```

### 11.2 Audio Effects

| Effect | Parameters | Description |
|---|---|---|
| **Parametric EQ** | bands[8] (frequency, gain, Q, type: peak/shelf/notch/highpass/lowpass) | 8-band parametric equalizer |
| **Compressor** | threshold, ratio, attack, release, knee, makeup_gain | Dynamic range compression |
| **Limiter** | ceiling, release, lookahead | Brick-wall peak limiter |
| **Reverb** | room_size, damping, wet/dry, pre_delay, stereo_width | Algorithmic reverb |
| **Convolution Reverb** | impulse_response (media ref), wet/dry, pre_delay | IR-based reverb |
| **Delay** | delay_time, feedback, wet/dry, stereo, sync_to_tempo | Delay / echo |
| **Chorus** | rate, depth, mix, voices, stereo_width | Chorus / flanger |
| **Noise Gate** | threshold, attack, hold, release, range | Remove background noise |
| **Noise Reduction** | reduction_amount, sensitivity, frequency_smoothing | AI-assisted noise profile subtraction |
| **De-esser** | frequency, threshold, range | Reduce sibilance |
| **Pitch Shift** | semitones (-24 to +24), cents, preserve_formants | Pitch without speed change |
| **Time Stretch** | rate (0.25 to 4.0), preserve_pitch | Speed without pitch change (phase vocoder) |
| **Bass Boost** | frequency, amount | Low frequency enhancement |
| **Vocal Enhancer** | presence, clarity, warmth | AI-enhanced vocal processing |
| **Volume Envelope** | keyframe-based volume automation | Precise fade in/out control |

### 11.3 Waveform and Spectrum Visualization

```cpp
namespace gp::audio {

struct WaveformData {
    std::vector<float> min_samples;     // Per-pixel minimum amplitude
    std::vector<float> max_samples;     // Per-pixel maximum amplitude
    std::vector<float> rms_samples;     // Per-pixel RMS
    int samples_per_pixel;
    int source_sample_rate;

    static WaveformData generate(const AudioBuffer& buffer,
                                 int target_width,
                                 Rational visible_range_start,
                                 Rational visible_range_end);
};

struct SpectrumData {
    std::vector<float> magnitudes;   // dB values for each frequency bin
    int fft_size;
    float frequency_resolution;       // Hz per bin

    static SpectrumData compute(const AudioBuffer& buffer,
                                int fft_size = 2048,
                                WindowFunction window = WindowFunction::Hann);
};

} // namespace gp::audio
```

### 11.4 Audio Sync and Beat Detection

```cpp
namespace gp::audio {

struct BeatInfo {
    Rational time;
    float strength;    // 0–1 beat intensity
    BeatType type;     // Downbeat, Beat, SubBeat
};

class BeatDetector {
public:
    std::vector<BeatInfo> detect(const AudioBuffer& buffer, int sample_rate);
    float estimate_bpm(const AudioBuffer& buffer, int sample_rate);

    // Generate snap points from beats for timeline editing
    std::vector<Rational> beat_snap_points(float bpm, Rational offset,
                                           Rational range_start, Rational range_end);
};

} // namespace gp::audio
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 4: Transitions/Text/Audio |
| **Sprint(s)** | VE-Sprint 12 (Weeks 23-24) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [10-text-typography](10-text-typography.md) |
| **Successor** | [12-transition-system](12-transition-system.md) |
| **Story Points Total** | 89 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-167 | As a C++ engine developer, I want AudioBuffer (interleaved/planar float, peak/rms) so that we have the core audio data structure | - float* data, channel_count, sample_count, sample_rate<br/>- planar flag for planar layout<br/>- peak(), rms(), apply_gain(), mix_into() | 3 | P0 | VE-031 |
| VE-168 | As a C++ engine developer, I want AudioNode abstract class with process/reset so that we have a uniform node interface | - process(input, output, sample_count, time)<br/>- reset() for stateful nodes<br/>- inputs, parameters | 3 | P0 | VE-167 |
| VE-169 | As a C++ engine developer, I want AudioGraph with add_node/connect/disconnect so that we can build audio graphs | - add_node returns node_id<br/>- connect(from, to, output_ch, input_ch)<br/>- disconnect, remove_node | 5 | P0 | VE-168 |
| VE-170 | As a C++ engine developer, I want AudioGraph topological sort and processing so that we process nodes in correct order | - Topological sort of nodes<br/>- process(output, sample_count, time) traverses graph<br/>- Handles cycles (error) | 5 | P0 | VE-169 |
| VE-171 | As a C++ engine developer, I want Source node (decoded audio from media) so that we can feed audio from timeline clips | - Source node type<br/>- Decoded audio from media_ref_id<br/>- Timeline time → source time mapping | 5 | P0 | VE-170 |
| VE-172 | As a C++ engine developer, I want Gain node so that we can control volume | - Gain node with gain parameter (0.0 - 2.0+)<br/>- process applies gain to input<br/>- Keyframeable | 2 | P0 | VE-168 |
| VE-173 | As a C++ engine developer, I want Pan node (stereo) so that we can pan mono/stereo audio | - Pan node: pan -1.0 (left) to 1.0 (right)<br/>- Stereo balance<br/>- Mono to stereo with pan | 3 | P0 | VE-168 |
| VE-174 | As a C++ engine developer, I want Mixer node (multi-input) so that we can mix multiple tracks | - Mixer node with N inputs<br/>- Sum or weighted mix<br/>- Per-input gain optional | 3 | P0 | VE-169 |
| VE-175 | As a C++ engine developer, I want Parametric EQ (8-band) so that we can equalize audio | - 8 bands: frequency, gain, Q, type (peak/shelf/notch/highpass/lowpass)<br/>- Biquad or similar filter implementation<br/>- Real-time processing | 8 | P0 | VE-168 |
| VE-176 | As a C++ engine developer, I want Compressor (threshold, ratio, attack, release, knee) so that we can compress dynamics | - threshold, ratio, attack, release, knee<br/>- Makeup gain<br/>- RMS or peak detection | 5 | P0 | VE-168 |
| VE-177 | As a C++ engine developer, I want Limiter (brick-wall peak) so that we can prevent clipping | - ceiling, release, lookahead<br/>- Brick-wall peak limiting<br/>- Transparent when under ceiling | 3 | P0 | VE-176 |
| VE-178 | As a C++ engine developer, I want Reverb (algorithmic) so that we can add reverb | - room_size, damping, wet/dry, pre_delay<br/>- Algorithmic (e.g., Schroeder) reverb<br/>- Stereo width | 5 | P1 | VE-168 |
| VE-179 | As a C++ engine developer, I want Delay/echo so that we can add delay effects | - delay_time, feedback, wet/dry<br/>- Stereo, sync_to_tempo optional<br/>- Feedback limiting | 3 | P0 | VE-168 |
| VE-180 | As a C++ engine developer, I want Noise gate so that we can gate quiet sections | - threshold, attack, hold, release, range<br/>- Gate below threshold<br/>- Smooth transitions | 3 | P1 | VE-168 |
| VE-181 | As a C++ engine developer, I want Pitch shift (preserve formants) so that we can change pitch without speed | - semitones (-24 to +24), cents<br/>- preserve_formants option<br/>- Phase vocoder or similar | 8 | P1 | VE-168 |
| VE-182 | As a C++ engine developer, I want Time stretch (phase vocoder) so that we can change speed without pitch | - rate (0.25 to 4.0)<br/>- Phase vocoder implementation<br/>- preserve_pitch | 8 | P1 | VE-168 |
| VE-183 | As a C++ engine developer, I want Real-time level metering (peak, RMS, LUFS) so that we can display levels | - get_peak_level, get_rms_level, get_lufs<br/>- Per node, per channel<br/>- Meter node or graph-level query | 5 | P0 | VE-170 |
| VE-184 | As a C++ engine developer, I want WaveformData, SpectrumData, and BeatDetector so that we can visualize audio and sync to beats | - WaveformData: min/max/rms per pixel for waveform display<br/>- SpectrumData: FFT magnitudes for spectrum display<br/>- BeatDetector: detect beats, estimate BPM, beat_snap_points for timeline | 8 | P0 | VE-167 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
