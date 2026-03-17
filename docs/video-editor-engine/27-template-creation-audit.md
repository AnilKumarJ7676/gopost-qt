# VE-27. Video Template Creation – Feature Support Audit

This document audits whether the implemented video editor engine features **fully support template creation**: save as template, load template, customize placeholders, and open in editor.

---

## 1. Save as Template

| Area | Status | Notes |
|------|--------|--------|
| **Serialization** | ✅ Supported | `VideoProject.toMap()` / `VideoClip.toMap()` include all implemented fields: `timelineIn/Out`, `sourceIn/Out`, `speed`, `opacity`, `blendMode`, `effectHash`, `effects`, `colorGrading`, `presetFilter`, `transitionIn`, `transitionOut`, `keyframes`, `audio`. Tracks include `audioSettings`. |
| **Placeholders** | ✅ Supported | `SaveAsVideoTemplateDialog` marks clips as placeholders (`placeholderKey`), exports `editableFields` with `clip_id` and `key`, and sends `contentBase64` (project JSON) to `createFromEditor`. |
| **Backend** | ✅ Assumed | `templateRemoteDataSource.createFromEditor` sends name, description, type `'video'`, categoryId, tags, width, height, layerCount, editableFields, contentBase64. |

**Conclusion:** Saving a video project as a template **fully supports** all current editor features; nothing is dropped in serialization.

---

## 2. Load Template Content

| Area | Status | Notes |
|------|--------|--------|
| **API** | ❌ Gap | `getTemplateById` (and `TemplateModel` / `TemplateEntity`) do **not** include template content (`content_base64` or equivalent). To load a template into the editor, the backend must return content for the template, or a separate endpoint (e.g. `getTemplateContent(id)`) must be used. |
| **Decode** | ✅ Ready | `VideoProject.fromMap()` and `VideoClip.fromMap()` deserialize all of the same fields that are written by `toMap()`, so once content JSON is available, it can be decoded. |
| **Placeholder substitution** | ⚠️ Not wired | Replacing placeholder clips with user-chosen values (from customization screen) is not implemented: no code path decodes template content and applies `_customValues` to clips by `placeholderKey` or `clip_id`. |

---

## 3. Open Editor with a Project or Template

| Area | Status | Notes |
|------|--------|--------|
| **Saved project** | ❌ Gap | `ProjectListScreen` navigates to `/editor/video?projectId=...`, but `VideoEditorScreen` does not read `projectId`; it always calls `initTimeline()` and creates a new project. So opening a **saved project** from the list does not load that project. |
| **Template (after customize)** | ❌ Gap | `VideoTemplateCustomizationScreen` "Export" does `context.push('/editor/video')` with no arguments. Template id and custom values are not passed; the editor opens blank. |
| **Route for video customize** | ❌ Gap | On template download complete, **video** templates with editable fields currently navigate to `/editor/video` (blank). They should navigate to `/editor/video/customize/:id` so the user can fill placeholders before opening the editor. |

---

## 4. Feature ↔ Template Support Matrix

| Editor feature | In toMap/fromMap? | In template save? | Restored when load? |
|----------------|-------------------|-------------------|----------------------|
| Tracks (video/audio/title) | ✅ | ✅ | ✅ (once load path exists) |
| Clips (timeline/source range, speed) | ✅ | ✅ | ✅ |
| Effects (per-clip list) | ✅ | ✅ | ✅ |
| Color grading | ✅ | ✅ | ✅ |
| Preset filter | ✅ | ✅ | ✅ |
| Transition in/out | ✅ | ✅ | ✅ |
| Keyframes | ✅ | ✅ | ✅ |
| Clip audio (volume, pan, fades, mute) | ✅ | ✅ | ✅ |
| Track audio (volume, pan, mute, solo) | ✅ | ✅ | ✅ |
| Placeholder clips (key, clip_id) | ✅ | ✅ | N/A (substitution not implemented) |

All **implemented** video editor features are serialized and would round-trip **if** a load path existed.

---

## 5. Gaps to Support Template Creation Fully

1. **Template detail → video customize**  
   When a **video** template has `editableFields.isNotEmpty`, navigate to `/editor/video/customize/:id` instead of `/editor/video`.

2. **Editor with projectId**  
   `VideoEditorScreen` should read `projectId` from route query params and, when present, load that project from `VideoProjectRepository` and call a load-from-project API on `TimelineNotifier` so the editor opens with the saved project.

3. **Template content API**  
   Backend (or a new endpoint) must return template content (e.g. the stored `content_base64` or project JSON) for a given template id so the app can decode it and apply placeholders.

4. **Customization → editor with template**  
   After the user fills placeholders on `VideoTemplateCustomizationScreen`, "Export" should:  
   - Fetch template content (once API exists).  
   - Decode to `VideoProject`.  
   - Substitute placeholder clips using `_customValues` (by `placeholderKey` / `clip_id` from editableFields).  
   - Open the video editor with this project (reuse the same mechanism as loading by projectId, or a dedicated "open with project" entry point).

5. **Native engine sync when loading project**  
   When the editor is opened with an existing project (saved or from template), `TimelineNotifier` must create a timeline and re-add all clips (and restore S10 state) so the native engine and Dart state stay in sync (same as undo/redo and project load today).

---

## 6. Summary

- **Save as template:** All current video editor features are supported; serialization and placeholder metadata are complete.
- **Load and use template:** Partially addressed:
  - **Done:** (1) Video templates with editable fields now route to `/editor/video/customize/:id`. (2) `VideoEditorScreen` reads `projectId` from the route and loads the project via `TimelineNotifier.loadProject()`. (3) `TimelineNotifier.loadProject(VideoProject)` creates a new native timeline, re-adds tracks and clips, and restores S10 state (effects, color grading, transitions, keyframes, audio). (4) `VideoTemplateCustomizationScreen` "Export" passes `templateId` and `customValues` (base64-encoded JSON) as query params to `/editor/video`.
  - **Still missing:** Template content is not returned by the API (`getTemplateById` has no content). Placeholder substitution (decode content, apply `customValues` to clips by key) and opening the editor with that project are not implemented until the backend exposes template content.
- With template content API and placeholder application in the app, template creation would be **fully** supported end-to-end.
