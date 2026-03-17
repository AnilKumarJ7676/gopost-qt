# Analytics & Event Tracking — Architecture Diagram

> Maps to [01-analytics-event-tracking.md](01-analytics-event-tracking.md)

---

## Dual-Pipeline Analytics Architecture

```mermaid
graph TB
    subgraph client ["Flutter Client"]
        AS["AnalyticsService<br/>(Abstraction)"]
        FA["Firebase Analytics<br/>Implementation"]
        CONSENT["Consent Manager<br/>GDPR opt-in/out"]
        ROUTE["AnalyticsRouteObserver<br/>Screen tracking"]
    end

    subgraph backend ["Go Backend"]
        EE["EventEmitter<br/>Server-side events"]
        AW["AnalyticsWorker<br/>NATS consumer"]
    end

    subgraph pipeline ["Data Pipeline"]
        NATS_Q["NATS Queue"]
        CH["ClickHouse<br/>Time-series analytics"]
        BQ["BigQuery<br/>(via Firebase)"]
    end

    subgraph viz ["Visualization"]
        GRAF["Grafana Dashboards<br/>DAU · MRR · Funnels"]
        FB_DASH["Firebase Dashboard<br/>Product analytics"]
        ADMIN_D["Admin Portal<br/>KPI cards"]
    end

    subgraph experiments ["A/B Testing"]
        RC["Firebase Remote Config"]
        FLAGS["Feature Flags<br/>(In-house)"]
    end

    AS --> FA --> BQ --> FB_DASH
    AS --> CONSENT
    ROUTE --> AS

    EE --> NATS_Q --> AW --> CH --> GRAF
    CH --> ADMIN_D

    RC & FLAGS --> AS
```

---

## Event Taxonomy (50+ Events)

```mermaid
graph LR
    subgraph categories ["Event Categories"]
        AUTH["Auth Events<br/>login · register · logout"]
        TMPL["Template Events<br/>view · preview · download · customize"]
        EDIT["Editor Events<br/>session_start · effect_applied · export"]
        SUB["Subscription Events<br/>paywall_shown · purchase · cancel"]
        PERF["Performance Events<br/>cold_start · frame_drop · crash"]
        ADMIN_E["Admin Events<br/>template_approved · user_banned"]
    end

    subgraph funnels ["Key Funnels"]
        ACT["Activation<br/>install → register → first_template"]
        USAGE["Template Usage<br/>browse → preview → customize → export"]
        CONV["Subscription<br/>paywall_shown → started → completed"]
        TRIAL["Trial<br/>trial_start → engagement → convert"]
    end

    categories --> funnels
```
