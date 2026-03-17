# GOPOST-APP — Complete Feature Documentation

## Table of Contents

1. [App Overview](#app-overview)
2. [Platform Support](#platform-support)
3. [Screens & Navigation](#screens--navigation)
4. [Authentication System](#authentication-system)
5. [Template Browser](#template-browser)
6. [Image Editor](#image-editor)
7. [Video Editor](#video-editor)
8. [Native C++ Engine](#native-c-engine)
9. [Networking & API](#networking--api)
10. [Local Persistence & Caching](#local-persistence--caching)
11. [Security](#security)
12. [Theme & Styling](#theme--styling)
13. [Dependencies](#dependencies)
14. [File Structure](#file-structure)

---

## App Overview

GOPOST-APP is a cross-platform media editing application built with Flutter and a native C++ rendering engine. It provides professional-grade image editing, video timeline editing, template browsing, and AI-assisted content creation (GoCraft).

**Tech Stack:**
- **Frontend:** Flutter (Dart) with Material 3
- **State Management:** Riverpod (reactive)
- **Navigation:** GoRouter (declarative, auth-guarded)
- **Native Engine:** C++ via FFI (image compositing, video timeline, GPU rendering)
- **Video Playback:** media_kit (libmpv)
- **Media Encoding:** FFmpeg via ffmpeg_kit_flutter
- **Networking:** Dio with interceptors

---

## Platform Support

| Platform | Status | GPU Backend |
|----------|--------|-------------|
| Windows  | Supported | ANGLE (OpenGL ES via Direct3D) |
| macOS    | Supported | Metal |
| Linux    | Supported | OpenGL ES |
| Android  | Supported | OpenGL ES |
| iOS      | Supported | Metal |
| Web      | Partial | Stubs for native features |

---

## Screens & Navigation

### Authentication
| Screen | Route | Purpose |
|--------|-------|---------|
| LoginScreen | `/auth/login` | Email/password login with social auth buttons |
| RegisterScreen | `/auth/register` | New user registration |

### Main App (Bottom Navigation Shell)
| Screen | Route | Purpose |
|--------|-------|---------|
| HomeScreen | `/` | Landing page — featured templates, trending, recently added, categories |
| BrowseScreen | `/templates` | Template library with search, filters, pagination |
| TemplateDetailScreen | `/templates/:id` | Individual template preview and details |
| GoCraftScreen | `/create` | AI-assisted content creation tool |
| Profile | `/profile` | User profile (placeholder) |

### Image Editor
| Screen | Route | Purpose |
|--------|-------|---------|
| ImageEditorScreen | `/editor/image` | Full image editor with canvas, layers, effects, export |
| ExportScreen | (push) | Image export with format/quality/resolution options |
| TemplateCustomizationScreen | `/editor/customize/:id` | Edit template fields |

### Video Editor
| Screen | Route | Purpose |
|--------|-------|---------|
| VideoEditorScreen | `/editor/video` | Full video editor with timeline, preview, sidebar |
| ProjectListScreen | `/projects/video` | Video project management |
| ExportScreen | (push) | Video export configuration |
| VideoTemplateCustomizationScreen | `/editor/video/customize/:id` | Video template editing |

### Navigation Features
- **Auth Guard:** Redirects unauthenticated users to login
- **Guest Mode:** Allows browsing templates; editor access requires login
- **Main Shell:** Bottom nav bar with 4 tabs (Home, Templates, Create, Profile) and neon glow effects

---

## Authentication System

### Features
- Email/password login and registration
- Social login buttons (Google, Apple, GitHub, Discord — UI ready)
- Token-based authentication (access + refresh tokens)
- Secure token storage (Keychain on iOS, Keystore on Android)
- Auto-refresh on token expiry
- Guest mode with limited access

### State Management
- `AuthNotifier` — Manages login, register, logout flows
- `AuthState` — Tracks status (unauthenticated, authenticating, authenticated, guest, error)

### Files
- `lib/auth/data/datasources/auth_remote_datasource.dart` — API calls
- `lib/auth/data/repositories/auth_repository_impl.dart` — Business logic
- `lib/auth/domain/usecases/login_usecase.dart` — Login use case
- `lib/auth/domain/usecases/register_usecase.dart` — Register use case
- `lib/auth/domain/usecases/logout_usecase.dart` — Logout use case

---

## Template Browser

### Features
- Browse templates with categories and filters
- Search templates by name/tag
- Featured, trending, and recently added sections
- Template detail view with preview
- Download and cache templates locally
- Request access to premium templates
- Save projects as reusable templates

### Widgets
| Widget | Purpose |
|--------|---------|
| Search bar | Keyword search with debounce |
| Category filter chips | Filter by template category |
| Sort button | Sort by date, popularity, name |
| Template card | Preview thumbnail with metadata |
| Download indicator | Progress during template download |

### State Management
- `HomeNotifier` — Featured, trending, recent, categories
- `TemplateListNotifier` — Paginated template list with filters
- `TemplateDetailNotifier` — Selected template details
- `TemplateSearchNotifier` — Search results
- `DownloadNotifier` — Download progress tracking

---

## Image Editor

### Canvas Management
| Feature | Description |
|---------|-------------|
| Create Canvas | Configurable dimensions, DPI (72–300), color space (sRGB, Display P3, Adobe RGB), transparency |
| Resize Canvas | Change dimensions while preserving content |
| Multi-Layer | Unlimited layers with independent transforms |

### Layer System
| Layer Type | Description |
|------------|-------------|
| Image | RGBA pixel layers from imported photos |
| Solid Color | Fill layers with any color |
| Text | Rich text with fonts, styles, shadows, outlines |
| Shape | Vector shapes |
| Group | Layer grouping |
| Adjustment | Non-destructive adjustment layers |
| Gradient | Gradient fill layers |
| Sticker | Decorative sticker elements |

### Layer Properties
- Visibility and lock toggle
- Opacity (0–100%)
- 26 blend modes (Normal, Multiply, Screen, Overlay, Darken, Lighten, Color Dodge, Color Burn, Hard Light, Soft Light, Difference, Exclusion, Hue, Saturation, Color, Luminosity, Linear Burn, Linear Dodge, Vivid Light, Linear Light, Pin Light, Hard Mix, Dissolve, Divide, Subtract, Grain Extract, Grain Merge)
- Transform (translate, scale, rotate)
- Name editing
- Drag-to-reorder

### Text Engine
| Property | Range |
|----------|-------|
| Font Family | System fonts |
| Font Style | Normal, Bold, Italic, Bold Italic |
| Font Size | Free |
| Color | Full RGBA |
| Alignment | Left, Center, Right, Justify |
| Line Height | Multiplier |
| Letter Spacing | Pixels |
| Shadow | Color, offset X/Y, blur radius |
| Outline | Color, width |

### Effects System
| Category | Effects |
|----------|---------|
| Adjustment | Brightness, Contrast, Saturation, Exposure, Hue, Vibrance |
| Blur | Gaussian Blur, Box Blur, Radial Blur, Tilt-Shift |
| Sharpen | Unsharp Mask |
| Artistic | Oil Paint, Watercolor, Sketch, Pixelate, Glitch, Halftone |
| Preset Filters | Intensity-controllable presets |

Each effect supports: enabled/disabled toggle, mix (0–100%), individual parameter tuning.

### Masking
| Feature | Description |
|---------|-------------|
| Mask Types | Raster, Vector, Clipping |
| Paint Mode | Paint or erase with configurable radius, hardness, opacity |
| Mask Fill | Fill entire mask |
| Mask Invert | Invert mask selection |
| Enable/Disable | Toggle mask without removing |

### Rendering
- Tile-based rendering for large images
- Dirty tile tracking for incremental updates
- GPU texture output with viewport control (pan, zoom, rotation)
- Full canvas or region render to RGBA

### Export Pipeline
| Setting | Options |
|---------|---------|
| Format | JPEG, PNG, WebP, HEIC, BMP, GIF, TIFF, TGA |
| Quality | 0–100 slider |
| Resolution | Original, 4K, 1080p, 720p, Instagram Square (1080x1080), Instagram Story (1080x1920), Custom |
| DPI | 72–300 |
| File Size | Pre-export estimation |
| Progress | Callback-based |

### Toolbar Buttons
| Button | Icon | Action |
|--------|------|--------|
| Layers | layers | Toggle layer side panel |
| Add Image | add_photo | Open photo picker |
| Add Text | text_fields | Open text editor panel |
| Sticker | emoji | Open sticker picker |
| Filter | filter | Toggle filter panel |
| Adjust | tune | Toggle adjustment panel |
| Crop | crop | Toggle crop tool |
| Draw | brush | Drawing mode |
| Export | download | Open export screen |
| Undo | undo | Undo last action (Cmd+Z) |
| Redo | redo | Redo undone action (Cmd+Shift+Z) |
| Save Template | bookmark | Save as reusable template |

### Keyboard Shortcuts
| Shortcut | Action |
|----------|--------|
| Cmd/Ctrl+Z | Undo |
| Cmd/Ctrl+Shift+Z / Ctrl+Y | Redo |

---

## Video Editor

### Timeline Management
| Feature | Description |
|---------|-------------|
| Multi-Track | Video, audio, title, effect, subtitle tracks |
| Dynamic Track Height | Resizable from 24px (collapsed) to 200px (expanded) |
| Auto-Fit Zoom | Clips auto-scale to fill viewport on import |
| Pinch-to-Zoom | All platforms — override auto-fit with gesture |
| Zoom Range | 1–400 pixels per second |
| Snap-to-Grid | 8px snap threshold on clip edges and playhead |
| Time Ruler | Dynamic interval ruler with in/out point markers |
| Playhead | Draggable with scrub preview |

### Track Operations
| Operation | Description |
|-----------|-------------|
| Add Track | Create video, audio, title, effect, or subtitle track |
| Remove Track | Delete track and its clips |
| Reorder Tracks | Drag-to-reorder |
| Track Visibility | Toggle track on/off |
| Track Lock | Prevent edits on track |
| Track Mute | Mute audio for track |
| Track Solo | Solo audio for track |
| Track Volume | Per-track volume (0–200%) |
| Track Pan | Per-track stereo panning (-1 to +1) |

### Clip Operations
| Operation | Description |
|-----------|-------------|
| Add Clip | Import video/image/audio into track |
| Remove Clip | Delete with optional ripple |
| Trim Clip | Adjust in/out points |
| Move Clip | Drag between tracks and positions |
| Split Clip | Divide at playhead position (S key) |
| Duplicate Clip | Copy clip to end |
| Ripple Delete | Remove range and collapse |
| Close Gaps | Remove gaps in track |

### NLE (Non-Linear Editing) Operations
| Operation | Description |
|-----------|-------------|
| Insert Edit | Insert clip, push existing clips right |
| Overwrite Edit | Place clip, replace overlapping content |
| Roll Edit | Adjust shared edge between adjacent clips |
| Slip Edit | Change source in/out without moving clip on timeline |
| Slide Edit | Move clip between neighbors, adjusting gaps |
| Rate Stretch | Change speed by adjusting duration |

### Playback
| Feature | Description |
|---------|-------------|
| Play/Pause | Toggle with Space key |
| JKL Shuttle | J (reverse), K (stop), L (forward) — multi-speed |
| Shuttle Speeds | -8x, -4x, -2x, -1x, 0, 1x, 2x, 4x, 8x |
| In/Out Points | Set with I/O keys |
| Jump to Start/End | Home/End keys |
| Frame Step | Arrow keys (single frame), Shift+Arrow (multi-frame) |
| Snap Points | Jump between clip edges and markers |
| Preview Panel | Live video preview with media_kit player |
| Double-Buffer Players | Seamless source switching without black flash |

### Effects (Per Clip — 22 Types)
| Category | Effects |
|----------|---------|
| Color/Tone | Brightness, Contrast, Saturation, Exposure, Temperature, Tint, Highlights, Shadows, Vibrance, Hue Rotation |
| Blur/Sharpen | Gaussian Blur, Radial Blur, Tilt-Shift, Sharpen |
| Distort | Pixelate, Glitch, Chromatic Aberration |
| Stylize | Vignette, Film Grain, Sepia, Invert, Posterize |

### Color Grading (Per Clip — 10 Sliders)
| Slider | Range |
|--------|-------|
| Brightness | -100 to +100 |
| Contrast | -100 to +100 |
| Saturation | -100 to +100 |
| Exposure | -2.0 to +2.0 |
| Temperature | -100 to +100 |
| Tint | -100 to +100 |
| Highlights | -100 to +100 |
| Shadows | -100 to +100 |
| Vibrance | -100 to +100 |
| Hue | -180 to +180 |

### Preset Filters (21)
| Category | Presets |
|----------|--------|
| None | none |
| Natural | natural, daylight, goldenHour, overcast |
| Portrait | portrait, softSkin, studio, warmPortrait |
| Vintage | vintage, polaroid, kodachrome, retro |
| Cinematic | cinematic, tealOrange |
| Black & White | noir, desaturated, bwClassic, bwHigh, bwSelenium, bwInfrared |

### Transitions (20 Types)
| Category | Transitions |
|----------|-------------|
| Fade | fade, dissolve |
| Slide | slideLeft, slideRight, slideUp, slideDown |
| Wipe | wipeLeft, wipeRight, wipeUp, wipeDown |
| Dynamic | zoom, push, reveal, iris, clock |
| Stylized | blur, glitch, morph, flash, spin |

Each transition supports 5 easing curves: linear, easeIn, easeOut, easeInOut, cubicBezier.

### Keyframes
| Feature | Description |
|---------|-------------|
| Animatable Properties | Opacity, scale, position X/Y, rotation, speed, volume |
| Interpolation | Linear, Ease In, Ease Out, Ease In-Out |
| Keyframe Editor | Visual timeline with add/remove/edit |

### Audio Features
| Feature | Description |
|---------|-------------|
| Clip Volume | 0–200% per clip |
| Clip Pan | -1.0 (left) to +1.0 (right) |
| Fade In/Out | Configurable duration in seconds |
| Track Volume | Master volume per track |
| Track Mute/Solo | Toggle per track |
| Audio Effects | Extensible audio effect system |

### Adjustment Layers
| Feature | Description |
|---------|-------------|
| Effect Track | Non-destructive effects above video tracks |
| Drag & Drop | Drop effects/presets onto timeline to create adjustment clips |
| Styles | colorWave, blurPulse, distortScan, stylizeDots, presetStrip |
| Visual Badges | FX, CLR, TR, SPD, KF, AUD indicators on clips |

### Masking & Tracking (Phase 4)
| Feature | Description |
|---------|-------------|
| Mask Types | Rectangle, Ellipse, Bezier, Freehand |
| Mask Properties | Points, feather, opacity, inverted, expansion |
| Motion Tracking | Start tracking at X,Y position and time |
| Stabilization | Configurable stabilization per clip |

### AI Features (Phase 6)
| Feature | Description |
|---------|-------------|
| AI Segmentation | Background, Person, Object, Sky segmentation |
| Progress Polling | Async job with progress tracking |
| Cancel | Cancel running segmentation jobs |

### Proxy Workflow
| Feature | Description |
|---------|-------------|
| Generate Proxy | Create low-res version for fast editing |
| Proxy Resolutions | Quarter, Half, Three-Quarter |
| Toggle Proxy Mode | Switch between proxy and full-res playback |
| Export Full-Res | Always exports from original source |

### Multi-Camera (Phase 6)
| Feature | Description |
|---------|-------------|
| Multi-Cam Clip | Create from multiple camera sources |
| Angle Switching | Switch active camera at any time point |
| Flatten | Convert multi-cam to individual clips |

### Video Export Pipeline
| Setting | Options |
|---------|---------|
| Video Codec | H.264, H.265, VP9 |
| Audio Codec | AAC, Opus |
| Container | MP4, MOV, WebM |
| Video Bitrate | Configurable (bps) |
| Audio Bitrate | Configurable (kbps) |
| Hardware Encoding | Auto-detect support |
| Progress | Async polling (0–100%) |
| Cancel | Cancel running export |
| Presets | 1080p60, 4K30, etc. |

### Media Probing (Duration Detection)
| Method | Priority | Description |
|--------|----------|-------------|
| ISO-BMFF Parser | 1 (highest) | Direct binary read of MP4/MOV moov→mvhd atom — instant, exact |
| ffprobe Process | 2 | Runs ffprobe CLI if installed — most formats supported |
| media_kit Player | 3 | Stream-based duration with stabilization waiting |
| File-Size Estimate | 4 (fallback) | Heuristic at 12 Mbps average bitrate |

### Toolbar Buttons
| Button | Icon | Action |
|--------|------|--------|
| Split | content_cut | Split clip at playhead (S key) |
| Delete | delete | Delete selected clip (Del key) |
| In/Out Display | — | Shows I: and O: timecodes when set |
| Position/Duration | — | MM:SS.FF / MM:SS.FF display |
| Zoom Out | remove | Decrease zoom |
| Zoom Level | — | Current px/sec value |
| Zoom In | add | Increase zoom |
| Fit Screen | fit_screen | Auto-fit all clips to viewport |
| Collapse Tracks | unfold_less | Set track height to minimum (24px) |
| Expand Tracks | unfold_more | Set track height to maximum (200px) |

### Keyboard Shortcuts
| Shortcut | Action |
|----------|--------|
| Space | Play/Pause |
| J | Reverse shuttle |
| K | Stop shuttle |
| L | Forward shuttle |
| S | Split clip at playhead |
| Delete | Delete selected clip |
| I | Set in point |
| O | Set out point |
| Home | Jump to start |
| End | Jump to end |
| Left Arrow | Step back 1 frame |
| Right Arrow | Step forward 1 frame |
| Shift+Left | Step back 10 frames |
| Shift+Right | Step forward 10 frames |
| Cmd/Ctrl+Z | Undo |
| Cmd/Ctrl+Shift+Z | Redo |
| Cmd/Ctrl+S | Save project |
| Cmd/Ctrl++ | Zoom in |
| Cmd/Ctrl+- | Zoom out |

---

## Native C++ Engine

### Core Engine (`native/gopost_core/`)
| Component | File | Purpose |
|-----------|------|---------|
| Engine Init | `engine.cpp` | Engine lifecycle management |
| Memory Allocator | `allocator.cpp` | Custom memory allocation |
| Thread Pool | `thread_pool.cpp` | Parallel processing |
| Frame Pool | `frame_pool.h` | Frame memory pooling |
| Logger | `logger.cpp` | Native logging |
| GPU Context | `gpu_context.cpp` | GPU factory and context creation |
| Metal Context | `metal_context.cpp` | macOS/iOS Metal GPU |
| GLES Context | `gles_context.cpp` | OpenGL ES GPU (Android/Linux/Windows) |

### Crypto Module (`native/gopost_core/src/crypto/`)
| Component | File | Purpose |
|-----------|------|---------|
| AES-GCM | `aes_gcm.cpp` | Symmetric encryption for template data |
| RSA-OAEP | `rsa_oaep.cpp` | Asymmetric key exchange for template access |
| Secure Memory | `secure_memory.cpp` | Secure memory wiping (prevents key leaks) |

### Template Parser (`native/gopost_core/src/template/`)
| Component | File | Purpose |
|-----------|------|---------|
| Template Parser | `template_parser.cpp` | Parse and decrypt encrypted template files |

### Image Engine (`native/gopost_image_engine/`)
| Component | File | Purpose |
|-----------|------|---------|
| Canvas | `canvas.h` | Canvas creation, layer management |
| Effects | `effects.h` | Effect registry, layer effects, presets |
| Text Engine | `text_engine.h` | Text layer operations |
| Mask System | `mask.h` | Layer masking |
| Export | `export.h` | Image export pipeline |
| Project | `project.h` | Project serialization |
| Blend Modes | `blend_modes.cpp` | All 26 blend mode implementations |
| Compositor | `compositor.cpp` | Layer composition pipeline |
| Effect Registry | `effect_registry.cpp` | Dynamic effect registration |
| Adjustment Filters | `adjustment_filters.cpp` | Brightness, contrast, etc. |
| Blur/Sharpen | `blur_sharpen_filters.cpp` | Gaussian, box, radial, tilt-shift, unsharp |
| Artistic Filters | `artistic_filters.cpp` | Oil paint, watercolor, sketch, etc. |
| Preset Filters | `preset_filters.cpp` | Preset filter engine |
| Tile Renderer | `tile_renderer.cpp` | Tile-based rendering for large images |
| Export Pipeline | `export_pipeline.cpp` | Multi-format export |
| Image Decoder | `image_decoder.cpp` | Decode JPG/PNG/WebP/HEIC/etc. |
| Image Encoder | `image_encoder.cpp` | Encode JPG/PNG/WebP/HEIC/etc. |
| Metal Renderer | `metal_renderer.cpp` | Metal GPU shader implementations |

### Video Engine (`native/gopost_video_engine/`)
| Component | File | Purpose |
|-----------|------|---------|
| Video Engine | `video_engine.cpp` | Main engine entry point |
| Timeline Model | `timeline_model.cpp` | Timeline data structure (tracks, clips, state) |
| Timeline Evaluator | `timeline_evaluator.cpp` | Frame evaluation and rendering |
| Frame Cache | `frame_cache.cpp` | Thread-safe frame caching |
| Keyframe Evaluator | `keyframe_evaluator.cpp` | Keyframe interpolation engine |
| Video Compositor | `video_compositor.cpp` | Multi-clip composition with blending |
| Video Decoder (FFmpeg) | `video_decoder_ffmpeg.cpp` | FFmpeg-based video decoding |
| Video Decoder (Stub) | `video_decoder_stub.cpp` | Fallback stub decoder |
| Audio Decoder (FFmpeg) | `audio_decoder_ffmpeg.cpp` | FFmpeg-based audio decoding |
| Audio Decoder (Stub) | `audio_decoder_stub.cpp` | Fallback stub decoder |
| Media Probe (FFmpeg) | `media_probe_ffmpeg.cpp` | FFmpeg-based metadata probing |
| Media Probe (Stub) | `media_probe_stub.cpp` | Fallback stub prober |
| Audio Mixer | `audio_mixer.cpp` | Multi-track audio mixing |
| Transition Renderer | `transition_renderer.cpp` | All 20 transition effects |
| Video Effects | `video_effects.cpp` | Color grading, visual effects |
| Video Exporter | `video_exporter.cpp` | Hardware-accelerated video export |
| Media Probe (AVF) | `media_probe_avf.mm` | macOS AVFoundation prober |

### Third-Party Libraries
| Library | Purpose |
|---------|---------|
| STB Image | Image loading (stb_image.h) |
| STB Image Write | Image writing (stb_image_write.h) |

---

## Networking & API

### HTTP Client
- **Library:** Dio
- **Base URL:** Defined in `api_constants.dart`
- **Interceptors:**

| Interceptor | Purpose |
|-------------|---------|
| AuthInterceptor | Adds auth tokens to requests |
| ErrorInterceptor | Centralized error handling |
| LoggingInterceptor | Request/response logging |
| RetryInterceptor | Auto-retry with exponential backoff |
| SSLPinningInterceptor | Certificate pinning for security |

### API Endpoints (via Data Sources)
| Data Source | Endpoints |
|-------------|-----------|
| AuthRemoteDataSource | Login, Register, Refresh Token |
| TemplateRemoteDataSource | List Templates, Get Template, Search, Get Categories, Get Featured, Request Access |

---

## Local Persistence & Caching

| Storage | Purpose |
|---------|---------|
| Image Projects | JSON files in app documents directory |
| Video Projects | JSON files with timeline, clips, tracks metadata |
| Templates | Encrypted template files cached locally |
| Image Cache | Network image cache (cached_network_image) |
| Thumbnail Cache | Video clip thumbnail previews |
| Frame Cache | Video timeline frame cache (configurable size) |
| Secure Storage | Auth tokens in platform keychain/keystore |

---

## Security

| Feature | Implementation |
|---------|----------------|
| Auth Tokens | Stored in flutter_secure_storage (Keychain/Keystore) |
| SSL Pinning | Certificate pinning via Dio interceptor |
| Template Encryption | AES-GCM symmetric + RSA-OAEP key exchange |
| Secure Memory | C++ secure memory wiping for crypto keys |
| Token Refresh | Automatic refresh on expiry |

---

## Theme & Styling

### Colors (`lib/core/theme/app_colors.dart`)
- Brand primary, secondary, tertiary
- Editor background and surface colors
- Semantic: success (green), warning (amber), error (red)
- Neon glow effects for active states

### Typography (`lib/core/theme/app_typography.dart`)
- Headings, body, labels, captions
- Consistent font scale

### Spacing (`lib/core/theme/app_spacing.dart`)
- Consistent spacing scale

### Theme (`lib/core/theme/app_theme.dart`)
- Light and dark themes
- Material 3 design system
- System theme detection
- Neon glow decorations (`neon_glow.dart`)

---

## Dependencies

### Production
| Package | Version | Purpose |
|---------|---------|---------|
| flutter_riverpod | ^2.6.1 | State management |
| go_router | ^14.8.1 | Declarative navigation |
| dio | ^5.7.0 | HTTP client |
| ffi | ^2.1.3 | Native FFI bridge |
| media_kit | ^1.2.6 | Video playback |
| media_kit_video | ^2.0.1 | Video widget |
| image_picker | ^1.1.2 | Photo/video picker |
| file_picker | ^10.3.10 | File browser |
| path_provider | ^2.1.5 | App directories |
| flutter_secure_storage | ^9.2.4 | Secure credential storage |
| crypto | ^3.0.6 | Cryptographic primitives |
| cached_network_image | ^3.4.1 | Image caching |
| connectivity_plus | ^6.1.1 | Network state detection |
| ffmpeg_kit_flutter_new_min_gpl | ^2.1.1 | FFmpeg video processing |
| device_info_plus | ^11.2.0 | Device information |
| package_info_plus | ^8.1.3 | App version info |
| intl | ^0.19.0 | Internationalization |
| logger | ^2.5.0 | Logging |
| equatable | ^2.0.7 | Value equality |
| freezed_annotation | ^2.4.4 | Immutable data classes |
| json_annotation | ^4.9.0 | JSON serialization |

### Development
| Package | Version | Purpose |
|---------|---------|---------|
| flutter_lints | ^5.0.0 | Lint rules |
| build_runner | ^2.4.14 | Code generation |
| freezed | ^2.5.7 | Immutable class generation |
| json_serializable | ^6.9.2 | JSON code generation |
| riverpod_generator | ^2.6.3 | Provider code generation |
| riverpod_lint | ^2.6.3 | Provider linting |
| mockito | ^5.4.5 | Test mocking |
| ffigen | ^20.1.1 | FFI binding generation |

---

## File Structure

```
GOPOST-APP/
├── gopost_app/
│   ├── lib/
│   │   ├── main.dart                          # App entry point
│   │   ├── app.dart                           # GopostApp widget
│   │   ├── auth/                              # Authentication module
│   │   │   ├── data/datasources/              # API data sources
│   │   │   ├── data/repositories/             # Repository implementations
│   │   │   ├── domain/entities/               # User, AuthTokens
│   │   │   ├── domain/repositories/           # Repository interfaces
│   │   │   ├── domain/usecases/               # Login, Register, Logout
│   │   │   └── presentation/                  # Screens, providers
│   │   ├── core/                              # Shared infrastructure
│   │   │   ├── cache/                         # Image cache manager
│   │   │   ├── config/                        # App configuration
│   │   │   ├── constants/                     # API constants
│   │   │   ├── di/                            # Dependency injection
│   │   │   ├── error/                         # Error handling
│   │   │   ├── logging/                       # AppLogger
│   │   │   ├── navigation/                    # GoRouter, MainShell
│   │   │   ├── network/                       # HTTP client, interceptors
│   │   │   ├── security/                      # SSL pinning, secure storage
│   │   │   └── theme/                         # Colors, typography, themes
│   │   ├── go_craft/                          # AI-assisted creation
│   │   │   ├── data/, domain/, presentation/
│   │   ├── image_editor/                      # Image editor module
│   │   │   ├── data/repositories/             # Canvas, filter, export repos
│   │   │   ├── domain/entities/               # Layer, canvas, text, filter
│   │   │   ├── domain/repositories/           # Repository interfaces
│   │   │   └── presentation/                  # Screens, widgets, providers
│   │   ├── rendering_bridge/                  # Native engine bridge
│   │   │   ├── engine_api.dart                # Abstract engine interfaces
│   │   │   ├── stub_video_timeline_engine.dart # Stub when native unavailable
│   │   │   ├── stub_image_editor_engine.dart  # Stub when native unavailable
│   │   │   ├── video_engine_providers.dart     # Riverpod provider
│   │   │   ├── template_bridge.dart           # Template encryption bridge
│   │   │   └── ffi/                           # FFI bindings to C++
│   │   ├── template_browser/                  # Template browsing module
│   │   │   ├── data/, domain/, presentation/
│   │   └── video_editor/                      # Video editor module
│   │       ├── data/                          # Local datasource, repository
│   │       ├── domain/models/                 # VideoProject, effects, etc.
│   │       ├── domain/services/               # Export, proxy, thumbnail
│   │       └── presentation/                  # Screens, widgets, providers
│   │           ├── providers/delegates/       # SRP delegates (playback, etc.)
│   │           ├── screens/                   # Editor, export, project list
│   │           └── widgets/                   # Timeline, preview, panels
│   ├── native/                                # C++ native engine
│   │   ├── gopost_core/                       # Core: engine, crypto, GPU
│   │   ├── gopost_image_engine/               # Image: canvas, effects, export
│   │   └── gopost_video_engine/               # Video: timeline, decode, export
│   └── pubspec.yaml                           # Dependencies
```

**Total Dart files:** ~196
**Total C/C++ files:** ~73
