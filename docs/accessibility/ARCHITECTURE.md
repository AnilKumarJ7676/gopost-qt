# Accessibility Standards — Architecture Diagram

> Maps to [01-accessibility-standards.md](01-accessibility-standards.md)

---

## Accessibility Architecture (WCAG 2.1 AA)

```mermaid
graph TB
    subgraph semantic ["Semantic Layer"]
        SEM["Flutter Semantics<br/>Labels · Hints · Actions"]
        LIVE["Live Regions<br/>Status announcements"]
        ORDER["Reading Order<br/>Semantic ordering"]
        CUSTOM["Custom Actions<br/>Editor operations"]
    end

    subgraph input ["Input Methods"]
        KB["Keyboard Navigation<br/>Tab · Shift+Tab · Arrows"]
        SC["Keyboard Shortcuts<br/>Ctrl+Z · Ctrl+S · Space"]
        FOCUS["Focus Management<br/>Visible focus ring"]
        TOUCH["Touch Targets<br/>≥ 44×44 pt"]
    end

    subgraph visual ["Visual Accessibility"]
        CONTRAST["Contrast Ratio<br/>≥ 4.5:1 text · ≥ 3:1 UI"]
        HC["High Contrast Mode"]
        DT["Dynamic Type<br/>System font scaling"]
        MOTION["Reduced Motion<br/>No auto-play"]
    end

    subgraph testing ["Testing Matrix"]
        VO["VoiceOver (iOS/macOS)"]
        TB["TalkBack (Android)"]
        NVDA2["NVDA (Windows)"]
        AXE["axe / Lighthouse (Web)"]
        CI_A["CI Lint Rules"]
    end

    semantic & input & visual --> testing
```

---

## Per-Module Accessibility Requirements

```mermaid
graph LR
    subgraph modules ["Modules"]
        BROWSE["Template Browser<br/>Card labels · Category announce"]
        EDITOR["Video/Image Editor<br/>Tool labels · Layer announce · Keyboard shortcuts"]
        PAYWALL["Paywall<br/>Plan comparison table · Focus trap"]
        ADMIN_A["Admin Portal<br/>Data tables · Form labels"]
        COLLAB_A["Collaboration<br/>Participant announce · Cursor labels"]
    end

    subgraph requirements ["WCAG Criteria"]
        PERCEIVE["Perceivable<br/>Text alternatives · Captions"]
        OPERATE["Operable<br/>Keyboard · No traps · Timing"]
        UNDERSTAND["Understandable<br/>Readable · Predictable"]
        ROBUST["Robust<br/>Compatible with AT"]
    end

    modules --> requirements
```
