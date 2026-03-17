# Gopost — System Architecture Diagrams

> Auto-generated architecture diagrams mapping to implementation documents in `docs/architecture/`.

---

## 1. System Context (C4 Level 1)

> Ref: [01-executive-summary.md](01-executive-summary.md), [02-system-architecture.md](02-system-architecture.md)

```mermaid
graph TB
    subgraph users ["Actors"]
        EU["End User<br/>iOS · Android · Windows · macOS · Web"]
        AD["Admin / Creator<br/>Web Portal"]
    end

    subgraph gopost ["Gopost Platform"]
        FA["Flutter Client App<br/>(Dart 3.x)"]
        AP["Admin Portal<br/>(Flutter Web)"]
        BE["Go Backend API<br/>(Gin · REST)"]
        ME["C/C++ Media Engine<br/>(FFI Embedded)"]
        CDN["CDN<br/>(CloudFront / Cloudflare)"]
    end

    subgraph ext ["External Services"]
        OAuth["OAuth2<br/>Google · Apple · Facebook"]
        Pay["Payments<br/>Stripe · StoreKit 2 · Play Billing"]
        Push["Push<br/>FCM · APNs"]
        Ana["Analytics<br/>Firebase · ClickHouse"]
    end

    EU --> FA
    AD --> AP
    FA --> BE
    FA --> CDN
    FA -->|dart:ffi| ME
    AP --> BE
    BE --> OAuth
    BE --> Pay
    BE --> Push
    BE --> Ana
    BE --> CDN
```

---

## 2. Container Diagram (C4 Level 2)

> Ref: [02-system-architecture.md](02-system-architecture.md), [04-backend-architecture.md](04-backend-architecture.md)

```mermaid
graph TB
    subgraph clients ["Client Layer"]
        FM["Flutter Mobile<br/>iOS / Android"]
        FD["Flutter Desktop<br/>Windows / macOS"]
        FW["Flutter Web"]
        NE["C/C++ Engine<br/>(FFI)"]
    end

    subgraph backend ["Backend Cluster"]
        GW["API Gateway<br/>Traefik / Nginx"]
        AUTH["Auth Service"]
        TMPL["Template Service"]
        MEDIA["Media Service"]
        USER["User Service"]
        ADMIN["Admin Service"]
        NOTIF["Notification Service"]
        SUB["Subscription Service"]
    end

    subgraph data ["Data Layer"]
        PG[("PostgreSQL 16")]
        RD[("Redis 7")]
        S3[("S3 Object Storage")]
        ES[("Elasticsearch 8")]
        NATS["NATS Message Queue"]
    end

    subgraph infra ["Infrastructure"]
        CDN2["CDN Edge"]
        K8S["Kubernetes<br/>3–10 API pods · 2–8 workers"]
        OBS["Observability<br/>Prometheus · Grafana · Loki · Jaeger"]
    end

    FM & FD & FW --> GW
    FM & FD -->|FFI| NE

    GW --> AUTH & TMPL & MEDIA & USER & ADMIN & NOTIF & SUB

    AUTH --> PG & RD
    TMPL --> PG & S3 & ES & RD
    MEDIA --> S3 & NATS
    USER --> PG
    ADMIN --> PG
    NOTIF --> NATS
    SUB --> PG & RD

    TMPL & MEDIA --> CDN2
    K8S -.hosts.-> GW & AUTH & TMPL & MEDIA & USER & ADMIN & NOTIF & SUB
    OBS -.monitors.-> K8S
```

---

## 3. Frontend Module Dependency Graph

> Ref: [03-frontend-architecture.md](03-frontend-architecture.md)

```mermaid
graph TD
    subgraph flutter ["Flutter App (Dart)"]
        CORE["core/<br/>DI · Network · Security · Theme · Error"]
        AUTH_M["auth/<br/>Login · Register · OAuth"]
        TB["template_browser/<br/>Browse · Search · Detail"]
        VE["video_editor/<br/>Timeline · Effects · Export"]
        IE["image_editor/<br/>Canvas · Layers · Filters"]
        ADM["admin/<br/>Dashboard · Upload · Moderation"]
        RB["rendering_bridge/<br/>FFI · Channels · Texture"]
    end

    AUTH_M --> CORE
    TB --> CORE & AUTH_M & RB
    VE --> CORE & AUTH_M & RB
    IE --> CORE & AUTH_M & RB
    ADM --> CORE & AUTH_M
    RB --> CORE

    style CORE fill:#e1f5fe
    style RB fill:#fff3e0
```

---

## 4. Backend Service Architecture

> Ref: [04-backend-architecture.md](04-backend-architecture.md)

```mermaid
graph LR
    subgraph middleware ["Middleware Stack"]
        direction TB
        M1["CORS"]
        M2["Rate Limiter"]
        M3["Structured Logger"]
        M4["JWT Auth"]
        M5["RBAC"]
        M6["Recovery"]
        M1 --> M2 --> M3 --> M4 --> M5 --> M6
    end

    subgraph controllers ["Controllers"]
        AC["AuthController"]
        UC["UserController"]
        TC["TemplateController"]
        MC["MediaController"]
        SC["SubscriptionController"]
        ADC["AdminController"]
    end

    subgraph services ["Services"]
        AS["AuthService"]
        US["UserService"]
        TS["TemplateService"]
        MS["MediaService"]
        SS["SubscriptionService"]
        ADS["AdminService"]
        ENC["EncryptionService"]
        NS["NotificationService"]
    end

    subgraph repos ["Repositories"]
        PR["postgres/"]
        RR["redis/"]
        SR["s3/"]
        ER["elasticsearch/"]
    end

    M6 --> AC & UC & TC & MC & SC & ADC
    AC --> AS
    UC --> US
    TC --> TS
    MC --> MS
    SC --> SS
    ADC --> ADS

    AS --> PR & RR
    TS --> PR & SR & ER & RR & ENC
    MS --> SR & NS
    SS --> PR & RR
    ENC --> SR
    NS --> NATS2["NATS"]
```

---

## 5. Platform Bridge (Flutter ↔ C/C++ Engine)

> Ref: [02-system-architecture.md](02-system-architecture.md), [03-frontend-architecture.md](03-frontend-architecture.md), [05-media-processing-engine.md](05-media-processing-engine.md)

```mermaid
graph LR
    subgraph dart ["Flutter / Dart"]
        UI["Widget Tree"]
        SM["Riverpod State"]
        BA["Bridge API<br/>engine_api.dart"]
    end

    subgraph bridge ["Bridge Layer"]
        FFI["dart:ffi<br/>(Sync Calls)"]
        PC["Platform Channels<br/>(Async)"]
        TR["Texture Registry<br/>(Zero-Copy GPU)"]
    end

    subgraph native ["C/C++ Engine"]
        CL["libgopost_engine"]
        VP["Video Pipeline"]
        IP["Image Pipeline"]
        GPU["GPU Backend<br/>Metal · Vulkan · GL"]
        MP["Memory Pool"]
    end

    UI --> SM --> BA
    BA --> FFI & PC & TR
    FFI --> CL
    PC --> CL
    TR --> GPU
    CL --> VP & IP & GPU & MP
```

---

## 6. Template Lifecycle (End-to-End)

> Ref: [06-secure-template-system.md](06-secure-template-system.md)

```mermaid
sequenceDiagram
    participant Admin
    participant Portal as Admin Portal
    participant API as Go Backend
    participant KMS as Key Management
    participant S3 as Object Storage
    participant CDN
    participant App as Flutter Client
    participant Engine as C/C++ Engine

    rect rgb(230,245,255)
        Note over Admin,S3: Upload Flow
        Admin->>Portal: Upload template (.zip)
        Portal->>API: POST /admin/templates (multipart)
        API->>API: Validate + safety scan
        API->>KMS: Generate DEK
        API->>API: Encrypt (AES-256-GCM)
        API->>S3: Store .gpt blob
        API->>CDN: Warm cache
        API-->>Portal: 201 Created
    end

    rect rgb(255,243,224)
        Note over App,Engine: Download & Render Flow
        App->>API: GET /templates/:id/access
        API->>API: Auth + subscription + rate limit
        API->>KMS: Re-encrypt DEK with session key
        API-->>App: Signed CDN URL + wrapped DEK
        App->>CDN: Download .gpt
        App->>Engine: Encrypted blob + session key (FFI)
        Engine->>Engine: RSA unwrap DEK → AES decrypt
        Engine->>Engine: Parse layers → GPU render
        Engine-->>App: Frames via Texture Registry
    end
```

---

## 7. Security Architecture Layers

> Ref: [07-security-architecture.md](07-security-architecture.md)

```mermaid
graph TB
    subgraph client ["Client Security"]
        CS1["TLS 1.3 + Cert Pinning"]
        CS2["Code Obfuscation<br/>ProGuard · R8 · Bitcode"]
        CS3["Secure Storage<br/>Keychain · Keystore"]
        CS4["Runtime Integrity<br/>Jailbreak · Root · Debug"]
        CS5["Anti-Tamper<br/>Checksum · HMAC"]
    end

    subgraph network ["Network Security"]
        NS1["TLS 1.3"]
        NS2["Certificate Pinning"]
        NS3["Request Signing (HMAC)"]
    end

    subgraph api ["API Security"]
        AS1["JWT Auth (15 min access)"]
        AS2["RBAC (user · admin · super_admin)"]
        AS3["Rate Limiting (Redis)"]
        AS4["Input Validation"]
        AS5["CORS · CSP Headers"]
    end

    subgraph infra_sec ["Infrastructure Security"]
        IS1["WAF (Cloudflare / AWS)"]
        IS2["DDoS Protection"]
        IS3["Secrets (HashiCorp Vault)"]
        IS4["Encryption at Rest<br/>PostgreSQL TDE · S3 SSE-KMS"]
        IS5["Audit Logging → Loki"]
    end

    subgraph template_sec ["Template Security"]
        TS1["AES-256-GCM Encryption"]
        TS2["RSA-2048 Key Exchange"]
        TS3["Memory-Only Decryption"]
        TS4["Device-Bound Session Keys"]
        TS5["Time-Limited Render Tokens"]
    end

    client --> network --> api --> infra_sec
    template_sec -.-> client & api & infra_sec
```

---

## 8. Infrastructure & DevOps

> Ref: [09-infrastructure-devops.md](09-infrastructure-devops.md)

```mermaid
graph TB
    subgraph ci ["CI/CD Pipelines"]
        FCI["flutter-ci.yml<br/>Lint → Test → Build"]
        BCI["backend-ci.yml<br/>golangci-lint → Test → Docker"]
        ECI["engine-ci.yml<br/>clang-tidy → GTest → CMake"]
        DS["deploy-staging.yml<br/>Docker → Helm (staging)"]
        DP["deploy-production.yml<br/>Tag v* → Canary → Full"]
    end

    subgraph edge ["Edge Layer"]
        CDN3["CDN<br/>CloudFront / Cloudflare"]
        WAF["WAF + DDoS"]
    end

    subgraph k8s ["Kubernetes Cluster"]
        LB["Load Balancer<br/>Traefik / ALB"]
        AP2["API Pods<br/>3–10 replicas · HPA 70% CPU"]
        WP["Worker Pods<br/>2–8 replicas · Queue-depth HPA"]
    end

    subgraph datastores ["Data Stores"]
        PG2[("PostgreSQL<br/>1 primary + 2 replicas")]
        RD2[("Redis Cluster<br/>3 nodes")]
        S32[("S3")]
        ES2[("Elasticsearch<br/>3 nodes")]
        NATS3[("NATS")]
    end

    subgraph obs ["Observability"]
        PROM["Prometheus"]
        GRAF["Grafana"]
        LOKI["Loki (Logs)"]
        JAEG["Jaeger (Traces)"]
        ALERT["AlertManager"]
    end

    FCI & BCI & ECI --> DS --> DP
    DP --> k8s
    CDN3 & WAF --> LB --> AP2 & WP
    AP2 --> PG2 & RD2 & S32 & ES2 & NATS3
    WP --> NATS3 & S32
    PROM --> GRAF
    LOKI --> GRAF
    JAEG --> GRAF
    ALERT -.-> PROM
    k8s -.-> PROM & LOKI & JAEG
```

---

## 9. Database Schema (ER Overview)

> Ref: [12-database-schema.md](12-database-schema.md)

```mermaid
erDiagram
    users ||--o{ sessions : "has"
    users ||--o{ user_roles : "assigned"
    roles ||--o{ user_roles : "grants"
    users ||--o{ subscriptions : "subscribes"
    subscriptions ||--o{ payments : "generates"
    users ||--o{ user_media : "uploads"

    templates ||--o{ template_versions : "versioned"
    templates ||--o{ template_assets : "contains"
    templates ||--o{ template_tags : "tagged"
    tags ||--o{ template_tags : "categorizes"
    categories ||--o{ templates : "groups"

    users ||--o{ audit_logs : "generates"
    users ||--o{ notifications : "receives"
    users ||--o{ device_registrations : "registers"

    users ||--o{ creator_profiles : "creator"
    creator_profiles ||--o{ marketplace_listings : "publishes"
    marketplace_listings ||--o{ creator_ledger : "earns"

    users ||--o{ collab_sessions : "participates"
    collab_sessions ||--o{ collab_participants : "has"
    collab_sessions ||--o{ collab_operations : "records"

    feature_flags ||--o{ experiments : "tests"
    experiments ||--o{ experiment_assignments : "assigns"
```

---

## 10. Performance Targets

> Ref: [08-performance-memory-strategy.md](08-performance-memory-strategy.md)

```mermaid
graph LR
    subgraph targets ["Performance Targets"]
        direction TB
        T1["Cold Start < 2s"]
        T2["Template Preview < 500ms"]
        T3["Editor 60 fps"]
        T4["API p95 < 200ms"]
        T5["Video Export 1080p/30s < 60s"]
        T6["Image Export 4K < 3s"]
    end

    subgraph flutter_opt ["Flutter Optimizations"]
        F1["ListView.builder"]
        F2["Isolate offloading"]
        F3["RepaintBoundary"]
        F4["Riverpod .select()"]
        F5["Shader warm-up"]
    end

    subgraph engine_opt ["C/C++ Optimizations"]
        E1["Pool allocators"]
        E2["Zero-copy textures"]
        E3["SIMD (NEON/SSE/AVX)"]
        E4["GPU compute"]
        E5["Frame recycling"]
    end

    subgraph backend_opt ["Backend Optimizations"]
        B1["pgxpool (10–100)"]
        B2["Redis caching"]
        B3["CDN-first delivery"]
        B4["NATS async workers"]
        B5["Cursor pagination"]
    end

    targets --- flutter_opt & engine_opt & backend_opt
```

---

## 11. Disaster Recovery

> Ref: [16-disaster-recovery-business-continuity.md](16-disaster-recovery-business-continuity.md)

```mermaid
graph TB
    subgraph primary ["Primary Region (us-east-1)"]
        P_K8S["K8s Cluster"]
        P_PG["PostgreSQL Primary"]
        P_RD["Redis Primary"]
        P_S3["S3 Primary"]
    end

    subgraph dr ["DR Region (us-west-2)"]
        D_K8S["K8s Standby"]
        D_PG["PostgreSQL Replica<br/>Streaming Replication"]
        D_RD["Redis Replica"]
        D_S3["S3 Cross-Region<br/>Replication"]
    end

    subgraph tiers ["Service Tiers"]
        T0["Tier 0: Auth · API Gateway<br/>RTO: 5 min · RPO: 0"]
        T1["Tier 1: Template · Media<br/>RTO: 15 min · RPO: 5 min"]
        T2["Tier 2: Analytics · Notifications<br/>RTO: 1 hr · RPO: 15 min"]
        T3["Tier 3: Admin Portal<br/>RTO: 4 hr · RPO: 1 hr"]
    end

    P_PG -->|streaming| D_PG
    P_RD -->|replication| D_RD
    P_S3 -->|CRR| D_S3
    P_K8S -.failover.-> D_K8S
```

---

## Cross-Module Dependency Map

```mermaid
graph TD
    EXEC["01 Executive Summary"] --> SYS["02 System Architecture"]
    SYS --> FE["03 Frontend"]
    SYS --> BE2["04 Backend"]
    SYS --> MPE["05 Media Engine"]
    SYS --> SEC_T["06 Secure Templates"]
    FE & BE2 & MPE --> SEC["07 Security"]
    FE & BE2 & MPE --> PERF["08 Performance"]
    SEC & PERF --> INFRA["09 Infrastructure"]
    INFRA --> DEV["10 Methodology"]
    BE2 --> API["11 API Reference"]
    BE2 --> DB["12 Database Schema"]
    FE & BE2 --> TEST["13 Testing"]
    TEST --> RISK["14 Risk Assessment"]
    INFRA --> DR["16 Disaster Recovery"]

    style EXEC fill:#e8f5e9
    style SYS fill:#e1f5fe
    style FE fill:#fff3e0
    style BE2 fill:#fce4ec
    style MPE fill:#f3e5f5
    style SEC_T fill:#fff8e1
    style SEC fill:#ffebee
```
