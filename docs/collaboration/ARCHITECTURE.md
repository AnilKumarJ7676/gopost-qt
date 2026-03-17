# Real-Time Collaboration — Architecture Diagram

> Maps to [01-real-time-collaboration.md](01-real-time-collaboration.md)

---

## Collaboration Architecture

```mermaid
graph TB
    subgraph clients ["Flutter Clients"]
        C1["Editor A"]
        C2["Editor B"]
        C3["Editor C"]
    end

    subgraph ws_cluster ["WebSocket Cluster"]
        WS1["WS Server Pod 1"]
        WS2["WS Server Pod 2"]
    end

    subgraph crdt ["CRDT Engine"]
        HLC["Hybrid Logical Clock<br/>Causal ordering"]
        LWW["LWW-Register<br/>Layer properties"]
        LWW_M["LWW-Map<br/>Nested structures"]
        ORS["OR-Set<br/>Layer add/remove"]
    end

    subgraph fanout ["Fan-out"]
        REDIS_PS["Redis Pub/Sub<br/>Cross-pod broadcast"]
    end

    subgraph persistence ["Persistence"]
        PG_C[("PostgreSQL<br/>Sessions · Participants")]
        S3_C[("S3<br/>Snapshots · Ops log")]
        NATS_C["NATS<br/>Operation queue"]
    end

    subgraph presence ["Presence System"]
        CURSOR["Cursors<br/>Position · Color"]
        SELECTION["Selections<br/>Layer highlights"]
        STATUS["Activity Status<br/>Active · Idle · Away"]
    end

    C1 & C2 & C3 -->|WebSocket| WS1 & WS2
    WS1 & WS2 --> REDIS_PS
    REDIS_PS --> WS1 & WS2
    WS1 & WS2 --> crdt
    WS1 & WS2 --> presence
    WS1 --> PG_C & S3_C & NATS_C
```

---

## Operation Flow

```mermaid
sequenceDiagram
    participant A as Editor A
    participant WS as WS Server
    participant Redis as Redis Pub/Sub
    participant WS2 as WS Server (Pod 2)
    participant B as Editor B

    A->>WS: Operation (move layer, HLC timestamp)
    WS->>WS: Validate + CRDT merge
    WS->>Redis: Publish to session channel
    Redis->>WS2: Broadcast
    WS2->>B: Forward operation
    B->>B: Apply CRDT operation locally

    Note over A,B: Conflict resolution via LWW-Register<br/>Last-writer-wins using HLC ordering
```

---

## Session Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Created : Host creates session
    Created --> Active : First participant joins
    Active --> Active : Participants join/leave
    Active --> Paused : All participants idle (5 min)
    Paused --> Active : Participant returns
    Active --> Saving : Snapshot triggered
    Saving --> Active : Snapshot complete
    Active --> Closing : Host ends session
    Closing --> Closed : All ops flushed
    Closed --> [*]

    Active --> Offline : Participant disconnects
    Offline --> Active : Reconnect + merge
```
