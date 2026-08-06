# order-matching-engine
 
An exchange matching engine with an event-sourced core and FIX protocol connectivity, built in progressive layers from single-threaded correctness up through concurrency and market data distribution.
 
**Status:** Remaining layers are done; repository history is being cleaned up prior to push.
 
---
 
## Architecture
 
| Layer | Description | Status |
| --- | --- | --- |
| 0 — Core | Event-sourced, single-threaded matching engine | Pushed |
| 1 — FIX Protocol | FIX tag=value connectivity over sockets | Not yet pushed |
| 2 — Memory & Speed | Low-latency data structures and allocation | Not yet pushed |
| 3 — Concurrency | Lock-free, multi-threaded pipeline | Not yet pushed |
| 4 — Market Data | UDP multicast market data distribution | Not yet pushed |
| Stretch — Recovery | Snapshotting and crash recovery | Not yet pushed |
 
---
 
## Layer 0 — Core
 
Single-threaded matching engine with an append-only event log (`OrderAdded`, `OrderCancelled`, `OrderModified`, `Trade`) as the source of truth. A sequencer assigns a strict monotonic sequence number to each event. Orders follow a `New → PartiallyFilled → Filled / Cancelled / Rejected` state machine, matched using price-time priority. Book state is derived entirely by replaying the event log, verified by wiping in-memory state and rebuilding it from scratch.
 
## Layer 1 — FIX Protocol
 
Hand-rolled parser and encoder for core FIX message types: `NewOrderSingle`, `OrderCancelRequest`, `ExecutionReport`, and `Reject`, targeting FIX 4.2/4.4. Replaces internal test structs with real FIX messages over a socket.
 
## Layer 2 — Memory & Speed
 
Intrusive data structures for price levels and order queues to eliminate per-order heap allocation, backed by a custom memory pool. Optimization is profiler-driven, measured with latency histograms (p50/p99/p99.9) rather than throughput alone.
 
## Layer 3 — Concurrency
 
Moves the sequencer to a lock-free, single-writer pipeline using a ring buffer (SPSC, extended to MPSC) based on the LMAX Disruptor pattern, with the sequencer and matching engine running on separate threads.
 
## Layer 4 — Market Data
 
Separates order entry from market data distribution. Top-of-book and depth updates are published over UDP multicast, reconstructed independently by a separate subscriber process.
 
## Stretch — Recovery
 
Periodic snapshotting to bound replay time, crash recovery from snapshot plus log replay, and FIX session-layer recovery via sequence gap detection and resend requests.
