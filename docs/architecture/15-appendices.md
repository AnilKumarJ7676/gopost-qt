## Appendix A: Glossary

| Term | Definition |
|------|-----------|
| **.gpt** | Gopost Template — custom encrypted binary format for template packages |
| **DEK** | Data Encryption Key — symmetric key used to encrypt template content |
| **FFI** | Foreign Function Interface — mechanism for Dart to call C/C++ functions |
| **HPA** | Horizontal Pod Autoscaler — Kubernetes resource for auto-scaling |
| **RBAC** | Role-Based Access Control — authorization model based on user roles |
| **Render Token** | Short-lived JWT issued by server authorizing client to decrypt and render a specific template |
| **Session Key** | Per-device, per-session RSA key pair used for secure key exchange |
| **Texture Registry** | Flutter mechanism to display GPU-rendered content without CPU-side pixel copies |

## Appendix B: Technology Decision Records

### B.1 Why Flutter over React Native / Native

- Single codebase for 7 platforms (including desktop and web)
- Dart AOT compilation delivers near-native performance
- Custom rendering engine (Skia/Impeller) ensures pixel-perfect consistency
- Strong FFI support for C/C++ interop — critical for media engine integration
- Growing ecosystem and Google backing

### B.2 Why Go over Node.js / Rust / Java

- Sub-millisecond goroutine scheduling for high-concurrency API workloads
- Extremely small memory footprint (10-20 MB per service instance)
- Fast compilation and simple deployment (single static binary)
- Strong standard library for HTTP, crypto, and JSON
- Easier hiring and onboarding compared to Rust
- Better performance characteristics than Node.js for CPU-bound crypto operations

### B.3 Why C/C++ over Rust for Media Engine

- Mature ecosystem for media processing (FFmpeg, OpenCV, FreeType)
- Better GPU API bindings (Metal, Vulkan, OpenGL)
- Flutter's FFI is designed for C calling convention
- Larger talent pool with media processing experience
- Consider Rust for new isolated components in the future (memory safety benefits)

### B.4 Why PostgreSQL over MongoDB / MySQL

- ACID compliance for financial transactions (subscriptions, payments)
- JSONB columns for flexible template metadata without schema migrations
- Built-in full-text search as fallback when Elasticsearch is unavailable
- Excellent performance with proper indexing
- Proven at scale (Instagram, Discord, Notion all use PostgreSQL)

---

## Appendix C: External Tool Template Export Specifications

### C.1 After Effects Export (Video Templates)

Supported export format for admin upload:

```
template_package/
├── manifest.json          # Template metadata, layer list, timing
├── composition.json       # Layer hierarchy, transforms, keyframes
├── assets/
│   ├── footage/           # Video clips (.mp4, .mov)
│   ├── images/            # Overlay images (.png, .webp)
│   ├── audio/             # Audio tracks (.aac, .mp3)
│   └── fonts/             # Embedded fonts (.ttf, .otf)
├── effects.json           # Effect definitions and parameters
└── thumbnail.webp         # Preview thumbnail
```

The admin upload API validates this structure, converts `composition.json` and `effects.json` into the internal .gpt layer/render format, encrypts everything, and produces the final `.gpt` file.

### C.2 Lightroom / Figma Export (Image Templates)

```
template_package/
├── manifest.json          # Template metadata, canvas dimensions
├── layers.json            # Layer definitions, transforms, blend modes
├── assets/
│   ├── backgrounds/       # Base images (.png, .webp)
│   ├── overlays/          # Overlay elements (.png with alpha)
│   ├── stickers/          # Sticker assets (.png, .svg)
│   └── fonts/             # Embedded fonts (.ttf, .otf)
├── filters.json           # Filter/adjustment definitions
└── thumbnail.webp         # Preview thumbnail
```

---

*This document is the canonical reference for all Gopost engineering decisions. It should be updated as the architecture evolves. All significant deviations from this plan require approval from the Tech Lead / Architect.*

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Pre-Sprint (Reference Document) |
| **Sprint(s)** | Pre-Sprint (Before Sprint 1) |
| **Team** | Tech Lead, Product Manager |
| **Predecessor** | [14-risk-assessment.md](14-risk-assessment.md) |
| **Successor** | Video Editor Engine docs |
| **Story Points Total** | 12 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| APP-191 | As a new team member, I want glossary documentation and team onboarding review so that I understand Gopost terminology | - Glossary (Appendix A) reviewed and complete<br/>- Onboarding doc references glossary<br/>- Team sign-off on definitions | 2 | P2 | — |
| APP-192 | As a Tech Lead, I want Flutter vs React Native decision record sign-off so that we have alignment on framework choice | - Decision record documents rationale (Appendix B.1)<br/>- Stakeholders sign off<br/>- Stored in Confluence/Notion | 2 | P2 | — |
| APP-193 | As a Tech Lead, I want Go vs Node.js/Rust decision record sign-off so that we have alignment on backend choice | - Decision record documents rationale (Appendix B.2)<br/>- Stakeholders sign off<br/>- Stored in Confluence/Notion | 2 | P2 | — |
| APP-194 | As a Tech Lead, I want C/C++ vs Rust decision record sign-off so that we have alignment on engine choice | - Decision record documents rationale (Appendix B.3)<br/>- Stakeholders sign off<br/>- Stored in Confluence/Notion | 2 | P2 | — |
| APP-195 | As a Tech Lead, I want PostgreSQL vs MongoDB decision record sign-off so that we have alignment on database choice | - Decision record documents rationale (Appendix B.4)<br/>- Stakeholders sign off<br/>- Stored in Confluence/Notion | 2 | P2 | — |
| APP-196 | As a backend developer, I want After Effects / Lightroom export spec validation with sample templates so that admin upload works correctly | - Sample AE and Lightroom export packages created<br/>- Validated against Appendix C spec<br/>- Admin upload pipeline tested with samples | 2 | P2 | — |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Integration tests passing
- [ ] Documentation updated
- [ ] No critical or high-severity bugs open
- [ ] Sprint review demo completed

---
