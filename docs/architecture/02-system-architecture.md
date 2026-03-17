## 2. High-Level System Architecture

### 2.1 System Context Diagram (C4 Level 1)

```mermaid
graph TB
    subgraph users ["Users"]
        EndUser["End User<br/>(iOS, Android, Windows, macOS, Web)"]
        AdminUser["Admin / Creator<br/>(Web Admin Portal)"]
    end

    subgraph gopostSystem ["Gopost System"]
        FlutterApp["Flutter Client App"]
        AdminPortal["Admin Portal<br/>(Flutter Web)"]
        BackendAPI["Go + Gin Backend API"]
        MediaEngine["C/C++ Media Engine<br/>(Embedded in Client)"]
        CDNNode["CDN<br/>(CloudFront / Cloudflare)"]
    end

    subgraph external ["External Services"]
        AuthProvider["OAuth2 Providers<br/>(Google, Apple, Facebook)"]
        PaymentGateway["Payment Gateway<br/>(Stripe / RevenueCat)"]
        PushService["Push Notification<br/>(FCM / APNs)"]
        Analytics["Analytics<br/>(Firebase / Mixpanel)"]
    end

    EndUser --> FlutterApp
    AdminUser --> AdminPortal
    FlutterApp --> BackendAPI
    FlutterApp --> CDNNode
    FlutterApp --> MediaEngine
    AdminPortal --> BackendAPI
    BackendAPI --> AuthProvider
    BackendAPI --> PaymentGateway
    BackendAPI --> PushService
    BackendAPI --> Analytics
    BackendAPI --> CDNNode
```

### 2.2 Container Diagram (C4 Level 2)

```mermaid
graph TB
    subgraph clientApps ["Client Applications"]
        FlutterMobile["Flutter Mobile<br/>(iOS / Android)"]
        FlutterDesktop["Flutter Desktop<br/>(Windows / macOS)"]
        FlutterWeb["Flutter Web"]
        NativeEngine["C/C++ Media Engine<br/>(FFI Embedded)"]
    end

    subgraph backendCluster ["Backend Cluster"]
        APIGateway["API Gateway<br/>(Nginx / Traefik)"]
        AuthSvc["Auth Service<br/>(Go)"]
        TemplateSvc["Template Service<br/>(Go)"]
        MediaSvc["Media Service<br/>(Go)"]
        UserSvc["User Service<br/>(Go)"]
        AdminSvc["Admin Service<br/>(Go)"]
        NotifSvc["Notification Service<br/>(Go)"]
    end

    subgraph dataStores ["Data Stores"]
        PostgresDB[("PostgreSQL")]
        RedisCache[("Redis")]
        S3Storage[("S3 Object Storage")]
        ElasticSearch[("Elasticsearch")]
    end

    subgraph infra ["Infrastructure"]
        CDN["CDN Edge Nodes"]
        MessageQueue["Message Queue<br/>(NATS / RabbitMQ)"]
    end

    FlutterMobile --> APIGateway
    FlutterDesktop --> APIGateway
    FlutterWeb --> APIGateway
    FlutterMobile --> NativeEngine
    FlutterDesktop --> NativeEngine

    APIGateway --> AuthSvc
    APIGateway --> TemplateSvc
    APIGateway --> MediaSvc
    APIGateway --> UserSvc
    APIGateway --> AdminSvc
    APIGateway --> NotifSvc

    AuthSvc --> PostgresDB
    AuthSvc --> RedisCache
    TemplateSvc --> PostgresDB
    TemplateSvc --> S3Storage
    TemplateSvc --> ElasticSearch
    TemplateSvc --> RedisCache
    MediaSvc --> S3Storage
    MediaSvc --> MessageQueue
    UserSvc --> PostgresDB
    AdminSvc --> PostgresDB
    NotifSvc --> MessageQueue

    TemplateSvc --> CDN
    MediaSvc --> CDN
```

### 2.3 Data Flow — Template Lifecycle

```mermaid
sequenceDiagram
    participant Admin as Admin / Creator
    participant Portal as Admin Portal
    participant API as Backend API
    participant Enc as Encryption Service
    participant S3 as Object Storage
    participant CDN as CDN
    participant App as Client App
    participant Engine as C/C++ Engine

    Admin->>Portal: Upload template (AE export / in-app save)
    Portal->>API: POST /api/v1/admin/templates (multipart)
    API->>API: Validate format, dimensions, safety scan
    API->>Enc: Encrypt template package (AES-256-GCM)
    Enc-->>API: Encrypted .gpt blob + metadata
    API->>S3: Store encrypted blob
    API->>CDN: Invalidate / warm cache
    API-->>Portal: 201 Created (template_id)

    App->>API: GET /api/v1/templates/:id/access
    API->>API: Verify auth, subscription, rate limit
    API-->>App: Signed CDN URL + session decryption key (RSA envelope)
    App->>CDN: Download encrypted .gpt blob
    CDN-->>App: Encrypted binary stream
    App->>Engine: Pass encrypted blob + session key (via FFI)
    Engine->>Engine: Decrypt in memory (AES-256-GCM)
    Engine->>Engine: Parse layers, assets, render instructions
    Engine->>Engine: GPU render pipeline
    Engine-->>App: Rendered frames via Texture Registry
```

### 2.4 Platform Bridge Architecture

```mermaid
graph LR
    subgraph flutterLayer ["Flutter (Dart)"]
        UI["Widget Tree / UI"]
        StateManager["State Management<br/>(Riverpod)"]
        BridgeAPI["Rendering Bridge API"]
    end

    subgraph bridgeLayer ["Bridge Layer"]
        FFI["dart:ffi<br/>(Synchronous)"]
        PlatformChannel["Platform Channels<br/>(Async Messages)"]
        TextureReg["Texture Registry<br/>(Zero-Copy GPU)"]
    end

    subgraph nativeLayer ["C/C++ Engine"]
        CoreLib["libgopost_engine"]
        VideoP["Video Pipeline"]
        ImageP["Image Pipeline"]
        GPUBackend["GPU Backend<br/>(Metal / Vulkan / GL)"]
        MemPool["Memory Pool Allocator"]
    end

    UI --> StateManager
    StateManager --> BridgeAPI
    BridgeAPI --> FFI
    BridgeAPI --> PlatformChannel
    BridgeAPI --> TextureReg

    FFI --> CoreLib
    PlatformChannel --> CoreLib
    TextureReg --> GPUBackend

    CoreLib --> VideoP
    CoreLib --> ImageP
    CoreLib --> GPUBackend
    CoreLib --> MemPool
```

---

## Development Sprint Plan

### Sprint Assignment

| Attribute | Value |
|---|---|
| **Phase** | Phase 1: Foundation |
| **Sprint(s)** | Sprint 1 (Weeks 1-2) |
| **Team** | Backend, Frontend, Platform Engineers |
| **Predecessor** | [01-executive-summary.md](01-executive-summary.md) |
| **Successor** | [03-frontend-architecture.md](03-frontend-architecture.md) |
| **Story Points Total** | 35 |

### User Stories

| ID | Story | Acceptance Criteria | Points | Priority | Dependencies |
|---|---|---|---|---|---|
| APP-004 | As a Tech Lead, I want to validate the C4 system context diagram so that all actors and external systems are correctly represented. | - System context diagram reviewed against requirements<br/>- All user types (End User, Admin) and external services documented<br/>- Diagram sign-off obtained | 2 | P0 | APP-001, APP-002, APP-003 |
| APP-005 | As a Tech Lead, I want to validate the C4 container diagram so that all backend services and data stores are correctly modeled. | - Container diagram reviewed for completeness<br/>- API Gateway, Auth, Template, Media, User, Admin, Notification services validated<br/>- Diagram sign-off obtained | 2 | P0 | APP-004 |
| APP-006 | As a Platform Engineer, I want to design the template lifecycle data flow so that the end-to-end flow from admin upload to client render is documented. | - Sequence diagram for admin upload flow complete<br/>- Sequence diagram for client download and decryption flow complete<br/>- Data flow validated with security team | 3 | P0 | APP-004 |
| APP-007 | As a Platform Engineer, I want to design the platform bridge architecture so that Flutter and C++ engine integration is clearly specified. | - Bridge layer (FFI, Platform Channels, Texture Registry) documented<br/>- Data flow between Flutter and native engine defined<br/>- Architecture diagram approved | 3 | P0 | APP-005 |
| APP-008 | As a Platform Engineer, I want to validate the platform channel vs FFI decision so that the correct interop strategy is chosen per use case. | - Decision matrix for FFI vs Platform Channels documented<br/>- Synchronous vs async call patterns defined<br/>- Decision documented and approved | 2 | P1 | APP-007 |
| APP-009 | As a Platform Engineer, I want to build a Flutter↔C++ FFI bridge prototype so that we can validate the interop approach. | - Minimal C library with one exported function<br/>- Dart FFI bindings generated and invoked successfully<br/>- Round-trip call verified on at least one platform | 5 | P0 | APP-007 |
| APP-010 | As a Platform Engineer, I want to implement a texture registry proof-of-concept so that zero-copy GPU texture delivery to Flutter is validated. | - Native texture registered with Flutter texture registry<br/>- Texture displayed in Flutter widget tree<br/>- No CPU pixel copy in hot path | 5 | P0 | APP-009 |
| APP-011 | As a Backend Engineer, I want to document the system context diagram interactions so that API boundaries are clear. | - All API touchpoints between Flutter app and backend documented<br/>- CDN and external service interactions specified<br/>- Document reviewed | 2 | P1 | APP-004 |
| APP-012 | As a Tech Lead, I want to obtain sign-off on the system context and container diagrams so that implementation can proceed. | - Stakeholder review completed<br/>- All feedback addressed<br/>- Formal sign-off documented | 2 | P0 | APP-004, APP-005 |

### Definition of Done

- [ ] All stories in this section marked complete
- [ ] Code reviewed and merged to `develop`
- [ ] Unit tests passing (≥ 90% coverage for new code)
- [ ] Integration tests passing
- [ ] Documentation updated
- [ ] No critical or high-severity bugs open
- [ ] Sprint review demo completed
