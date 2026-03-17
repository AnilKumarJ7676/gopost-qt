# Offline & Caching Strategy — Architecture Diagram

> Maps to [01-offline-caching-strategy.md](01-offline-caching-strategy.md)

---

## Multi-Tier Cache Architecture

```mermaid
graph TB
    subgraph l1 ["L1: In-Memory (Session)"]
        RIV["Riverpod State"]
        ENT_MEM["Entitlements"]
        UI_STATE["UI State"]
    end

    subgraph l2 ["L2: Disk Hot (Structured)"]
        HIVE["Hive / SQLite (Drift)"]
        TMPL_META["Template Metadata"]
        USER_PREFS["User Preferences"]
        AUTOSAVE["Auto-save Snapshots<br/>Every 30s · 5 per project"]
    end

    subgraph l3 ["L3: Disk Cold (Binary)"]
        IMG_CACHE["cached_network_image<br/>Preview images"]
        GPT_CACHE[".gpt File Cache<br/>LRU eviction"]
        EXPORT_Q["Export Queue<br/>Offline exports"]
    end

    subgraph server ["Server Cache"]
        REDIS_C["Redis<br/>Template metadata 5 min<br/>Categories 15 min<br/>Sessions 7 days"]
        CDN_C["CDN<br/>Template assets<br/>Preview images"]
    end

    l1 --> l2 --> l3 --> server
```

---

## Offline Feature Availability

```mermaid
graph LR
    subgraph available ["Available Offline"]
        A1["Cached Templates<br/>Browse & preview"]
        A2["Editor<br/>Full editing offline"]
        A3["Export<br/>Local export"]
        A4["Auto-save<br/>Continuous snapshots"]
        A5["Entitlements<br/>Cached for 7 days"]
    end

    subgraph degraded ["Degraded Offline"]
        D1["Template Browser<br/>Cached items only"]
        D2["AI Features<br/>On-device models only"]
    end

    subgraph unavailable ["Unavailable Offline"]
        U1["New Template Download"]
        U2["Account Management"]
        U3["Collaboration"]
        U4["Marketplace Purchases"]
    end
```

---

## Reconnect Protocol

```mermaid
sequenceDiagram
    participant App as Flutter App
    participant Network as connectivity_plus
    participant API as Go Backend
    participant Cache as Local Cache

    Note over App,Cache: Device goes offline
    App->>Cache: Continue with cached data
    App->>Cache: Auto-save every 30s

    Note over App,Cache: Network restored
    Network-->>App: Connectivity change event
    App->>API: Refresh auth token
    API-->>App: New access token
    App->>API: GET /users/me/entitlements
    API-->>App: Updated entitlements
    App->>API: Sync queued actions
    App->>API: GET /templates (delta since last_sync)
    API-->>App: Updated metadata
    App->>Cache: Update local cache
```

---

## Storage Budgets

```mermaid
graph LR
    subgraph mobile ["Mobile (iOS/Android)"]
        M_PREVIEW["Preview Cache: 200 MB"]
        M_TEMPLATE[".gpt Cache: 500 MB"]
        M_AUTOSAVE["Auto-save: 100 MB"]
        M_TOTAL["Total: ~800 MB max"]
    end

    subgraph desktop ["Desktop (Windows/macOS)"]
        D_PREVIEW["Preview Cache: 500 MB"]
        D_TEMPLATE[".gpt Cache: 2 GB"]
        D_AUTOSAVE["Auto-save: 500 MB"]
        D_TOTAL["Total: ~3 GB max"]
    end
```
