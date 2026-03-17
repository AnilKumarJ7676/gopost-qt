# Gopost — Documentation Index

> **Version:** 1.0.0
> **Date:** February 23, 2026
> **Classification:** Internal — Engineering Reference

---

## Part 1: Application Architecture

The complete Gopost application development master plan covering the Flutter frontend, Go backend, media engine, security, infrastructure, and deployment.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Executive Summary | [01-executive-summary.md](architecture/01-executive-summary.md) | Purpose, platforms, tech stack, priorities |
| 2 | System Architecture | [02-system-architecture.md](architecture/02-system-architecture.md) | C4 diagrams, data flow, platform bridge |
| 3 | Frontend Architecture | [03-frontend-architecture.md](architecture/03-frontend-architecture.md) | Flutter project structure, Riverpod, routing, rendering bridge |
| 4 | Backend Architecture | [04-backend-architecture.md](architecture/04-backend-architecture.md) | Go + Gin API, DI, repositories, auth, rate limiting |
| 5 | Media Processing Engine | [05-media-processing-engine.md](architecture/05-media-processing-engine.md) | C/C++ engine structure, FFI, video/image pipelines, GPU backends |
| 6 | Secure Template System | [06-secure-template-system.md](architecture/06-secure-template-system.md) | .gpt format, encryption pipeline, anti-extraction |
| 7 | Security Architecture | [07-security-architecture.md](architecture/07-security-architecture.md) | Client, network, API, infra security, GDPR compliance |
| 8 | Performance & Memory | [08-performance-memory-strategy.md](architecture/08-performance-memory-strategy.md) | Performance targets, Flutter/C++/backend optimizations |
| 9 | Infrastructure & DevOps | [09-infrastructure-devops.md](architecture/09-infrastructure-devops.md) | Kubernetes, Docker, CI/CD, observability |
| 10 | Development Methodology | [10-development-methodology.md](architecture/10-development-methodology.md) | Agile/Scrum, team structure, sprint roadmap, code standards |
| 11 | API Reference | [11-api-reference.md](architecture/11-api-reference.md) | Auth, user, template, admin, subscription, media endpoints |
| 12 | Database Schema | [12-database-schema.md](architecture/12-database-schema.md) | ER diagram, tables, indexes, migration strategy |
| 13 | Testing Strategy | [13-testing-strategy.md](architecture/13-testing-strategy.md) | Test pyramid, unit/integration/E2E, security & performance testing |
| 14 | Risk Assessment | [14-risk-assessment.md](architecture/14-risk-assessment.md) | Technical, security, business, operational risks |
| 15 | Appendices | [15-appendices.md](architecture/15-appendices.md) | Glossary, technology decisions (Flutter/Go/C++/PostgreSQL), export specs |

---

## Part 2: Video Editor Engine

The professional-grade shared C/C++ video editor engine (`libgopost_ve`) targeting iOS, macOS, Android, and Windows. Comparable to After Effects, Premiere Pro, CapCut, and DaVinci Resolve.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Vision & Scope | [01-vision-and-scope.md](video-editor-engine/01-vision-and-scope.md) | Feature parity matrix, platform targets, non-functional requirements |
| 2 | Engine Architecture | [02-engine-architecture.md](video-editor-engine/02-engine-architecture.md) | Module layout, dependency graph, threading architecture |
| 3 | Core Foundation | [03-core-foundation.md](video-editor-engine/03-core-foundation.md) | Memory allocators, SIMD math, task system, event system |
| 4 | Timeline Engine | [04-timeline-engine.md](video-editor-engine/04-timeline-engine.md) | NLE data model, tracks/clips, edit operations, evaluation pipeline |
| 5 | Composition Engine | [05-composition-engine.md](video-editor-engine/05-composition-engine.md) | Layer compositor, blend modes, parenting, track mattes |
| 6 | GPU Rendering | [06-gpu-rendering-pipeline.md](video-editor-engine/06-gpu-rendering-pipeline.md) | Render graph, GPU abstraction, shader management |
| 7 | Effects & Filters | [07-effects-filter-system.md](video-editor-engine/07-effects-filter-system.md) | Effect graph, 80+ built-in effects catalog |
| 8 | Keyframe Animation | [08-keyframe-animation.md](video-editor-engine/08-keyframe-animation.md) | Bezier interpolation, graph editor, expression engine (Lua) |
| 9 | Motion Graphics | [09-motion-graphics.md](video-editor-engine/09-motion-graphics.md) | Shape layers, vector operators, path animation |
| 10 | Text & Typography | [10-text-typography.md](video-editor-engine/10-text-typography.md) | FreeType/HarfBuzz, text animators, font management |
| 11 | Audio Engine | [11-audio-engine.md](video-editor-engine/11-audio-engine.md) | Audio graph, effects, waveform/spectrum, beat detection |
| 12 | Transition System | [12-transition-system.md](video-editor-engine/12-transition-system.md) | 40+ GPU transitions, custom shader API |
| 13 | AI Features | [13-ai-features.md](video-editor-engine/13-ai-features.md) | ML inference, background removal, auto-captions, tracking |
| 14 | Color Science | [14-color-science.md](video-editor-engine/14-color-science.md) | Grading pipeline, HDR, LUT engine, tone mapping |
| 15 | Media I/O & Codecs | [15-media-io-codec.md](video-editor-engine/15-media-io-codec.md) | Decoder/encoder, HW acceleration, export presets |
| 16 | Project & Serialization | [16-project-serialization.md](video-editor-engine/16-project-serialization.md) | .gpproj format, undo/redo, auto-save, proxy workflow |
| 17 | Plugin Architecture | [17-plugin-architecture.md](video-editor-engine/17-plugin-architecture.md) | Plugin SDK, custom shader effects, extension system |
| 18 | Platform Integration | [18-platform-integration.md](video-editor-engine/18-platform-integration.md) | Platform abstraction, per-platform details, thermal throttling |
| 19 | Memory & Performance | [19-memory-performance.md](video-editor-engine/19-memory-performance.md) | Frame cache, memory budgets, optimization strategies |
| 20 | Build System | [20-build-system.md](video-editor-engine/20-build-system.md) | CMake project, cross-compilation, build matrix |
| 21 | Public C API | [21-public-c-api.md](video-editor-engine/21-public-c-api.md) | Complete FFI boundary header for Flutter integration |
| 22 | Testing & Validation | [22-testing-validation.md](video-editor-engine/22-testing-validation.md) | Test categories, reference rendering, fuzz testing |
| 23 | Development Roadmap | [23-development-roadmap.md](video-editor-engine/23-development-roadmap.md) | 48-week phased roadmap (8 phases) |
| 24 | Appendices | [24-appendices.md](video-editor-engine/24-appendices.md) | Third-party dependencies, shader source organization |

---

## Part 3: Image Editor Engine

The professional-grade shared C/C++ image editor engine (`libgopost_ie`) targeting iOS, macOS, Android, and Windows. Comparable to Photoshop, Canva, PicsArt, Snapseed, and CapCut image editing. Purpose-built for template creation with a shared C/C++ core.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Vision & Scope | [01-vision-and-scope.md](image-editor-engine/01-vision-and-scope.md) | Feature parity matrix, platform targets, non-functional requirements |
| 2 | Engine Architecture | [02-engine-architecture.md](image-editor-engine/02-engine-architecture.md) | Module layout, dependency graph, threading architecture, tile-based rendering |
| 3 | Core Foundation | [03-core-foundation.md](image-editor-engine/03-core-foundation.md) | Shared core with VE, IE-specific memory budgets, math extensions |
| 4 | Canvas Engine | [04-canvas-engine.md](image-editor-engine/04-canvas-engine.md) | Canvas data model, layer system, layer types, smart objects, media pool |
| 5 | Composition Engine | [05-composition-engine.md](image-editor-engine/05-composition-engine.md) | Layer compositor, blend modes, clipping masks, groups, adjustment layers |
| 6 | GPU Rendering | [06-gpu-rendering-pipeline.md](image-editor-engine/06-gpu-rendering-pipeline.md) | Shared render graph, IE shaders, large texture management |
| 7 | Effects & Filters | [07-effects-filter-system.md](image-editor-engine/07-effects-filter-system.md) | 100+ photo filters, image-specific effects, filter presets |
| 8 | Image Processing | [08-image-processing.md](image-editor-engine/08-image-processing.md) | RAW processing, retouching, healing, cloning, HDR merge |
| 9 | Text & Typography | [09-text-typography.md](image-editor-engine/09-text-typography.md) | Text effects, text warp, text on shape, text style presets |
| 10 | Vector Graphics | [10-vector-graphics.md](image-editor-engine/10-vector-graphics.md) | Shape system, pen tool, boolean operations, SVG import/export |
| 11 | Template System | [11-template-system.md](image-editor-engine/11-template-system.md) | Template model, placeholders, style controls, collage, element library |
| 12 | Masking & Selection | [12-masking-selection.md](image-editor-engine/12-masking-selection.md) | Selection tools, magic wand, refine edge, layer masks |
| 13 | Color Science | [13-color-science.md](image-editor-engine/13-color-science.md) | ICC profiles, soft proofing, histogram, color harmony |
| 14 | AI Features | [14-ai-features.md](image-editor-engine/14-ai-features.md) | Background removal, object removal, upscale, style transfer, face AI |
| 15 | Transform & Warp | [15-transform-warp.md](image-editor-engine/15-transform-warp.md) | Mesh warp, liquify, perspective correction, content-aware scale |
| 16 | Brush Engine | [16-brush-engine.md](image-editor-engine/16-brush-engine.md) | Brush system, dynamics, painting tools, dodge/burn/sponge |
| 17 | Export Pipeline | [17-export-pipeline.md](image-editor-engine/17-export-pipeline.md) | Multi-format export, batch, social media presets, print pipeline |
| 18 | Project & Serialization | [18-project-serialization.md](image-editor-engine/18-project-serialization.md) | .gpimg format, undo/redo, auto-save, history panel |
| 19 | Plugin Architecture | [19-plugin-architecture.md](image-editor-engine/19-plugin-architecture.md) | Plugin SDK, shader filters, brush plugins |
| 20 | Platform Integration | [20-platform-integration.md](image-editor-engine/20-platform-integration.md) | Per-platform details, stylus input, Flutter FFI bridge |
| 21 | Memory & Performance | [21-memory-performance.md](image-editor-engine/21-memory-performance.md) | Tile-based rendering, layer cache, memory budgets, optimization |
| 22 | Build System | [22-build-system.md](image-editor-engine/22-build-system.md) | CMake project, shared core linking, cross-compilation |
| 23 | Public C API | [23-public-c-api.md](image-editor-engine/23-public-c-api.md) | Complete FFI boundary header for Flutter integration |
| 24 | Testing & Validation | [24-testing-validation.md](image-editor-engine/24-testing-validation.md) | Test categories, reference rendering, fuzz testing |
| 25 | Development Roadmap | [25-development-roadmap.md](image-editor-engine/25-development-roadmap.md) | 48-week phased roadmap (8 phases) |
| 26 | Appendices | [26-appendices.md](image-editor-engine/26-appendices.md) | Third-party dependencies, ML models, shader organization, presets |

---

## Part 4: Design System

The Flutter UI/UX design system — color tokens, typography, spacing, component library, responsive breakpoints, platform adaptation, dark mode, motion, and accessibility standards.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Design System | [01-design-system.md](design-system/01-design-system.md) | Colors, typography, spacing, components, navigation, motion, dark mode, accessibility, implementation guide |

---

## Part 5: Admin Portal

Dedicated architecture and design document for the admin portal — screen inventory, moderation workflows, external upload pipeline, analytics, batch operations.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Admin Portal | [01-admin-portal.md](admin-portal/01-admin-portal.md) | Access control, navigation, screen wireframes, moderation state machine, upload pipeline, state management, batch operations, API mapping |

---

## Part 6: Monetization and Subscriptions

Complete monetization architecture — pricing tiers, paywall design, IAP integration (StoreKit 2 / Google Play Billing / Stripe), receipt validation, subscription lifecycle, feature gating, revenue analytics.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Monetization System | [01-monetization-system.md](monetization/01-monetization-system.md) | Plans, feature gating, paywall UI, IAP flows, receipt validation, lifecycle state machine, offers, analytics, DB schema additions |

---

## Part 7: API Specification

Machine-readable OpenAPI 3.0.3 specification for all backend endpoints. Serves as the contract between frontend and backend teams. Can be rendered via Swagger UI and used for client code generation.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | OpenAPI Spec | [openapi.yaml](api/openapi.yaml) | All endpoints (auth, users, templates, media, subscriptions, admin, categories), request/response schemas, security, pagination, rate limiting |

---

## Part 8: Offline and Caching Strategy

Comprehensive offline-first philosophy, multi-tier caching (in-memory, disk hot, disk cold, Redis, CDN), template/media caching policies, editor offline mode with auto-save, entitlement cache, sync-on-reconnect protocol, storage budgets per platform, and cache cleanup rules.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Offline & Caching Strategy | [01-offline-caching-strategy.md](offline-caching/01-offline-caching-strategy.md) | Cache tiers (L1/L2/L3/Redis/CDN), feature availability matrix, template binary cache, editor auto-save, entitlement cache, reconnect handler, storage budgets |

---

## Part 9: Localization and Internationalization

Flutter `gen_l10n` framework, ARB file structure, pluralization (including 6-form Arabic plurals), RTL layout support, dynamic locale switching, backend localized errors and push notifications, template metadata localization (JSONB), translation workflow with Crowdin, pseudo-locale CI checks.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Localization Architecture | [01-localization-architecture.md](localization/01-localization-architecture.md) | 6 launch locales, ARB naming convention, plural/gender ICU, RTL rules, locale provider, backend i18n middleware, JSONB metadata migration, Crowdin pipeline, CI checks |

---

## Part 10: Accessibility Standards

Cross-module WCAG 2.1 AA compliance plan extending the Design System baseline — module-specific requirements (editor, template browser, paywall, admin portal), screen reader semantics, keyboard/focus management, keyboard shortcuts, motion/contrast/typography rules, platform-specific considerations, testing matrix, audit and remediation process.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Accessibility Standards | [01-accessibility-standards.md](accessibility/01-accessibility-standards.md) | WCAG AA targets, per-module a11y requirements, semantic patterns, keyboard navigation, reduced motion, high contrast, dynamic type, VoiceOver/TalkBack testing, audit process |

---

## Part 11: Push Notification Architecture

FCM and APNs integration, notification types catalog (user-facing + silent/data), payload schemas, backend notification service (NATS queue, multi-device fan-out, locale-aware), Flutter client integration, notification channels, deep linking, user preference management, rate limiting, analytics.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Push Notification Architecture | [01-push-notification-architecture.md](notifications/01-push-notification-architecture.md) | FCM/APNs providers, 9 notification types, payload format, Go service + worker, Flutter firebase_messaging, deep link routing, silent push, preferences API, throttling |

---

## Part 12: Analytics and Event Tracking

Dual-pipeline analytics (client → Firebase Analytics, server → NATS → ClickHouse), 50+ event taxonomy, funnel definitions (activation, template usage, subscription, trial), A/B testing via Firebase Remote Config, privacy/consent flow, user properties, admin analytics dashboard integration, alerting.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Analytics & Event Tracking | [01-analytics-event-tracking.md](analytics/01-analytics-event-tracking.md) | Event naming convention, auth/template/editor/subscription/perf events, AnalyticsService abstraction, consent mode, funnel targets, A/B test framework, ClickHouse pipeline, Grafana alerts |

---

## Part 13: Creator Marketplace

Two-sided marketplace architecture enabling third-party creators to publish, sell, and earn revenue from templates. Revenue sharing model (70/30 split), Stripe Connect payouts, creator onboarding and verification, template review pipeline, marketplace storefront, ratings/reviews, fraud prevention, and creator analytics.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Creator Marketplace Architecture | [01-creator-marketplace-architecture.md](marketplace/01-creator-marketplace-architecture.md) | Creator profiles, submission pipeline, storefront, pricing model, Stripe Connect payouts, ledger, analytics, fraud prevention, DB schema, API endpoints |

---

## Part 14: Real-Time Collaboration

WebSocket-based real-time collaboration for multi-user template editing. CRDT conflict resolution (LWW-Register, OR-Set), presence system (cursors, selections, activity status), session management, permission model, offline editing with reconnection merge, and performance targets (<200ms p95 latency for 8 concurrent editors).

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Real-Time Collaboration | [01-real-time-collaboration.md](collaboration/01-real-time-collaboration.md) | WebSocket infrastructure, Redis Pub/Sub fan-out, CRDT engine (HLC ordering), presence, soft locking, session lifecycle, invite system, offline merge, DB schema |

---

## Part 15: Disaster Recovery & Business Continuity

Service tier classification, RTO/RPO targets per tier, backup strategy for all data stores, automated and manual failover procedures, multi-region active-passive architecture (us-east-1 primary, us-west-2 DR), incident response framework, runbooks, chaos engineering, and DR testing schedule.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Disaster Recovery & Business Continuity | [16-disaster-recovery-business-continuity.md](architecture/16-disaster-recovery-business-continuity.md) | Service tiers (0–3), RTO/RPO targets, backup matrix, PostgreSQL/Redis/S3/CDN failover, regional failover, incident severity levels, 10 runbooks, Chaos Mesh, DR test schedule |

---

## Part 16: App Store Submission Playbook

Step-by-step guidance for all target app stores (Apple, Google, Microsoft, Web). Pre-submission checklists, platform-specific compliance, screenshot and media specifications, metadata and localization, common rejection reasons with mitigations, staged rollout strategy, forced update mechanism, and CI/CD release pipelines with Fastlane.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | App Store Submission Playbook | [01-app-store-submission-playbook.md](app-store/01-app-store-submission-playbook.md) | iOS/Android/Windows/Web submission, IAP product setup, screenshot specs, metadata, rejection mitigations, staged rollout, hotfix process, Fastlane CI/CD |

---

## Part 17: Feature Flag System

In-house feature flag system with server-side (Go) and client-side (Flutter) evaluation. Boolean/string/number/JSON flags with targeting rules, percentage-based gradual rollout with sticky hashing, A/B experiment framework with statistical analysis, kill switches with <1s server propagation, admin dashboard, and flag lifecycle management.

| # | Section | File | Description |
|---|---------|------|-------------|
| 1 | Feature Flag System | [01-feature-flag-system.md](feature-flags/01-feature-flag-system.md) | Flag types, targeting rules (conditions + rollout), murmur3 hashing, A/B experiments, kill switches, Flutter SDK, Go evaluator, Redis caching, admin UI, DB schema |

---

## Master Sprint Overview

### App Architecture — 16 Sprints (32 Weeks)

| Phase | Sprints | Weeks | Focus | Stories | Points |
|---|---|---|---|---|---|
| Phase 1: Foundation | Sprint 1-2 | Wk 1-4 | Project scaffolding, auth, CI/CD, DB migrations | APP-004 to APP-042 | ~141 |
| Phase 2: Template Browser | Sprint 3-4 | Wk 5-8 | Template system, encryption, CDN, search | APP-059 to APP-070 | ~68 |
| Phase 3: Image Editor | Sprint 5-7 | Wk 9-14 | C++ image engine, FFI bridge, GPU, filters | APP-043 to APP-058 | ~88 |
| Phase 4: Video Editor | Sprint 8-11 | Wk 15-22 | C++ video engine, timeline, effects, export | APP-043 to APP-058 | ~88 |
| Phase 5: Admin Portal | Sprint 12-13 | Wk 23-26 | Dashboard, moderation, subscriptions, analytics | APP-124 to APP-148 | ~88 |
| Phase 6: Polish & Launch | Sprint 14-16 | Wk 27-32 | Performance, security, testing, app store submission | APP-087 to APP-196 | ~289 |
| **Total** | **16 sprints** | **32 weeks** | | **196 stories** | **~762 pts** |

### Video Editor Engine — 24 Sprints (48 Weeks)

| Phase | Sprints | Weeks | Focus | Stories | Points |
|---|---|---|---|---|---|
| Phase 1: Core Foundation | VE-Sprint 1-3 | Wk 1-6 | Memory, math, GPU, codecs, FFI, build system | VE-006 to VE-088, VE-241-258, VE-289-303, VE-319-330 | ~368 |
| Phase 2: Timeline Engine | VE-Sprint 4-6 | Wk 7-12 | Timeline, composition, project save, undo | VE-031 to VE-072, VE-259-275, VE-331-350 | ~350 |
| Phase 3: Effects & Color | VE-Sprint 7-9 | Wk 13-18 | Effects, keyframes, color grading, LUTs, HDR | VE-089 to VE-130, VE-226-240 | ~304 |
| Phase 4: Transitions/Text/Audio | VE-Sprint 10-12 | Wk 19-24 | Transitions, text engine, audio graph | VE-149 to VE-200 | ~225 |
| Phase 5: Motion Graphics | VE-Sprint 13-15 | Wk 25-30 | Shape layers, masking, tracking, expressions | VE-131 to VE-148 | ~72 |
| Phase 6: AI Features | VE-Sprint 16-18 | Wk 31-36 | ML inference, background removal, captions, tracking | VE-201 to VE-225 | ~98 |
| Phase 7: Plugin & Polish | VE-Sprint 19-21 | Wk 37-42 | Plugins, performance, memory, testing | VE-276-288, VE-304-318, VE-351-365 | ~234 |
| Phase 8: Hardening & Launch | VE-Sprint 22-24 | Wk 43-48 | Platform hardening, validation, RC builds | VE-289-303 (platform) | ~75 |
| **Total** | **24 sprints** | **48 weeks** | | **375 stories** | **~1,606 pts** |

### Image Editor Engine — 24 Sprints (48 Weeks)

| Phase | Sprints | Weeks | Focus | Stories | Points |
|---|---|---|---|---|---|
| Phase 1: Core Foundation | IE-Sprint 1-3 | Wk 1-5 | Shared core, canvas, media pool, tiles, FFI | IE-006 to IE-042, IE-061-072, IE-326-335 | ~237 |
| Phase 2: Layer & Compositing | IE-Sprint 4-6 | Wk 6-10 | Layers, blend modes, masks, styles, undo | IE-043 to IE-060, IE-277-290 | ~170 |
| Phase 3: Effects & Filters | IE-Sprint 7-9 | Wk 11-16 | Adjustments, filters, LUTs, color science | IE-073 to IE-098, IE-191-204 | ~170 |
| Phase 4: Text & Vector | IE-Sprint 10-12 | Wk 17-22 | Text engine, shapes, pen tool, SVG, booleans | IE-113 to IE-148 | ~150 |
| Phase 5: Template System | IE-Sprint 13-15 | Wk 23-28 | Templates, placeholders, palettes, collage | IE-149 to IE-170 | ~108 |
| Phase 6: Selection & Brush | IE-Sprint 16-18 | Wk 29-34 | Selection tools, brush engine, retouching | IE-171 to IE-260 | ~175 |
| Phase 7: Transform & AI | IE-Sprint 19-21 | Wk 35-40 | Warp, liquify, AI removal/upscale/style, RAW | IE-099-112, IE-205-240 | ~228 |
| Phase 8: Export & Launch | IE-Sprint 22-24 | Wk 41-48 | Export pipeline, plugins, performance, testing | IE-261-276, IE-291-370 | ~470 |
| **Total** | **24 sprints** | **48 weeks** | | **380 stories** | **~1,936 pts** |

### Tier 2 Cross-Cutting Concerns

| Document | Sprint(s) | Stories | Points |
|---|---|---|---|
| Offline & Caching | 1, 2, 3, 4, 5, 15 | 11 (OFF-001 to OFF-011) | 32 |
| Localization (i18n) | 15 | 12 (L10N-001 to L10N-012) | 42 |
| Accessibility | 15, 16 | 12 (A11Y-001 to A11Y-012) | 38 |
| Push Notifications | 13, 14 | 14 (PUSH-001 to PUSH-014) | 47 |
| Analytics & Event Tracking | 13, 14 | 16 (ANA-001 to ANA-016) | 48 |
| **Tier 2 Total** | | **65** | **207** |

### Tier 3 Nice-to-Have (Post-Launch / Parallel)

| Document | Sprint(s) | Stories | Points |
|---|---|---|---|
| App Store Submission Playbook | 15–16 | 13 (ASP-001 to ASP-013) | 52 |
| Creator Marketplace | 17–20 | 20 (MKT-001 to MKT-020) | 115 |
| Real-Time Collaboration | 21–24 | 20 (COL-001 to COL-020) | 108 |
| Disaster Recovery & Business Continuity | 25–26 | 12 (DR-001 to DR-012) | 55 |
| Feature Flag System | 27–28 | 14 (FF-001 to FF-014) | 62 |
| **Tier 3 Total** | | **79** | **392** |

### Grand Total

| Stream | Stories | Points | Sprints | Duration |
|---|---|---|---|---|
| App Architecture | 196 | ~762 | 16 | 32 weeks |
| Video Editor Engine | 375 | ~1,606 | 24 | 48 weeks |
| Image Editor Engine | 380 | ~1,936 | 24 | 48 weeks |
| Tier 1 Cross-Cutting (Design, Admin, Monetization, API) | 39 | ~173 | 1, 12-13 | Integrated |
| Tier 2 Cross-Cutting (Offline, i18n, A11y, Push, Analytics) | 65 | ~207 | 1-5, 13-16 | Integrated |
| Tier 3 Nice-to-Have (Marketplace, Collab, DR, App Store, Flags) | 79 | ~392 | 15-28 | Post-launch |
| **Grand Total** | **1,134** | **~5,076** | **64+14** | **48 weeks + 14 post-launch** |

> All three streams run in parallel. App Architecture (32 weeks) completes first; VE and IE engines run the full 48 weeks. Tier 1 and Tier 2 cross-cutting stories are distributed across the relevant sprints. Tier 3 stories begin post-launch (Sprint 15+) and run in parallel with ongoing maintenance. Cross-team sync points occur at FFI integration milestones.

---

## Architecture Diagrams

Each documentation area includes a `ARCHITECTURE.md` file with Mermaid diagrams that visually map to the implementation documents.

| Area | Diagram File | Key Diagrams |
|------|-------------|--------------|
| System Architecture | [architecture/ARCHITECTURE.md](architecture/ARCHITECTURE.md) | C4 Context · Container · Frontend Modules · Backend Services · Platform Bridge · Template Lifecycle · Security · Infrastructure · DB Schema · DR |
| Video Editor Engine | [video-editor-engine/ARCHITECTURE.md](video-editor-engine/ARCHITECTURE.md) | Module Graph · Threading · Timeline Model · Eval Pipeline · Composition · GPU · Effects · Audio · Media I/O · C API · Roadmap |
| Image Editor Engine | [image-editor-engine/ARCHITECTURE.md](image-editor-engine/ARCHITECTURE.md) | Module Graph · Shared vs IE-Only · Threading · Canvas Model · Tiles · Composition · Filters · Templates · Brush · C API · Roadmap |
| Design System | [design-system/ARCHITECTURE.md](design-system/ARCHITECTURE.md) | Token Hierarchy · Component Library · Platform Adaptation |
| Admin Portal | [admin-portal/ARCHITECTURE.md](admin-portal/ARCHITECTURE.md) | Portal Architecture · Moderation State Machine · Dashboard Flow |
| Monetization | [monetization/ARCHITECTURE.md](monetization/ARCHITECTURE.md) | Subscription Architecture · Purchase Flow · Lifecycle State Machine |
| Analytics | [analytics/ARCHITECTURE.md](analytics/ARCHITECTURE.md) | Dual-Pipeline Architecture · Event Taxonomy |
| Collaboration | [collaboration/ARCHITECTURE.md](collaboration/ARCHITECTURE.md) | WebSocket/CRDT Architecture · Operation Flow · Session Lifecycle |
| Notifications | [notifications/ARCHITECTURE.md](notifications/ARCHITECTURE.md) | Push Architecture · Notification Types |
| Marketplace | [marketplace/ARCHITECTURE.md](marketplace/ARCHITECTURE.md) | Marketplace Architecture · Submission Flow · Revenue Flow |
| Offline Caching | [offline-caching/ARCHITECTURE.md](offline-caching/ARCHITECTURE.md) | Multi-Tier Cache · Offline Availability · Reconnect Protocol |
| Localization | [localization/ARCHITECTURE.md](localization/ARCHITECTURE.md) | L10n Architecture · Locale Data Flow · RTL Support |
| Accessibility | [accessibility/ARCHITECTURE.md](accessibility/ARCHITECTURE.md) | A11y Architecture · Per-Module Requirements |
| Feature Flags | [feature-flags/ARCHITECTURE.md](feature-flags/ARCHITECTURE.md) | Flag Architecture · Evaluation Flow · A/B Experiments |
| App Store | [app-store/ARCHITECTURE.md](app-store/ARCHITECTURE.md) | Release Pipeline · Forced Update · Platform Checklists |

---

## File Structure

```
docs/
├── 00-INDEX.md                          ← You are here
├── architecture/
│   ├── 01-executive-summary.md
│   ├── 02-system-architecture.md
│   ├── 03-frontend-architecture.md
│   ├── 04-backend-architecture.md
│   ├── 05-media-processing-engine.md
│   ├── 06-secure-template-system.md
│   ├── 07-security-architecture.md
│   ├── 08-performance-memory-strategy.md
│   ├── 09-infrastructure-devops.md
│   ├── 10-development-methodology.md
│   ├── 11-api-reference.md
│   ├── 12-database-schema.md
│   ├── 13-testing-strategy.md
│   ├── 14-risk-assessment.md
│   ├── 15-appendices.md
│   └── 16-disaster-recovery-business-continuity.md
├── design-system/
│   └── 01-design-system.md
├── admin-portal/
│   └── 01-admin-portal.md
├── monetization/
│   └── 01-monetization-system.md
├── api/
│   └── openapi.yaml
├── offline-caching/
│   └── 01-offline-caching-strategy.md
├── localization/
│   └── 01-localization-architecture.md
├── accessibility/
│   └── 01-accessibility-standards.md
├── notifications/
│   └── 01-push-notification-architecture.md
├── analytics/
│   └── 01-analytics-event-tracking.md
├── marketplace/
│   └── 01-creator-marketplace-architecture.md
├── collaboration/
│   └── 01-real-time-collaboration.md
├── app-store/
│   └── 01-app-store-submission-playbook.md
├── feature-flags/
│   └── 01-feature-flag-system.md
├── video-editor-engine/
│   ├── 01-vision-and-scope.md
│   ├── 02-engine-architecture.md
│   ├── 03-core-foundation.md
│   ├── 04-timeline-engine.md
│   ├── 05-composition-engine.md
│   ├── 06-gpu-rendering-pipeline.md
│   ├── 07-effects-filter-system.md
│   ├── 08-keyframe-animation.md
│   ├── 09-motion-graphics.md
│   ├── 10-text-typography.md
│   ├── 11-audio-engine.md
│   ├── 12-transition-system.md
│   ├── 13-ai-features.md
│   ├── 14-color-science.md
│   ├── 15-media-io-codec.md
│   ├── 16-project-serialization.md
│   ├── 17-plugin-architecture.md
│   ├── 18-platform-integration.md
│   ├── 19-memory-performance.md
│   ├── 20-build-system.md
│   ├── 21-public-c-api.md
│   ├── 22-testing-validation.md
│   ├── 23-development-roadmap.md
│   └── 24-appendices.md
└── image-editor-engine/
    ├── 01-vision-and-scope.md
    ├── 02-engine-architecture.md
    ├── 03-core-foundation.md
    ├── 04-canvas-engine.md
    ├── 05-composition-engine.md
    ├── 06-gpu-rendering-pipeline.md
    ├── 07-effects-filter-system.md
    ├── 08-image-processing.md
    ├── 09-text-typography.md
    ├── 10-vector-graphics.md
    ├── 11-template-system.md
    ├── 12-masking-selection.md
    ├── 13-color-science.md
    ├── 14-ai-features.md
    ├── 15-transform-warp.md
    ├── 16-brush-engine.md
    ├── 17-export-pipeline.md
    ├── 18-project-serialization.md
    ├── 19-plugin-architecture.md
    ├── 20-platform-integration.md
    ├── 21-memory-performance.md
    ├── 22-build-system.md
    ├── 23-public-c-api.md
    ├── 24-testing-validation.md
    ├── 25-development-roadmap.md
    └── 26-appendices.md
```

---

> **Note:** The original monolithic `GOPOST_ARCHITECTURE.md` is retained at the project root for reference. These split files are the canonical source for ongoing development.
