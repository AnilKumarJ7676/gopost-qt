
## VE-16. Project Format and Serialization

### 16.1 Project File Format (.gpproj)

```
.gpproj structure (MessagePack binary + optional embedded assets):

┌──────────────────────────────────────┐
│ Header                                │
│   Magic: "GPVE" (4 bytes)            │
│   Version: uint32                     │
│   Flags: uint32                       │
│   Created: int64 (unix timestamp)     │
│   Modified: int64                     │
│   Engine Version: string              │
└──────────────────────────────────────┘
│ Project Data (MessagePack)            │
│   ├─ timeline_settings               │
│   ├─ tracks[]                        │
│   │   ├─ clips[]                     │
│   │   │   ├─ media_ref               │
│   │   │   ├─ effects[]               │
│   │   │   ├─ keyframes[]             │
│   │   │   ├─ masks[]                 │
│   │   │   └─ transitions[]           │
│   │   └─ track_effects[]             │
│   ├─ compositions[]                  │
│   │   └─ layers[]                    │
│   ├─ media_pool[]                    │
│   │   ├─ path / uri                  │
│   │   ├─ proxy_path                  │
│   │   ├─ media_info cache            │
│   │   └─ thumbnail cache ref         │
│   ├─ master_effects[]                │
│   ├─ markers[]                       │
│   └─ project_settings                │
└──────────────────────────────────────┘
│ Thumbnail Cache (optional embedded)   │
│   WebP thumbnails for media pool     │
└──────────────────────────────────────┘
│ Waveform Cache (optional embedded)    │
│   Pre-computed waveform data          │
└──────────────────────────────────────┘
```

### 16.2 Undo/Redo System

```cpp
namespace gp::project {

// Command pattern for undo/redo
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string description() const = 0;
    virtual size_t memory_cost() const = 0;

    // Coalescing: merge with previous command of same type
    virtual bool can_merge(const Command& other) const { return false; }
    virtual void merge(const Command& other) {}
};

class UndoStack {
public:
    void push(std::unique_ptr<Command> cmd);    // Execute + push to undo stack
    void undo();
    void redo();

    bool can_undo() const;
    bool can_redo() const;
    std::string undo_description() const;
    std::string redo_description() const;
    std::vector<std::string> undo_history(int max_count = 50) const;

    void set_max_depth(int depth);     // Default 200
    void clear();

    // Save point tracking for "unsaved changes" detection
    void mark_save_point();
    bool is_at_save_point() const;

private:
    std::deque<std::unique_ptr<Command>> undo_stack_;
    std::deque<std::unique_ptr<Command>> redo_stack_;
    int max_depth_ = 200;
    int save_point_ = -1;
};

// Concrete commands
class AddClipCommand : public Command { /* ... */ };
class RemoveClipCommand : public Command { /* ... */ };
class MoveClipCommand : public Command { /* ... */ };
class TrimClipCommand : public Command { /* ... */ };
class SplitClipCommand : public Command { /* ... */ };
class AddEffectCommand : public Command { /* ... */ };
class RemoveEffectCommand : public Command { /* ... */ };
class ModifyEffectParamCommand : public Command { /* ... */ };
class AddKeyframeCommand : public Command { /* ... */ };
class RemoveKeyframeCommand : public Command { /* ... */ };
class ModifyKeyframeCommand : public Command { /* ... */ };
class AddTrackCommand : public Command { /* ... */ };
class RemoveTrackCommand : public Command { /* ... */ };
class ReorderLayerCommand : public Command { /* ... */ };
class ModifyTransformCommand : public Command { /* ... */ };
class AddTransitionCommand : public Command { /* ... */ };
class ModifyTextCommand : public Command { /* ... */ };
class ModifyMaskCommand : public Command { /* ... */ };

// Compound command: group multiple commands as one undo step
class CompoundCommand : public Command {
    std::vector<std::unique_ptr<Command>> children_;
    /* ... */
};

} // namespace gp::project
```

### 16.3 Auto-Save System

```cpp
namespace gp::project {

class AutoSaveManager {
public:
    AutoSaveManager(ProjectSerializer& serializer);

    void enable(int interval_seconds = 30);
    void disable();
    void force_save();

    // Recovery
    bool has_recovery_data(const std::string& project_path) const;
    bool recover(const std::string& project_path);
    void discard_recovery(const std::string& project_path);

    // Auto-save runs in background thread, serializes project
    // to a recovery file alongside the main project file
    // Format: <project_name>.gpproj.recovery

private:
    void auto_save_loop();
    std::thread save_thread_;
    std::atomic<bool> active_;
    std::atomic<bool> dirty_;   // Set on any project modification
    int interval_seconds_;
};

} // namespace gp::project
```

### 16.4 Proxy Workflow

```cpp
namespace gp::project {

struct ProxyConfig {
    int max_width;          // 1280 for half-res proxy
    int max_height;
    std::string codec;      // "h264" (fast decode)
    int crf;                // 23 (lighter quality OK for proxy)
    Rational frame_rate;    // Match source or halved
};

class ProxyManager {
public:
    // Generate proxy for a media asset (async, returns task handle)
    TaskHandle generate_proxy(int32_t media_ref_id, const ProxyConfig& config);

    // Check if proxy exists and is up-to-date
    bool has_proxy(int32_t media_ref_id) const;

    // Switch between original and proxy for all media
    void enable_proxies(bool enabled);
    bool proxies_enabled() const;

    // Get path to proxy file
    std::string proxy_path(int32_t media_ref_id) const;

    // Cleanup unused proxies
    void cleanup_unused(const std::vector<int32_t>& active_media_refs);
};

} // namespace gp::project
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 2: Timeline Engine |
| **Sprint(s)** | VE-Sprint 6 (Weeks 11-12) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [15-media-io-codec](15-media-io-codec.md) |
| **Successor** | [17-plugin-architecture](17-plugin-architecture.md) |
| **Story Points Total** | 98 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| VE-259 | As a C++ engine developer, I want .gpproj file format header (magic, version, timestamps) so that project files are identifiable and versioned | - Magic "GPVE" (4 bytes)<br/>- Version uint32, Flags uint32<br/>- Created/Modified timestamps, Engine Version string | 3 | P0 | — |
| VE-260 | As a C++ engine developer, I want MessagePack serialization of timeline/tracks/clips so that project structure is persisted | - Timeline settings, tracks array<br/>- Clips with media_ref, in/out points<br/>- Round-trip fidelity | 5 | P0 | VE-259 |
| VE-261 | As a C++ engine developer, I want MessagePack serialization of compositions/layers so that composition data is saved | - Compositions array with layers<br/>- Layer hierarchy, blend modes, transforms<br/>- Round-trip fidelity | 5 | P0 | VE-260 |
| VE-262 | As a C++ engine developer, I want MessagePack serialization of media pool references so that media links are preserved | - media_pool with path/uri, proxy_path<br/>- media_info cache, thumbnail ref<br/>- Relative path resolution | 3 | P0 | VE-260 |
| VE-263 | As a C++ engine developer, I want MessagePack serialization of effects/keyframes/masks so that clip state is complete | - effects[], keyframes[], masks[] per clip<br/>- Parameter values, keyframe curves<br/>- Round-trip fidelity | 5 | P0 | VE-260 |
| VE-264 | As a C++ engine developer, I want optional thumbnail cache embedding so that projects can be self-contained | - Optional WebP thumbnails in .gpproj<br/>- Configurable (embed vs external)<br/>- Size limit for embedded cache | 3 | P2 | VE-262 |
| VE-265 | As a C++ engine developer, I want optional waveform cache embedding so that audio visualization loads fast | - Pre-computed waveform data in .gpproj<br/>- Optional embedding<br/>- Lazy load when external | 2 | P2 | VE-262 |
| VE-266 | As a C++ engine developer, I want Command abstract class (execute/undo/description/memory_cost) so that undo/redo is consistent | - Command base with execute(), undo()<br/>- description(), memory_cost()<br/>- can_merge(), merge() for coalescing | 3 | P0 | — |
| VE-267 | As a C++ engine developer, I want UndoStack (push/undo/redo, 200 depth) so that user actions can be reversed | - push() executes and adds to stack<br/>- undo(), redo() with depth limit 200<br/>- can_undo(), can_redo(), undo_description() | 5 | P0 | VE-266 |
| VE-268 | As a C++ engine developer, I want all concrete commands (AddClip, RemoveClip, MoveClip, TrimClip, SplitClip, etc.) so that all edits are undoable | - AddClip, RemoveClip, MoveClip, TrimClip, SplitClip<br/>- AddEffect, RemoveEffect, ModifyParam<br/>- AddKeyframe, RemoveKeyframe, ModifyKeyframe<br/>- AddTrack, RemoveTrack, ReorderLayer, ModifyTransform<br/>- AddTransition, ModifyText, ModifyMask | 13 | P0 | VE-267 |
| VE-269 | As a C++ engine developer, I want CompoundCommand for grouped operations so that multi-step edits undo as one | - CompoundCommand holds child commands<br/>- execute/undo runs all children<br/>- Single undo step for group | 3 | P1 | VE-267 |
| VE-270 | As a C++ engine developer, I want command coalescing (merge rapid parameter changes) so that undo history is clean | - can_merge() identifies mergeable commands<br/>- merge() combines parameter changes<br/>- Time window for coalescing | 5 | P1 | VE-267 |
| VE-271 | As a C++ engine developer, I want AutoSaveManager (30s interval, background thread, dirty flag) so that work is not lost | - enable(30), disable(), force_save()<br/>- Background thread, non-blocking<br/>- dirty flag on project modification | 5 | P0 | VE-260 |
| VE-272 | As a C++ engine developer, I want recovery file (.gpproj.recovery) so that crash recovery is possible | - Save to .gpproj.recovery alongside project<br/>- has_recovery_data(), recover(), discard_recovery()<br/>- Clear recovery on successful save | 5 | P0 | VE-271 |
| VE-273 | As a C++ engine developer, I want ProxyManager (generate, enable/disable, cleanup) so that proxy workflow is supported | - generate_proxy() async with config<br/>- has_proxy(), enable_proxies(), proxy_path()<br/>- cleanup_unused() for orphaned proxies | 5 | P1 | VE-262 |
| VE-274 | As a QA engineer, I want project save/load round-trip validation so that no data is lost | - Save project, load, compare to original<br/>- All tracks, clips, effects, keyframes match<br/>- Automated test suite | 5 | P0 | VE-263 |
| VE-275 | As a Tech Lead, I want backward compatibility handling for project version upgrades so that old projects open correctly | - Version check on load<br/>- Migration path for older versions<br/>- Document version history | 5 | P1 | VE-259 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
