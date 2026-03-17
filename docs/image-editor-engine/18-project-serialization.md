
## IE-18. Project Format and Serialization

### 18.1 Project File Format (.gpimg)

```
.gpimg structure (MessagePack binary + optional embedded assets):

┌──────────────────────────────────────┐
│ Header                                │
│   Magic: "GPIE" (4 bytes)            │
│   Version: uint32                     │
│   Flags: uint32                       │
│   Created: int64 (unix timestamp)     │
│   Modified: int64                     │
│   Engine Version: string              │
└──────────────────────────────────────┘
│ Project Data (MessagePack)            │
│   ├─ canvas_settings                 │
│   │   ├─ width, height, dpi          │
│   │   ├─ color_space, bit_depth      │
│   │   └─ background_color            │
│   ├─ layers[]                        │
│   │   ├─ type, name, visibility      │
│   │   ├─ transform                   │
│   │   ├─ opacity, blend_mode         │
│   │   ├─ effects[]                   │
│   │   ├─ layer_style                 │
│   │   ├─ layer_mask                  │
│   │   ├─ vector_mask                 │
│   │   ├─ keyframe_tracks[]           │
│   │   ├─ content_data                │
│   │   │   ├─ image: media_ref + crop │
│   │   │   ├─ text: TextData          │
│   │   │   ├─ shape: ShapeData        │
│   │   │   ├─ group: children[]       │
│   │   │   ├─ adjustment: params      │
│   │   │   └─ smart_object: ref       │
│   │   └─ children[] (for groups)     │
│   ├─ media_pool[]                    │
│   │   ├─ path / uri                  │
│   │   ├─ embedded_data (optional)    │
│   │   └─ media_info cache            │
│   ├─ guides[]                        │
│   ├─ grid_settings                   │
│   ├─ viewport_state                  │
│   ├─ template_data (optional)        │
│   │   ├─ placeholders[]              │
│   │   ├─ style_controls[]            │
│   │   └─ palette                     │
│   └─ project_settings                │
│       ├─ color_management            │
│       ├─ snap_settings               │
│       └─ history_state_index         │
└──────────────────────────────────────┘
│ Embedded Assets (optional)            │
│   ├─ Images (WebP compressed)        │
│   ├─ Fonts (used in text layers)     │
│   ├─ Patterns (used in fills)        │
│   ├─ Custom brushes                  │
│   └─ Layer mask data                 │
└──────────────────────────────────────┘
│ Thumbnail Cache (optional)            │
│   Layer thumbnails (128x128 WebP)    │
└──────────────────────────────────────┘
│ History Snapshot (optional)           │
│   Last N undo states (compressed)    │
└──────────────────────────────────────┘
```

### 18.2 Undo/Redo System

```cpp
namespace gp::project_ie {

// Command pattern for undo/redo (shared architecture from VE-16)
// Extended with image-editor-specific commands

// Shared base (from VE)
// class Command { execute(), undo(), description(), memory_cost(), can_merge(), merge() }
// class UndoStack { push(), undo(), redo(), mark_save_point(), ... }
// class CompoundCommand { children_ }

// IE-specific commands
class AddLayerCommand : public Command { /* ... */ };
class RemoveLayerCommand : public Command { /* ... */ };
class ReorderLayerCommand : public Command { /* ... */ };
class DuplicateLayerCommand : public Command { /* ... */ };
class MergeLayersCommand : public Command { /* ... */ };
class FlattenCommand : public Command { /* ... */ };
class ModifyTransformCommand : public Command { /* ... */ };
class ModifyLayerPropertyCommand : public Command { /* ... */ };  // opacity, blend, visibility
class AddEffectCommand : public Command { /* ... */ };
class RemoveEffectCommand : public Command { /* ... */ };
class ModifyEffectParamCommand : public Command { /* ... */ };
class ModifyLayerStyleCommand : public Command { /* ... */ };
class AddKeyframeCommand : public Command { /* ... */ };

// Content modification commands
class ModifyPixelsCommand : public Command {
    // Stores affected region + before/after pixel data
    // For brush strokes, healing, cloning, etc.
    Rect affected_region;
    std::vector<uint8_t> before_pixels;  // Compressed
    std::vector<uint8_t> after_pixels;   // Compressed
};

class ModifyTextCommand : public Command { /* ... */ };
class ModifyShapeCommand : public Command { /* ... */ };
class ModifyPathCommand : public Command { /* ... */ };
class ModifySelectionCommand : public Command { /* ... */ };
class ModifyMaskCommand : public Command { /* ... */ };
class ModifyGuidesCommand : public Command { /* ... */ };
class CanvasResizeCommand : public Command { /* ... */ };
class CropCommand : public Command { /* ... */ };

// Transform commands (coalescing during drag)
class InteractiveTransformCommand : public Command {
    // Merges repeated transform updates during drag interaction
    bool can_merge(const Command& other) const override;
    void merge(const Command& other) override;
};

// Brush stroke command (combines all stamps in one stroke)
class BrushStrokeCommand : public Command {
    int32_t layer_id;
    Rect affected_region;
    std::vector<uint8_t> before_pixels;
    std::vector<uint8_t> after_pixels;
    size_t memory_cost() const override;
};

// History panel data
struct HistoryEntry {
    int32_t index;
    std::string description;
    std::string icon;              // "brush", "transform", "filter", "text", etc.
    GpuTexture snapshot;           // Canvas thumbnail at this state
    size_t memory_bytes;
    bool is_save_point;
};

class HistoryManager {
public:
    std::vector<HistoryEntry> get_history(int max_count = 50) const;
    GpuTexture get_snapshot(int32_t index) const;

    // Jump to specific history state (Photoshop-style history panel)
    void go_to_state(int32_t index);

    // Create snapshot for history brush
    void create_snapshot(const std::string& name);
    std::vector<std::string> snapshots() const;

    // Memory management
    size_t total_history_memory() const;
    void purge_old_states(size_t max_bytes);
};

} // namespace gp::project_ie
```

### 18.3 Auto-Save System

```cpp
namespace gp::project_ie {

class AutoSaveManager {
public:
    AutoSaveManager(ProjectSerializer& serializer);

    void enable(int interval_seconds = 20);
    void disable();
    void force_save();

    // Recovery
    bool has_recovery_data(const std::string& project_path) const;
    bool recover(const std::string& project_path);
    void discard_recovery(const std::string& project_path);

    // Auto-save writes to: <project_name>.gpimg.recovery
    // On successful save, recovery file is deleted
    // On crash, recovery file is detected on next launch

    // Incremental save: only changed layers + undo delta
    void enable_incremental(bool enabled);

private:
    void auto_save_loop();
    std::thread save_thread_;
    std::atomic<bool> active_;
    std::atomic<bool> dirty_;
    int interval_seconds_;
    bool incremental_ = true;
};

} // namespace gp::project_ie
```

### 18.4 Project Serializer

```cpp
namespace gp::project_ie {

class ProjectSerializer {
public:
    // Save project to .gpimg file
    bool save(const canvas::Canvas& canvas, const std::string& path,
              const SaveOptions& options = {});

    // Load project from .gpimg file
    canvas::Canvas* load(const std::string& path);

    // Import from other formats
    canvas::Canvas* import_psd(const std::string& path);  // Adobe PSD (partial)
    canvas::Canvas* import_svg(const std::string& path);  // SVG as editable layers
    canvas::Canvas* import_image(const std::string& path); // Single image as canvas

    // Export to other formats
    bool export_psd(const canvas::Canvas& canvas, const std::string& path);

    struct SaveOptions {
        bool embed_media;              // Embed all referenced images
        bool embed_fonts;              // Embed used fonts
        bool include_history;          // Save undo history
        bool include_thumbnails;       // Save layer thumbnails
        int compression_level;         // 0-9
        bool portable;                 // Make fully self-contained
    };
};

} // namespace gp::project_ie
```

### 18.5 Sprint Planning

#### Sprint Assignment

| Sprint | Weeks | Stories | Focus |
|---|---|---|---|
| Sprint 6 | Wk 11-12 | IE-229 to IE-234 | Project format, save/load, undo base |
| Sprint 22 | Wk 41-42 | IE-235 to IE-240 | Commands, auto-save, recovery, PSD import |

#### User Stories

| ID | Story | Acceptance Criteria | Story Points | Sprint | Dependencies |
|---|---|---|---|---|---|
| IE-229 | As a developer, I want .gpimg format header + MessagePack schema so that projects have a defined format | - Magic "GPIE", version, flags, timestamps<br/>- MessagePack schema for canvas, layers, media pool<br/>- Engine version in header | 5 | Sprint 6 | — |
| IE-230 | As a user, I want project save (serialize canvas + layers) so that I can persist my work | - Serialize canvas settings, layers, effects<br/>- Media refs or embedded data<br/>- SaveOptions (embed, compress, portable) | 5 | Sprint 6 | IE-229 |
| IE-231 | As a user, I want project load (deserialize + reconstruct) so that I can reopen saved projects | - Deserialize MessagePack to canvas<br/>- Reconstruct layer tree and content<br/>- Load embedded or referenced media | 5 | Sprint 6 | IE-230 |
| IE-232 | As a developer, I want undo/redo command base class so that all edits are reversible | - Command base: execute(), undo(), description(), memory_cost()<br/>- can_merge(), merge() for coalescing<br/>- Shared with video editor (VE-16) | 3 | Sprint 6 | — |
| IE-233 | As a developer, I want undo stack (200 depth, coalescing) so that history is bounded and efficient | - Stack depth 200 (configurable)<br/>- Coalescing for compatible commands<br/>- mark_save_point() support | 3 | Sprint 6 | IE-232 |
| IE-234 | As a developer, I want pixel modification command (compress before/after) so that brush/healing undo is memory-efficient | - ModifyPixelsCommand with Rect + before/after<br/>- Compressed pixel storage<br/>- memory_cost() for purging | 5 | Sprint 6 | IE-232 |
| IE-235 | As a developer, I want interactive transform command (merge during drag) so that transform undo is single-step | - InteractiveTransformCommand merges during drag<br/>- can_merge() for consecutive transforms<br/>- Final state on release | 3 | Sprint 22 | IE-232 |
| IE-236 | As a developer, I want brush stroke command so that each stroke is one undo step | - BrushStrokeCommand per stroke<br/>- Affected region + before/after pixels<br/>- Combines all stamps in stroke | 3 | Sprint 22 | IE-234 |
| IE-237 | As a user, I want auto-save manager (20-second interval, incremental) so that I don't lose work | - Auto-save every 20 seconds when dirty<br/>- Incremental save (changed layers only)<br/>- Writes to .gpimg.recovery | 5 | Sprint 22 | IE-230 |
| IE-238 | As a user, I want crash recovery (detect + recover) so that I can restore after a crash | - Detect .gpimg.recovery on launch<br/>- Recover option in UI<br/>- Discard recovery option | 3 | Sprint 22 | IE-237 |
| IE-239 | As a user, I want history panel data (snapshots) so that I can see and jump to history states | - HistoryEntry: index, description, icon, snapshot thumbnail<br/>- get_history(), go_to_state()<br/>- Snapshot creation for history brush | 5 | Sprint 22 | IE-233 |
| IE-240 | As a user, I want PSD import (partial layers) so that I can open Adobe PSD files | - import_psd() parses PSD format<br/>- Partial layer support (raster, groups)<br/>- Smart objects, adjustment layers where supported | 8 | Sprint 22 | IE-231 |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 2: Layer System & Compositing |
| **Sprint(s)** | IE-Sprint 5-6 (Weeks 9-12) |
| **Team** | C/C++ Engine Developer (2), Tech Lead |
| **Predecessor** | [17-export-pipeline.md](17-export-pipeline.md) |
| **Successor** | [19-plugin-architecture.md](19-plugin-architecture.md) |
| **Story Points Total** | 75 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| IE-277 | As a developer, I want .gpimg header (magic "GPIE", version, timestamps) so that projects have a defined format | - Magic "GPIE" (4 bytes), version uint32, flags uint32<br/>- Created/Modified unix timestamps<br/>- Engine version string in header | 3 | P0 | — |
| IE-278 | As a developer, I want MessagePack serialization of canvas settings so that project structure is persisted | - Serialize width, height, dpi, color_space, bit_depth<br/>- background_color, grid_settings<br/>- MessagePack schema for canvas_settings | 5 | P0 | IE-277 |
| IE-279 | As a developer, I want MessagePack serialization of layer tree so that layers are persisted | - Serialize layer type, name, visibility, transform<br/>- opacity, blend_mode, effects, layer_style, layer_mask<br/>- Recursive children for groups | 8 | P0 | IE-277 |
| IE-280 | As a developer, I want MessagePack serialization of effects/styles/masks so that layer appearance is persisted | - Serialize effects[], layer_style, layer_mask, vector_mask<br/>- Effect parameters and keyframe tracks<br/>- Content data (image, text, shape, etc.) | 8 | P0 | IE-279 |
| IE-281 | As a developer, I want MessagePack serialization of media pool so that referenced assets are tracked | - Serialize media_pool[] with path/uri, embedded_data<br/>- media_info cache<br/>- Optional embedding of images, fonts, patterns | 5 | P0 | IE-277 |
| IE-282 | As a developer, I want thumbnail embedding so that layer thumbnails are stored in the project | - Thumbnail cache (128x128 WebP) per layer<br/>- Optional include_thumbnails in SaveOptions<br/>- Load thumbnails for history panel | 3 | P1 | IE-279 |
| IE-283 | As a developer, I want history state embedding so that undo states can be persisted | - Optional history snapshot in .gpimg<br/>- Last N undo states (compressed)<br/>- history_state_index in project_settings | 5 | P1 | IE-279 |
| IE-284 | As a developer, I want a Command abstract class so that all edits are reversible | - Command base: execute(), undo(), description(), memory_cost()<br/>- can_merge(), merge() for coalescing<br/>- Shared architecture with video editor | 3 | P0 | — |
| IE-285 | As a developer, I want UndoStack (200 steps) so that history is bounded and efficient | - Stack depth 200 (configurable)<br/>- push(), undo(), redo(), mark_save_point()<br/>- Coalescing for compatible commands | 3 | P0 | IE-284 |
| IE-286 | As a developer, I want concrete commands (AddLayer, RemoveLayer, ReorderLayer, etc.) so that layer operations are undoable | - AddLayerCommand, RemoveLayerCommand, ReorderLayerCommand<br/>- DuplicateLayerCommand, MergeLayersCommand, FlattenCommand<br/>- ModifyTransformCommand, ModifyLayerPropertyCommand | 5 | P0 | IE-284 |
| IE-287 | As a developer, I want CompoundCommand so that multi-step operations are single undo | - CompoundCommand with children_<br/>- execute/undo all children atomically<br/>- Support for grouped operations | 3 | P0 | IE-284 |
| IE-288 | As a user, I want AutoSaveManager (30s, dirty tracking) so that I don't lose work | - Auto-save every 30 seconds when dirty<br/>- Dirty flag tracking on modifications<br/>- Writes to .gpimg.recovery | 5 | P0 | IE-278 |
| IE-289 | As a user, I want recovery file (.gpimg.recovery) so that I can restore after a crash | - Detect .gpimg.recovery on launch<br/>- Recover option in UI<br/>- Discard recovery option, delete on successful save | 3 | P0 | IE-288 |
| IE-290 | As a developer, I want history panel data model so that the UI can display and navigate history | - HistoryEntry: index, description, icon, snapshot thumbnail<br/>- get_history(), go_to_state()<br/>- Snapshot creation for history brush | 5 | P0 | IE-285 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Google Test suite green
- [ ] Memory leak check (ASan) passing
- [ ] Performance benchmark recorded (no regression)
- [ ] C API header updated if public interface changed
- [ ] Sprint review demo completed
