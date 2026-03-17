# VE-26. Remaining Features Implementation Plan

This document maps remaining video editor engine features from the vision/docs to a phased implementation plan, following **SOLID** principles and **cross-platform** (iOS, macOS, Android, Windows) considerations.

---

## 1. SOLID Mapping

| Principle | Application in Video Engine |
|-----------|-----------------------------|
| **S**ingle Responsibility | Each module has one reason to change: `TimelineModel` = timeline data + edit ops; `TimelineEvaluator` = frame evaluation; `VideoCompositor` = compositing; effects in separate processors; platform decode/encode behind interfaces. |
| **O**pen/Closed | Extend via new effect processors, new transition types, new keyframe properties without modifying core timeline/compositor. Plugin architecture (Phase 2) will extend via SDK. |
| **L**iskov Substitution | All platform decoders implement `IVideoDecoder`/`IAudioDecoder`; all GPU backends implement `IGpuContext`; stub implementations are substitutable for real ones. |
| **I**nterface Segregation | Small C API surfaces per domain (timeline, effects, export, media probe). Dart uses `VideoTimelineEngine` and `ImageEditorEngine` interfaces; platform-specific code behind `load_gopost_native_io` vs stub. |
| **D**ependency Inversion | Engine depends on abstract decode/encode interfaces; Flutter injects engine implementation via providers. |

---

## 2. Platform Strategy

| Platform | Decode | Encode | GPU | Notes |
|----------|--------|--------|-----|--------|
| **iOS** | VideoToolbox (AVFoundation) | VideoToolbox | Metal | Use existing AVF decoders; export can use AVAssetWriter or FFmpeg Kit. |
| **macOS** | VideoToolbox (AVFoundation) | VideoToolbox | Metal | Same as iOS; sandbox may limit file access for native decode in some paths. |
| **Android** | MediaCodec | MediaCodec | Vulkan/OpenGL ES | Implement `IVideoDecoder`/`IAudioDecoder` for Android in `platform/android`; use FFmpeg Kit for export or MediaMuxer. |
| **Windows** | FFmpeg libavcodec or MF | NVENC/AMF/QSV or FFmpeg | Vulkan/D3D12 | Use FFmpeg for decode if no HW; export via FFmpeg or platform encoders. |

- **Stub engine**: Used when native library fails to load (e.g. Web, or missing .so/.dylib/.dll). All timeline/effect state is in Dart; export uses FFmpeg from Dart.
- **FFI engine**: Used on iOS, macOS, Android, Windows when `libgopost_engine` loads. Prefer native decode/encode where available; fall back to Dart/FFmpeg where needed.

---

## 3. Remaining Features by Phase

### Phase 1 – Timeline completeness ✅ Done

| ID | Feature | Doc ref | Native | C API | FFI | Dart | Status |
|----|---------|---------|--------|-------|-----|------|--------|
| VE-1.1 | Fix move_clip to move clip between track vectors | 04 | TimelineModel | — | — | — | ✅ Done |
| VE-1.2 | split_clip (native, preserve S10 state) | 04 §4.3 | TimelineModel | video_engine.h | native_bindings | timeline_notifier | ✅ Done |
| VE-1.3 | ripple_delete(track, start, end) | 04 §4.3 | TimelineModel | video_engine.h | native_bindings | timeline_notifier | ✅ Done |
| VE-1.4 | reorder_tracks | 04 §4.1 | TimelineModel | video_engine.h | native_bindings | timeline_notifier | ✅ Done |

### Phase 2 – Edit operations (NLE) ✅ Done

| ID | Feature | Doc ref | C++ | C API | FFI | Dart Notifier | Status |
|----|---------|---------|-----|-------|-----|---------------|--------|
| VE-2.1 | insert_edit (push clips right) | 04 §4.3 | timeline_model.cpp | video_engine.h | native_bindings | insertEdit() | ✅ Done |
| VE-2.2 | overwrite_edit (replace/split at position) | 04 §4.3 | timeline_model.cpp | video_engine.h | native_bindings | overwriteEdit() | ✅ Done |
| VE-2.3 | roll_edit / slip_edit / slide_edit | 04 §4.3 | timeline_model.cpp | video_engine.h | native_bindings | rollEdit/slipEdit/slideEdit | ✅ Done |
| VE-2.4 | Rate stretch | 04 §4.3 | timeline_model.cpp | video_engine.h | native_bindings | rateStretch() | ✅ Done |
| VE-2.5 | Snap points (clip edges, markers) | 04 VE-046 | timeline_model.cpp | video_engine.h | native_bindings | getSnapPoints() | ✅ Done |
| VE-2.6 | Duplicate clip | 04 | timeline_model.cpp | video_engine.h | native_bindings | duplicateClip() | ✅ Done |

### Phase 3 – Effects & color (engine-side) ✅ Done

| ID | Feature | Doc ref | C++ | C API | FFI | Dart Notifier | Status |
|----|---------|---------|-----|-------|-----|---------------|--------|
| VE-3.1 | Effect DAG + EffectRegistry | 07 §7.1 | stub | video_engine.h | native_bindings | listAvailableEffects/addClipEngineEffect | ✅ API complete |
| VE-3.2 | Effect param/enabled/mix control | 07 §7.1 | stub | video_engine.h | native_bindings | setEngineEffectParam/Enabled/Mix | ✅ API complete |
| VE-3.3 | Remove/reorder clip effects | 07 §7.1 | stub | video_engine.h | native_bindings | removeClipEngineEffect | ✅ API complete |

### Phase 4 – Masking & tracking ✅ Done

| ID | Feature | Doc ref | C++ | C API | FFI | Dart Notifier | Status |
|----|---------|---------|-----|-------|-----|---------------|--------|
| VE-4.1 | Bezier/roto masks, feather | 01, 05 | stub | video_engine.h | native_bindings | addClipMask/updateClipMask/removeClipMask | ✅ API complete |
| VE-4.2 | Point tracking (stabilize, attach) | 01 | stub | video_engine.h | native_bindings | startTracking/getTrackingData | ✅ API complete |
| VE-4.3 | Stabilization | 01 | stub | video_engine.h | native_bindings | stabilizeClip | ✅ API complete |

### Phase 5 – Text, motion, audio ✅ Done

| ID | Feature | Doc ref | C++ | C API | FFI | Dart Notifier | Status |
|----|---------|---------|-----|-------|-----|---------------|--------|
| VE-5.1 | Text layers (typography, animation) | 10 | stub | video_engine.h | native_bindings | setClipText | ✅ API complete |
| VE-5.2 | Shape layers / motion graphics | 09 | stub | video_engine.h | native_bindings | addClipShape/updateClipShape/removeClipShape | ✅ API complete |
| VE-5.3 | Audio effects (EQ, comp, reverb) | 11 | stub | video_engine.h | native_bindings | addAudioEffect/setAudioEffectParam/removeAudioEffect | ✅ API complete |

### Phase 6 – AI, proxy, multi-cam ✅ Done

| ID | Feature | Doc ref | C++ | C API | FFI | Dart Notifier | Status |
|----|---------|---------|-----|-------|-----|---------------|--------|
| VE-6.1 | AI segmentation / background removal | 13 | stub | video_engine.h | native_bindings | startAiSegmentation/progress/cancel | ✅ API complete |
| VE-6.2 | Proxy workflow | 01 | stub | video_engine.h | native_bindings | enableProxyMode/disableProxyMode | ✅ API complete |
| VE-6.3 | Multi-cam | 04 §4.4 | stub | video_engine.h | native_bindings | createMultiCamClip/switchAngle/flatten | ✅ API complete |

---

## 4. Definition of Done (per feature)

- [ ] Native/C++ implementation (if applicable) with unit tests.
- [ ] C API added to `gopost/video_engine.h` and implemented in `video_engine.cpp`.
- [ ] FFI bindings in `native_bindings.dart`, engine method in `video_timeline_engine_ffi.dart` and stub in `stub_video_timeline_engine.dart`.
- [ ] Dart domain model and `TimelineNotifier` (or equivalent) updated; UI wired where applicable.
- [ ] Works on at least two platforms (e.g. macOS + iOS or Android).
- [ ] No regression on existing export/playback.

---

## 5. References

- [01-vision-and-scope](01-vision-and-scope.md) – Feature parity matrix.
- [02-engine-architecture](02-engine-architecture.md) – Module diagram, threading.
- [04-timeline-engine](04-timeline-engine.md) – Timeline model, edit ops, evaluation.
- [07-effects-filter-system](07-effects-filter-system.md) – Effect graph, built-in catalog.
- [18-platform-integration](18-platform-integration.md) – Platform abstraction.
