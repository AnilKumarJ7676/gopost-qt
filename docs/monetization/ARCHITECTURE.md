# Monetization System — Architecture Diagram

> Maps to [01-monetization-system.md](01-monetization-system.md)

---

## Subscription Architecture

```mermaid
graph TB
    subgraph plans ["Subscription Plans"]
        FREE["Free Tier<br/>5 templates/month · watermark · 720p"]
        PRO["Pro — $9.99/mo · $79.99/yr<br/>Unlimited templates · no watermark · 4K"]
        CREATOR["Creator — $19.99/mo · $159.99/yr<br/>Pro + marketplace · collaboration · priority AI"]
    end

    subgraph client ["Flutter Client"]
        PW["Paywall Screen<br/>Plan toggle · Feature matrix"]
        SG["SubscriptionGuard<br/>Feature gating"]
        ENT_C["Entitlement Cache<br/>(Redis → Local)"]
    end

    subgraph iap ["IAP Providers"]
        SK2["StoreKit 2<br/>iOS · macOS"]
        GPB["Google Play Billing<br/>Android"]
        STRIPE["Stripe<br/>Web · Desktop"]
    end

    subgraph backend_sub ["Go Backend"]
        ENT_S["EntitlementService"]
        RV["Receipt Verification"]
        WH["Stripe Webhooks"]
        LIFECYCLE["Subscription Lifecycle<br/>State Machine"]
    end

    subgraph data_sub ["Data Layer"]
        PG_S[("PostgreSQL<br/>subscriptions · payments")]
        RD_S[("Redis<br/>Entitlement cache")]
    end

    FREE & PRO & CREATOR --> PW --> SG
    PW --> SK2 & GPB & STRIPE
    SK2 & GPB -->|receipt| RV
    STRIPE -->|webhook| WH
    RV & WH --> ENT_S --> LIFECYCLE
    ENT_S --> PG_S & RD_S
    RD_S --> ENT_C
```

---

## Purchase Flow

```mermaid
sequenceDiagram
    participant User
    participant App as Flutter App
    participant Store as App Store / Play Store
    participant API as Go Backend
    participant Verify as Apple/Google API
    participant DB as PostgreSQL

    User->>App: Tap "Subscribe to Pro"
    App->>Store: Initiate purchase
    Store-->>User: Payment sheet
    User->>Store: Confirm payment
    Store-->>App: Purchase receipt
    App->>API: POST /subscriptions/verify (receipt)
    API->>Verify: Validate receipt
    Verify-->>API: Valid + transaction details
    API->>DB: Create subscription record
    API->>API: Update entitlements (Redis)
    API-->>App: 200 OK (entitlements)
    App->>App: Refresh UI → Pro features unlocked
```

---

## Subscription Lifecycle State Machine

```mermaid
stateDiagram-v2
    [*] --> Free
    Free --> TrialActive : Start trial
    Free --> Active : Purchase
    TrialActive --> Active : Convert
    TrialActive --> TrialExpired : Trial ends
    TrialExpired --> Active : Purchase
    TrialExpired --> Free : No action
    Active --> GracePeriod : Payment failed
    GracePeriod --> Active : Payment retry success
    GracePeriod --> Expired : Grace period ends
    Active --> Cancelled : User cancels
    Cancelled --> Active : Re-subscribe
    Cancelled --> Expired : Period ends
    Expired --> Active : Re-subscribe
    Expired --> Free : No action
    Active --> Refunded : Refund issued
    Refunded --> Free : Entitlements revoked
```
