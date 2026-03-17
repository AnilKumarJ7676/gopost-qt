# App Store Submission — Architecture Diagram

> Maps to [01-app-store-submission-playbook.md](01-app-store-submission-playbook.md)

---

## Release Pipeline

```mermaid
graph TB
    subgraph ci_cd ["CI/CD (GitHub Actions + Fastlane)"]
        MERGE["Merge to main"]
        BUILD_IOS["Flutter Build iOS<br/>Fastlane → IPA"]
        BUILD_AND["Flutter Build Android<br/>Fastlane → AAB"]
        BUILD_WIN["Flutter Build Windows<br/>MSIX Package"]
        BUILD_WEB["Flutter Build Web<br/>S3 + CloudFront"]
        BUILD_MAC["Flutter Build macOS<br/>Fastlane → DMG"]
    end

    subgraph test_phase ["Testing"]
        TF["TestFlight<br/>(iOS/macOS)"]
        IT["Internal Track<br/>(Android)"]
        BETA_WIN["Beta Channel<br/>(Windows)"]
        QA["QA Validation"]
    end

    subgraph submit ["Submission"]
        ASC["App Store Connect"]
        GPC["Google Play Console"]
        MSC["Microsoft Partner Center"]
        WEB_DEPLOY["Web Deploy<br/>CloudFront"]
    end

    subgraph rollout ["Staged Rollout"]
        R1["Day 1: 1%"]
        R2["Day 2: 5%"]
        R3["Day 3: 25%"]
        R4["Day 5: 50%"]
        R5["Day 7: 100%"]
    end

    MERGE --> BUILD_IOS & BUILD_AND & BUILD_WIN & BUILD_WEB & BUILD_MAC
    BUILD_IOS --> TF
    BUILD_AND --> IT
    BUILD_WIN --> BETA_WIN
    BUILD_MAC --> TF
    TF & IT & BETA_WIN --> QA
    QA --> ASC & GPC & MSC
    BUILD_WEB --> WEB_DEPLOY
    ASC & GPC --> rollout
```

---

## Forced Update Mechanism

```mermaid
sequenceDiagram
    participant App as Flutter App
    participant API as Go Backend
    participant Store as App Store

    App->>API: GET /config/app
    API-->>App: { min_version: "2.1.0", latest: "2.3.0" }
    App->>App: Compare current version

    alt Current < min_version
        App->>App: Show blocking dialog
        App->>Store: Open store listing
    else Current < latest
        App->>App: Show optional update banner
    else Up to date
        App->>App: Continue normally
    end
```

---

## Platform Submission Checklist

```mermaid
graph LR
    subgraph ios_check ["iOS / macOS"]
        I1["Privacy nutrition labels"]
        I2["App Transport Security"]
        I3["StoreKit 2 products"]
        I4["Screenshots (6.7 · 6.5 · 12.9)"]
        I5["Review notes for IAP"]
    end

    subgraph android_check ["Android"]
        A1["Data safety form"]
        A2["Content rating questionnaire"]
        A3["Play Billing products"]
        A4["Screenshots (phone · 7 · 10)"]
        A5["Target API level"]
    end

    subgraph windows_check ["Windows"]
        W1["MSIX signing"]
        W2["Store listing screenshots"]
        W3["Age rating"]
    end

    subgraph web_check ["Web"]
        W_CSP["CSP headers"]
        W_PWA["PWA manifest"]
        W_SSL["SSL certificate"]
    end
```
