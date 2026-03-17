## 13. Testing Strategy

### 13.1 Testing Pyramid

```
         /‾‾‾‾‾‾‾‾‾‾‾\
        /   E2E (5%)   \
       /   (Integration  \
      /    Tests on Real  \
     /     Devices/CI)     \
    /‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\
   / Integration Tests (20%)  \
  / (API, DB, FFI bridge,     \
 /   service-to-service)       \
/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\
/     Unit Tests (75%)           \
/ (Services, repos, widgets,     \
/  engine functions, utils)       \
‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
```

### 13.2 Unit Tests

**Flutter (Dart):**

| Target | Framework | Coverage Goal |
|--------|-----------|--------------|
| Use cases | `flutter_test` + `mockito` | 95% |
| Repositories | `flutter_test` + `mockito` | 90% |
| State notifiers | `flutter_test` + `riverpod_test` | 90% |
| Widgets | `flutter_test` (widget tests) | 80% |
| Utils/helpers | `flutter_test` | 95% |

Example:
```dart
@GenerateMocks([TemplateRepository])
void main() {
  late MockTemplateRepository mockRepo;
  late GetTemplates useCase;

  setUp(() {
    mockRepo = MockTemplateRepository();
    useCase = GetTemplates(repository: mockRepo);
  });

  test('returns paginated template list on success', () async {
    when(mockRepo.getTemplates(any, any))
        .thenAnswer((_) async => Right(templateListFixture));

    final result = await useCase(const TemplateFilter(), const Pagination());

    expect(result.isRight(), true);
    verify(mockRepo.getTemplates(any, any)).called(1);
  });
}
```

**Go (Backend):**

| Target | Framework | Coverage Goal |
|--------|-----------|--------------|
| Services | `testing` + `testify` + `mockery` | 90% |
| Repositories | `testing` + `testcontainers` | 85% |
| Controllers | `testing` + `httptest` | 85% |
| Middleware | `testing` + `httptest` | 90% |
| Crypto module | `testing` | 95% |

Example:
```go
func TestTemplateService_GetByID(t *testing.T) {
    mockRepo := new(mocks.TemplateRepository)
    svc := service.NewTemplateService(mockRepo, nil, nil, nil)

    expectedTemplate := &entity.Template{
        ID:   uuid.New(),
        Name: "Test Template",
        Type: "video",
    }
    mockRepo.On("GetByID", mock.Anything, expectedTemplate.ID).Return(expectedTemplate, nil)

    result, err := svc.GetByID(context.Background(), expectedTemplate.ID)

    assert.NoError(t, err)
    assert.Equal(t, expectedTemplate.Name, result.Name)
    mockRepo.AssertExpectations(t)
}
```

**C++ (Engine):**

| Target | Framework | Coverage Goal |
|--------|-----------|--------------|
| Memory allocator | Google Test | 95% |
| Crypto (AES/RSA) | Google Test | 95% |
| Template parser | Google Test | 90% |
| Filter pipeline | Google Test | 85% |
| Compositor | Google Test | 85% |

Example:
```cpp
TEST(FramePoolAllocatorTest, AcquireAndRelease) {
    constexpr size_t kFrameSize = 1920 * 1080 * 4;  // RGBA
    constexpr size_t kPoolCount = 4;

    FramePoolAllocator pool(kFrameSize, kPoolCount);

    std::vector<uint8_t*> frames;
    for (size_t i = 0; i < kPoolCount; ++i) {
        auto* frame = pool.acquire();
        ASSERT_NE(frame, nullptr);
        frames.push_back(frame);
    }

    // Pool exhausted
    ASSERT_EQ(pool.acquire(), nullptr);

    // Release and re-acquire
    pool.release(frames[0]);
    auto* reused = pool.acquire();
    ASSERT_NE(reused, nullptr);
    ASSERT_EQ(reused, frames[0]);
}
```

### 13.3 Integration Tests

| Scope | Tools | Description |
|-------|-------|-------------|
| API endpoints | `httptest` + real DB (testcontainers-go) | Full request -> controller -> service -> repository -> PostgreSQL round-trip |
| Redis integration | `testcontainers-go` | Session management, cache operations, rate limiting |
| S3 integration | MinIO testcontainer | File upload, download, signed URL generation |
| Elasticsearch | ES testcontainer | Template indexing, full-text search queries |
| FFI bridge | Flutter integration test + compiled engine | Dart -> FFI -> C++ -> render frame round-trip |
| Encryption pipeline | Integration test | Encrypt on server -> download -> decrypt in engine -> verify content integrity |

### 13.4 End-to-End Tests

| Platform | Tool | Scope |
|----------|------|-------|
| Android | Firebase Test Lab + Patrol | Full user flows: register -> browse -> open editor -> export |
| iOS | Xcode Test + Patrol | Same flows on multiple iPhone/iPad models |
| Web | Playwright | Browser-based testing of web deployment |
| Desktop | Flutter integration_test | Automated flows on macOS and Windows |

**Key E2E scenarios:**

1. New user registration -> email verification -> first template browse
2. Template preview -> subscribe -> download -> edit -> export
3. Admin login -> upload template -> review -> publish -> verify in client
4. In-app video editor -> create project -> add clips -> effects -> export
5. In-app image editor -> layers -> filters -> text -> export

### 13.5 Security Testing

| Test Type | Tool | Frequency |
|-----------|------|-----------|
| OWASP Top 10 | ZAP (DAST) | Every release |
| Dependency CVEs | Snyk / Trivy | Every CI run |
| API fuzzing | RESTler / Schemathesis | Weekly |
| Penetration testing | Manual (external firm) | Quarterly |
| Template extraction attempts | Custom test suite | Every release |
| Binary analysis resistance | Manual review | Quarterly |

**Security test checklist:**

- [ ] SQL injection via all input fields
- [ ] JWT manipulation (alg:none, expired tokens, forged tokens)
- [ ] RBAC bypass attempts (user accessing admin endpoints)
- [ ] Rate limit bypass (distributed requests)
- [ ] File upload validation (malicious files, oversized uploads)
- [ ] Template decryption key extraction (memory dump, debugger attach)
- [ ] SSL pinning bypass (proxy interception)
- [ ] OAuth token substitution
- [ ] CORS misconfiguration

### 13.6 Performance Testing

| Test Type | Tool | Metrics |
|-----------|------|---------|
| API load testing | k6 | RPS, latency (p50/p95/p99), error rate |
| Database benchmarks | pgbench | TPS, query latency under load |
| Mobile profiling | Xcode Instruments / Android Profiler | Memory, CPU, GPU, battery |
| Flutter rendering | DevTools performance overlay | Frame times, jank rate |
| Engine benchmarks | Custom C++ benchmark suite | Frame render time, memory usage, decode throughput |

**Load test targets:**

| Scenario | Target | Condition |
|----------|--------|-----------|
| Template browse | 10,000 RPS | p95 < 100ms |
| Template access | 2,000 RPS | p95 < 200ms |
| Auth endpoints | 1,000 RPS | p95 < 150ms |
| Media upload | 500 concurrent | All complete within 30s |
| Sustained load | 5,000 RPS | 0% errors over 1 hour |

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Ongoing (every sprint), focused Sprints 14-16 |
| **Sprint(s)** | Sprints 1-16 (ongoing), Sprints 14-16 (focused test sprints) |
| **Team** | QA Engineer, Flutter Developer, Go Backend Developer, C++ Engine Developer |
| **Predecessor** | All development sprints |
| **Successor** | [14-risk-assessment.md](14-risk-assessment.md) |
| **Story Points Total** | 68 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| APP-164 | As a Flutter developer, I want Flutter unit test framework setup (mockito, riverpod_test) so that I can write and run unit tests | - mockito and riverpod_test in pubspec<br/>- @GenerateMocks annotation configured<br/>- Example test for use case and repository | 3 | P0 | — |
| APP-165 | As a Flutter developer, I want Flutter widget test framework so that I can test UI components in isolation | - flutter_test configured for widget tests<br/>- pumpWidget, find, expect patterns documented<br/>- Example widget test for core component | 3 | P0 | APP-164 |
| APP-166 | As a Go developer, I want Go unit test framework (testify, mockery) so that I can write and run unit tests | - testify and mockery in go.mod<br/>- mockery generates mocks from interfaces<br/>- Example test for service with mocked repo | 3 | P0 | — |
| APP-167 | As a Go developer, I want Go integration test setup (testcontainers) so that I can test against real PostgreSQL, Redis | - testcontainers-go for PostgreSQL, Redis, MinIO, Elasticsearch<br/>- Helper to spin up stack, run migrations<br/>- Example API integration test | 5 | P0 | APP-099 |
| APP-168 | As a C++ developer, I want C++ Google Test framework setup so that I can write and run engine unit tests | - Google Test in CMake, linked to engine lib<br/>- Example test for allocator or crypto<br/>- ctest integration for CI | 3 | P0 | — |
| APP-169 | As a developer, I want FFI bridge integration test so that we verify Dart→C++→render round-trip | - Flutter integration test that invokes FFI, receives frame<br/>- Validates pixel output or frame metadata<br/>- Runs on at least one platform in CI | 5 | P0 | APP-168 |
| APP-170 | As a developer, I want encryption pipeline integration test so that we verify encrypt→download→decrypt integrity | - Server encrypts template, client decrypts in engine<br/>- Asserts content matches original<br/>- Covers key exchange and AES decryption | 5 | P0 | APP-169 |
| APP-171 | As a QA engineer, I want E2E test: Android (Firebase Test Lab + Patrol) so that we validate flows on real devices | - Patrol configured for Flutter E2E<br/>- Firebase Test Lab matrix (20+ device profiles)<br/>- At least one smoke test (launch, login) | 5 | P0 | — |
| APP-172 | As a QA engineer, I want E2E test: iOS (Xcode Test + Patrol) so that we validate flows on iPhone/iPad | - Patrol or Xcode UI tests configured<br/>- Multiple device simulators in matrix<br/>- Same smoke scenarios as Android | 5 | P0 | APP-171 |
| APP-173 | As a QA engineer, I want E2E test: Web (Playwright) so that we validate web deployment | - Playwright configured for Flutter web build<br/>- Cross-browser (Chrome, Safari, Firefox)<br/>- Smoke test: load, browse, login | 5 | P0 | — |
| APP-174 | As a QA engineer, I want E2E scenario: registration→browse→edit→export so that we validate the core user journey | - Full flow: register, verify, browse templates, open editor, export<br/>- Assert key UI states and API responses<br/>- Runs in CI on staging | 5 | P0 | APP-171, APP-172 |
| APP-175 | As a QA engineer, I want E2E scenario: admin upload→review→publish so that we validate admin workflow | - Admin login, upload template, review, publish<br/>- Verify template appears in client browse<br/>- Runs in CI on staging | 3 | P0 | APP-174 |
| APP-176 | As a Security Engineer, I want security test: OWASP ZAP scan so that we detect common vulnerabilities | - ZAP baseline or full scan in CI or weekly<br/>- No high/critical findings before release<br/>- Report archived for audit | 3 | P1 | — |
| APP-177 | As a Security Engineer, I want security test: API fuzzing (Schemathesis) so that we find API edge cases | - Schemathesis generates tests from OpenAPI spec<br/>- Runs in CI or weekly<br/>- Failures triaged and fixed | 3 | P1 | APP-147 |
| APP-178 | As a QA engineer, I want performance test: k6 load testing setup so that we validate API under load | - k6 script for template browse, auth, media upload<br/>- Targets: 10k RPS browse, p95 < 100ms<br/>- Runs on staging before release | 5 | P1 | — |
| APP-179 | As a QA engineer, I want performance test: mobile profiling (Instruments/Android Profiler) so that we catch memory/CPU issues | - Profiling runbook for Instruments and Android Profiler<br/>- Baseline metrics documented<br/>- Run before each major release | 3 | P1 | — |
| APP-180 | As a Tech Lead, I want code coverage reporting and CI gate (>80%) so that we maintain test quality | - Coverage reported for Flutter, Go, C++<br/>- CI fails if coverage drops below 80%<br/>- Coverage badge or report in PR | 5 | P1 | APP-164, APP-166, APP-168 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Integration tests passing
- [ ] Documentation updated
- [ ] No critical or high-severity bugs open
- [ ] Sprint review demo completed

---
