# Feature Flag System — Architecture Diagram

> Maps to [01-feature-flag-system.md](01-feature-flag-system.md)

---

## Feature Flag Architecture

```mermaid
graph TB
    subgraph admin_ff ["Admin Dashboard"]
        FF_UI["Flag Management UI<br/>Create · Edit · Archive"]
        EXP_UI["Experiment Setup<br/>Variants · Traffic split"]
        KS_UI["Kill Switches<br/>Instant disable"]
    end

    subgraph backend_ff ["Go Backend"]
        EVAL["Flag Evaluator<br/>Conditions + Rollout"]
        HASH["Murmur3 Hashing<br/>Sticky user assignment"]
        CACHE_FF["Redis Cache<br/>Flag state · Invalidation"]
    end

    subgraph client_ff ["Flutter SDK"]
        SYNC["5-min Sync Cycle"]
        LOCAL["Local Cache<br/>(SharedPreferences)"]
        SDK["FlagService<br/>getBool · getString · getNumber · getJSON"]
    end

    subgraph data_ff ["Data Layer"]
        PG_FF[("PostgreSQL<br/>feature_flags · experiments")]
        RD_FF[("Redis<br/>Pub/Sub invalidation")]
    end

    FF_UI & EXP_UI & KS_UI --> PG_FF
    PG_FF --> RD_FF
    RD_FF -->|invalidation| CACHE_FF

    SDK -->|POST /flags/evaluate| EVAL
    EVAL --> CACHE_FF --> HASH
    EVAL --> PG_FF
    SDK --> LOCAL
    SYNC --> SDK
```

---

## Flag Evaluation Flow

```mermaid
flowchart TB
    REQ["Evaluate Flag<br/>flag_key · user_context"]
    REDIS_CHECK["Redis Cache Hit?"]
    DB_FETCH["Fetch from PostgreSQL"]
    ENABLED["Flag Enabled?"]
    RULES["Evaluate Targeting Rules<br/>Conditions: eq · in · regex · semver"]
    ROLLOUT["Percentage Rollout<br/>Murmur3(user_id + flag_key) % 100"]
    VARIANT["Resolve Variant"]
    RESULT["Return Value"]

    REQ --> REDIS_CHECK
    REDIS_CHECK -->|hit| ENABLED
    REDIS_CHECK -->|miss| DB_FETCH --> ENABLED
    ENABLED -->|no| RESULT
    ENABLED -->|yes| RULES
    RULES -->|match| ROLLOUT --> VARIANT --> RESULT
    RULES -->|no match| RESULT
```

---

## A/B Experiment Framework

```mermaid
graph LR
    subgraph setup ["Experiment Setup"]
        HYP["Hypothesis"]
        VARIANTS["Variants<br/>Control (50%) · Treatment (50%)"]
        METRIC["Success Metric<br/>Conversion · Engagement"]
    end

    subgraph assign ["Assignment"]
        STICKY["Sticky Hash<br/>Murmur3(user_id + experiment_id)"]
        ASSIGN2["Variant Assignment<br/>Stored in experiment_assignments"]
    end

    subgraph measure ["Measurement"]
        EVENTS["Analytics Events<br/>Tagged with variant"]
        STATS["Statistical Analysis<br/>Chi-squared · T-test"]
        DECISION["Decision<br/>Ship · Iterate · Kill"]
    end

    setup --> assign --> measure
```
