
## VE-15. Media I/O and Codec Layer

### 15.1 Decoder Architecture

```cpp
namespace gp::media {

enum class DecoderType { Hardware, Software };

struct MediaInfo {
    // Container
    std::string format;          // "mp4", "mov", "mkv", "webm"
    Rational duration;
    int64_t file_size;

    // Video streams
    struct VideoStream {
        int index;
        std::string codec;       // "h264", "hevc", "vp9", "av1", "prores"
        int width, height;
        Rational frame_rate;
        Rational pixel_aspect;
        PixelFormat pixel_format;
        ColorSpace color_space;
        TransferFunction transfer;
        ColorPrimaries primaries;
        int bit_depth;           // 8, 10, 12
        int64_t bitrate;
        bool has_alpha;
        RotationAngle rotation;  // 0, 90, 180, 270 (mobile video)
    };

    // Audio streams
    struct AudioStream {
        int index;
        std::string codec;       // "aac", "opus", "flac", "pcm"
        int sample_rate;
        int channels;
        ChannelLayout layout;    // Mono, Stereo, 5.1, 7.1
        int bit_depth;
        int64_t bitrate;
    };

    std::vector<VideoStream> video_streams;
    std::vector<AudioStream> audio_streams;
    std::vector<SubtitleStream> subtitle_streams;
    std::unordered_map<std::string, std::string> metadata;   // Title, artist, etc.
};

class MediaDecoder {
public:
    static std::unique_ptr<MediaDecoder> open(const std::string& path, DecoderHints hints = {});

    MediaInfo info() const;

    // Video decode
    bool seek_video(Rational time);
    bool decode_video_frame(VideoFrame& out);
    bool decode_video_frame_at(Rational time, VideoFrame& out);  // Random access

    // Audio decode
    bool seek_audio(Rational time);
    bool decode_audio(AudioBuffer& out, int sample_count);

    // Thumbnail generation (fast seek + single frame decode)
    bool decode_thumbnail(Rational time, int max_width, VideoFrame& out);

    // Hardware acceleration status
    DecoderType active_decoder_type() const;
    std::string decoder_name() const;

private:
    AVFormatContext* fmt_ctx_;
    AVCodecContext* video_ctx_;
    AVCodecContext* audio_ctx_;
    HardwareAccelerator hw_accel_;
};

struct DecoderHints {
    bool prefer_hardware;          // Default true
    int thread_count;              // For software decode (0 = auto)
    PixelFormat output_format;     // RGBA8 or NV12
    bool enable_alpha;             // Decode alpha channel if available
};

struct VideoFrame {
    uint8_t* data[4];             // Plane pointers (RGBA or Y/UV planes)
    int linesize[4];              // Stride per plane
    int width, height;
    PixelFormat format;
    Rational pts;                  // Presentation timestamp
    bool keyframe;

    // GPU texture (if hardware-decoded)
    GpuTexture gpu_texture;
    bool is_on_gpu() const;
};

} // namespace gp::media
```

### 15.2 Hardware Acceleration Matrix

| Platform | HW Decode API | Codecs | HW Encode API | Codecs |
|---|---|---|---|---|
| iOS / macOS | VideoToolbox | H.264, HEVC, ProRes, VP9* | VideoToolbox | H.264, HEVC, ProRes |
| Android | MediaCodec | H.264, HEVC, VP9, AV1* | MediaCodec | H.264, HEVC |
| Windows (NVIDIA) | NVDEC (via FFmpeg) | H.264, HEVC, VP9, AV1 | NVENC | H.264, HEVC, AV1 |
| Windows (AMD) | D3D11VA | H.264, HEVC, VP9, AV1 | AMF | H.264, HEVC |
| Windows (Intel) | D3D11VA / QSV | H.264, HEVC, VP9, AV1 | QSV | H.264, HEVC, AV1 |

\* Device/OS version dependent

### 15.3 Encoder and Export Pipeline

```cpp
namespace gp::media {

struct ExportConfig {
    // Video
    std::string video_codec;       // "h264", "hevc", "prores", "vp9"
    int width, height;
    Rational frame_rate;
    int64_t video_bitrate;         // bps (0 = auto/CRF)
    int crf;                       // Constant Rate Factor (0–51, lower = better)
    std::string profile;           // "high", "main", "baseline"
    PixelFormat pixel_format;      // yuv420p, yuv422p10le, etc.
    bool hardware_encode;

    // Audio
    std::string audio_codec;       // "aac", "opus", "flac", "pcm_s16le"
    int sample_rate;
    int audio_channels;
    int64_t audio_bitrate;

    // Container
    std::string container;         // "mp4", "mov", "mkv", "webm"

    // HDR metadata (if applicable)
    HDRMetadata* hdr_metadata;

    // Range
    TimeRange export_range;        // Full timeline or sub-range
};

// Preset system
struct ExportPreset {
    std::string id;
    std::string name;
    std::string description;
    ExportConfig config;
};

// Built-in presets
const std::vector<ExportPreset> BUILT_IN_PRESETS = {
    {"social_instagram_reel",   "Instagram Reel",        "1080x1920, H.264, 30fps", {...}},
    {"social_instagram_post",   "Instagram Post",        "1080x1080, H.264, 30fps", {...}},
    {"social_tiktok",           "TikTok",                "1080x1920, H.264, 60fps", {...}},
    {"social_youtube_1080",     "YouTube 1080p",         "1920x1080, H.264, 30fps", {...}},
    {"social_youtube_4k",       "YouTube 4K",            "3840x2160, HEVC, 30fps",  {...}},
    {"social_twitter",          "Twitter/X",             "1280x720, H.264, 30fps",  {...}},
    {"social_facebook",         "Facebook",              "1280x720, H.264, 30fps",  {...}},
    {"pro_prores_422",          "ProRes 422",            "Source res, ProRes, source fps", {...}},
    {"pro_prores_4444",         "ProRes 4444",           "Source res, ProRes, source fps", {...}},
    {"pro_dnxhd",               "DNxHD",                 "1920x1080, DNxHD, 30fps",  {...}},
    {"web_h264_high",           "Web H.264 High",        "1920x1080, H.264, CRF 18", {...}},
    {"web_h265_high",           "Web H.265 High",        "1920x1080, HEVC, CRF 20",  {...}},
    {"mobile_efficient",        "Mobile-Friendly",       "720x1280, H.264, CRF 23",  {...}},
    {"gif_animated",            "Animated GIF",          "480px max, 15fps, 256 colors", {...}},
    {"audio_only_aac",          "Audio Only (AAC)",      "AAC 256kbps",               {...}},
    {"audio_only_wav",          "Audio Only (WAV)",      "PCM 16-bit 48kHz",         {...}},
};

class ExportSession {
public:
    ExportSession(Timeline& timeline, const ExportConfig& config, const std::string& output_path);

    void start();
    void pause();
    void resume();
    void cancel();

    float progress() const;           // 0.0 – 1.0
    Rational current_time() const;
    int64_t estimated_remaining_ms() const;
    int64_t output_file_size() const;
    ExportState state() const;         // Idle, Encoding, Paused, Complete, Error, Cancelled

    // Callbacks
    Signal<float> on_progress;
    Signal<> on_complete;
    Signal<ErrorCode, std::string> on_error;

private:
    void encode_loop();
    void encode_video_frame(const VideoFrame& frame);
    void encode_audio_chunk(const AudioBuffer& buffer);
    void write_trailer();

    AVFormatContext* output_ctx_;
    AVCodecContext* video_enc_ctx_;
    AVCodecContext* audio_enc_ctx_;
    HardwareAccelerator hw_accel_;
    std::atomic<ExportState> state_;
    std::atomic<float> progress_;
    std::thread encode_thread_;
};

} // namespace gp::media
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Core Foundation |
| **Sprint(s)** | VE-Sprint 3 (Weeks 5-6) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [14-color-science](14-color-science.md) |
| **Successor** | [16-project-serialization](16-project-serialization.md) |
| **Story Points Total** | 85 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-241 | As a C++ engine developer, I want MediaDecoder::open with format detection so that any supported file can be opened | - open() accepts path and optional DecoderHints<br/>- Auto-detects container format via FFmpeg<br/>- Returns nullptr on failure with error code | 3 | P0 | — |
| VE-242 | As a C++ engine developer, I want MediaInfo extraction (streams, codecs, resolution, fps) so that media properties are known before decode | - info() returns MediaInfo struct<br/>- Video/audio/subtitle stream metadata<br/>- Duration, file size, metadata map | 5 | P0 | VE-241 |
| VE-243 | As a C++ engine developer, I want video seek (keyframe-accurate) so that random access to frames works | - seek_video(Rational time) positions decoder<br/>- Keyframe-accurate seek<br/>- Decode next keyframe then advance to target | 5 | P0 | VE-241 |
| VE-244 | As a C++ engine developer, I want video frame decode (to VideoFrame) so that frames can be rendered | - decode_video_frame() fills VideoFrame<br/>- decode_video_frame_at() for random access<br/>- RGBA or NV12 output format | 5 | P0 | VE-243 |
| VE-245 | As a C++ engine developer, I want audio decode (to AudioBuffer) so that audio can be mixed and played | - decode_audio() fills AudioBuffer<br/>- seek_audio() for positioning<br/>- PCM output at target sample rate | 5 | P0 | VE-241 |
| VE-246 | As a C++ engine developer, I want hardware decoder: VideoToolbox (iOS/macOS) so that decode is efficient on Apple devices | - VTDecompressionSession integration<br/>- H.264, HEVC, ProRes support<br/>- Zero-copy to GPU texture where possible | 8 | P0 | VE-244 |
| VE-247 | As a C++ engine developer, I want hardware decoder: MediaCodec (Android) so that decode is efficient on Android | - AMediaCodec integration<br/>- H.264, HEVC, VP9 support<br/>- Surface output for GPU interop | 8 | P0 | VE-244 |
| VE-248 | As a C++ engine developer, I want hardware decoder: NVDEC/D3D11VA (Windows) so that decode is efficient on Windows | - NVDEC or D3D11VA via FFmpeg<br/>- H.264, HEVC, VP9, AV1 support<br/>- GPU texture output | 8 | P0 | VE-244 |
| VE-249 | As a C++ engine developer, I want thumbnail decode (fast seek + single frame) so that media browser is responsive | - decode_thumbnail() with max_width<br/>- Fast seek to approximate time<br/>- Single frame decode, scaled | 3 | P1 | VE-243 |
| VE-250 | As a C++ engine developer, I want DecoderHints (prefer HW, thread count, output format) so that decode behavior is configurable | - prefer_hardware, thread_count, output_format<br/>- enable_alpha for alpha channel<br/>- Hints passed to decoder selection | 2 | P1 | VE-241 |
| VE-251 | As a C++ engine developer, I want ExportConfig data model (codec, resolution, bitrate, CRF) so that export settings are structured | - ExportConfig struct with all fields<br/>- Video/audio/container/HDR metadata<br/>- TimeRange for export range | 3 | P0 | — |
| VE-252 | As a C++ engine developer, I want ExportPreset system (15+ built-in presets) so that common export targets are one-click | - social_instagram_reel, tiktok, youtube, etc.<br/>- pro_prores_422, pro_prores_4444<br/>- web_h264_high, mobile_efficient, audio_only | 5 | P0 | VE-251 |
| VE-253 | As a C++ engine developer, I want ExportSession (start/pause/resume/cancel) so that export can be controlled | - start(), pause(), resume(), cancel()<br/>- State machine: Idle, Encoding, Paused, Complete, Error<br/>- Thread-safe operations | 5 | P0 | VE-251 |
| VE-254 | As a C++ engine developer, I want ExportSession progress tracking and callbacks so that UI can show export status | - progress() 0.0–1.0<br/>- on_progress, on_complete, on_error signals<br/>- estimated_remaining_ms, output_file_size | 3 | P0 | VE-253 |
| VE-255 | As a C++ engine developer, I want hardware encoder: VideoToolbox so that export is fast on Apple devices | - VTCompressionSession for H.264, HEVC, ProRes<br/>- Hardware encode when available<br/>- Fallback to software | 5 | P0 | VE-253 |
| VE-256 | As a C++ engine developer, I want hardware encoder: MediaCodec so that export is fast on Android | - AMediaCodec for H.264, HEVC<br/>- Hardware encode when available<br/>- Fallback to software | 5 | P0 | VE-253 |
| VE-257 | As a C++ engine developer, I want hardware encoder: NVENC/AMF/QSV so that export is fast on Windows | - NVENC (NVIDIA), AMF (AMD), QSV (Intel)<br/>- Per-vendor encoder selection<br/>- Fallback to software | 5 | P0 | VE-253 |
| VE-258 | As a C++ engine developer, I want HDR metadata passthrough in export so that HDR exports preserve metadata | - HDR10, HLG metadata in output<br/>- Mastering display, content light level<br/>- Passthrough from source when applicable | 3 | P1 | VE-253 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
