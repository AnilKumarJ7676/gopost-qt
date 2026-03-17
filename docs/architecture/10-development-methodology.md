## 10. Development Methodology

### 10.1 Agile Framework

| Aspect | Detail |
|--------|--------|
| Methodology | Scrum |
| Sprint length | 2 weeks |
| Ceremonies | Sprint Planning (Monday), Daily Standup (15 min), Sprint Review (Friday W2), Retrospective (Friday W2) |
| Backlog tool | Jira / Linear |
| Documentation | Confluence / Notion, synced with this architecture doc |
| Communication | Slack, daily async updates for remote team members |

### 10.2 Recommended Team Structure

| Role | Count | Responsibility |
|------|-------|---------------|
| Tech Lead / Architect | 1 | Architecture decisions, code review, cross-team coordination |
| Flutter Developer | 3 | UI modules, state management, platform integration |
| Go Backend Developer | 2 | API services, database, infrastructure |
| C/C++ Engine Developer | 2 | Media processing, GPU pipelines, memory management |
| DevOps Engineer | 1 | CI/CD, Kubernetes, monitoring, infrastructure-as-code |
| QA Engineer | 1 | Test strategy, automation, performance testing |
| Security Engineer | 1 (part-time/consultant) | Security review, penetration testing, compliance |
| UI/UX Designer | 1 | Design system, user flows, prototypes |
| Product Manager | 1 | Roadmap, priorities, stakeholder communication |

### 10.3 Sprint Roadmap

#### Phase 1: Foundation (Weeks 1–4, Sprints 1–2)

| Sprint | Deliverables |
|--------|-------------|
| Sprint 1 | Flutter project scaffolding with module structure; Go project scaffolding with clean architecture; CI/CD pipelines for all three codebases (Flutter, Go, C++); Development environment (docker-compose); Core module: networking layer, error handling, logging; Database migrations: users, roles, sessions |
| Sprint 2 | Auth module (Flutter): login, registration, forgot password screens; Auth service (Go): JWT issuance, refresh token rotation, OAuth2 (Google, Apple); RBAC middleware; Rate limiting middleware; Secure storage integration; SSL pinning setup |

#### Phase 2: Template Browser (Weeks 5–8, Sprints 3–4)

| Sprint | Deliverables |
|--------|-------------|
| Sprint 3 | Database migrations: templates, categories, tags; Template CRUD API endpoints; Template search with Elasticsearch; Admin Portal: basic template upload flow; Encryption service: AES-256-GCM encrypt/decrypt |
| Sprint 4 | Flutter template browser: browse screen, category filter, search; Template detail screen with preview; CDN integration for template delivery; Signed URL generation; Client-side encrypted template download flow |

#### Phase 3: Image Editor (Weeks 9–14, Sprints 5–7)

| Sprint | Deliverables |
|--------|-------------|
| Sprint 5 | C++ image engine: decoder, encoder, canvas, basic layer support; FFI bridge: Flutter <-> C++ for image operations; GPU context setup (Metal, Vulkan, GLES); Basic image editor screen with canvas widget |
| Sprint 6 | Layer management UI (add, remove, reorder, opacity, blend modes); Filter pipeline: brightness, contrast, saturation, color grading; Text tool with font selection and styling; Sticker placement with transform gestures |
| Sprint 7 | Masking tools; Advanced filters (artistic, blur, sharpen); Image export pipeline; Save-as-template flow (in-app creation); Image template .gpt format packing |

#### Phase 4: Video Editor (Weeks 15–22, Sprints 8–11)

| Sprint | Deliverables |
|--------|-------------|
| Sprint 8 | C++ video engine: FFmpeg decoder/encoder integration; Timeline data model (tracks, clips, transitions); FFI bridge for video operations; Basic video editor screen with timeline widget |
| Sprint 9 | Timeline UI: drag-and-drop clips, trimming, splitting; Multi-track support (video, audio, text overlay); Playback preview via Texture widget (zero-copy GPU); Seek and scrub functionality |
| Sprint 10 | Effects system: color grading, blur, speed adjustment; Transition engine: fade, slide, zoom, custom transitions; Keyframe animation: position, scale, rotation, opacity; Audio mixing: volume, fade, multiple audio tracks |
| Sprint 11 | Video export pipeline (H.264/H.265, AAC, MP4); Export progress UI with cancel support; Save-as-template flow for video; Video template .gpt format packing; Performance optimization pass |

#### Phase 5: Admin Portal (Weeks 23–26, Sprints 12–13)

| Sprint | Deliverables |
|--------|-------------|
| Sprint 12 | Admin dashboard: statistics, recent uploads, user counts; Template management: list, review, publish, unpublish, delete; External template upload: validation, format conversion, encryption pipeline; User management: list, roles, ban/unban |
| Sprint 13 | Content moderation tools; Analytics dashboard (template popularity, user engagement); Subscription management and payment integration (Stripe/RevenueCat); Push notification service; Audit log viewer |

#### Phase 6: Polish and Launch (Weeks 27–32, Sprints 14–16)

| Sprint | Deliverables |
|--------|-------------|
| Sprint 14 | Performance optimization: profiling all platforms, memory leak fixes; Security hardening: penetration testing, anti-debug, obfuscation; Anti-extraction final implementation and testing |
| Sprint 15 | End-to-end testing on all 7 platforms; Accessibility audit and fixes; Localization framework (i18n); App Store / Play Store metadata preparation; Beta testing program (TestFlight, Play Console internal track) |
| Sprint 16 | Bug fixes from beta feedback; Final security audit; Production infrastructure setup (Kubernetes, monitoring, alerting); App Store submission (iOS, macOS); Play Store submission (Android); Web deployment; Windows/macOS distribution (Microsoft Store / direct download) |

### 10.4 Code Quality Standards

| Standard | Tool | Policy |
|----------|------|--------|
| Dart formatting | `dart format` | Enforced in CI; auto-format on save |
| Dart analysis | `flutter analyze` | Zero warnings policy |
| Go formatting | `gofmt` / `goimports` | Enforced in CI |
| Go linting | `golangci-lint` | Config: errcheck, govet, staticcheck, gosec, revive |
| C++ formatting | `clang-format` | Google style, enforced in CI |
| C++ analysis | `clang-tidy` | modernize-*, performance-*, bugprone-* |
| Code review | GitHub PRs | 1 approval minimum; 2 for security-sensitive code |
| Commit messages | Conventional Commits | `feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `chore:` |
| Branch strategy | Git Flow | `main`, `develop`, `feature/*`, `release/*`, `hotfix/*` |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Pre-Sprint (Reference Document — Process Setup) |
| **Sprint(s)** | Pre-Sprint (Before Sprint 1) |
| **Team** | Tech Lead, Product Manager, DevOps Engineer |
| **Predecessor** | [09-infrastructure-devops.md](09-infrastructure-devops.md) |
| **Successor** | [11-api-reference.md](11-api-reference.md) |
| **Story Points Total** | 16 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| APP-116 | As a Product Manager, I want Jira/Linear project setup with backlog so that we can track and prioritize work | - Project created with Gopost app structure<br/>- Epics and stories imported from architecture docs<br/>- Backlog groomed and prioritized | 3 | P0 | — |
| APP-117 | As a Tech Lead, I want Scrum ceremony schedule establishment so that the team has predictable cadence | - Sprint Planning (Monday), Daily Standup (15 min), Sprint Review (Friday W2), Retrospective (Friday W2) documented<br/>- Calendar invites sent to all team members<br/>- Remote async updates process defined | 2 | P0 | — |
| APP-118 | As a Tech Lead, I want team role assignment and onboarding so that everyone knows their responsibilities | - Roles assigned per 10.2 team structure<br/>- Onboarding checklist for each role<br/>- Access to tools (Slack, repo, Jira) provisioned | 2 | P0 | APP-116 |
| APP-119 | As a Tech Lead, I want sprint planning template creation so that planning is consistent and efficient | - Template includes: capacity, committed stories, definition of ready<br/>- Velocity tracking mechanism<br/>- Template stored in Confluence/Notion | 2 | P0 | APP-117 |
| APP-120 | As a Tech Lead, I want code quality standards documentation and CI enforcement so that quality is maintained | - Standards from 10.4 documented (Dart, Go, C++)<br/>- CI fails on format/lint violations<br/>- Zero warnings policy communicated to team | 2 | P0 | — |
| APP-121 | As a DevOps engineer, I want branch strategy (Git Flow) setup with protected branches so that we enforce workflow | - main, develop branches created and protected<br/>- feature/*, release/*, hotfix/* naming documented<br/>- PR required for merge to develop/main | 2 | P0 | — |
| APP-122 | As a DevOps engineer, I want conventional commit enforcement (commitlint) so that changelogs and releases are consistent | - commitlint configured with conventional config<br/>- CI fails on invalid commit messages<br/>- feat:, fix:, refactor:, docs:, test:, chore: prefixes enforced | 2 | P0 | APP-121 |
| APP-123 | As a Tech Lead, I want sprint retrospective cadence establishment so that we continuously improve | - Retro scheduled for Friday W2 of each sprint<br/>- Format: what went well, what to improve, action items<br/>- Action items tracked in backlog | 1 | P0 | APP-117 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Integration tests passing
- [ ] Documentation updated
- [ ] No critical or high-severity bugs open
- [ ] Sprint review demo completed

---
