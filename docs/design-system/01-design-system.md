# Gopost — Flutter UI/UX Design System

> **Version:** 1.0.0
> **Date:** February 23, 2026
> **Classification:** Internal — Engineering + Design Reference
> **Audience:** Flutter Engineers, UI/UX Designer, QA Engineers

---

## Table of Contents

1. [Design Principles](#1-design-principles)
2. [Color System](#2-color-system)
3. [Typography](#3-typography)
4. [Spacing and Layout](#4-spacing-and-layout)
5. [Responsive Breakpoints](#5-responsive-breakpoints)
6. [Iconography](#6-iconography)
7. [Component Library](#7-component-library)
8. [Navigation Patterns](#8-navigation-patterns)
9. [Motion and Animation](#9-motion-and-animation)
10. [Platform Adaptation](#10-platform-adaptation)
11. [Dark Mode](#11-dark-mode)
12. [Accessibility](#12-accessibility)
13. [Implementation Guide](#13-implementation-guide)
14. [Sprint Stories](#14-sprint-stories)

---

## 1. Design Principles

| Principle | Meaning |
|-----------|---------|
| **Content-First** | Templates and media are the product — UI chrome stays minimal and recedes behind content. |
| **Speed of Interaction** | Every tap has immediate feedback; editing operations feel instantaneous. |
| **Platform-Respectful** | Use native navigation patterns per platform while keeping visual identity consistent. |
| **Progressive Disclosure** | Simple surfaces for browsing; power features revealed contextually in editors. |
| **Accessible by Default** | Contrast, touch targets, and semantic labels are non-negotiable baselines. |
| **Dark-Biased Editors** | Editing screens default to dark to reduce eye strain and let media colors shine. |

---

## 2. Color System

### 2.1 Brand Colors

| Token | Light Mode | Dark Mode | Usage |
|-------|-----------|-----------|-------|
| `brand.primary` | `#6C5CE7` | `#A29BFE` | Primary actions, active states, links |
| `brand.primaryContainer` | `#E8E5FD` | `#2D2A5E` | Selected backgrounds, chips |
| `brand.secondary` | `#00B894` | `#55EFC4` | Success accents, creator badges, upgrades |
| `brand.secondaryContainer` | `#D5F5ED` | `#1A4A3A` | Success backgrounds |
| `brand.tertiary` | `#FD79A8` | `#FAB1C8` | Promotional accents, premium badges |
| `brand.tertiaryContainer` | `#FDE8F0` | `#5E2A3A` | Promo backgrounds |

### 2.2 Neutral Colors

| Token | Light Mode | Dark Mode | Usage |
|-------|-----------|-----------|-------|
| `neutral.background` | `#FFFFFF` | `#121212` | Page background |
| `neutral.surface` | `#FAFAFA` | `#1E1E1E` | Cards, sheets, dialogs |
| `neutral.surfaceVariant` | `#F0F0F0` | `#2C2C2C` | Secondary surfaces |
| `neutral.surfaceElevated` | `#FFFFFF` | `#252525` | Elevated cards (shadow equivalent in dark) |
| `neutral.outline` | `#D1D1D6` | `#3A3A3C` | Borders, dividers |
| `neutral.outlineVariant` | `#E5E5EA` | `#2C2C2E` | Subtle borders |

### 2.3 Text Colors

| Token | Light Mode | Dark Mode | Usage |
|-------|-----------|-----------|-------|
| `text.primary` | `#1A1A2E` | `#F5F5F7` | Headings, body text |
| `text.secondary` | `#636366` | `#AEAEB2` | Subtitles, captions, hints |
| `text.tertiary` | `#8E8E93` | `#636366` | Disabled text, timestamps |
| `text.inverse` | `#FFFFFF` | `#1A1A2E` | Text on primary-colored backgrounds |
| `text.onPrimary` | `#FFFFFF` | `#FFFFFF` | Text on brand.primary buttons |

### 2.4 Semantic Colors

| Token | Light Mode | Dark Mode | Usage |
|-------|-----------|-----------|-------|
| `semantic.error` | `#E74C3C` | `#FF6B6B` | Validation errors, destructive actions |
| `semantic.errorContainer` | `#FDE8E6` | `#4A1C1C` | Error backgrounds |
| `semantic.warning` | `#F39C12` | `#FDCB6E` | Caution states, expiration notices |
| `semantic.warningContainer` | `#FEF3D5` | `#4A3A1C` | Warning backgrounds |
| `semantic.success` | `#27AE60` | `#55EFC4` | Success states, published badges |
| `semantic.successContainer` | `#D5F5E3` | `#1A4A2E` | Success backgrounds |
| `semantic.info` | `#3498DB` | `#74B9FF` | Informational banners, tips |
| `semantic.infoContainer` | `#E0F0FD` | `#1C2E4A` | Info backgrounds |

### 2.5 Editor-Specific Colors

| Token | Value | Usage |
|-------|-------|-------|
| `editor.background` | `#0D0D0D` | Editor canvas background (always dark) |
| `editor.surface` | `#1A1A1A` | Timeline, panels in editor |
| `editor.surfaceActive` | `#2A2A2A` | Hovered/selected panel item |
| `editor.timeline` | `#141414` | Timeline track background |
| `editor.playhead` | `#FF3B30` | Playhead indicator |
| `editor.selection` | `rgba(108, 92, 231, 0.3)` | Selected region highlight |
| `editor.grid` | `rgba(255, 255, 255, 0.06)` | Canvas grid lines |

### 2.6 Dart Implementation

```dart
// lib/core/theme/app_colors.dart

abstract class AppColors {
  // Brand
  static const primary = Color(0xFF6C5CE7);
  static const primaryContainer = Color(0xFFE8E5FD);
  static const secondary = Color(0xFF00B894);
  static const secondaryContainer = Color(0xFFD5F5ED);
  static const tertiary = Color(0xFFFD79A8);
  static const tertiaryContainer = Color(0xFFFDE8F0);

  // Semantic
  static const error = Color(0xFFE74C3C);
  static const errorContainer = Color(0xFFFDE8E6);
  static const warning = Color(0xFFF39C12);
  static const success = Color(0xFF27AE60);
  static const info = Color(0xFF3498DB);
}

abstract class AppColorsDark {
  static const primary = Color(0xFFA29BFE);
  static const primaryContainer = Color(0xFF2D2A5E);
  static const secondary = Color(0xFF55EFC4);
  static const secondaryContainer = Color(0xFF1A4A3A);
  static const tertiary = Color(0xFFFAB1C8);
  static const tertiaryContainer = Color(0xFF5E2A3A);

  static const error = Color(0xFFFF6B6B);
  static const errorContainer = Color(0xFF4A1C1C);
  static const warning = Color(0xFFFDCB6E);
  static const success = Color(0xFF55EFC4);
  static const info = Color(0xFF74B9FF);
}
```

---

## 3. Typography

### 3.1 Font Families

| Role | Font | Fallback | Rationale |
|------|------|----------|-----------|
| **Primary (Sans-Serif)** | Inter | SF Pro (iOS), Roboto (Android), Segoe UI (Windows) | Excellent readability at all sizes; wide language support |
| **Monospace** | JetBrains Mono | SF Mono, Roboto Mono | Code/technical displays (timecodes, hex values) |
| **Display** | Inter Display | Same as primary | Optical sizing for headings > 32sp |

### 3.2 Type Scale

| Token | Size (sp) | Weight | Line Height | Letter Spacing | Usage |
|-------|-----------|--------|-------------|----------------|-------|
| `display.large` | 57 | 400 | 64 | -0.25 | Hero sections (marketing/onboarding) |
| `display.medium` | 45 | 400 | 52 | 0 | Large feature titles |
| `display.small` | 36 | 400 | 44 | 0 | Section headers |
| `headline.large` | 32 | 600 | 40 | 0 | Screen titles |
| `headline.medium` | 28 | 600 | 36 | 0 | Card titles |
| `headline.small` | 24 | 600 | 32 | 0 | Section sub-headers |
| `title.large` | 22 | 500 | 28 | 0 | App bar titles |
| `title.medium` | 16 | 500 | 24 | 0.15 | Tab labels, list titles |
| `title.small` | 14 | 500 | 20 | 0.1 | Small section titles |
| `body.large` | 16 | 400 | 24 | 0.5 | Primary body text |
| `body.medium` | 14 | 400 | 20 | 0.25 | Secondary body text |
| `body.small` | 12 | 400 | 16 | 0.4 | Captions, metadata |
| `label.large` | 14 | 500 | 20 | 0.1 | Button labels, nav labels |
| `label.medium` | 12 | 500 | 16 | 0.5 | Chip labels, badges |
| `label.small` | 11 | 500 | 16 | 0.5 | Tiny labels, timestamps |

### 3.3 Dart Implementation

```dart
// lib/core/theme/app_typography.dart

abstract class AppTypography {
  static const _fontFamily = 'Inter';

  static TextTheme get textTheme => const TextTheme(
    displayLarge: TextStyle(fontFamily: _fontFamily, fontSize: 57, fontWeight: FontWeight.w400, height: 1.12, letterSpacing: -0.25),
    displayMedium: TextStyle(fontFamily: _fontFamily, fontSize: 45, fontWeight: FontWeight.w400, height: 1.16),
    displaySmall: TextStyle(fontFamily: _fontFamily, fontSize: 36, fontWeight: FontWeight.w400, height: 1.22),
    headlineLarge: TextStyle(fontFamily: _fontFamily, fontSize: 32, fontWeight: FontWeight.w600, height: 1.25),
    headlineMedium: TextStyle(fontFamily: _fontFamily, fontSize: 28, fontWeight: FontWeight.w600, height: 1.29),
    headlineSmall: TextStyle(fontFamily: _fontFamily, fontSize: 24, fontWeight: FontWeight.w600, height: 1.33),
    titleLarge: TextStyle(fontFamily: _fontFamily, fontSize: 22, fontWeight: FontWeight.w500, height: 1.27),
    titleMedium: TextStyle(fontFamily: _fontFamily, fontSize: 16, fontWeight: FontWeight.w500, height: 1.5, letterSpacing: 0.15),
    titleSmall: TextStyle(fontFamily: _fontFamily, fontSize: 14, fontWeight: FontWeight.w500, height: 1.43, letterSpacing: 0.1),
    bodyLarge: TextStyle(fontFamily: _fontFamily, fontSize: 16, fontWeight: FontWeight.w400, height: 1.5, letterSpacing: 0.5),
    bodyMedium: TextStyle(fontFamily: _fontFamily, fontSize: 14, fontWeight: FontWeight.w400, height: 1.43, letterSpacing: 0.25),
    bodySmall: TextStyle(fontFamily: _fontFamily, fontSize: 12, fontWeight: FontWeight.w400, height: 1.33, letterSpacing: 0.4),
    labelLarge: TextStyle(fontFamily: _fontFamily, fontSize: 14, fontWeight: FontWeight.w500, height: 1.43, letterSpacing: 0.1),
    labelMedium: TextStyle(fontFamily: _fontFamily, fontSize: 12, fontWeight: FontWeight.w500, height: 1.33, letterSpacing: 0.5),
    labelSmall: TextStyle(fontFamily: _fontFamily, fontSize: 11, fontWeight: FontWeight.w500, height: 1.45, letterSpacing: 0.5),
  );
}
```

---

## 4. Spacing and Layout

### 4.1 Spacing Scale

Base unit: **4px**. All spacing derives from multiples of 4.

| Token | Value | Usage |
|-------|-------|-------|
| `space.xxs` | 2px | Icon-to-label gaps inside compact chips |
| `space.xs` | 4px | Tight inline gaps |
| `space.sm` | 8px | Between related elements (icon + text) |
| `space.md` | 12px | Default content padding within cards |
| `space.base` | 16px | Standard page margins, section gaps |
| `space.lg` | 20px | Between card groups |
| `space.xl` | 24px | Major section separation |
| `space.2xl` | 32px | Screen-level vertical gaps |
| `space.3xl` | 48px | Hero spacing, large visual breaks |
| `space.4xl` | 64px | Maximum spacing (splash, onboarding) |

### 4.2 Content Padding

| Context | Horizontal | Vertical |
|---------|-----------|----------|
| Screen body | 16px | 16px top / 0 bottom (scroll) |
| Card content | 12px | 12px |
| Dialog content | 24px | 20px |
| Bottom sheet | 16px | 16px |
| List item | 16px | 12px |
| App bar | 16px | 0 (intrinsic height) |

### 4.3 Grid System

| Layout | Columns | Gutter | Margin |
|--------|---------|--------|--------|
| Mobile portrait | 4 | 16px | 16px |
| Mobile landscape | 8 | 16px | 16px |
| Tablet portrait | 8 | 24px | 24px |
| Tablet landscape | 12 | 24px | 24px |
| Desktop | 12 | 24px | 32px |

### 4.4 Dart Implementation

```dart
// lib/core/theme/app_spacing.dart

abstract class AppSpacing {
  static const double xxs = 2;
  static const double xs = 4;
  static const double sm = 8;
  static const double md = 12;
  static const double base = 16;
  static const double lg = 20;
  static const double xl = 24;
  static const double xxl = 32;
  static const double xxxl = 48;
  static const double xxxxl = 64;

  static const screenPadding = EdgeInsets.symmetric(horizontal: base);
  static const cardPadding = EdgeInsets.all(md);
  static const dialogPadding = EdgeInsets.symmetric(horizontal: xl, vertical: lg);
  static const listItemPadding = EdgeInsets.symmetric(horizontal: base, vertical: md);
}
```

---

## 5. Responsive Breakpoints

### 5.1 Breakpoint Definitions

| Name | Min Width | Max Width | Target Devices |
|------|-----------|-----------|---------------|
| `compact` | 0px | 599px | Phones (portrait and landscape) |
| `medium` | 600px | 839px | Small tablets, phones in landscape |
| `expanded` | 840px | 1199px | Tablets, small desktops |
| `large` | 1200px | 1599px | Desktop, large tablets in landscape |
| `extraLarge` | 1600px | -- | Wide desktop monitors |

### 5.2 Layout Adaptation Table

| Component | Compact | Medium | Expanded | Large+ |
|-----------|---------|--------|----------|--------|
| Navigation | Bottom nav bar (5 items) | Navigation rail (icons + labels) | Side nav drawer (persistent) | Side nav drawer (persistent) |
| Template grid | 2 columns | 3 columns | 4 columns | 5-6 columns |
| Template card | Vertical (image top) | Vertical | Horizontal (image left) | Horizontal |
| Editor layout | Full-screen, tool bar bottom | Full-screen, tool bar bottom | Canvas center, panels left+right | Canvas center, panels left+right |
| Timeline | Bottom sheet, collapsed | Bottom sheet, expanded | Bottom panel, resizable | Bottom panel, resizable |
| Admin dashboard | Stacked cards, scrollable | 2-column grid | 3-column grid | 4-column grid |
| Dialogs | Full-screen sheet | Centered dialog (max 560px) | Centered dialog (max 560px) | Centered dialog (max 560px) |

### 5.3 Dart Implementation

```dart
// lib/core/theme/app_breakpoints.dart

enum ScreenSize { compact, medium, expanded, large, extraLarge }

abstract class AppBreakpoints {
  static const double compact = 0;
  static const double medium = 600;
  static const double expanded = 840;
  static const double large = 1200;
  static const double extraLarge = 1600;

  static ScreenSize of(BuildContext context) {
    final width = MediaQuery.sizeOf(context).width;
    if (width >= extraLarge) return ScreenSize.extraLarge;
    if (width >= large) return ScreenSize.large;
    if (width >= expanded) return ScreenSize.expanded;
    if (width >= medium) return ScreenSize.medium;
    return ScreenSize.compact;
  }
}

class ResponsiveBuilder extends StatelessWidget {
  final Widget Function(BuildContext, ScreenSize) builder;
  const ResponsiveBuilder({super.key, required this.builder});

  @override
  Widget build(BuildContext context) {
    return builder(context, AppBreakpoints.of(context));
  }
}
```

---

## 6. Iconography

### 6.1 Icon Set

**Primary:** Material Symbols (Outlined, weight 400, grade 0, optical size 24). Provides 3,000+ icons with variable font support.

**Custom icons:** For Gopost-specific concepts (template type badges, editor tool icons), use custom SVG assets following these constraints:
- 24x24 viewport
- 2px stroke weight
- Round line caps and joins
- Single color (use `ColorFilter` for theming)

### 6.2 Icon Sizes

| Token | Size | Touch Target | Usage |
|-------|------|--------------|-------|
| `icon.xs` | 16px | 32x32 | Inline with small text (badges, chips) |
| `icon.sm` | 20px | 40x40 | List item trailing, secondary actions |
| `icon.md` | 24px | 48x48 | Standard icon buttons, nav items, app bar |
| `icon.lg` | 32px | 48x48 | Feature icons, editor tool bar |
| `icon.xl` | 48px | 56x56 | Empty states, onboarding illustrations |

### 6.3 Dart Implementation

```dart
// lib/core/theme/app_icons.dart

abstract class AppIconSizes {
  static const double xs = 16;
  static const double sm = 20;
  static const double md = 24;
  static const double lg = 32;
  static const double xl = 48;
}
```

---

## 7. Component Library

### 7.1 Buttons

#### Variants

| Variant | Usage | Background | Text/Icon Color |
|---------|-------|-----------|----------------|
| **Filled** (Primary) | Main CTAs: "Apply Template", "Export", "Subscribe" | `brand.primary` | `text.onPrimary` |
| **Filled Tonal** | Secondary CTAs: "Save Draft", "Add Layer" | `brand.primaryContainer` | `brand.primary` |
| **Outlined** | Tertiary actions: "Cancel", "Reset" | Transparent | `brand.primary` |
| **Text** | Low-emphasis: "Skip", "Learn More" | Transparent | `brand.primary` |
| **Icon** | Tool actions: "Undo", "Redo", "Delete" | Transparent | `text.secondary` |
| **FAB** | Primary creation: "New Project", "Add Media" | `brand.primary` | `text.onPrimary` |

#### States

| State | Opacity | Visual Change |
|-------|---------|--------------|
| Default | 100% | Base appearance |
| Hover (desktop) | 100% | +8% overlay tint |
| Pressed | 100% | +12% overlay tint, scale 0.97 |
| Focused | 100% | 2px focus ring (brand.primary, 50% opacity) |
| Disabled | 38% content, 12% container | Greyed out |
| Loading | 100% | Text replaced by 20px circular progress indicator |

#### Sizing

| Size | Height | Horizontal Padding | Icon Size | Font |
|------|--------|-------------------|-----------|------|
| Small | 32px | 12px | 16px | `label.medium` |
| Medium (default) | 40px | 16px | 20px | `label.large` |
| Large | 48px | 24px | 24px | `label.large` |

```dart
// lib/core/theme/widgets/gp_button.dart

enum GpButtonSize { small, medium, large }
enum GpButtonVariant { filled, tonal, outlined, text }

class GpButton extends StatelessWidget {
  final String label;
  final VoidCallback? onPressed;
  final GpButtonVariant variant;
  final GpButtonSize size;
  final IconData? icon;
  final bool isLoading;

  const GpButton({
    super.key,
    required this.label,
    this.onPressed,
    this.variant = GpButtonVariant.filled,
    this.size = GpButtonSize.medium,
    this.icon,
    this.isLoading = false,
  });

  @override
  Widget build(BuildContext context) {
    // Delegates to FilledButton, OutlinedButton, TextButton, etc.
    // with size-dependent padding and text style from AppSpacing/AppTypography
    throw UnimplementedError();
  }
}
```

### 7.2 Input Fields

#### Variants

| Variant | Usage |
|---------|-------|
| **Standard** | Single-line text input (name, email, search) |
| **Password** | Obscured input with visibility toggle |
| **Multi-line** | Description, notes (auto-grow up to 4 lines) |
| **Search** | Prefix search icon, clear button, rounded corners |
| **Dropdown** | Select from list; opens bottom sheet on mobile, menu on desktop |
| **Date/Time Picker** | Taps to open platform-native picker |
| **Numeric Stepper** | +/- buttons flanking a numeric value (opacity, font size) |

#### States

| State | Border Color | Fill | Helper Text |
|-------|-------------|------|-------------|
| Default | `neutral.outline` | Transparent | Hint text in `text.tertiary` |
| Focused | `brand.primary` (2px) | Transparent | Label animated to top |
| Error | `semantic.error` (2px) | `semantic.errorContainer` | Error message in `semantic.error` |
| Success | `semantic.success` | Transparent | Checkmark suffix icon |
| Disabled | `neutral.outlineVariant` | 4% `neutral.outline` | Greyed text |
| Read-only | `neutral.outline` | 4% `neutral.outline` | Normal text, no cursor |

#### Sizing

| Size | Height | Usage |
|------|--------|-------|
| Compact | 40px | Editor property panels, dense forms |
| Standard | 48px | Default for all forms |
| Large | 56px | Auth screens, onboarding |

### 7.3 Cards

#### Template Card

```
┌─────────────────────────┐
│  ┌─────────────────────┐ │
│  │                     │ │  <- Thumbnail (16:9 video, 1:1 image, 9:16 story)
│  │     Thumbnail       │ │     Aspect ratio preserved; rounded 8px corners
│  │                     │ │     Shimmer loading placeholder
│  │  ┌─────┐            │ │
│  │  │ PRO │            │ │  <- Premium badge (top-left overlay)
│  │  └─────┘            │ │
│  └─────────────────────┘ │
│  Template Name            │  <- title.medium, text.primary
│  Category • 15.4K uses    │  <- body.small, text.secondary
│  ┌──────┐ ┌──────┐       │
│  │ neon │ │ dark │       │  <- Tag chips (label.small)
│  └──────┘ └──────┘       │
└─────────────────────────┘
   Padding: 12px all sides
   Corner radius: 12px
   Shadow: 0 1px 3px rgba(0,0,0,0.08) (light), none (dark)
   Tap: Navigate to template detail
   Long-press: Quick preview (bottom sheet)
```

#### Stats Card (Admin)

```
┌─────────────────────────┐
│  ↑ 12.4%                │  <- Trend indicator (semantic.success or semantic.error)
│  15,420                  │  <- headline.large, text.primary
│  Total Templates         │  <- body.small, text.secondary
│  [====-------] 62%      │  <- Linear progress bar (brand.primary)
└─────────────────────────┘
   Width: Flexible (grid column)
   Height: 120px
   Padding: 16px
   Corner radius: 12px
```

#### Media Card (Editor)

```
┌─────────────────────────┐
│  ┌─────────────────────┐ │
│  │     Media Thumb      │ │  <- Square thumbnail, corner radius 8px
│  │       ▶ 0:15        │ │  <- Duration badge (video only, bottom-right)
│  └─────────────────────┘ │
│  filename.mp4             │  <- body.small, text.primary, single-line ellipsis
│  2.4 MB                   │  <- label.small, text.tertiary
└─────────────────────────┘
   Width: Grid column (2-4 per row depending on breakpoint)
   Tap: Insert into editor / preview
```

### 7.4 Dialogs and Overlays

| Component | Mobile Behavior | Desktop Behavior | Max Width |
|-----------|----------------|-----------------|-----------|
| **Alert dialog** | Centered overlay | Centered overlay | 400px |
| **Confirmation dialog** | Centered overlay | Centered overlay | 400px |
| **Bottom sheet** | Slides up from bottom, drag to dismiss | Centered dialog | 560px |
| **Full-screen dialog** | Covers entire screen with close button | Centered dialog, scrim | 720px |
| **Snackbar** | Bottom, above nav bar | Bottom-left, 344px max width | 344px |
| **Toast** | Center, fades in/out | Center, fades in/out | 280px |
| **Dropdown menu** | Bottom sheet with list | Popup menu at anchor | 280px |
| **Tooltip** | Long-press to show | Hover to show | 200px |

### 7.5 Template-Specific Components

#### Preview Player

- Aspect ratio container matching template dimensions
- Play/pause overlay button (centered, 56x56, semi-transparent background)
- Progress bar (bottom edge, 3px height, brand.primary)
- Muted by default with tap-to-unmute indicator
- Looping playback for video templates
- Pinch-to-zoom on detail screen

#### Filter Chip Bar

- Horizontally scrollable row of filter chips
- "All" chip always first and selected by default
- Chips: 32px height, 12px horizontal padding, 8px gap
- Selected state: `brand.primary` background, `text.onPrimary` text
- Unselected state: `neutral.surfaceVariant` background, `text.secondary` text

#### Layer Stack Item

```
┌────────────────────────────────────────────────────┐
│ ≡  [Thumb]  Layer Name              👁  🔒  ...  │
│              body.small metadata                   │
└────────────────────────────────────────────────────┘
   Height: 48px
   ≡ = drag handle
   👁 = visibility toggle
   🔒 = lock toggle
   ... = overflow menu (duplicate, delete, merge)
   Selected state: brand.primaryContainer background
   Reorderable via drag
```

#### Timeline Track (Video Editor)

```
┌──────────────────────────────────────────────────────────┐
│ V1 │ [==Clip A==][T][====Clip B====]                    │
├──────────────────────────────────────────────────────────┤
│ V2 │           [===Overlay===]                           │
├──────────────────────────────────────────────────────────┤
│ A1 │ [=======Audio Track=======]                         │
├──────────────────────────────────────────────────────────┤
│ A2 │ [==Music==]                                         │
└──────────────────────────────────────────────────────────┘
  ▼ Playhead (editor.playhead color, 2px wide, full height)

   Track height: 48px (video), 32px (audio)
   Clip: rounded 4px corners, thumbnail filmstrip for video
   [T] = Transition marker (12px diamond)
   Pinch to zoom timeline
   Tap clip to select; double-tap to enter trim mode
```

---

## 8. Navigation Patterns

### 8.1 Primary Navigation Destinations

| Index | Label | Icon | Route |
|-------|-------|------|-------|
| 0 | Home | `home_outlined` / `home_filled` | `/` |
| 1 | Templates | `dashboard_outlined` / `dashboard_filled` | `/templates` |
| 2 | Create | `add_circle_outlined` / `add_circle_filled` | Opens creation sheet |
| 3 | Projects | `folder_outlined` / `folder_filled` | `/projects` |
| 4 | Profile | `person_outlined` / `person_filled` | `/profile` |

### 8.2 Platform-Adaptive Navigation

| Breakpoint | Navigation Type | Details |
|------------|----------------|---------|
| `compact` | BottomNavigationBar | 5 items, label always visible, 80px height |
| `medium` | NavigationRail | Left edge, 72px width, icons + short labels |
| `expanded`+ | NavigationDrawer (persistent) | Left, 256px width, full labels, grouped sections |

### 8.3 Admin Navigation (Web)

Admin portal uses a persistent sidebar with grouped sections:

```
┌────────────────────────┐
│  GOPOST ADMIN          │
│                        │
│  ── Overview ──        │
│  📊 Dashboard          │
│  📈 Analytics          │
│                        │
│  ── Content ──         │
│  📄 Templates          │
│  ✅ Review Queue       │
│  📤 Upload             │
│                        │
│  ── Users ──           │
│  👥 User Management    │
│  🔑 Roles              │
│                        │
│  ── System ──          │
│  📋 Audit Logs         │
│  ⚙️ Settings           │
└────────────────────────┘
   Width: 240px (collapsible to 72px rail)
   Active item: brand.primaryContainer background
```

---

## 9. Motion and Animation

### 9.1 Duration Tokens

| Token | Duration | Usage |
|-------|----------|-------|
| `duration.instant` | 100ms | Micro-feedback (checkbox, toggle) |
| `duration.fast` | 150ms | Hover effects, tooltip show |
| `duration.normal` | 300ms | Page transitions, dialog open/close, card expand |
| `duration.slow` | 500ms | Complex transitions, skeleton to content |
| `duration.xSlow` | 800ms | Onboarding animations, celebrations |

### 9.2 Easing Curves

| Token | Curve | Usage |
|-------|-------|-------|
| `easing.standard` | `Curves.easeInOutCubic` | Default for all animations |
| `easing.decelerate` | `Curves.easeOut` | Elements entering screen |
| `easing.accelerate` | `Curves.easeIn` | Elements leaving screen |
| `easing.emphasize` | `Curves.easeInOutCubicEmphasized` | Prominent transitions (hero, page) |
| `easing.spring` | `SpringSimulation(1.0, 0, 1, 0.8)` | Bounce-back interactions (drag, pull-to-refresh) |

### 9.3 Transition Specifications

| Transition | Animation | Duration | Curve |
|-----------|-----------|----------|-------|
| Page push (forward) | Slide in from right + fade | 300ms | `easing.emphasize` |
| Page pop (back) | Slide out to right + fade | 300ms | `easing.emphasize` |
| Dialog open | Scale from 0.9 + fade in | 300ms | `easing.decelerate` |
| Dialog close | Scale to 0.9 + fade out | 150ms | `easing.accelerate` |
| Bottom sheet open | Slide up from bottom | 300ms | `easing.decelerate` |
| Bottom sheet close | Slide down | 200ms | `easing.accelerate` |
| Snackbar show | Slide up + fade in | 200ms | `easing.decelerate` |
| Snackbar hide | Fade out | 150ms | `easing.accelerate` |
| Tab switch | Cross-fade | 200ms | `easing.standard` |
| List item insert | Slide down + fade in | 300ms | `easing.decelerate` |
| List item remove | Slide left + fade out | 200ms | `easing.accelerate` |

### 9.4 Micro-Interactions

| Interaction | Animation |
|-------------|-----------|
| Button press | Scale to 0.97 (100ms), back to 1.0 (100ms) |
| FAB press | Scale to 0.92 (100ms), back to 1.0 (150ms, spring) |
| Card tap | Subtle elevation increase (light) or brightness increase (dark), 150ms |
| Toggle switch | Thumb slides 150ms with color interpolation |
| Pull-to-refresh | Overscroll indicator → rotating spinner → content refreshes with slide-down |
| Template favorite | Heart icon: scale 1.0 → 1.3 → 1.0 with color change (200ms, spring) |
| Export complete | Checkmark circle: stroke draws in (500ms, decelerate) |

### 9.5 Dart Implementation

```dart
// lib/core/theme/app_motion.dart

abstract class AppDurations {
  static const instant = Duration(milliseconds: 100);
  static const fast = Duration(milliseconds: 150);
  static const normal = Duration(milliseconds: 300);
  static const slow = Duration(milliseconds: 500);
  static const xSlow = Duration(milliseconds: 800);
}

abstract class AppCurves {
  static const standard = Curves.easeInOutCubic;
  static const decelerate = Curves.easeOut;
  static const accelerate = Curves.easeIn;
  static const emphasize = Curves.easeInOutCubicEmphasized;
}
```

---

## 10. Platform Adaptation

### 10.1 Adaptive Component Map

| Concept | Android / Web / Windows | iOS / macOS |
|---------|------------------------|-------------|
| Primary button shape | Rounded rectangle (20px radius) | Rounded rectangle (20px radius) |
| App bar | Material 3 TopAppBar | CupertinoNavigationBar |
| Back button | Arrow left icon | Chevron left + "Back" label |
| Page transitions | Shared axis (horizontal) | Slide from right (iOS style) |
| Dialogs | Material AlertDialog | CupertinoAlertDialog |
| Date picker | Material date picker | CupertinoDatePicker |
| Switches | Material Switch | CupertinoSwitch |
| Pull-to-refresh | Material overscroll indicator | Cupertino sliver refresh |
| Scrollbar | Thin, auto-hide | Thin, auto-hide |
| Text selection | Material handles | iOS loupe + handles |
| Context menu | Material popup menu | Cupertino context menu |
| Haptic feedback | Light vibration on key actions | UIImpactFeedbackGenerator |

### 10.2 Desktop-Specific Patterns

| Pattern | Implementation |
|---------|---------------|
| Hover states | All interactive elements show hover highlight (+8% tint) |
| Right-click menus | ContextMenuRegion with platform-aware items |
| Keyboard shortcuts | Ctrl/Cmd + Z (undo), Ctrl/Cmd + S (save), Ctrl/Cmd + E (export), Space (play/pause) |
| Window resizing | Responsive layout reflows; editor panels are draggable/resizable |
| Multi-window | Not in V1; future consideration for editor detaching panels |
| Drag-and-drop | File drop zone for media import; drag clips on timeline |
| Tooltips | Appear on hover after 500ms delay; disappear on mouse leave |
| Scroll | Mouse wheel scrolling; horizontal scroll with Shift+wheel on timeline |

### 10.3 Dart Implementation Pattern

```dart
// lib/core/theme/widgets/adaptive_widget.dart

Widget adaptiveDialog({
  required BuildContext context,
  required String title,
  required String content,
  required String confirmLabel,
  required VoidCallback onConfirm,
}) {
  final isApple = Theme.of(context).platform == TargetPlatform.iOS ||
                  Theme.of(context).platform == TargetPlatform.macOS;

  if (isApple) {
    return CupertinoAlertDialog(
      title: Text(title),
      content: Text(content),
      actions: [
        CupertinoDialogAction(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
        CupertinoDialogAction(isDefaultAction: true, onPressed: onConfirm, child: Text(confirmLabel)),
      ],
    );
  }

  return AlertDialog(
    title: Text(title),
    content: Text(content),
    actions: [
      TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
      FilledButton(onPressed: onConfirm, child: Text(confirmLabel)),
    ],
  );
}
```

---

## 11. Dark Mode

### 11.1 Strategy

- **App-level:** Light and dark themes; user can choose system, light, or dark via settings.
- **Editor screens:** Always dark, regardless of app theme setting. Media editing benefits from dark surroundings for color accuracy.
- **Admin portal:** Follows system/user preference.

### 11.2 Surface Elevation Model (Dark Mode)

In dark mode, elevation is represented by lighter surface tones rather than shadows:

| Elevation Level | Surface Tint | Usage |
|----------------|--------------|-------|
| 0 (Background) | `#121212` | Page backgrounds |
| 1 | `#1E1E1E` (+5% white) | Cards, bottom sheets |
| 2 | `#232323` (+7% white) | App bar, navigation rail |
| 3 | `#252525` (+8% white) | Elevated cards, menus |
| 4 | `#2C2C2C` (+9% white) | Dialogs, FAB |
| 5 | `#333333` (+11% white) | Popovers, tooltips |

### 11.3 Dark Mode Color Mapping

All color tokens in Section 2 already include both light and dark variants. The `ThemeData` construction switches based on brightness:

```dart
// lib/core/theme/app_theme.dart

abstract class AppTheme {
  static ThemeData light() {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.light,
      colorScheme: ColorScheme(
        brightness: Brightness.light,
        primary: AppColors.primary,
        onPrimary: Colors.white,
        primaryContainer: AppColors.primaryContainer,
        secondary: AppColors.secondary,
        onSecondary: Colors.white,
        secondaryContainer: AppColors.secondaryContainer,
        tertiary: AppColors.tertiary,
        tertiaryContainer: AppColors.tertiaryContainer,
        error: AppColors.error,
        onError: Colors.white,
        errorContainer: AppColors.errorContainer,
        surface: const Color(0xFFFAFAFA),
        onSurface: const Color(0xFF1A1A2E),
        surfaceContainerHighest: const Color(0xFFF0F0F0),
        outline: const Color(0xFFD1D1D6),
        outlineVariant: const Color(0xFFE5E5EA),
      ),
      textTheme: AppTypography.textTheme,
      splashFactory: InkSparkle.splashFactory,
      cardTheme: CardTheme(
        elevation: 1,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
        margin: EdgeInsets.zero,
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: false,
        border: OutlineInputBorder(borderRadius: BorderRadius.circular(8)),
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      ),
      filledButtonTheme: FilledButtonThemeData(
        style: FilledButton.styleFrom(
          minimumSize: const Size(64, 40),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
          padding: const EdgeInsets.symmetric(horizontal: 16),
        ),
      ),
    );
  }

  static ThemeData dark() {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.dark,
      colorScheme: ColorScheme(
        brightness: Brightness.dark,
        primary: AppColorsDark.primary,
        onPrimary: Colors.white,
        primaryContainer: AppColorsDark.primaryContainer,
        secondary: AppColorsDark.secondary,
        onSecondary: const Color(0xFF1A1A2E),
        secondaryContainer: AppColorsDark.secondaryContainer,
        tertiary: AppColorsDark.tertiary,
        tertiaryContainer: AppColorsDark.tertiaryContainer,
        error: AppColorsDark.error,
        onError: Colors.white,
        errorContainer: AppColorsDark.errorContainer,
        surface: const Color(0xFF1E1E1E),
        onSurface: const Color(0xFFF5F5F7),
        surfaceContainerHighest: const Color(0xFF2C2C2C),
        outline: const Color(0xFF3A3A3C),
        outlineVariant: const Color(0xFF2C2C2E),
      ),
      textTheme: AppTypography.textTheme.apply(
        bodyColor: const Color(0xFFF5F5F7),
        displayColor: const Color(0xFFF5F5F7),
      ),
      splashFactory: InkSparkle.splashFactory,
      cardTheme: CardTheme(
        elevation: 0,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
        margin: EdgeInsets.zero,
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: false,
        border: OutlineInputBorder(borderRadius: BorderRadius.circular(8)),
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      ),
      filledButtonTheme: FilledButtonThemeData(
        style: FilledButton.styleFrom(
          minimumSize: const Size(64, 40),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
          padding: const EdgeInsets.symmetric(horizontal: 16),
        ),
      ),
    );
  }
}
```

---

## 12. Accessibility

### 12.1 Minimum Requirements

| Requirement | Target | Standard |
|-------------|--------|----------|
| Color contrast (normal text) | 4.5:1 | WCAG AA |
| Color contrast (large text / UI components) | 3:1 | WCAG AA |
| Touch target size | 48x48 dp minimum | Material Design / Apple HIG |
| Focus indicators | Visible 2px ring on all interactive elements | WCAG 2.4.7 |
| Screen reader support | All images, icons, and interactive elements have semantic labels | WCAG 1.1.1 |
| Motion reduction | Respect `MediaQuery.disableAnimations`; provide reduced-motion alternative | WCAG 2.3.3 |
| Dynamic type | Support system font scale from 0.8x to 2.0x without layout breaking | iOS Dynamic Type / Android font scale |

### 12.2 Semantic Label Policy

| Element Type | Requirement |
|-------------|------------|
| Icon buttons | `Semantics(label: 'Undo last action')` |
| Images | `Semantics(label: 'Template preview: Neon Story Pack')` |
| Decorative images | `Semantics(excludeSemantics: true)` or `Image(semanticLabel: '')` |
| Badges/chips | `Semantics(label: 'Premium template')` |
| Progress indicators | `Semantics(label: 'Export progress: 45%')` |
| Custom painters | Wrap in `Semantics` with appropriate `label` and `textDirection` |
| Timeline clips | `Semantics(label: 'Video clip: sunset.mp4, 0:05 to 0:12 on track 1')` |

### 12.3 Keyboard Navigation (Desktop/Web)

| Key | Action |
|-----|--------|
| Tab / Shift+Tab | Move focus forward / backward through interactive elements |
| Enter / Space | Activate focused element |
| Escape | Close dialog / sheet / cancel operation |
| Arrow keys | Navigate within lists, grids, timeline tracks |
| Ctrl/Cmd + Z | Undo |
| Ctrl/Cmd + Shift + Z | Redo |
| Ctrl/Cmd + S | Save project |
| Ctrl/Cmd + E | Export |
| Space | Play / pause (in editor) |
| Home / End | Jump to timeline start / end |

### 12.4 Testing

- Use Flutter `SemanticsDebugger` during development
- Run `flutter test --enable-impeller` with accessibility checks
- Audit with VoiceOver (iOS/macOS), TalkBack (Android), NVDA (Windows)
- Automated contrast check via a custom lint rule on color token usage

---

## 13. Implementation Guide

### 13.1 File Organization

All design system code lives in `lib/core/theme/`:

```
lib/core/theme/
├── app_colors.dart          # AppColors, AppColorsDark
├── app_typography.dart      # AppTypography (TextTheme)
├── app_spacing.dart         # AppSpacing constants + EdgeInsets
├── app_breakpoints.dart     # AppBreakpoints, ScreenSize, ResponsiveBuilder
├── app_icons.dart           # AppIconSizes, custom icon data
├── app_motion.dart          # AppDurations, AppCurves
├── app_theme.dart           # AppTheme.light(), AppTheme.dark()
├── app_shadows.dart         # Elevation-based shadow definitions
└── widgets/
    ├── gp_button.dart       # GpButton (all variants/sizes)
    ├── gp_input.dart        # GpTextField (all variants)
    ├── gp_card.dart         # GpCard (template, stats, media)
    ├── gp_chip.dart         # GpFilterChip, GpTagChip
    ├── gp_dialog.dart       # showGpDialog, showGpConfirmation
    ├── gp_sheet.dart        # showGpBottomSheet
    ├── gp_snackbar.dart     # showGpSnackbar, showGpToast
    ├── gp_avatar.dart       # GpAvatar (user, creator)
    ├── gp_badge.dart        # GpBadge (PRO, NEW, count)
    ├── gp_empty_state.dart  # GpEmptyState (icon + message + action)
    ├── gp_loading.dart      # GpSkeleton, GpShimmer, GpSpinner
    ├── gp_image.dart        # GpNetworkImage (cached, placeholder, error)
    ├── adaptive_widget.dart # Platform-adaptive helpers
    └── responsive_builder.dart
```

### 13.2 Theme Application

```dart
// lib/app.dart

class GopostApp extends ConsumerWidget {
  const GopostApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final themeMode = ref.watch(themeModeProvider);
    final router = ref.watch(routerProvider);

    return MaterialApp.router(
      title: 'Gopost',
      theme: AppTheme.light(),
      darkTheme: AppTheme.dark(),
      themeMode: themeMode,
      routerConfig: router,
      debugShowCheckedModeBanner: false,
    );
  }
}
```

### 13.3 Conventions

1. **Never use raw color literals** in widget code. Always reference `Theme.of(context).colorScheme` or `AppColors` constants.
2. **Never use raw TextStyle** in widget code. Always reference `Theme.of(context).textTheme`.
3. **Never use raw numeric padding/margin.** Always reference `AppSpacing` constants.
4. **Always use `GpButton`, `GpTextField`, etc.** instead of raw `ElevatedButton`, `TextField`. This ensures consistency.
5. **Always wrap images in `Semantics`** with descriptive labels.
6. **Always test with dark mode** before merging any UI PR.
7. **Always test at `compact` and `expanded` breakpoints** before merging.

---

## 14. Sprint Stories

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Foundation |
| **Sprint(s)** | Sprint 1 (Weeks 1-2) |
| **Team** | Flutter Engineers, UI/UX Designer |
| **Predecessor** | [03-frontend-architecture.md](../architecture/03-frontend-architecture.md) |
| **Story Points Total** | 34 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| DS-001 | As a Flutter Engineer, I want `app_colors.dart` with all brand, neutral, semantic, and editor color tokens for light and dark modes so that color usage is centralized. | - `AppColors` and `AppColorsDark` classes defined<br/>- All tokens from Section 2 present<br/>- Unit test verifying contrast ratios for text/background pairs | 3 | P0 | APP-013 |
| DS-002 | As a Flutter Engineer, I want `app_typography.dart` with the complete type scale so that text styling is consistent. | - `AppTypography.textTheme` defined with all 15 styles<br/>- Inter font bundled in assets<br/>- Unit test verifying all text styles are non-null | 2 | P0 | DS-001 |
| DS-003 | As a Flutter Engineer, I want `app_spacing.dart` with all spacing constants and common EdgeInsets so that layout is on-grid. | - All tokens from Section 4 defined<br/>- `screenPadding`, `cardPadding`, `dialogPadding`, `listItemPadding` helpers present | 1 | P0 | APP-013 |
| DS-004 | As a Flutter Engineer, I want `app_breakpoints.dart` with `ScreenSize` enum, breakpoint constants, and `ResponsiveBuilder` widget so that responsive layouts work. | - `AppBreakpoints.of(context)` returns correct `ScreenSize`<br/>- `ResponsiveBuilder` widget renders correctly at all breakpoints<br/>- Widget test at 375, 768, 1024, 1440 widths | 3 | P0 | DS-003 |
| DS-005 | As a Flutter Engineer, I want `app_motion.dart` with duration and easing constants so that animations are consistent. | - `AppDurations` and `AppCurves` defined<br/>- All tokens from Section 9 present | 1 | P0 | APP-013 |
| DS-006 | As a Flutter Engineer, I want `app_theme.dart` with `AppTheme.light()` and `AppTheme.dark()` wired into the app so that Material 3 theming works end-to-end. | - Both themes build without error<br/>- `MaterialApp` uses both themes<br/>- `themeMode` switchable via Riverpod provider<br/>- Visual verification on Android, iOS, Web | 5 | P0 | DS-001, DS-002 |
| DS-007 | As a Flutter Engineer, I want `GpButton` widget with all variants (filled, tonal, outlined, text) and sizes so that buttons are standardized. | - All 4 variants render correctly<br/>- 3 sizes (small, medium, large) with correct dimensions<br/>- States: default, hover, pressed, disabled, loading<br/>- Widget tests for each variant/size/state | 5 | P0 | DS-006 |
| DS-008 | As a Flutter Engineer, I want `GpTextField` widget with all variants so that form inputs are standardized. | - Standard, password, search, multi-line variants<br/>- States: default, focused, error, success, disabled<br/>- Compact and standard sizes<br/>- Widget tests | 3 | P0 | DS-006 |
| DS-009 | As a Flutter Engineer, I want `GpCard` widget (template card, stats card, media card) so that card layouts are consistent. | - Template card with thumbnail, name, metadata, tags, premium badge<br/>- Stats card with trend, value, label, progress<br/>- Media card with thumbnail, filename, size<br/>- Dark mode verified | 5 | P0 | DS-006 |
| DS-010 | As a Flutter Engineer, I want adaptive dialog and sheet helpers (`showGpDialog`, `showGpBottomSheet`) so that overlay patterns are platform-correct. | - Material dialog on Android/Web/Windows<br/>- Cupertino dialog on iOS/macOS<br/>- Bottom sheet on mobile, centered dialog on desktop | 3 | P1 | DS-006 |
| DS-011 | As a Flutter Engineer, I want `GpEmptyState`, `GpSkeleton`, and `GpSnackbar` utility widgets so that loading, empty, and feedback states are covered. | - Skeleton/shimmer loading placeholder<br/>- Empty state with icon, message, optional CTA<br/>- Snackbar with auto-dismiss, action button | 3 | P1 | DS-006 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit / widget tests passing
- [ ] Dark mode verified for every component
- [ ] Responsive behavior verified at compact + expanded breakpoints
- [ ] Documentation updated
