# Creator Marketplace — Architecture Diagram

> Maps to [01-creator-marketplace-architecture.md](01-creator-marketplace-architecture.md)

---

## Marketplace Architecture

```mermaid
graph TB
    subgraph actors ["Actors"]
        CREATOR["Creator<br/>Template author"]
        BUYER["Buyer<br/>End user"]
        ADMIN_M["Admin<br/>Moderator"]
    end

    subgraph storefront ["Storefront"]
        FEAT["Featured"]
        TREND["Trending"]
        SEARCH_M["Search<br/>(Elasticsearch)"]
        CATS["Categories"]
        RATINGS["Ratings & Reviews"]
    end

    subgraph submission ["Submission Pipeline"]
        UPLOAD_M["Creator Upload"]
        AUTO["Automated Checks<br/>Format · NSFW · Malware"]
        QUEUE["Review Queue"]
        APPROVE["Approve / Reject"]
    end

    subgraph commerce ["Commerce"]
        IAP_M["IAP / Stripe<br/>Purchase"]
        SPLIT["70/30 Revenue Split"]
        LEDGER["Creator Ledger"]
        PAYOUT["Stripe Connect<br/>Monthly Payouts"]
    end

    subgraph data_m ["Data Layer"]
        PG_M[("PostgreSQL<br/>creator_profiles · listings · ledger")]
        ES_M[("Elasticsearch<br/>Storefront search")]
        S3_M[("S3<br/>.gpt assets")]
    end

    CREATOR --> UPLOAD_M --> AUTO --> QUEUE
    ADMIN_M --> QUEUE --> APPROVE
    APPROVE --> storefront
    BUYER --> storefront --> IAP_M --> SPLIT
    SPLIT --> LEDGER --> PAYOUT --> CREATOR

    storefront --> ES_M
    UPLOAD_M --> S3_M
    APPROVE --> PG_M
    LEDGER --> PG_M
```

---

## Submission & Review Flow

```mermaid
sequenceDiagram
    participant Creator
    participant App as Flutter App
    participant API as Go Backend
    participant Auto as Auto Checker
    participant Admin as Admin Portal
    participant Notif as Notification Service

    Creator->>App: Submit template
    App->>API: POST /marketplace/submissions
    API->>Auto: Format validation
    Auto->>Auto: NSFW scan (AWS Rekognition)
    Auto->>Auto: Malware scan (ClamAV)
    Auto-->>API: Automated result

    alt Automated pass
        API->>API: Queue for manual review
        API-->>Admin: New review item
        Admin->>API: Approve / Reject
        API->>Notif: Notify creator
        Notif-->>Creator: Push notification
    else Automated fail
        API->>Notif: Notify creator (rejection reason)
        Notif-->>Creator: Push notification
    end
```

---

## Revenue Flow

```mermaid
flowchart LR
    PURCHASE["Buyer Purchase<br/>$4.99"] --> VERIFY["Receipt Verify"]
    VERIFY --> RECORD["Record Sale"]
    RECORD --> SPLIT2["Revenue Split"]
    SPLIT2 --> CREATOR_SHARE["Creator 70%<br/>$3.49"]
    SPLIT2 --> PLATFORM_SHARE["Platform 30%<br/>$1.50"]
    CREATOR_SHARE --> LEDGER2["Creator Ledger"]
    LEDGER2 -->|Monthly| STRIPE_T["Stripe Connect Transfer"]
    STRIPE_T --> BANK["Creator Bank Account"]
```
