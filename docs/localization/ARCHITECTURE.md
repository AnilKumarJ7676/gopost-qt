# Localization & Internationalization — Architecture Diagram

> Maps to [01-localization-architecture.md](01-localization-architecture.md)

---

## Localization Architecture

```mermaid
graph TB
    subgraph client_l10n ["Flutter Client"]
        GEN["gen_l10n<br/>Code generation"]
        ARB["ARB Files<br/>6 launch locales"]
        LOC_P["LocaleNotifier<br/>(Riverpod)"]
        APP_L["AppLocalizations<br/>Generated accessors"]
    end

    subgraph backend_l10n ["Go Backend"]
        MW["Locale Middleware<br/>Accept-Language parsing"]
        BUNDLES["i18n Bundles<br/>Error messages · Push text"]
        JSONB["JSONB Metadata<br/>Template names · descriptions"]
    end

    subgraph translation ["Translation Pipeline"]
        CROWDIN["Crowdin<br/>Translation management"]
        CI_CHECK["CI Pseudo-Locale Check<br/>Missing key detection"]
    end

    ARB --> GEN --> APP_L
    LOC_P --> APP_L
    LOC_P -->|Accept-Language| MW
    MW --> BUNDLES
    MW --> JSONB

    ARB -->|upload| CROWDIN
    CROWDIN -->|translated ARB| ARB
    ARB --> CI_CHECK
```

---

## Locale Data Flow

```mermaid
sequenceDiagram
    participant User
    participant App as Flutter App
    participant Prefs as SharedPreferences
    participant API as Go Backend

    User->>App: Change language to Arabic
    App->>Prefs: Save locale (ar)
    App->>App: LocaleNotifier → rebuild UI
    App->>App: RTL layout activated
    App->>API: Accept-Language: ar
    API->>API: Locale middleware → Arabic bundles
    API-->>App: Localized responses
    App->>API: PATCH /users/me (preferred_locale: ar)
```

---

## RTL Support

```mermaid
graph LR
    subgraph rtl ["RTL Languages (Arabic, Hebrew)"]
        LAYOUT["Mirrored Layout<br/>start/end not left/right"]
        NAV["Navigation<br/>Back arrow → forward"]
        TEXT_DIR["Text Direction<br/>TextDirection.rtl"]
        ICONS["Directional Icons<br/>Mirrored arrows"]
    end

    subgraph ltr ["LTR Languages (en, es, fr, de, ja, ko)"]
        STD["Standard Layout"]
    end

    subgraph shared_rules ["Shared"]
        NO_MIRROR["Numbers · Media controls<br/>Not mirrored"]
    end
```
