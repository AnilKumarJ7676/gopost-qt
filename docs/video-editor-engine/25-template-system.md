
## VE-25. Video Template System

### 25.1 Video Template Data Model

```cpp
namespace gp::vtemplates {

// A video template is a pre-designed timeline with replaceable media,
// text, audio, and configurable style controls. Unlike image templates,
// video templates are temporal: they have sections, beat-synced cuts,
// transitions between segments, and duration adaptation.

struct VideoTemplate {
    std::string template_id;            // UUID
    std::string name;
    std::string description;
    std::string category;               // "Social Media", "Promo", "Vlog", "Slideshow", etc.
    std::vector<std::string> tags;
    std::string author_id;

    // Timeline definition
    timeline::Timeline timeline;        // Full pre-built timeline
    Rational base_duration;             // Default duration before adaptation

    // Sections (semantic segments of the template)
    std::vector<TemplateSection> sections;

    // Replaceable elements
    std::vector<VideoPlaceholder> placeholders;

    // Style controls (exposed to end users)
    std::vector<VideoStyleControl> style_controls;

    // Color palette
    VideoTemplatePalette palette;

    // Music / audio
    TemplateMusicConfig music;

    // Target platform
    SocialPlatform target_platform;     // Instagram_Reel, TikTok, YouTube_Short, etc.
    int32_t width, height;
    Rational frame_rate;

    // Duration constraints
    float min_duration_seconds;
    float max_duration_seconds;
    bool auto_adapt_duration;           // Stretch/compress to fit user media

    // Metadata
    int version;
    std::string created_at;
    std::string updated_at;
    std::string preview_url;            // Preview video thumbnail
    std::string preview_video_url;      // Animated preview
    int usage_count;
    float rating;
    bool is_premium;
};

enum class SocialPlatform {
    Instagram_Reel,         // 1080x1920, 9:16, 15/30/60/90s
    Instagram_Story,        // 1080x1920, 9:16, 15s
    TikTok,                 // 1080x1920, 9:16, 15/60/180s
    YouTube_Short,          // 1080x1920, 9:16, ≤60s
    YouTube_Standard,       // 1920x1080, 16:9, any
    YouTube_4K,             // 3840x2160, 16:9, any
    Facebook_Reel,          // 1080x1920, 9:16, ≤90s
    Facebook_Story,         // 1080x1920, 9:16, 20s
    Facebook_Video,         // 1920x1080, 16:9, any
    LinkedIn_Video,         // 1920x1080, 16:9, ≤10min
    Twitter_Video,          // 1920x1080, 16:9, ≤140s
    Snapchat_Spotlight,     // 1080x1920, 9:16, ≤60s
    Pinterest_Idea,         // 1080x1920, 9:16, 15-60s
    WhatsApp_Status,        // 1080x1920, 9:16, 30s
    Custom,
};

} // namespace gp::vtemplates
```

### 25.2 Template Section System

```cpp
namespace gp::vtemplates {

// Templates are divided into semantic sections: Intro, Content, Transition, Outro.
// Sections can repeat (e.g., multiple "content" sections for a slideshow)
// and can stretch/compress based on user media.

enum class SectionType {
    Intro,              // Branded opening (logo reveal, title)
    ContentSlide,       // Single media + text overlay segment
    ContentMontage,     // Multi-clip rapid montage
    BRoll,              // B-roll/filler footage section
    TextOnly,           // Full-screen text/quote
    Transition,         // Decorative transition section between content
    CallToAction,       // CTA screen (subscribe, follow, link)
    Outro,              // Branded closing (logo, credits)
    LowerThird,         // Persistent lower-third bar
    Countdown,          // Timer/countdown section
    SplitScreen,        // Multi-panel split screen
};

struct TemplateSection {
    std::string id;
    std::string label;                  // "Intro", "Photo 1", "Closing"
    SectionType type;
    int32_t order;                      // Section sequence position

    // Timing
    Rational start_time;                // Start on master timeline
    Rational duration;                  // Section duration
    Rational min_duration;              // Cannot shrink below this
    Rational max_duration;              // Cannot stretch beyond this
    bool duration_locked;               // Fixed duration, no adaptation

    // Tracks and clips owned by this section
    std::vector<int32_t> clip_ids;      // Clips belonging to this section

    // Transitions at section boundaries
    std::string entry_transition_id;    // Transition when entering section
    std::string exit_transition_id;     // Transition when leaving section

    // Repetition rules (for variable-count content)
    bool repeatable;                    // Can duplicate this section for more media
    int min_repeats;
    int max_repeats;                    // -1 = unlimited

    // Beat synchronization
    bool beat_synced;                   // Align to music beats
    int beat_offset;                    // Which beat this section starts on
    int beat_count;                     // How many beats this section spans

    // Attached placeholders
    std::vector<std::string> placeholder_ids;
};

class TemplateSectionEngine {
public:
    // Reorder sections
    void reorder_sections(VideoTemplate& tmpl, const std::vector<std::string>& section_order);

    // Add/remove repeatable content sections
    TemplateSection duplicate_section(VideoTemplate& tmpl, const std::string& section_id);
    void remove_section(VideoTemplate& tmpl, const std::string& section_id);

    // Get sections by type
    std::vector<const TemplateSection*> sections_of_type(const VideoTemplate& tmpl,
                                                          SectionType type);

    // Compute section boundaries after duration changes
    void recompute_timing(VideoTemplate& tmpl);

    // Snap section boundaries to beat grid
    void snap_to_beats(VideoTemplate& tmpl, float bpm, Rational beat_offset);
};

} // namespace gp::vtemplates
```

### 25.3 Video Placeholder System

```cpp
namespace gp::vtemplates {

enum class VideoPlaceholderType {
    Media,          // Video or photo slot (user drops footage)
    Text,           // Editable title/subtitle/caption
    Audio,          // Music or voiceover slot
    Logo,           // Brand logo overlay
    ColorGrade,     // LUT/grade placeholder (user picks color mood)
    Sticker,        // Animated sticker/emoji slot
    Watermark,      // Persistent watermark overlay
    Voiceover,      // Voice recording slot
    Subtitle,       // Auto-caption / manual subtitle
};

struct VideoPlaceholder {
    std::string id;                     // "hero_video", "title_1", "background_music"
    std::string label;                  // User-facing: "Main Video", "Your Logo"
    VideoPlaceholderType type;
    std::string section_id;             // Which section this belongs to
    int32_t clip_id;                    // The clip this placeholder controls
    int32_t track_id;                   // Track this placeholder is on
    bool required;

    // Media placeholder specifics
    struct MediaConfig {
        enum AcceptType { VideoOnly, PhotoOnly, VideoOrPhoto };
        AcceptType accept;
        Rational min_duration;          // Minimum source duration
        float min_width, min_height;    // Minimum resolution
        float aspect_ratio;             // 0 = any
        FitMode default_fit;            // Fill, Fit, Letterbox, PanAndScan
        bool allow_trim;                // User can trim in/out
        bool allow_speed;               // User can adjust speed
        bool allow_filters;             // User can apply filters
        bool auto_face_crop;            // AI face-aware crop
        bool auto_motion;               // Auto Ken Burns / parallax
    } media_config;

    // Text placeholder specifics
    struct TextConfig {
        std::string default_text;
        std::string hint_text;
        int max_characters;
        int max_lines;
        bool allow_font_change;
        bool allow_color_change;
        bool allow_animation_change;
        std::vector<std::string> suggested_fonts;
        text::TextAnimatorGroup* default_animation;  // Pre-set text reveal animation
    } text_config;

    // Audio placeholder specifics
    struct AudioConfig {
        enum AudioType { Music, Voiceover, SoundEffect };
        AudioType audio_type;
        Rational max_duration;
        bool auto_duck;                 // Auto-duck under voiceover
        float default_volume;           // 0.0-1.0
        bool allow_trim;
        bool beat_sync_available;       // Can sync template cuts to this music
        std::vector<std::string> suggested_tracks;  // Bundled music IDs
    } audio_config;

    // Logo placeholder specifics
    struct LogoConfig {
        int max_width, max_height;
        bool preserve_aspect;
        Vec2 position;                  // Normalized 0-1
        float opacity;
        Rational display_start;         // When logo appears
        Rational display_duration;      // How long
        LogoAnimation animation;
    } logo_config;

    // Color grade placeholder specifics
    struct ColorGradeConfig {
        std::string default_lut_id;
        float default_intensity;        // 0-1
        std::vector<std::string> suggested_lut_ids;
        bool allow_manual_grading;      // User can adjust curves/lift-gamma-gain
    } color_grade_config;
};

enum class FitMode { Fill, Fit, Letterbox, PanAndScan, Stretch };
enum class LogoAnimation { None, FadeIn, SlideIn, ScaleIn, Bounce };

} // namespace gp::vtemplates
```

### 25.4 Video Style Controls

```cpp
namespace gp::vtemplates {

// High-level controls exposed to end users that affect
// multiple timeline elements at once.

enum class VideoStyleControlType {
    ColorGrade,         // Overall color mood (LUT + grade)
    FontStyle,          // Typography across all text placeholders
    TransitionStyle,    // Swap all transitions to a different set
    MusicMood,          // Swap background music to different mood
    PacingSpeed,        // Overall pacing (fast cuts vs slow)
    TextAnimation,      // Text reveal animation style
    OverlayStyle,       // Overlay/decoration style (grain, light leaks)
    AspectRatio,        // Switch 9:16 / 16:9 / 1:1 / 4:5
    Slider,             // Generic numeric control
    Toggle,             // Show/hide element groups
};

struct VideoStyleControl {
    std::string id;
    std::string label;
    VideoStyleControlType type;

    struct ColorGradeControl {
        std::string current_lut_id;
        float intensity;
        std::vector<std::string> lut_options;       // LUT IDs to choose from
        std::vector<std::string> lut_thumbnails;
    } color_grade;

    struct FontStyleControl {
        std::string current_heading_family;
        std::string current_body_family;
        std::vector<FontPairing> pairings;          // Pre-matched font pairs
        std::vector<int32_t> affected_text_clip_ids;
    } font_style;

    struct TransitionStyleControl {
        std::string current_transition_set;
        std::vector<TransitionSet> options;
    } transition_style;

    struct MusicMoodControl {
        std::string current_track_id;
        std::vector<MusicOption> options;
    } music_mood;

    struct PacingControl {
        float current_speed;            // 0.5 = slow, 1.0 = normal, 2.0 = fast
        float min_speed, max_speed;
        bool beat_sync;                 // Maintain beat sync when adjusting
    } pacing;

    struct TextAnimationControl {
        std::string current_preset_id;
        std::vector<TextAnimationOption> options;
        std::vector<int32_t> affected_text_clip_ids;
    } text_animation;

    struct OverlayStyleControl {
        std::string current_overlay_id;
        float intensity;
        std::vector<OverlayOption> options;
    } overlay_style;
};

struct FontPairing {
    std::string id;
    std::string name;
    std::string heading_family;
    std::string body_family;
    std::string thumbnail;
};

struct TransitionSet {
    std::string id;
    std::string name;                   // "Clean", "Glitch", "Smooth", "Cinematic"
    std::vector<std::string> transition_ids;  // The transitions in this set
    std::string thumbnail;
};

struct MusicOption {
    std::string track_id;
    std::string name;
    std::string mood;                   // "Upbeat", "Chill", "Epic", "Emotional"
    float bpm;
    Rational duration;
    std::string preview_url;
};

struct TextAnimationOption {
    std::string preset_id;
    std::string name;                   // "Typewriter", "Slide Up", "Fade In", "Bounce"
    std::string thumbnail;
    text::TextAnimatorGroup animator;
};

struct OverlayOption {
    std::string id;
    std::string name;                   // "Film Grain", "Light Leak", "Dust", "VHS"
    std::string thumbnail;
    std::vector<EffectInstance> effects;
};

} // namespace gp::vtemplates
```

### 25.5 Smart Duration Engine

```cpp
namespace gp::vtemplates {

// Automatically adapts template duration based on user's media.
// Handles variable clip counts, different source durations,
// and beat-synced timing adjustments.

class SmartDurationEngine {
public:
    struct DurationInput {
        int media_clip_count;           // How many clips user provided
        std::vector<Rational> clip_durations;  // Duration of each user clip
        Rational music_duration;        // Background music length
        float target_bpm;              // BPM of selected music (0 = no beat sync)
    };

    struct DurationPlan {
        Rational total_duration;
        std::vector<SectionTiming> section_timings;
        std::vector<CutPoint> cut_points;       // Where each clip is cut
        bool music_trimmed;                      // Did we need to trim music?
        Rational music_fade_out_start;           // Where music fade-out begins
    };

    struct SectionTiming {
        std::string section_id;
        Rational start;
        Rational duration;
        bool was_stretched;
        bool was_compressed;
    };

    struct CutPoint {
        std::string placeholder_id;
        Rational timeline_start;
        Rational timeline_duration;
        Rational source_in;             // Optimal in-point in user's clip
        Rational source_out;
    };

    // Compute optimal duration plan
    DurationPlan compute_plan(const VideoTemplate& tmpl, const DurationInput& input);

    // Apply duration plan to template
    void apply_plan(VideoTemplate& tmpl, const DurationPlan& plan);

    // Beat-synced cut timing: align cuts to beats
    std::vector<Rational> compute_beat_aligned_cuts(
        float bpm, Rational offset,
        int cut_count, Rational total_duration,
        BeatPattern pattern = BeatPattern::EveryBeat);

    enum class BeatPattern {
        EveryBeat,          // Cut on every beat
        EveryOtherBeat,     // Cut on every 2nd beat
        EveryBar,           // Cut every 4 beats
        Syncopated,         // Offbeat cuts
        Accelerating,       // Cuts get faster toward end
        Custom,
    };

    // Smart source trimming: pick best in/out points for each clip
    struct SmartTrimResult {
        Rational source_in;
        Rational source_out;
        float interest_score;           // How "interesting" this segment is
    };

    SmartTrimResult smart_trim(IGpuContext& gpu,
                                int32_t media_ref_id,
                                Rational target_duration);

    // Scene detection within a user clip: find the best segments
    struct SceneSegment {
        Rational start;
        Rational duration;
        float visual_interest;          // Saliency/motion score
        bool has_faces;
        bool has_motion;
    };

    std::vector<SceneSegment> detect_scenes(IGpuContext& gpu, int32_t media_ref_id);
};

} // namespace gp::vtemplates
```

### 25.6 Video Template Palette & Color Grade System

```cpp
namespace gp::vtemplates {

struct VideoTemplatePalette {
    std::string name;
    Color4f primary;
    Color4f secondary;
    Color4f accent;
    Color4f text_primary;
    Color4f text_secondary;
    Color4f background;

    // Associated color grading
    std::string lut_id;                 // Color grading LUT
    float lut_intensity;
    color::GradingParams grading;       // Lift/Gamma/Gain/Offset

    static std::vector<VideoTemplatePalette> built_in_palettes();
    static VideoTemplatePalette from_video(IGpuContext& gpu, int32_t media_ref_id);
    void apply_to_template(VideoTemplate& tmpl) const;
};

struct ColorGradePreset {
    std::string id;
    std::string name;
    std::string category;               // "Cinematic", "Vintage", "Bright", "Moody", "B&W"
    std::string lut_path;
    color::GradingParams grading;
    float default_intensity;
    std::string thumbnail;
};

class VideoColorGradeLibrary {
public:
    std::vector<ColorGradePreset> all_presets();
    std::vector<ColorGradePreset> by_category(const std::string& category);
    const ColorGradePreset* find(const std::string& id);

    // Apply grade to entire template (as adjustment layer on master)
    void apply_to_template(VideoTemplate& tmpl, const std::string& preset_id,
                            float intensity = 1.0f);

    // Categories:
    // - Cinematic (15 presets): Teal & Orange, Blockbuster, Film Noir, etc.
    // - Vintage (10 presets): Kodak, Fuji, 70s, 80s, Polaroid
    // - Bright & Clean (10 presets): Daylight, Pastel, Airy, High Key
    // - Moody & Dark (10 presets): Dark Drama, Night City, Gothic, Storm
    // - B&W (8 presets): Classic, High Contrast, Selenium, Infrared
    // - Warm (8 presets): Golden Hour, Amber, Autumn, Sunset
    // - Cool (8 presets): Ice, Moonlight, Cyan Shift, Arctic
    // - Social (10 presets): Instagram-style, TikTok trend, VSCO
};

} // namespace gp::vtemplates
```

### 25.7 Music & Audio Template System

```cpp
namespace gp::vtemplates {

struct TemplateMusicConfig {
    // Primary background music
    std::string primary_track_id;       // Bundled music asset
    Rational music_start;               // In-point in music
    float bpm;                          // Detected BPM
    float volume;                       // 0.0-1.0

    // Beat map (pre-analyzed beat positions)
    std::vector<Rational> beat_times;
    std::vector<Rational> downbeat_times;
    std::vector<Rational> bar_times;

    // Music sections
    struct MusicSection {
        std::string label;              // "Buildup", "Drop", "Verse", "Chorus", "Outro"
        Rational start;
        Rational duration;
        float energy;                   // 0-1 energy level
    };
    std::vector<MusicSection> music_sections;

    // Audio ducking (auto-lower music under voiceover/dialogue)
    bool auto_duck;
    float duck_level;                   // How much to duck (0.0-1.0, default 0.3)
    float duck_attack_ms;
    float duck_release_ms;

    // Fade configuration
    float fade_in_duration_ms;
    float fade_out_duration_ms;

    // Sound effects (attached to transitions or sections)
    struct SoundEffect {
        std::string sfx_id;
        Rational time;                  // Timeline placement
        float volume;
        std::string trigger;            // "transition", "text_appear", "beat"
    };
    std::vector<SoundEffect> sound_effects;
};

class TemplateMusicEngine {
public:
    // Analyze music and build beat map
    TemplateMusicConfig analyze_music(int32_t audio_media_ref_id);

    // Swap music: re-sync template cuts to new music's beat map
    void swap_music(VideoTemplate& tmpl, int32_t new_music_media_ref_id);

    // Auto-duck: configure ducking around voiceover placeholder
    void configure_auto_duck(VideoTemplate& tmpl,
                              const std::string& voiceover_placeholder_id);

    // Music library browsing
    struct MusicTrack {
        std::string id;
        std::string title;
        std::string artist;
        std::string genre;
        std::string mood;
        float bpm;
        Rational duration;
        std::string preview_url;
        std::string asset_url;
        bool is_premium;
        std::string license;
    };

    std::vector<MusicTrack> search_music(const std::string& query,
                                          const std::string& mood = "",
                                          float min_bpm = 0, float max_bpm = 300);
    std::vector<MusicTrack> trending_music(int limit = 20);
    std::vector<MusicTrack> music_for_template(const VideoTemplate& tmpl, int limit = 10);

    // AI music matching: suggest best music for user's footage
    std::vector<MusicTrack> smart_match(IGpuContext& gpu,
                                         const std::vector<int32_t>& media_refs,
                                         int limit = 5);
};

} // namespace gp::vtemplates
```

### 25.8 Template Creator Studio (Video)

```cpp
namespace gp::vtemplates {

class VideoTemplateCreatorStudio {
public:
    // Authoring lifecycle
    VideoTemplate create_new(const std::string& name, SocialPlatform platform);
    VideoTemplate import_from_project(const std::string& gpproj_path);
    VideoTemplate fork_template(const std::string& source_template_id);

    // Section management
    TemplateSection add_section(VideoTemplate& tmpl, SectionType type,
                                 const std::string& label, Rational duration);
    void remove_section(VideoTemplate& tmpl, const std::string& section_id);
    void reorder_sections(VideoTemplate& tmpl, const std::vector<std::string>& order);

    // Placeholder authoring
    VideoPlaceholder promote_clip_to_placeholder(VideoTemplate& tmpl,
                                                  int32_t clip_id,
                                                  VideoPlaceholderType type,
                                                  const std::string& label);
    void demote_placeholder(VideoTemplate& tmpl, const std::string& placeholder_id);

    // Constraint editing
    struct PlaceholderConstraints {
        bool lock_position;
        bool lock_duration;
        bool lock_effects;
        Rational min_duration, max_duration;
        int min_characters, max_characters;     // For text
        float min_volume, max_volume;           // For audio
        bool allow_trim;
        bool allow_speed_change;
        bool allow_filter_change;
    };
    void set_constraints(VideoTemplate& tmpl, const std::string& placeholder_id,
                          const PlaceholderConstraints& constraints);

    // Preview mode
    void enter_preview_mode(VideoTemplate& tmpl);
    void exit_preview_mode(VideoTemplate& tmpl);

    // Validation
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> tips;
    };
    ValidationResult validate(const VideoTemplate& tmpl);

    // Packaging
    bool package(const VideoTemplate& tmpl, const std::string& output_path,
                 const EncryptionKey& key);

    // Preview generation
    struct PreviewSet {
        std::string preview_video_path;         // 15s animated preview
        GpuTexture cover_thumbnail;             // 1080×1920 still
        std::vector<GpuTexture> frame_thumbnails;  // Key frames
    };
    PreviewSet generate_previews(IGpuContext& gpu, const VideoTemplate& tmpl);
};

} // namespace gp::vtemplates
```

### 25.9 Template Encryption (.gpvt Format)

```
.gpvt structure (Encrypted Video Template):

┌──────────────────────────────────────┐
│ Header                                │
│   Magic: "GPVT" (4 bytes)            │
│   Version: uint32                     │
│   Flags: uint32                       │
│   Template ID: UUID (16 bytes)        │
│   Content Hash: SHA-256 (32 bytes)    │
│   Platform: uint8 (SocialPlatform)    │
│   Duration: uint32 (ms)              │
└──────────────────────────────────────┘
│ Encrypted Payload (AES-256-GCM)       │
│   ├─ Timeline Settings               │
│   ├─ Track Tree (MessagePack)         │
│   ├─ Clip Definitions                │
│   ├─ Section Definitions             │
│   ├─ Placeholder Definitions          │
│   ├─ Style Controls                   │
│   ├─ Music Config + Beat Map          │
│   ├─ Palette + Color Grade            │
│   ├─ Embedded Assets                  │
│   │   ├─ Music tracks (compressed)   │
│   │   ├─ Sound effects               │
│   │   ├─ Fonts                       │
│   │   ├─ LUTs                        │
│   │   ├─ Overlay textures            │
│   │   └─ Logo/sticker assets         │
│   └─ Preview thumbnails              │
└──────────────────────────────────────┘
│ Signature (Ed25519)                   │
└──────────────────────────────────────┘
```

### 25.10 AI Video Template Generation

```cpp
namespace gp::vtemplates {

class AIVideoTemplateGenerator {
public:
    // Text-to-video-template: describe what you want
    struct GenerationPrompt {
        std::string description;            // "TikTok product reveal with upbeat music"
        SocialPlatform platform;
        std::string mood;                   // "Energetic", "Calm", "Dramatic", "Fun"
        std::string style;                  // "Minimal", "Bold", "Cinematic", "Trendy"
        int media_slot_count;               // How many media slots (0 = auto)
        float target_duration_seconds;      // 0 = platform default
        std::string music_mood;             // "Upbeat", "Chill", "Epic"
        int num_variations;                 // 1-4
        float creativity;                   // 0.0-1.0
    };

    struct GeneratedVideoTemplate {
        VideoTemplate tmpl;
        float confidence;
        std::string rationale;
        std::vector<std::string> design_principles;
    };

    std::vector<GeneratedVideoTemplate> generate(IGpuContext& gpu,
                                                   const GenerationPrompt& prompt);

    // Auto-edit: create a template from raw footage + music
    struct AutoEditInput {
        std::vector<int32_t> media_refs;    // User's raw clips
        int32_t music_ref;                  // Music track (-1 = auto-select)
        SocialPlatform platform;
        std::string style;
        float target_duration_seconds;
    };

    VideoTemplate auto_edit(IGpuContext& gpu, const AutoEditInput& input);

    // AI scene analysis: detect best moments in user's footage
    struct SceneAnalysis {
        struct Moment {
            int32_t media_ref_id;
            Rational time;
            Rational duration;
            float interest_score;           // 0-1
            std::string description;        // "person smiling", "scenic view"
            bool has_faces;
            bool has_action;
            bool has_text;
        };
        std::vector<Moment> best_moments;
        float overall_quality;
    };

    SceneAnalysis analyze_footage(IGpuContext& gpu,
                                   const std::vector<int32_t>& media_refs);

    // AI layout suggestions for video templates
    struct LayoutSuggestion {
        std::vector<TemplateSection> sections;
        float aesthetic_score;
        std::string layout_name;            // "Montage", "Slideshow", "Story Arc"
    };

    std::vector<LayoutSuggestion> suggest_layouts(
        const VideoTemplate& tmpl, int num_suggestions = 3);

    // AI text suggestions for title/caption placeholders
    struct TextSuggestion {
        std::string placeholder_id;
        std::string suggested_text;
        float relevance_score;
    };

    std::vector<TextSuggestion> suggest_text(
        const VideoTemplate& tmpl, const std::string& context);

    // AI color grade suggestion based on footage mood
    std::vector<ColorGradePreset> suggest_grades(
        IGpuContext& gpu, const std::vector<int32_t>& media_refs, int count = 5);

    // AI music matching to footage energy
    std::vector<MusicTrack> suggest_music(
        IGpuContext& gpu, const std::vector<int32_t>& media_refs, int count = 5);
};

} // namespace gp::vtemplates
```

### 25.11 Video Brand Kit

```cpp
namespace gp::vtemplates {

struct VideoBrandKit {
    std::string kit_id;
    std::string name;
    std::string owner_id;

    // Logos (multiple variants)
    struct BrandLogo {
        std::string id;
        std::string label;              // "Primary", "Icon", "Animated"
        std::string asset_url;
        bool is_animated;               // Video/GIF logo
        Rational animation_duration;
        Color4f background_hint;
    };
    std::vector<BrandLogo> logos;

    // Branded intro/outro video clips
    struct BrandedSegment {
        std::string id;
        std::string label;              // "Standard Intro", "Short Intro", "Outro"
        SectionType type;               // Intro or Outro
        std::string video_asset_url;
        Rational duration;
        std::vector<VideoPlaceholder> placeholders;  // Editable elements within
    };
    std::vector<BrandedSegment> branded_segments;

    // Lower third templates
    struct LowerThird {
        std::string id;
        std::string name;
        std::string style;              // "Clean", "Bold", "Animated"
        std::vector<timeline::Clip> clip_template;  // Pre-built lower-third clips
        std::vector<VideoPlaceholder> placeholders;  // Name, title, etc.
    };
    std::vector<LowerThird> lower_thirds;

    // Brand color palette
    VideoTemplatePalette palette;

    // Brand typography
    struct BrandTypography {
        std::string heading_family;
        std::string body_family;
        Color4f text_primary;
        Color4f text_secondary;
    } typography;

    // Brand color grade
    std::string brand_lut_id;
    float brand_grade_intensity;

    // Brand sound (audio sting, jingle)
    struct BrandAudio {
        std::string id;
        std::string label;              // "Intro Jingle", "Transition Sound", "Outro Sting"
        std::string asset_url;
        Rational duration;
    };
    std::vector<BrandAudio> audio_assets;

    // Watermark
    struct BrandWatermark {
        std::string asset_url;
        Vec2 position;
        float opacity;
        float scale;
    } watermark;
};

class VideoBrandKitManager {
public:
    VideoBrandKit create_kit(const std::string& name);
    VideoBrandKit get_kit(const std::string& kit_id);
    void update_kit(const VideoBrandKit& kit);
    void delete_kit(const std::string& kit_id);

    // Apply brand to video template
    void apply_to_template(VideoTemplate& tmpl, const VideoBrandKit& kit,
                            BrandApplyMode mode = BrandApplyMode::Full);

    enum class BrandApplyMode {
        Full,               // Intro + outro + lower thirds + grade + logo + watermark
        IntroOutroOnly,
        GradeOnly,
        LogoWatermarkOnly,
        Selective,
    };

    struct BrandApplyOptions {
        bool apply_intro;
        bool apply_outro;
        bool apply_lower_thirds;
        bool apply_color_grade;
        bool apply_logo;
        bool apply_watermark;
        bool apply_typography;
        bool apply_audio_sting;
    };
    void apply_selective(VideoTemplate& tmpl, const VideoBrandKit& kit,
                          const BrandApplyOptions& options);

    // Brand audit
    struct BrandAuditResult {
        bool consistent;
        std::vector<std::string> violations;
        std::vector<std::string> suggestions;
    };
    BrandAuditResult audit_template(const VideoTemplate& tmpl, const VideoBrandKit& kit);
};

} // namespace gp::vtemplates
```

### 25.12 Animation & Kinetic Typography Presets

```cpp
namespace gp::vtemplates {

// Pre-built animation presets for text reveals, transitions between
// text blocks, and kinetic typography sequences.

struct TextRevealPreset {
    std::string id;
    std::string name;
    std::string category;               // "Basic", "Kinetic", "Glitch", "Elegant", "Bold"
    text::TextAnimatorGroup animator;
    Rational duration;
    std::string thumbnail;
    bool supports_per_character;        // True = animates per character/word
};

struct KineticTypographyPreset {
    std::string id;
    std::string name;
    std::string style;                  // "Lyric Video", "Quote", "Headline Reveal"

    // Full sequence: multiple text blocks with coordinated animations
    struct TextBlock {
        std::string placeholder_id;
        Rational appear_time;
        Rational duration;
        text::TextAnimatorGroup entrance;
        text::TextAnimatorGroup emphasis;
        text::TextAnimatorGroup exit;
        Transform2D start_transform;
        Transform2D end_transform;
    };
    std::vector<TextBlock> blocks;

    // Camera movement during sequence
    struct CameraMove {
        Rational start_time;
        Rational duration;
        Transform2D from;
        Transform2D to;
        animation::InterpolationType easing;
    };
    std::vector<CameraMove> camera_moves;
};

class AnimationPresetLibrary {
public:
    // Text reveal presets (80+)
    std::vector<TextRevealPreset> text_reveal_presets();
    std::vector<TextRevealPreset> text_reveals_by_category(const std::string& category);

    // Kinetic typography presets (30+)
    std::vector<KineticTypographyPreset> kinetic_presets();

    // Apply to template
    void apply_text_reveal(VideoTemplate& tmpl, const std::string& text_placeholder_id,
                            const std::string& preset_id);
    void apply_kinetic_sequence(VideoTemplate& tmpl, const std::string& section_id,
                                 const std::string& preset_id);

    // Preset categories:
    // Text Reveals - Basic: Fade In, Slide Up, Slide Left, Scale Up, Drop In (10)
    // Text Reveals - Kinetic: Typewriter, Bounce In, Elastic, Spring, Letter Pop (15)
    // Text Reveals - Glitch: RGB Split, Pixel, Digital Noise, Scan Line (10)
    // Text Reveals - Elegant: Wipe, Mask Reveal, Unfold, Grow Line (10)
    // Text Reveals - Bold: Slam, Shake, Stomp, Flash, Zoom Punch (10)
    // Text Reveals - Social: TikTok Style, Reel Style, Subtitle Pop (10)
    // Text Reveals - 3D: Rotate In, Flip, Perspective (8)
    // Kinetic Typography: Lyric Video, Quote Reveal, Word-by-Word,
    //                     Headline Stack, Split Screen Text, Counter (30)
};

} // namespace gp::vtemplates
```

### 25.13 Data-Driven Video Templates (Batch Personalization)

```cpp
namespace gp::vtemplates {

// Bulk-generate personalized videos from a single template
// and external data (CSV, JSON). Each row produces a unique video.

struct VideoDataField {
    std::string column_name;
    std::string placeholder_id;
    VideoFieldType type;
};

enum class VideoFieldType {
    Text,               // Direct text replacement
    MediaUrl,           // Video/image URL to download
    MediaPath,          // Local file path
    AudioUrl,           // Music/voiceover URL
    Color,              // Hex color string
    LogoUrl,            // Logo image URL
    Boolean,            // Show/hide toggle
    Number,             // Formatted number for text
    Duration,           // Override section duration
};

struct VideoDataSource {
    enum Format { CSV, JSON, GoogleSheets, APIEndpoint };
    Format format;
    std::string path_or_url;
    std::vector<VideoDataField> field_mappings;
    int row_count;
};

struct VideoBatchConfig {
    VideoTemplate tmpl;
    VideoDataSource data;

    // Export settings
    struct ExportSettings {
        int width, height;
        Rational frame_rate;
        std::string codec;              // "h264", "h265", "vp9"
        int quality;                    // 1-100
        std::string container;          // "mp4", "mov", "webm"
    } export_settings;

    std::string output_directory;
    std::string filename_pattern;       // "video_{row}_{name}"

    int start_row, end_row;             // -1 = all
    bool skip_errors;
    int max_concurrent;                 // Parallel render threads
};

struct VideoBatchResult {
    int total_rows;
    int successful;
    int failed;
    int skipped;
    float total_duration_ms;
    std::vector<std::string> output_paths;
    std::vector<std::string> error_messages;
};

class DataDrivenVideoEngine {
public:
    VideoDataSource parse_data_source(const std::string& path,
                                       VideoDataSource::Format format);
    std::vector<VideoDataField> auto_map_fields(const VideoDataSource& data,
                                                 const VideoTemplate& tmpl);

    // Preview single row
    void preview_row(IGpuContext& gpu, const VideoTemplate& tmpl,
                      const VideoDataSource& data, int row_index,
                      std::function<void(GpuTexture frame, Rational time)> frame_callback);

    // Execute batch render
    VideoBatchResult execute(IGpuContext& gpu, const VideoBatchConfig& config,
                              std::function<void(int current, int total, float percent)> progress);

    void cancel();

    // Validation
    struct DataValidation {
        bool valid;
        std::vector<std::string> missing_required;
        std::vector<std::string> type_mismatches;
        std::vector<int> rows_with_errors;
    };
    DataValidation validate(const VideoBatchConfig& config);

private:
    std::atomic<bool> cancelled_{false};
};

} // namespace gp::vtemplates
```

### 25.14 Smart Resize / Multi-Platform Adaptation

```cpp
namespace gp::vtemplates {

struct VideoPlatformTarget {
    std::string name;                   // "Instagram Reel", "YouTube Short"
    SocialPlatform platform;
    int width, height;
    float aspect_ratio;
    Rational max_duration;
    Rect safe_zone;                     // Normalized 0-1, avoids platform UI
};

struct VideoResizeResult {
    VideoPlatformTarget target;
    VideoTemplate resized_template;
    float fidelity_score;
    std::vector<std::string> warnings;
};

class VideoSmartResizeEngine {
public:
    static std::vector<VideoPlatformTarget> all_targets();

    // Resize to single target
    VideoResizeResult resize(IGpuContext& gpu,
                              const VideoTemplate& source,
                              const VideoPlatformTarget& target);

    // Batch resize to multiple platforms
    std::vector<VideoResizeResult> resize_batch(
        IGpuContext& gpu,
        const VideoTemplate& source,
        const std::vector<VideoPlatformTarget>& targets,
        std::function<void(int current, int total)> progress);

    // Resize strategy
    struct ResizeStrategy {
        bool auto_reframe;              // AI subject tracking to keep subjects centered
        bool allow_crop;                // Crop for different aspect
        bool allow_pillarbox;           // Add black/blurred bars
        bool blur_background_fill;      // Blurred copy of video as background fill
        bool preserve_text_safe_zone;   // Keep text within safe area
        bool auto_resize_text;          // Scale text for readability
        float min_text_scale;
        bool adapt_duration;            // Trim to platform max duration
    };

    void set_strategy(const ResizeStrategy& strategy);

    // AI auto-reframe: track subject and compute pan/zoom per frame
    struct ReframeTrack {
        std::vector<Rect> subject_rects;    // Per-frame subject position
        std::vector<Transform2D> transforms; // Per-frame camera transform
    };

    ReframeTrack compute_auto_reframe(IGpuContext& gpu,
                                       int32_t media_ref_id,
                                       float source_aspect,
                                       float target_aspect);

private:
    ResizeStrategy strategy_;
};

} // namespace gp::vtemplates
```

### 25.15 Auto-Edit Engine

```cpp
namespace gp::vtemplates {

// Fully automatic video editing from raw footage.
// Combines scene detection, beat matching, highlight detection,
// and AI pacing to produce a polished edit.

class AutoEditEngine {
public:
    struct AutoEditConfig {
        std::vector<int32_t> media_refs;    // Raw footage clips
        int32_t music_ref;                  // Music (-1 = auto select)
        SocialPlatform platform;
        float target_duration_seconds;      // 0 = auto
        std::string style;                  // "Fast Cuts", "Cinematic", "Chill", "Dramatic"
        std::string mood;

        // Control parameters
        float cut_frequency;                // 0 = few cuts, 1 = many cuts
        float variety;                      // 0 = use best clips, 1 = use all clips
        bool include_transitions;
        bool include_text_overlays;         // Auto-generate title/captions
        bool include_color_grade;
        bool auto_audio_levels;             // Normalize and mix audio
    };

    struct AutoEditResult {
        VideoTemplate generated_template;
        float quality_score;
        std::string summary;                // "30s montage from 12 clips, 6 scenes selected"
        std::vector<std::string> decisions; // Log of AI decisions
    };

    AutoEditResult auto_edit(IGpuContext& gpu, const AutoEditConfig& config,
                              std::function<void(float)> progress);

    // Individual analysis steps (can be called separately)

    // Scene detection: find cuts and segments in raw footage
    struct DetectedScene {
        int32_t media_ref_id;
        Rational start, duration;
        float motion_score;             // Camera/subject motion
        float visual_quality;           // Sharpness, exposure
        float face_score;               // Face presence/expression
        float audio_energy;
        std::string auto_label;         // "close-up", "wide shot", "b-roll"
    };

    std::vector<DetectedScene> detect_all_scenes(IGpuContext& gpu,
                                                   const std::vector<int32_t>& media_refs);

    // Highlight detection: rank scenes by interest
    std::vector<DetectedScene> rank_highlights(const std::vector<DetectedScene>& scenes,
                                                float target_duration_seconds);

    // Beat-matched cutting: assign scenes to beats
    struct BeatMatchedEdit {
        std::vector<SmartDurationEngine::CutPoint> cuts;
        float sync_quality;             // How well cuts align with beats
    };

    BeatMatchedEdit match_to_beats(const std::vector<DetectedScene>& ranked_scenes,
                                    const TemplateMusicConfig& music,
                                    float target_duration_seconds);

    // Auto transition selection
    std::vector<std::string> suggest_transitions(
        const std::vector<DetectedScene>& scene_sequence,
        const std::string& style);

    // Auto color grade selection
    std::string suggest_color_grade(IGpuContext& gpu,
                                     const std::vector<int32_t>& media_refs,
                                     const std::string& mood);
};

} // namespace gp::vtemplates
```

### 25.16 Template Marketplace & Publishing Pipeline

```cpp
namespace gp::vtemplates {

struct VideoTemplatePublication {
    std::string publication_id;
    std::string template_id;
    std::string creator_id;

    std::string title;
    std::string description;
    std::string category;
    std::vector<std::string> tags;
    SocialPlatform target_platform;
    std::string language;

    enum Tier { Free, Premium, Exclusive };
    Tier tier;
    float price;
    std::string currency;

    int version_major, version_minor, version_patch;
    std::string changelog;
    std::string min_app_version;

    enum Status { Draft, Submitted, InReview, Approved, Rejected, Published, Deprecated };
    Status status;

    int64_t total_uses;
    int64_t total_downloads;
    float average_rating;
    int rating_count;

    std::string gpvt_url;
    std::string cover_image_url;
    std::string preview_video_url;
    std::vector<std::string> sample_video_urls;

    std::string created_at;
    std::string published_at;
};

class VideoTemplatePublisher {
public:
    VideoTemplatePublication submit(const VideoTemplate& tmpl,
                                     const std::string& title,
                                     const std::string& description,
                                     const std::vector<std::string>& tags,
                                     VideoTemplatePublication::Tier tier,
                                     float price = 0.0f);

    VideoTemplatePublication update_version(const std::string& publication_id,
                                             const VideoTemplate& tmpl,
                                             const std::string& changelog);

    void deprecate(const std::string& publication_id);

    struct SubmissionCheck {
        bool passes;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        bool has_media_placeholders;
        bool has_music;
        bool has_preview_video;
        bool fonts_embeddable;
        bool music_licensed;
        float estimated_file_size_mb;
    };
    SubmissionCheck pre_check(const VideoTemplate& tmpl);

    struct ListingAssets {
        std::string preview_video_path;
        GpuTexture cover_image;
        std::vector<GpuTexture> variations;
    };
    ListingAssets generate_listing_assets(IGpuContext& gpu, const VideoTemplate& tmpl);
};

class VideoTemplateStore {
public:
    struct SearchQuery {
        std::string keyword;
        std::string category;
        std::vector<std::string> tags;
        SocialPlatform platform;
        VideoTemplatePublication::Tier tier;
        SortOrder sort;
        int limit, offset;
    };

    enum class SortOrder { Trending, Newest, TopRated, MostUsed };

    struct SearchResult {
        std::vector<VideoTemplatePublication> items;
        int total_count;
    };

    SearchResult search(const SearchQuery& query);
    std::vector<VideoTemplatePublication> trending(int limit = 20);
    std::vector<VideoTemplatePublication> recommended(const std::string& user_id, int limit = 20);

    VideoTemplate download(const std::string& publication_id);
    void rate(const std::string& publication_id, int stars, const std::string& review);

    struct TemplateCollection {
        std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> publication_ids;
    };
    std::vector<TemplateCollection> featured_collections();
};

} // namespace gp::vtemplates
```

### 25.17 Template Versioning & Collaboration

```cpp
namespace gp::vtemplates {

struct VideoTemplateVersion {
    std::string version_id;
    std::string template_id;
    int version_number;
    std::string author_id;
    std::string message;
    std::string created_at;
    std::string snapshot_url;
    std::string thumbnail_url;

    struct VersionDiff {
        std::vector<int32_t> added_clips;
        std::vector<int32_t> removed_clips;
        std::vector<int32_t> modified_clips;
        std::vector<std::string> added_sections;
        std::vector<std::string> removed_sections;
        bool music_changed;
        bool grade_changed;
        bool duration_changed;
    } diff;
};

class VideoTemplateVersionControl {
public:
    VideoTemplateVersion save_version(const VideoTemplate& tmpl,
                                       const std::string& message);
    std::vector<VideoTemplateVersion> version_history(const std::string& template_id);
    VideoTemplate restore_version(const std::string& version_id);
    VideoTemplateVersion::VersionDiff compare(const std::string& version_a,
                                               const std::string& version_b);
    void enable_auto_save(const std::string& template_id, int interval_seconds = 60);
};

class VideoTemplateCollaboration {
public:
    enum class Permission { View, Comment, Edit, Admin };

    void share(const std::string& template_id, const std::string& user_id,
               Permission permission);
    void revoke(const std::string& template_id, const std::string& user_id);

    struct Collaborator {
        std::string user_id;
        std::string display_name;
        Permission permission;
        bool is_online;
    };
    std::vector<Collaborator> list_collaborators(const std::string& template_id);

    struct Comment {
        std::string id;
        std::string author_id;
        std::string content;
        Rational timeline_time;         // Pinned to a moment in the timeline
        int32_t clip_id;               // Pinned to a specific clip (-1 = global)
        bool resolved;
        std::string parent_id;
    };

    Comment add_comment(const std::string& template_id, const Comment& comment);
    void resolve_comment(const std::string& comment_id);
    std::vector<Comment> list_comments(const std::string& template_id);

    // Shared libraries
    struct SharedLibrary {
        std::string library_id;
        std::string name;
        std::string team_id;
        std::vector<std::string> template_ids;
    };
    SharedLibrary create_library(const std::string& name, const std::string& team_id);
    void add_to_library(const std::string& library_id, const std::string& template_id);
    std::vector<SharedLibrary> list_libraries(const std::string& team_id);
};

} // namespace gp::vtemplates
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 5b: Video Template System |
| **Sprint(s)** | VE-Sprint 15-22 (Weeks 29-44) |
| **Team** | C/C++ Engine Developer (2), Flutter Developer, Tech Lead |
| **Predecessor** | [12-transition-system.md](12-transition-system.md) |
| **Successor** | [13-ai-features.md](13-ai-features.md) |
| **Story Points Total** | 358 |

### Sprint Breakdown

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 15 | Wk 29-30 | VE-301 to VE-310 | Template data model, sections, placeholders, encryption |
| Sprint 16 | Wk 31-32 | VE-311 to VE-318 | Style controls, palette, music engine, beat sync |
| Sprint 17 | Wk 33-34 | VE-319 to VE-326 | Smart duration, Creator Studio, preview/validation |
| Sprint 18 | Wk 35-36 | VE-327 to VE-334 | AI generation, auto-edit, scene analysis |
| Sprint 19 | Wk 37-38 | VE-335 to VE-342 | Brand kit, animation presets, kinetic typography |
| Sprint 20 | Wk 39-40 | VE-343 to VE-350 | Data-driven batch, smart resize, auto-reframe |
| Sprint 21 | Wk 41-42 | VE-351 to VE-358 | Marketplace, publishing, store, collections |
| Sprint 22 | Wk 43-44 | VE-359 to VE-366 | Versioning, collaboration, comments, shared libraries |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-301 | As a template creator, I want a VideoTemplate data model so that I can define pre-designed timeline-based templates | - VideoTemplate: template_id, name, category, timeline, sections, placeholders<br/>- SocialPlatform target, width/height/fps<br/>- Duration constraints, auto_adapt_duration | 5 | P0 | — |
| VE-302 | As a template creator, I want template sections (Intro/Content/Transition/Outro) so that I can structure templates semantically | - TemplateSection: id, type, order, start_time, duration, min/max duration<br/>- SectionType: Intro, ContentSlide, ContentMontage, TextOnly, CTA, Outro<br/>- repeatable flag with min/max repeats | 5 | P0 | VE-301 |
| VE-303 | As a template creator, I want media placeholders (video/photo slots with fit modes) so that users can swap their footage | - VideoPlaceholder with MediaConfig: accept type, min_duration, resolution<br/>- FitMode: Fill, Fit, Letterbox, PanAndScan<br/>- allow_trim, allow_speed, allow_filters, auto_face_crop | 5 | P0 | VE-302 |
| VE-304 | As a template creator, I want text placeholders with animation presets so that users can edit titles | - TextConfig: default_text, hint, max_characters, max_lines<br/>- allow_font_change, allow_color_change, allow_animation_change<br/>- default_animation (TextAnimatorGroup) | 5 | P0 | VE-302 |
| VE-305 | As a template creator, I want audio placeholders (music/voiceover/SFX) so that users can customize audio | - AudioConfig: type (Music/Voiceover/SFX), max_duration, auto_duck<br/>- beat_sync_available, suggested_tracks<br/>- Volume control | 5 | P0 | VE-302 |
| VE-306 | As a template creator, I want logo and watermark placeholders so that users can brand their videos | - LogoConfig: max_width/height, position, opacity, display timing<br/>- LogoAnimation: None, FadeIn, SlideIn, ScaleIn, Bounce<br/>- Watermark persistent overlay | 3 | P0 | VE-302 |
| VE-307 | As a template creator, I want color grade placeholders so that users can change the video mood | - ColorGradeConfig: default_lut_id, intensity, suggested_lut_ids<br/>- allow_manual_grading<br/>- Applied as adjustment layer on master | 3 | P0 | VE-302 |
| VE-308 | As a template creator, I want .gpvt encrypted template format so that templates are protected | - .gpvt header: GPVT magic, version, template ID, hash, platform, duration<br/>- AES-256-GCM payload with timeline, clips, assets, music, LUTs<br/>- Ed25519 signature | 5 | P0 | VE-301 |
| VE-309 | As a template creator, I want beat-synced sections so that cuts align with music | - beat_synced flag on TemplateSection, beat_offset, beat_count<br/>- snap_to_beats() aligns section boundaries<br/>- Beat map stored in TemplateMusicConfig | 5 | P0 | VE-302, VE-311 |
| VE-310 | As a template creator, I want section repetition rules so that content sections expand for more media | - repeatable, min_repeats, max_repeats on TemplateSection<br/>- duplicate_section() / remove_section()<br/>- recompute_timing() after add/remove | 5 | P0 | VE-302 |
| VE-311 | As a template creator, I want a music/audio template engine (beat analysis, swap, auto-duck) so that audio is intelligent | - analyze_music() → TemplateMusicConfig with beat_times, bpm, sections<br/>- swap_music() re-syncs template cuts<br/>- configure_auto_duck() | 8 | P0 | VE-301 |
| VE-312 | As a template creator, I want a music library with search/browse so that users can find music | - search_music(query, mood, bpm range)<br/>- trending_music(), music_for_template()<br/>- MusicTrack: title, artist, genre, mood, bpm, preview | 5 | P0 | VE-311 |
| VE-313 | As a template creator, I want style control: color grade so that users can switch color mood | - ColorGradeControl with lut_options and thumbnails<br/>- Apply to template as adjustment layer<br/>- 80+ color grade presets across 8 categories | 5 | P0 | VE-307 |
| VE-314 | As a template creator, I want style control: font style so that users can change typography | - FontStyleControl with FontPairing options<br/>- affected_text_clip_ids<br/>- Apply heading + body font pairing | 3 | P0 | VE-304 |
| VE-315 | As a template creator, I want style control: transition style so that users can swap all transitions | - TransitionStyleControl with TransitionSet options<br/>- Sets: "Clean", "Glitch", "Smooth", "Cinematic"<br/>- Swap all transitions at once | 5 | P0 | VE-301 |
| VE-316 | As a template creator, I want style control: music mood so that users can swap background music | - MusicMoodControl with MusicOption list<br/>- Preview URL, BPM, duration per option<br/>- Re-sync beats on swap | 5 | P0 | VE-311 |
| VE-317 | As a template creator, I want style control: pacing speed so that users can adjust cut frequency | - PacingControl: speed 0.5-2.0, beat_sync maintain<br/>- Adjusts section durations proportionally<br/>- Beat-aligned pacing | 5 | P0 | VE-309 |
| VE-318 | As a template creator, I want style control: overlay style so that users can add film grain/VHS/light leaks | - OverlayStyleControl with OverlayOption list<br/>- "Film Grain", "Light Leak", "Dust", "VHS"<br/>- Intensity slider | 3 | P0 | VE-301 |
| VE-319 | As a template creator, I want a smart duration engine so that templates adapt to user's media count | - compute_plan(template, DurationInput) → DurationPlan<br/>- Section timing with stretch/compress<br/>- Music fade-out if template shorter | 8 | P0 | VE-310 |
| VE-320 | As a template creator, I want beat-aligned cut computation so that cuts sync to music | - compute_beat_aligned_cuts(bpm, offset, count, duration, pattern)<br/>- BeatPattern: EveryBeat, EveryOtherBeat, EveryBar, Accelerating<br/>- CutPoint with timeline and source positions | 5 | P0 | VE-319, VE-309 |
| VE-321 | As a template creator, I want smart source trimming (pick best segment) so that auto-selected clips look good | - smart_trim(media_ref, target_duration) → SmartTrimResult<br/>- interest_score based on saliency/motion/faces<br/>- detect_scenes() for segment analysis | 5 | P0 | VE-319 |
| VE-322 | As a template creator, I want a Creator Studio for video templates so that I have authoring tools | - create_new(), import_from_project(), fork_template()<br/>- add_section(), promote_clip_to_placeholder()<br/>- set_constraints() per placeholder | 5 | P0 | VE-301 |
| VE-323 | As a template creator, I want preview mode and validation so that I can test before publishing | - enter_preview_mode() / exit_preview_mode()<br/>- validate() → errors, warnings, tips<br/>- Only placeholders editable in preview | 5 | P0 | VE-322 |
| VE-324 | As a template creator, I want .gpvt packaging with preview video generation so that I can distribute templates | - package() encrypts to .gpvt<br/>- generate_previews(): preview_video_path, cover_thumbnail, frame_thumbnails<br/>- 15-second animated preview | 5 | P0 | VE-308, VE-323 |
| VE-325 | As a template creator, I want social media platform presets so that templates have correct dimensions/duration | - SocialPlatform enum: 15 platforms<br/>- Per-platform: width/height, aspect, max_duration, safe_zone<br/>- create_new() with SocialPlatform | 3 | P0 | VE-301 |
| VE-326 | As a template creator, I want video template palette (colors + LUT) so that templates have coordinated color | - VideoTemplatePalette: primary/secondary/accent + lut_id + grading<br/>- built_in_palettes(), from_video()<br/>- apply_to_template() | 5 | P0 | VE-313 |
| VE-327 | As a user, I want AI text-to-video-template generation so that I can create templates from a description | - GenerationPrompt: description, platform, mood, style, media_slot_count<br/>- generate() returns variations with confidence<br/>- Generates sections, placeholders, music | 8 | P1 | VE-301 |
| VE-328 | As a user, I want AI auto-edit from raw footage so that I get a polished video automatically | - AutoEditInput: media_refs, music_ref, platform, style, target_duration<br/>- auto_edit() → VideoTemplate with cuts, transitions, grade<br/>- quality_score and decision log | 8 | P1 | VE-327 |
| VE-329 | As a user, I want AI scene detection + highlight ranking so that best moments are selected | - detect_all_scenes() → DetectedScene with motion, quality, face, audio scores<br/>- rank_highlights() by interest for target duration<br/>- Auto labels: "close-up", "wide shot", "b-roll" | 5 | P1 | VE-328 |
| VE-330 | As a user, I want AI beat-matched cutting so that clips are assigned to music beats | - match_to_beats(scenes, music, duration) → BeatMatchedEdit<br/>- sync_quality score<br/>- Cut points aligned to downbeats and beats | 5 | P1 | VE-329, VE-311 |
| VE-331 | As a user, I want AI transition and color grade suggestions so that the style matches my footage | - suggest_transitions(scene_sequence, style)<br/>- suggest_color_grade(media_refs, mood)<br/>- Suggestions based on footage analysis | 3 | P1 | VE-329 |
| VE-332 | As a user, I want AI text suggestions for video templates so that captions are contextual | - suggest_text(template, context) for placeholder text<br/>- AI-generated headlines, CTAs, hashtags<br/>- Platform-aware suggestions | 3 | P1 | VE-327 |
| VE-333 | As a user, I want AI layout suggestions for templates so that section structure is optimized | - suggest_layouts(template) → LayoutSuggestion<br/>- "Montage", "Slideshow", "Story Arc" patterns<br/>- aesthetic_score per suggestion | 3 | P1 | VE-327 |
| VE-334 | As a user, I want AI music matching to footage energy so that music fits the content | - suggest_music(media_refs) and smart_match()<br/>- Energy analysis of footage<br/>- BPM and mood matching | 5 | P1 | VE-311 |
| VE-335 | As a brand owner, I want a video brand kit (logos, intros, lower thirds, grade, audio) so that my videos are brand-consistent | - VideoBrandKit: logos, branded_segments (intro/outro), lower_thirds<br/>- Brand palette, typography, LUT, audio stings, watermark<br/>- CRUD operations | 5 | P0 | VE-301 |
| VE-336 | As a brand owner, I want to apply my brand kit to any video template so that all videos match my brand | - apply_to_template() with Full/IntroOutroOnly/GradeOnly/Selective modes<br/>- BrandApplyOptions: intro, outro, lower_thirds, grade, logo, watermark<br/>- Brand audit for consistency | 5 | P0 | VE-335 |
| VE-337 | As a user, I want 80+ text reveal animation presets so that text looks professional | - TextRevealPreset: id, name, category, animator, duration<br/>- Categories: Basic, Kinetic, Glitch, Elegant, Bold, Social, 3D<br/>- apply_text_reveal() to placeholder | 5 | P0 | VE-304 |
| VE-338 | As a user, I want 30+ kinetic typography presets so that I can create lyric-video-style sequences | - KineticTypographyPreset: multi-block text sequences<br/>- Coordinated entrance/emphasis/exit per block<br/>- Camera moves during sequence | 5 | P1 | VE-337 |
| VE-339 | As a template creator, I want sound effects attached to transitions/beats so that videos have audio polish | - SoundEffect in TemplateMusicConfig with sfx_id, time, volume, trigger<br/>- Triggers: "transition", "text_appear", "beat"<br/>- Sound effect library integration | 3 | P0 | VE-311 |
| VE-340 | As a template creator, I want music section analysis (buildup/drop/verse/chorus) so that I can map sections to music | - MusicSection in TemplateMusicConfig: label, start, duration, energy<br/>- "Buildup", "Drop", "Verse", "Chorus", "Outro" labels<br/>- Energy curve for pacing | 5 | P0 | VE-311 |
| VE-341 | As a user, I want split screen sections so that I can show multiple clips simultaneously | - SectionType::SplitScreen<br/>- Multiple media placeholders per section<br/>- Grid layouts: 2x1, 1x2, 2x2, custom | 5 | P1 | VE-302 |
| VE-342 | As a user, I want countdown/timer sections so that I can add countdowns to templates | - SectionType::Countdown<br/>- Configurable start value, end value, format<br/>- Animation style presets | 3 | P1 | VE-302 |
| VE-343 | As a power user, I want CSV/JSON data source parsing for video batch personalization so that I can prepare bulk jobs | - parse_data_source() for CSV, JSON, Google Sheets, API<br/>- auto_map_fields() auto-detects column→placeholder mapping<br/>- VideoFieldType: Text, MediaUrl, AudioUrl, Color, Boolean | 5 | P0 | VE-301 |
| VE-344 | As a power user, I want batch video render execution so that I can generate hundreds of personalized videos | - VideoBatchConfig: template, data, export settings, output dir<br/>- execute() with progress callback<br/>- VideoBatchResult: successful, failed, output paths | 8 | P0 | VE-343 |
| VE-345 | As a power user, I want batch preview and validation so that I can verify before rendering | - preview_row() for single row frame callback<br/>- DataValidation: missing_required, type_mismatches<br/>- cancel() for running batches | 3 | P0 | VE-344 |
| VE-346 | As a user, I want smart resize to multiple platforms (Reels/TikTok/Shorts/YouTube) so that one video works everywhere | - VideoSmartResizeEngine::resize(source, target)<br/>- resize_batch() for multiple platforms<br/>- VideoResizeResult with fidelity_score and warnings | 8 | P0 | VE-301 |
| VE-347 | As a user, I want AI auto-reframe (subject tracking) for aspect ratio change so that subjects stay centered | - compute_auto_reframe(media_ref, source_aspect, target_aspect)<br/>- Per-frame subject tracking and camera transform<br/>- blur_background_fill option | 5 | P0 | VE-346 |
| VE-348 | As a user, I want resize strategy (crop/pillarbox/blur fill/text safe zone) so that I control how resize works | - ResizeStrategy: auto_reframe, allow_crop, blur_background_fill<br/>- preserve_text_safe_zone, auto_resize_text<br/>- adapt_duration for platform limits | 3 | P0 | VE-346 |
| VE-349 | As a user, I want auto-edit: full pipeline (detect→rank→match→grade→export) so that I get a finished video from raw clips | - AutoEditConfig: media_refs, music, platform, style, cut_frequency<br/>- auto_edit() runs full pipeline<br/>- quality_score and summary | 8 | P1 | VE-329, VE-330 |
| VE-350 | As a user, I want auto-edit style presets (Fast Cuts/Cinematic/Chill/Dramatic) so that auto-edits have personality | - Style presets with cut_frequency, variety, transition preferences<br/>- Grade and pacing per style<br/>- User can adjust parameters | 3 | P1 | VE-349 |
| VE-351 | As a template creator, I want to submit video templates to the marketplace so that I can publish and monetize | - VideoTemplatePublisher::submit() with title, tags, tier, price<br/>- SubmissionCheck: pre_check() validates before submission<br/>- VideoTemplatePublication with status lifecycle | 5 | P0 | VE-324 |
| VE-352 | As a template creator, I want auto-generated marketplace listing assets so that my templates look professional | - generate_listing_assets(): preview_video, cover, variations<br/>- update_version() for new versions<br/>- deprecate() to retire templates | 5 | P0 | VE-351 |
| VE-353 | As a user, I want video template store search and discovery so that I can find templates | - VideoTemplateStore::search() with keyword, category, platform, sort<br/>- trending(), recommended()<br/>- SortOrder: Trending, Newest, TopRated, MostUsed | 5 | P0 | VE-351 |
| VE-354 | As a user, I want template collections and ratings so that I can browse curated lists | - rate() with stars and review<br/>- featured_collections()<br/>- TemplateCollection with publication_ids | 3 | P0 | VE-353 |
| VE-355 | As a user, I want video template download and use so that I can apply marketplace templates | - download() returns VideoTemplate<br/>- Decrypt .gpvt, extract assets<br/>- Ready to customize | 3 | P0 | VE-353 |
| VE-356 | As a user, I want template version history so that I can track template evolution | - save_version() with message<br/>- version_history(), restore_version()<br/>- compare() with VersionDiff | 5 | P0 | VE-301 |
| VE-357 | As a user, I want template auto-save so that work is never lost | - enable_auto_save(template_id, interval_seconds)<br/>- Background periodic save<br/>- Recoverable from last auto-save | 3 | P0 | VE-356 |
| VE-358 | As a team member, I want template sharing with permissions so that teams can collaborate | - share(template_id, user_id, permission)<br/>- Permission: View, Comment, Edit, Admin<br/>- list_collaborators() | 5 | P1 | VE-356 |
| VE-359 | As a team member, I want timeline-pinned comments so that I can give time-specific feedback | - Comment with timeline_time and clip_id pinning<br/>- Threaded replies<br/>- resolve_comment() | 3 | P1 | VE-358 |
| VE-360 | As a team lead, I want shared template libraries so that teams share approved templates | - SharedLibrary: library_id, name, team_id, template_ids<br/>- create_library(), add_to_library()<br/>- Team-level organization | 5 | P1 | VE-358 |

### Story Points Summary

| Phase | Sprints | Weeks | Story Points |
|---|---|---|---|
| Core Template System (25.1–25.3) | Sprint 15 | Wk 29-30 | 46 |
| Style Controls & Music (25.4–25.7) | Sprint 16 | Wk 31-32 | 47 |
| Smart Duration & Creator Studio (25.5, 25.8) | Sprint 17 | Wk 33-34 | 41 |
| AI Generation & Auto-Edit (25.10, 25.15) | Sprint 18 | Wk 35-36 | 43 |
| Brand Kit & Animation Presets (25.11–25.12) | Sprint 19 | Wk 37-38 | 36 |
| Data-Driven & Smart Resize (25.13–25.14) | Sprint 20 | Wk 39-40 | 37 |
| Marketplace & Publishing (25.16) | Sprint 21 | Wk 41-42 | 26 |
| Versioning & Collaboration (25.17) | Sprint 22 | Wk 43-44 | 21 |
| **Total Video Template System** | **Sprint 15-22** | **Wk 29-44** | **358** |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
