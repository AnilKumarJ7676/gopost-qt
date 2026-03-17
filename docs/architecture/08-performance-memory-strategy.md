## 8. Performance and Memory Strategy

### 8.1 Performance Targets

| Metric | Target | Measurement |
|--------|--------|-------------|
| App cold start | < 2 seconds | Time from tap to first frame on mid-range device |
| Template preview load | < 500 ms | Time from tap to first preview frame |
| Editor timeline FPS | 60 fps (preview) | Sustained during scrubbing/playback |
| Template browsing scroll | 60 fps | No jank during infinite scroll |
| API response (p95) | < 200 ms | Backend processing time (excluding network) |
| API response (p99) | < 500 ms | Including database queries |
| Video export (1080p, 30s) | < 60 seconds | On mid-range mobile device |
| Image export (4K) | < 3 seconds | On mid-range mobile device |
| Memory (mobile) | < 512 MB peak | During editing with 10+ layers |
| Memory (desktop) | < 1.5 GB peak | During editing with 20+ layers |

### 8.2 Flutter Performance Strategies

| Strategy | Implementation |
|----------|----------------|
| **Lazy loading** | `ListView.builder` for all scrollable lists; templates load on-demand |
| **Image caching** | `cached_network_image` with 200 MB disk cache, 50 MB memory cache |
| **Isolate offloading** | JSON parsing, encryption, thumbnail generation run in `Isolate.run()` |
| **Const constructors** | All stateless widgets use `const` where possible |
| **RepaintBoundary** | Wrap complex widgets (editor canvas, timeline) to isolate repaints |
| **Widget rebuild minimization** | `select()` on Riverpod providers to only watch needed fields |
| **Deferred loading** | Video/image editor modules loaded via deferred imports on Web |
| **Shader warm-up** | Pre-compile common shaders during splash screen |

### 8.3 C/C++ Engine Performance Strategies

| Strategy | Implementation |
|----------|----------------|
| **Memory pools** | Pre-allocated frame buffers (see Section 5.5) to eliminate malloc/free churn |
| **Zero-copy textures** | GPU textures shared directly with Flutter Texture widget — no CPU readback |
| **Streaming decode** | Video frames decoded on-demand, never full-file buffered |
| **Thread pool** | Fixed-size thread pool (core count - 1) for parallel filter execution |
| **SIMD** | Use NEON (ARM) / SSE4/AVX2 (x86) for pixel operations where GPU unavailable |
| **GPU compute shaders** | Color grading, blur, blend modes run as GPU compute pipelines |
| **Frame recycling** | Ring buffer of pre-allocated frames for preview playback |
| **Texture atlas** | Small stickers/icons packed into atlas to reduce draw calls |

### 8.4 Backend Performance Strategies

| Strategy | Implementation |
|----------|----------------|
| **Connection pooling** | `pgxpool` with min=10, max=100 connections |
| **Query optimization** | Indexes on all foreign keys + common query patterns; `EXPLAIN ANALYZE` in dev |
| **Redis caching** | Template metadata: 5-min TTL; category lists: 15-min TTL; user sessions: 7-day TTL |
| **Response compression** | gzip for JSON, brotli for static assets |
| **CDN-first delivery** | All template binaries and preview images served from CDN edge |
| **Pagination** | Cursor-based pagination for large lists (no OFFSET) |
| **Async processing** | Video transcoding, thumbnail generation via message queue (NATS) + worker pool |
| **Database read replicas** | Read traffic routed to replicas; writes to primary only |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 6: Polish & Launch |
| **Sprint(s)** | Sprint 14 (Weeks 27-28) — plus ongoing benchmarks every sprint |
| **Team** | Platform, Backend, QA Engineers |
| **Predecessor** | All Sprints 1-13 complete |
| **Successor** | [09-infrastructure-devops.md](09-infrastructure-devops.md) |
| **Story Points Total** | 48 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| APP-087 | As a QA Engineer, I want to establish performance baseline benchmarking (all platforms) so that we have measurable targets. | - Benchmark suite for cold start, template load, editor FPS, API latency, export times<br/>- Baselines recorded for iOS, Android, Windows, macOS<br/>- Documented in CI or dashboard | 5 | P0 | All Sprints 1-13 |
| APP-088 | As a Flutter Engineer, I want to optimize cold start to <2s target so that the app feels responsive. | - Time from tap to first frame measured on mid-range device<br/>- Deferred loading, shader warm-up, lazy init applied<br/>- Target met on 80% of supported devices | 5 | P0 | APP-087 |
| APP-089 | As a Flutter Engineer, I want to optimize template preview load to <500ms so that browsing is snappy. | - Time from tap to first preview frame measured<br/>- Caching, CDN, parallel fetch applied<br/>- Target met for typical template size | 3 | P0 | APP-087 |
| APP-090 | As a Platform Engineer, I want to validate editor timeline 60fps so that scrubbing/playback is smooth. | - FPS measured during timeline scrubbing and playback<br/>- RepaintBoundary, frame recycling, GPU path verified<br/>- 60fps sustained on mid-range device | 5 | P0 | APP-087 |
| APP-091 | As a Flutter Engineer, I want to validate template browsing scroll at 60fps so that infinite scroll has no jank. | - ListView.builder, image caching, lazy load verified<br/>- Scroll FPS measured<br/>- No dropped frames during scroll | 3 | P0 | APP-087 |
| APP-092 | As a Backend Engineer, I want to validate API p95 <200ms so that backend latency meets SLA. | - p95 and p99 measured for key endpoints<br/>- Query optimization, connection pooling verified<br/>- Target met for template browse, auth, media | 3 | P0 | APP-087 |
| APP-093 | As a Platform Engineer, I want to benchmark video export (1080p 30s <60s) so that export performance is acceptable. | - Export time measured on mid-range mobile<br/>- HW encode used where available<br/>- Target met | 5 | P0 | APP-087 |
| APP-094 | As a Platform Engineer, I want to benchmark image export (4K <3s) so that image export is fast. | - 4K export time measured on mid-range device<br/>- Target met<br/>- Memory during export within budget | 3 | P0 | APP-087 |
| APP-095 | As a Platform Engineer, I want to enforce memory ceiling (512MB mobile / 1.5GB desktop) so that OOM is prevented. | - Memory profiled during editing with 10+ layers (mobile) and 20+ (desktop)<br/>- Pool allocator, texture cache limits enforced<br/>- Peak memory under ceiling | 5 | P0 | APP-054 |
| APP-096 | As a Flutter Engineer, I want to apply Flutter performance strategies (lazy loading, isolates, const, RepaintBoundary, shader warm-up) so that the UI is performant. | - Lazy loading for lists and deferred imports<br/>- Isolates for JSON parsing, encryption<br/>- Const constructors, RepaintBoundary, shader warm-up applied<br/>- Checklist verified | 3 | P0 | APP-088 |
| APP-097 | As a Platform Engineer, I want to apply C++ engine SIMD + GPU compute optimization so that filter/effect performance is maximized. | - NEON/SSE4/AVX2 for pixel ops where GPU unavailable<br/>- GPU compute shaders for color grading, blur, blend<br/>- Measurable improvement in filter pipeline | 5 | P1 | APP-052 |
| APP-098 | As a Backend Engineer, I want to perform backend query optimization (EXPLAIN ANALYZE pass) so that slow queries are eliminated. | - EXPLAIN ANALYZE on all major queries<br/>- Indexes added for missing coverage<br/>- N+1 and full scans eliminated | 3 | P0 | APP-092 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Integration tests passing
- [ ] Documentation updated
- [ ] No critical or high-severity bugs open
- [ ] Sprint review demo completed
