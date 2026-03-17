# Gopost — Accessibility Standards and Implementation

> **Version:** 1.0.0
> **Date:** February 23, 2026
> **Audience:** Flutter Engineers, QA Engineers, UX Designers
> **References:** [Design System — Section 12](../design-system/01-design-system.md), WCAG 2.1 AA

---

## Table of Contents

1. [Accessibility Goals](#1-accessibility-goals)
2. [Compliance Targets](#2-compliance-targets)
3. [Cross-Cutting Standards](#3-cross-cutting-standards)
4. [Module-Specific Requirements](#4-module-specific-requirements)
5. [Screen Reader Semantics](#5-screen-reader-semantics)
6. [Keyboard and Focus Management](#6-keyboard-and-focus-management)
7. [Motion and Animation](#7-motion-and-animation)
8. [Color and Contrast](#8-color-and-contrast)
9. [Typography and Dynamic Type](#9-typography-and-dynamic-type)
10. [Platform-Specific Considerations](#10-platform-specific-considerations)
11. [Testing Matrix](#11-testing-matrix)
12. [Audit and Remediation Process](#12-audit-and-remediation-process)
13. [Implementation Guide](#13-implementation-guide)
14. [Sprint Stories](#14-sprint-stories)

---

## 1. Accessibility Goals

| Goal | Measure |
|------|---------|
| WCAG 2.1 AA compliance for all user-facing screens | Zero AA violations in audit report |
| Fully usable with VoiceOver (iOS/macOS) and TalkBack (Android) | Complete user journey possible: browse → preview → edit → export |
| Keyboard-navigable on desktop and web | All interactive elements reachable via Tab/Shift-Tab; activated via Enter/Space |
| Respects system accessibility settings | Large text, reduced motion, high contrast, bold text honored |
| No information conveyed by color alone | Icons, labels, or patterns supplement color-coded status |

---

## 2. Compliance Targets

| Standard | Level | Applies To |
|----------|-------|-----------|
| **WCAG 2.1 AA** | All success criteria | All platforms |
| **Section 508** | Conformance | If government/enterprise users targeted (post-launch) |
| **EN 301 549** | Applicable clauses | If EU distribution (post-launch) |
| **Apple Accessibility Guidelines** | Best practices | iOS, iPadOS, macOS |
| **Android Accessibility Guidelines** | Best practices | Android mobile, tablets |

---

## 3. Cross-Cutting Standards

These rules apply to every screen and component in the application.

### 3.1 Semantic Labels

| Element Type | Requirement | Example |
|-------------|-------------|---------|
| **Buttons** | Label describes the action, not the icon | "Save project" not "disk icon" |
| **Images** | Decorative: `excludeFromSemantics: true`. Informative: `semanticLabel` | Template thumbnail: "Travel vlog template preview" |
| **Icons** | Always pair with `Semantics` or `tooltip` | Back arrow: "Go back" |
| **Text fields** | `labelText` or `Semantics(label:)` | "Email address" |
| **Toggles / checkboxes** | Label + state (on/off) | "Dark mode, currently off" |
| **Lists** | Each item has role and content described | "Template: Cinematic Opener, 4.5 stars, Pro" |
| **Dialogs** | Title + purpose + actions described | "Confirm export. Export this template as 1080p MP4? Cancel or Export" |

### 3.2 Touch Targets

| Rule | Value |
|------|-------|
| Minimum touch target (mobile) | 48 × 48 dp (WCAG), expanded to 44 × 44 pt (Apple HIG) |
| Minimum clickable area (desktop/web) | 24 × 24 dp (with hover state) |
| Spacing between targets | ≥ 8 dp to prevent accidental taps |

### 3.3 Focus Indicators

| Platform | Focus Style |
|----------|-------------|
| iOS/Android | System default focus ring (high-contrast outline) |
| Desktop | 2dp outline in `AppColors.primary` with 4dp offset |
| Web | Browser default `:focus-visible` ring; do NOT suppress |

### 3.4 Error Identification

| Rule | Implementation |
|------|---------------|
| Errors announced by screen reader | `Semantics(liveRegion: true)` on error messages |
| Errors not identified by color alone | Error icon + text + red border |
| Error linked to field | `Semantics(label: 'Error: $errorMessage')` on the field |

---

## 4. Module-Specific Requirements

### 4.1 Template Browser

| Feature | Accessibility Requirement |
|---------|--------------------------|
| Template grid | Each card announces: template name, type (video/image), rating, tier (Free/Pro/Creator) |
| Infinite scroll | Screen reader announces "Loading more templates" when new page loads |
| Pull-to-refresh | Announced as "Refreshing templates" |
| Search | `Semantics(textField: true, label: 'Search templates')` |
| Filter chips | Each chip announces state (selected/unselected) and filter name |
| Empty state | "No templates found" announced |

### 4.2 Template Preview

| Feature | Accessibility Requirement |
|---------|--------------------------|
| Video preview player | Play/Pause announced; current time and duration readable; progress slider labeled |
| Image preview | Full-screen image has semantic description |
| "Use Template" CTA | Clearly labeled; announces subscription requirement if applicable |
| Swipe between previews | Screen reader announces navigation ("Template 2 of 5") |

### 4.3 Video Editor

| Feature | Accessibility Requirement |
|---------|--------------------------|
| Timeline | Each layer announced: "Layer 1: Video clip, 0 to 5 seconds". Layers reorderable via accessibility action |
| Playback controls | Play, Pause, Seek described. Current position announced on change |
| Tool palette | Each tool has name + description ("Trim tool: trim clip start and end") |
| Properties panel | Sliders have label, current value, min/max ("Opacity: 80%, range 0 to 100") |
| Undo/Redo | Action announced ("Undo: removed text layer") |
| Export dialog | Progress announced as percentage ("Exporting: 45%") with `Semantics(liveRegion: true)` |
| Canvas (GPU-rendered) | Not directly accessible; provide alternative text description of current frame composition when screen reader is active |

### 4.4 Image Editor

Same as video editor where applicable (layers, tools, properties, export). Additional:

| Feature | Accessibility Requirement |
|---------|--------------------------|
| Filter selection | Filter name + preview description ("Vintage: warm tones, reduced contrast") |
| Text tool | Text input field accessible; font/size/color selectors labeled |
| Sticker placement | Position announced ("Sticker placed at center") |

### 4.5 Subscription / Paywall

| Feature | Accessibility Requirement |
|---------|--------------------------|
| Plan comparison | Table/list with plan name, price, and feature list clearly structured for screen reader |
| Feature checkmarks | "Included" / "Not included" announced (not just ✓/✗ symbols) |
| Purchase button | "Subscribe to Pro for $9.99 per month" — full context in one label |
| Trial notice | "7-day free trial, then $9.99/month" announced |

### 4.6 Admin Portal (Web)

| Feature | Accessibility Requirement |
|---------|--------------------------|
| Data tables | Proper `<table>`, `<thead>`, `<th scope>` semantics; sortable columns announced |
| Charts | Alternative text summary for each chart ("User growth: 15% increase this month, 12,400 total users") |
| Moderation actions | Confirm dialogs accessible; keyboard-operable |
| Navigation | Skip-to-content link; landmark regions (`<nav>`, `<main>`, `<aside>`) |

### 4.7 Settings

| Feature | Accessibility Requirement |
|---------|--------------------------|
| Theme toggle | "Dark mode, currently on. Double-tap to turn off" |
| Language picker | Each language announced in its native name |
| Clear cache | Confirmation dialog fully accessible |
| Account deletion | Multi-step confirmation accessible with clear warnings |

---

## 5. Screen Reader Semantics

### 5.1 Semantic Widget Wrappers

```dart
// Standard patterns used across the app

// Announce changes to live content
Semantics(
  liveRegion: true,
  child: Text(statusMessage),
)

// Group related content as one announcement
MergeSemantics(
  child: ListTile(
    leading: Icon(Icons.star),
    title: Text('4.5 stars'),
    subtitle: Text('128 ratings'),
  ),
)

// Exclude decorative elements
Semantics(
  excludeFromSemantics: true,
  child: DecorativeBackground(),
)

// Custom actions for complex widgets
Semantics(
  customSemanticsActions: {
    CustomSemanticsAction(label: 'Move layer up'): _moveUp,
    CustomSemanticsAction(label: 'Move layer down'): _moveDown,
    CustomSemanticsAction(label: 'Delete layer'): _deleteLayer,
  },
  child: LayerWidget(),
)
```

### 5.2 Reading Order

| Rule | Implementation |
|------|---------------|
| Logical reading order matches visual order | Use `Semantics(sortKey: OrdinalSortKey(n))` when visual order differs from DOM order |
| Modal dialogs trap focus | Overlay uses `Semantics(scopesRoute: true)` |
| Bottom sheets | `Semantics(namesRoute: true, label: 'Filter options')` |

### 5.3 Announcements

| Event | Announcement |
|-------|-------------|
| Page navigation | Route name announced via `GoRouter` + `Semantics(namesRoute)` |
| Toast / snackbar | `Semantics(liveRegion: true)` or `SemanticsService.announce()` |
| Loading state | "Loading" announced once; "Loaded" on complete |
| Error | Error message read aloud immediately |
| Export progress | Percentage changes announced at 25% intervals |

---

## 6. Keyboard and Focus Management

### 6.1 Tab Order

| Screen | Tab Order |
|--------|----------|
| Template browser | Search → Filter chips → Template cards (left-to-right, top-to-bottom) → Pagination |
| Editor | Toolbar → Canvas → Timeline → Properties panel |
| Paywall | Plan cards → Purchase button → Restore purchases |
| Settings | Settings items top-to-bottom |

### 6.2 Keyboard Shortcuts (Desktop / Web)

| Shortcut | Action | Scope |
|----------|--------|-------|
| `Ctrl/Cmd + Z` | Undo | Editor |
| `Ctrl/Cmd + Shift + Z` | Redo | Editor |
| `Ctrl/Cmd + S` | Save project | Editor |
| `Ctrl/Cmd + E` | Export | Editor |
| `Space` | Play/Pause | Editor (when timeline focused) |
| `←` / `→` | Seek ±1 frame | Editor (when timeline focused) |
| `Delete` / `Backspace` | Delete selected layer | Editor |
| `Tab` | Next focusable element | Global |
| `Shift + Tab` | Previous focusable element | Global |
| `Escape` | Close dialog/modal/panel | Global |
| `/` | Focus search field | Template browser |

### 6.3 Focus Management Rules

| Rule | Implementation |
|------|---------------|
| Dialog opened → focus moves to first interactive element | `FocusScope.autofocus` on dialog |
| Dialog closed → focus returns to trigger element | Store previous focus; restore on dismiss |
| Page navigation → focus moves to page heading | `Semantics(header: true)` on page title with `FocusScope` |
| Error appears → focus moves to error | `FocusNode.requestFocus()` on error widget |

---

## 7. Motion and Animation

### 7.1 Reduced Motion

```dart
// Detect system preference
final reduceMotion = MediaQuery.of(context).disableAnimations;

// Apply globally
final duration = reduceMotion ? Duration.zero : AppDurations.medium;
final curve = reduceMotion ? Curves.linear : AppCurves.standard;
```

### 7.2 Rules

| Rule | Implementation |
|------|---------------|
| No auto-playing animations that can't be paused | Template previews auto-play only if `disableAnimations` is false; controls always visible |
| No content that flashes > 3 times per second | Verified in code review; no strobing effects |
| Parallax and scroll-based motion respect reduced motion | Disabled when `disableAnimations` is true |
| Transition animations are crossfade only when reduced | Page transitions become instant crossfade |

---

## 8. Color and Contrast

### 8.1 Contrast Ratios

| Element | Minimum Ratio (AA) | Gopost Target |
|---------|-------------------|---------------|
| Normal text (< 18pt) | 4.5:1 | ≥ 5:1 |
| Large text (≥ 18pt or ≥ 14pt bold) | 3:1 | ≥ 4:1 |
| UI components (borders, icons) | 3:1 | ≥ 3.5:1 |
| Focus indicators | 3:1 | ≥ 3:1 |

### 8.2 Color-Independent Information

| Pattern | Wrong | Correct |
|---------|-------|---------|
| Status indicator | Red dot only | Red dot + "Error" label |
| Plan comparison | Green ✓ / Red ✗ | ✓ "Included" / ✗ "Not included" with labels |
| Required field | Red asterisk only | Red asterisk + "(required)" text |
| Chart data | Color-only series | Color + pattern (dashed, dotted) + legend |

### 8.3 High Contrast Mode

If the system requests high contrast (`MediaQuery.of(context).highContrast`):

| Adjustment | Implementation |
|-----------|---------------|
| Text color | Force to `Colors.black` / `Colors.white` |
| Borders | Add 2dp solid borders to all interactive elements |
| Icons | Increase opacity to 1.0 |
| Backgrounds | Remove gradients; use solid colors |

---

## 9. Typography and Dynamic Type

### 9.1 Dynamic Type Support

```dart
// All text scales with system font size
Text(
  l10n.templateBrowseTitle,
  style: Theme.of(context).textTheme.headlineSmall,
  // textScaler applied automatically by MaterialApp
)
```

### 9.2 Text Scaling Rules

| Rule | Implementation |
|------|---------------|
| Support 1.0× to 2.0× text scaling | Test at both extremes |
| No text truncation at 2.0× | Use `overflow: TextOverflow.visible` or increase container size |
| Fixed-size containers must scroll | Wrap in `SingleChildScrollView` when text may overflow |
| Minimum font size | 12sp (body) even at 1.0× scale |
| Bold text preference | Respect `MediaQuery.boldTextOf(context)` |

### 9.3 Exceptions

| Element | Scaling Behavior |
|---------|-----------------|
| Editor timeline labels | Fixed size (functional, not readable text); provide accessible alternative |
| Editor canvas text | User-controlled font size; not affected by system scaling |
| Video export watermark | Fixed; not user-facing content |

---

## 10. Platform-Specific Considerations

### 10.1 iOS / macOS

| Feature | Implementation |
|---------|---------------|
| VoiceOver | Semantic tree verified; custom actions for complex widgets |
| Switch Control | All interactive elements reachable |
| Voice Control | All buttons have visible text matching semantic label |
| Increase Contrast | `MediaQuery.highContrast` honored |
| Reduce Transparency | Blur effects removed; solid backgrounds used |
| Button Shapes | Underlines added to text buttons when system preference enabled |

### 10.2 Android

| Feature | Implementation |
|---------|---------------|
| TalkBack | Semantic tree verified; `contentDescription` on all interactive elements |
| Switch Access | Focus order logical; all elements reachable |
| Magnification | UI scales correctly without clipping |
| Color Correction / Inversion | UI remains usable with system color correction |
| Font Size (system) | Respected via `textScaleFactor` |

### 10.3 Web

| Feature | Implementation |
|---------|---------------|
| ARIA landmarks | `Semantics(container: true, label: 'Main content')` generates `role="region"` |
| Screen readers (NVDA, JAWS, VoiceOver) | HTML semantic output verified |
| Keyboard only | Full navigation without mouse |
| Browser zoom (200%) | Layout remains usable |
| Skip navigation | "Skip to main content" link at top |

### 10.4 Desktop (Windows/macOS)

| Feature | Implementation |
|---------|---------------|
| Narrator (Windows) | Semantic tree verified |
| Keyboard shortcuts | See Section 6.2 |
| Tooltip on hover | All icon-only buttons have tooltip |
| Resize/zoom | Window resizing triggers responsive breakpoints; no content lost |

---

## 11. Testing Matrix

### 11.1 Automated Testing

| Test | Tool | Frequency |
|------|------|-----------|
| Semantic label coverage | Custom lint rule: every `GestureDetector`/`IconButton`/`Image` must have a semantic label | Every PR (CI) |
| Contrast ratio verification | `accessibility_tools` package overlay in debug mode | Development |
| Golden tests at 2.0× text scale | `flutter test` with `textScaleFactor: 2.0` | Every PR (CI) |
| Golden tests in RTL + accessibility | `flutter test` with `ar` locale + `boldText: true` | Every PR (CI) |
| `SemanticsDebugger` snapshot | Screenshot in `SemanticsDebugger` mode for key screens | Weekly CI job |

### 11.2 Manual Testing

| Test | Device | Tester | Frequency |
|------|--------|--------|-----------|
| VoiceOver full journey | iPhone (latest iOS) | QA | Per sprint |
| TalkBack full journey | Pixel (latest Android) | QA | Per sprint |
| Keyboard-only navigation | macOS / Windows / Chrome | QA | Per sprint |
| NVDA + Chrome (web) | Windows | QA | Per sprint |
| Switch Control | iPhone | QA or external accessibility tester | Pre-launch |
| User testing with assistive tech users | Mixed | External | Pre-launch |

### 11.3 Testing User Journeys

| Journey | Steps |
|---------|-------|
| **Browse and Preview** | Launch → browse templates → filter by category → open preview → play video → return |
| **Edit and Export** | Select template → open editor → add text layer → change font → export → save to gallery |
| **Subscribe** | Open premium template → see paywall → read plan details → subscribe → use template |
| **Settings** | Open settings → change language → toggle dark mode → clear cache → sign out |

---

## 12. Audit and Remediation Process

### 12.1 Pre-Launch Audit

| Step | Owner | Timeline |
|------|-------|---------|
| Automated scan (all screens) | QA Engineer | Sprint 15 |
| Manual audit (key journeys) | QA + External a11y consultant | Sprint 15 |
| Issue classification (Critical / Major / Minor) | QA | Sprint 15 |
| Remediation of Critical + Major | Flutter Engineers | Sprint 16 |
| Re-audit | External a11y consultant | Before launch |

### 12.2 Severity Classification

| Severity | Definition | Example | SLA |
|----------|-----------|---------|-----|
| **Critical** | Feature completely unusable with assistive tech | Login button has no semantic label; screen reader can't proceed | Fix before launch |
| **Major** | Feature usable but with significant difficulty | Timeline layers not announced; user guesses layer order | Fix before launch |
| **Minor** | Cosmetic or minor inconvenience | Decorative image has unnecessary description | Fix in first post-launch update |

### 12.3 Ongoing Process

| Activity | Cadence |
|----------|---------|
| New PR: semantic label check (CI) | Every PR |
| Sprint: manual screen reader test of changed screens | Every sprint |
| Quarterly: full audit with external consultant | Every 3 months post-launch |
| Annual: VPAT update (if needed for enterprise) | Annually |

---

## 13. Implementation Guide

### 13.1 Package Dependencies

| Package | Purpose |
|---------|---------|
| `accessibility_tools` | Debug overlay showing contrast, touch targets, semantic labels |
| `flutter_test` (built-in) | `SemanticsDebugger`, golden tests |

### 13.2 Code Patterns

**Accessible Button:**
```dart
Semantics(
  button: true,
  label: l10n.template_useButton,
  child: GpButton(
    onPressed: _useTemplate,
    child: Text(l10n.template_useButton),
  ),
)
```

**Accessible Slider:**
```dart
Semantics(
  slider: true,
  label: l10n.editor_opacitySlider,
  value: '${(opacity * 100).round()}%',
  increasedValue: '${((opacity + 0.1).clamp(0, 1) * 100).round()}%',
  decreasedValue: '${((opacity - 0.1).clamp(0, 1) * 100).round()}%',
  child: Slider(
    value: opacity,
    onChanged: _setOpacity,
    label: '${(opacity * 100).round()}%',
  ),
)
```

**Accessible Progress:**
```dart
Semantics(
  liveRegion: true,
  label: l10n.editor_exportProgress(progress),
  child: LinearProgressIndicator(value: progress),
)
```

### 13.3 Developer Checklist (Per PR)

- [ ] Every interactive widget has a semantic label
- [ ] Decorative images excluded from semantics
- [ ] Touch targets ≥ 48 dp
- [ ] No information conveyed by color alone
- [ ] Focus order is logical
- [ ] Dialogs trap and return focus correctly
- [ ] Reduced motion respected
- [ ] Text scales to 2.0× without overflow
- [ ] Tested with VoiceOver or TalkBack on one screen

---

## 14. Sprint Stories

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 6: Polish & Launch |
| **Sprint(s)** | Sprint 15 (Weeks 29-30) |
| **Team** | Flutter Engineers, QA Engineers |
| **Predecessor** | All UI sprints complete (Sprints 1-14), Design System (Sprint 1) |
| **Story Points Total** | 38 |

### Stories

| ID | Story | Acceptance Criteria | Points | Priority | Sprint |
|---|---|---|---|---|---|
| A11Y-001 | Add semantic labels to all interactive widgets across the app | CI lint passes; zero `GestureDetector`/`IconButton`/`Image` without semantic label | 5 | P0 | 15 |
| A11Y-002 | Implement keyboard navigation and focus management for desktop/web | All elements Tab-reachable; focus indicators visible; dialog focus trap works | 5 | P0 | 15 |
| A11Y-003 | Implement keyboard shortcuts for editor (Ctrl+Z, Ctrl+S, Space, arrows) | Shortcuts work on macOS, Windows, and web; no conflicts with system shortcuts | 3 | P0 | 15 |
| A11Y-004 | Make video editor timeline accessible (layer announcements, reorder actions, playback position) | VoiceOver/TalkBack reads layer info; custom actions allow reorder; seek position announced | 5 | P0 | 15 |
| A11Y-005 | Make image editor accessible (filters, tools, layers, properties) | Same as A11Y-004 applied to image editor; filter names announced | 3 | P0 | 15 |
| A11Y-006 | Implement reduced motion support (crossfade transitions, no auto-play when disabled) | `disableAnimations` check in all animated widgets; previews don't auto-play | 2 | P0 | 15 |
| A11Y-007 | Implement dynamic type support (1.0× to 2.0×) — audit all screens for overflow | Golden tests at 2.0× for 10 key screens; no truncation or overflow | 3 | P0 | 15 |
| A11Y-008 | Implement high contrast mode support | `MediaQuery.highContrast` detection; solid backgrounds, stronger borders, full-opacity icons | 2 | P1 | 15 |
| A11Y-009 | Admin portal web accessibility (ARIA landmarks, skip navigation, table semantics) | Landmarks correct; skip link works; data tables have proper `<th>` scope | 3 | P0 | 15 |
| A11Y-010 | VoiceOver + TalkBack full journey testing (browse → edit → export) | QA sign-off on 4 key journeys; zero critical/major issues | 3 | P0 | 15 |
| A11Y-011 | External accessibility audit (pre-launch) | Audit report received; all critical and major issues logged as stories | 2 | P0 | 15 |
| A11Y-012 | Remediate critical and major audit findings | All critical and major items fixed; re-tested by QA | 2 | P0 | 16 |

### Definition of Done

- [ ] All stories marked complete
- [ ] CI lint for semantic labels passing
- [ ] VoiceOver (iOS) and TalkBack (Android) full journey sign-off
- [ ] Keyboard-only navigation verified on desktop and web
- [ ] External audit report — zero critical/major open issues
- [ ] Code reviewed and merged
